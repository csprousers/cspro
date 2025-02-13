#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "ParameterManager.h"
#include "VariableWorker.h"
#include <zEngineO/Versioning.h>
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/Screen.h>
#include <zAppO/Application.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zDictO/DDClass.h>
#include <zFormO/FormFile.h>
#include <ZBRIDGEO/npff.h>
#include <zParadataO/Logger.h>


namespace
{
    struct InvalidValueException : public CSProException
    {
        InvalidValueException(const std::variant<double, CString>& value_, bool type_error = false)
            :   CSProException("Invalid value"),
                value(value_),
                error_number(type_error ? 1109 : 1107)
        {
        }

        std::variant<double, CString> value;
        int error_number;
    };

    CString ValueToString(const std::variant<double, CString>& value)
    {
        return std::holds_alternative<double>(value) ? WS2CS(DoubleToString(std::get<double>(value))) :
                                                       std::get<CString>(value);
    }
}


ParameterManager::Parameter CIntDriver::GetSetPropertyParser(int iExpr, std::set<int>* symbol_set,
    std::variant<double, CString>* out_value/* = nullptr*/)
{
    const auto& various_node = GetNode<FNVARIOUS_NODE>(iExpr);
    bool set_function = ( various_node.fn_code == FunctionCode::FNSETPROPERTY_CODE );
    const int* arguments = various_node.fn_expr;
    std::unique_ptr<std::vector<int>> pre80_arguments;

    if( set_function && Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_7_000_1) )
    {
        pre80_arguments = std::make_unique<std::vector<int>>();
        *pre80_arguments = { various_node.fn_expr[0], various_node.fn_expr[1], various_node.fn_expr[2] };

        bool symbol_used = ( various_node.fn_expr[2] != -1 );
        pre80_arguments->insert(pre80_arguments->begin() + ( symbol_used ? 2 : 1 ), (int)DataType::String);

        arguments = pre80_arguments->data();
    }

    size_t argument_counter = set_function ? 3 : 1;

    if( arguments[argument_counter] == -1 )
        --argument_counter;


    // process the value
    if( set_function )
    {
        ASSERT(out_value != nullptr);

        if( arguments[argument_counter - 1] == (int)DataType::String )
        {
            *out_value = EvalAlphaExpr<CString>(arguments[argument_counter]);
            std::get<CString>(*out_value).Trim();
        }

        else
        {
            ASSERT(arguments[argument_counter - 1] == (int)DataType::Numeric);
            *out_value = evalexpr(arguments[argument_counter]);
        }

        argument_counter -= 2;
    }


    // process the property
    CString property = EvalAlphaExpr<CString>(arguments[argument_counter--]);

    ParameterManager::Parameter parameter = ParameterManager::Parse(FNGETPROPERTY_CODE, property);

    if( parameter == ParameterManager::Parameter::Invalid )
    {
        issaerror(MessageType::Error, 1100, property.GetString());
        throw std::exception();
    }

    if( set_function && ParameterManager::Parse(FNSETPROPERTY_CODE, property) == ParameterManager::Parameter::Invalid )
    {
        issaerror(MessageType::Error, 1102, property.GetString());
        throw std::exception();
    }


    // process the symbol
    Symbol* symbol = nullptr;

    if( argument_counter == 0 )
        symbol = NPT(arguments[0]);

    ParameterManager::ParameterArgument additional_argument = ParameterManager::GetAdditionalArgument(parameter);

    if( bool application_property = ( additional_argument == ParameterManager::ParameterArgument::ApplicationProperty );
        application_property || additional_argument == ParameterManager::ParameterArgument::SystemProperty )
    {
        if( symbol != nullptr )
        {
            issaerror(MessageType::Error, 1106, application_property ? _T("application") : _T("system"), property.GetString());
            throw std::exception();
        }
    }

    // an item or field property
    else 
    {
        if( symbol == nullptr )
        {
            issaerror(MessageType::Error, 1105, property.GetString());
            throw std::exception();
        }

        bool item_property = ( additional_argument == ParameterManager::ParameterArgument::ItemProperty );

        auto get_set_item_populator = [&](VART* pVarT) -> bool
        {
            if( pVarT->GetDictItem() != nullptr ) // don't add working variables
            {
                if( item_property || GetCDEFieldFromVART(pVarT) != nullptr )
                {
                    symbol_set->insert(pVarT->GetSymbolIndex());
                    return true;
                }
            }

            return false;
        };

        VariableWorker(GetSymbolTable(), symbol, get_set_item_populator);

        if( !set_function && symbol_set->size() != 1 )
        {
            issaerror(MessageType::Error, item_property ? 1103 : 1104, property.GetString());
            throw std::exception();
        }
    }

    return parameter;
}


CString PropertyValueToString(bool value)
{
    return value ? CSPRO_ARG_YES :
                   CSPRO_ARG_NO;
}

bool StringToPropertyValueBool(const std::variant<double, CString>& value)
{
    if( std::holds_alternative<double>(value) )
    {
        return ( std::get<double>(value) != 0 );
    }

    else if( std::get<CString>(value).CompareNoCase(CSPRO_ARG_YES) == 0 )
    {
        return true;
    }

    else if( std::get<CString>(value).CompareNoCase(CSPRO_ARG_NO) == 0 )
    {
        return false;
    }

    else
    {
        throw InvalidValueException(value);
    }
}


CString PropertyValueToString(int value)
{
    return IntToString(value);
}

int StringToPropertyValueInt(const std::variant<double, CString>& value)
{
    if( std::holds_alternative<double>(value) )
    {
        return (int)std::get<double>(value);
    }

    else
    {
        return _ttoi(std::get<CString>(value));
    }
}


const int PropertyForceOutOfRangeFlags = CANENTER_NOTAPPL | CANENTER_OUTOFRANGE;
const int PropertyValidationMethodFlags = PropertyForceOutOfRangeFlags | CANENTER_SET_VIA_VALIDATION_METHOD;
const int PropertyValidationMethodNoConfirmFlags =  CANENTER_NOTAPPL_NOCONFIRM | CANENTER_OUTOFRANGE_NOCONFIRM;

const TCHAR* const ARG_DEFAULT = _T("Default");
const TCHAR* const ARG_CUSTOM  = _T("Custom");

CString PropertyValueConfirmToString(TCHAR iBehavior, TCHAR iOn, TCHAR iOnNoConfirm)
{
    if( ( iBehavior & iOn ) != 0 )
    {
        return ( ( iBehavior & iOnNoConfirm ) != 0 ) ? CSPRO_ARG_NOCONFIRM : CSPRO_ARG_CONFIRM;
    }

    else
    {
        return CSPRO_ARG_NO;
    }
}

TCHAR StringToPropertyValueConfirm(const CString& value, TCHAR iBehavior, TCHAR iOn, TCHAR iOnNoConfirm)
{
    TCHAR new_behavior;

    if( value.CompareNoCase(CSPRO_ARG_CONFIRM) == 0 )
    {
        new_behavior = ( iBehavior | iOn & ~iOnNoConfirm );
    }

    else if( value.CompareNoCase(CSPRO_ARG_NOCONFIRM) == 0 )
    {
        new_behavior = ( iBehavior | iOn | iOnNoConfirm );
    }

    else if( value.CompareNoCase(CSPRO_ARG_NO) == 0 )
    {
        new_behavior = ( iBehavior & ~iOn );
    }

    else
    {
        throw InvalidValueException(value);
    }

    return ( new_behavior & ~CANENTER_SET_VIA_VALIDATION_METHOD );
}

CString PropertyValueValidationMethodToString(TCHAR iBehavior)
{
    // some checks when both are turned on
    if( ( iBehavior & PropertyValidationMethodFlags ) == PropertyValidationMethodFlags )
    {
        if( ( iBehavior & PropertyValidationMethodNoConfirmFlags ) == PropertyValidationMethodNoConfirmFlags  )
            return CSPRO_ARG_NOCONFIRM;

        if( ( iBehavior & PropertyValidationMethodNoConfirmFlags ) == 0 )
            return CSPRO_ARG_CONFIRM;
    }

    // 'Default' if none are turned on
    if( ( iBehavior & PropertyValidationMethodFlags ) == 0 )
        return ARG_DEFAULT;

    // 'Custom' if only one is turned or when a mismatch of the confirmation flags
    return ARG_CUSTOM;
}

TCHAR StringToPropertyValueValidationMethod(const CString& value, TCHAR iBehavior)
{
    if( value.CompareNoCase(CSPRO_ARG_NOCONFIRM) == 0 )
    {
        return ( iBehavior | PropertyValidationMethodFlags | PropertyValidationMethodNoConfirmFlags );
    }

    else if( value.CompareNoCase(CSPRO_ARG_CONFIRM) == 0 )
    {
        return ( ( iBehavior | PropertyValidationMethodFlags ) & ~PropertyValidationMethodNoConfirmFlags );
    }

    else if( value.CompareNoCase(ARG_DEFAULT) == 0 )
    {
        return ( iBehavior & ~( PropertyValidationMethodFlags | PropertyValidationMethodNoConfirmFlags ) );
    }

    else
    {
        throw InvalidValueException(value);
    }
}


CString PropertyValueToString(CaptureType capture_type)
{
    return CaptureInfo::GetCaptureTypeName(capture_type);
}

CaptureType StringToPropertyValueCaptureType(const CString& value)
{
    std::optional<CaptureType> capture_type = CaptureInfo::GetCaptureTypeFromSerializableName(value);

    if( capture_type.has_value() )
    {
        return *capture_type;
    }

    else
    {
        throw InvalidValueException(value);
    }
}


CString PropertyValueToString(CaseTreeType case_tree_type)
{
    switch( case_tree_type )
    {
        case CaseTreeType::Always:      return _T("Always");
        case CaseTreeType::MobileOnly:  return _T("Mobile");
        case CaseTreeType::DesktopOnly: return _T("Desktop");
        case CaseTreeType::Never:
        default:                        return _T("Never");
    }
}


CString PropertyValueToString(ParadataProperties::CollectionType collection_type)
{
    return ( collection_type == ParadataProperties::CollectionType::AllEvents )  ? _T("AllEvents") :
           ( collection_type == ParadataProperties::CollectionType::SomeEvents ) ? _T("SomeEvents") :
                                                                                   CSPRO_ARG_NO;
}


template<typename T> CString PropertyValueToString(T); // this will prevent any automatic casts


CString CIntDriver::GetProperty(ParameterManager::Parameter parameter, std::set<int>* symbol_set/* = nullptr*/)
{
    CString property;

    ParameterManager::ParameterArgument additional_argument = ParameterManager::GetAdditionalArgument(parameter);

    // application properties
    if( additional_argument == ParameterManager::ParameterArgument::ApplicationProperty )
    {
        ASSERT(symbol_set == nullptr || symbol_set->size() == 0);

        const Application* application = m_pEngineDriver->m_pPifFile->GetApplication();
        const auto& application_properties = application->GetApplicationProperties();

        switch( parameter )
        {
            case ParameterManager::Parameter::Property_AutoAdvanceOnSelection:
                property = PropertyValueToString(application->GetAutoAdvanceOnSelection());
                break;

            case ParameterManager::Parameter::Property_ComboBoxShowOnlyDiscreteValues:
                property = PropertyValueToString(application->GetComboBoxShowOnlyDiscreteValues());
                break;

            case ParameterManager::Parameter::Property_DisplayCodesAlongsideLabels:
                property = PropertyValueToString(application->GetDisplayCodesAlongsideLabels());
                break;

            case ParameterManager::Parameter::Property_NotesDeleteOtherOperators:
                property = PropertyValueToString(application->GetEditNotePermissions(EditNotePermissions::DeleteOtherOperators));
                break;

            case ParameterManager::Parameter::Property_NotesEditOtherOperators:
                property = PropertyValueToString(application->GetEditNotePermissions(EditNotePermissions::EditOtherOperators));
                break;

            case ParameterManager::Parameter::Property_PartialSave:
                property = PropertyValueToString(application->GetPartialSave());
                break;

            case ParameterManager::Parameter::Property_AutoPartialSaveMinutes:
                property = PropertyValueToString(application->GetAutoPartialSaveMinutes());
                break;

            case ParameterManager::Parameter::Property_ParadataRecordIteratorLoadCases:
                property = PropertyValueToString(application_properties.GetParadataProperties().GetRecordIteratorLoadCases());
                break;

            case ParameterManager::Parameter::Property_ParadataRecordValues:
                property = PropertyValueToString(application_properties.GetParadataProperties().GetRecordValues());
                break;

            case ParameterManager::Parameter::Property_ParadataRecordCoordinates:
                property = PropertyValueToString(application_properties.GetParadataProperties().GetRecordCoordinates());
                break;

            case ParameterManager::Parameter::Property_ParadataDeviceStateIntervalMinutes:
                property = PropertyValueToString(application_properties.GetParadataProperties().GetDeviceStateIntervalMinutes());
                break;

            case ParameterManager::Parameter::Property_ParadataGpsLocationIntervalMinutes:
                property = PropertyValueToString(application_properties.GetParadataProperties().GetGpsLocationIntervalMinutes());
                break;

            case ParameterManager::Parameter::Property_ShowEndCaseDialog:
                property = PropertyValueToString(application->GetShowEndCaseMessage());
                break;

            case ParameterManager::Parameter::Property_ShowErrorMessageNumbers:
                property = PropertyValueToString(application->GetShowErrorMessageNumbers());
                break;

            case ParameterManager::Parameter::Property_ShowRefusals:
                property = PropertyValueToString(application->GetShowRefusals());
                break;

            case ParameterManager::Parameter::Property_SpecialValuesZero:
                property = PropertyValueToString(m_pEngineSettings->GetTreatSpecialValuesAsZero());
                break;

            case ParameterManager::Parameter::Property_UpdateSaveArrayFile:
                property = PropertyValueToString(application->GetUpdateSaveArrayFile());
                break;

            case ParameterManager::Parameter::Property_UseHtmlComponentsInsteadOfNativeVersions:
                property = PropertyValueToString(application->GetApplicationProperties().GetUseHtmlComponentsInsteadOfNativeVersions());
                break;

            case ParameterManager::Parameter::Property_AppType:
            {
                if( application->GetEngineAppType() == EngineAppType::Entry )
                {
                    property = _T("DataEntry");
                }

                else
                {
                    property = ToString(application->GetEngineAppType());
                    property.SetAt(0, std::towupper(property[0]));
                }

                break;
            }

            case ParameterManager::Parameter::Property_CAPI:
                property = PropertyValueToString(application->GetUseQuestionText());
                break;

            case ParameterManager::Parameter::Property_CaseTree:
                property = PropertyValueToString(application->GetCaseTreeType());
                break;

            case ParameterManager::Parameter::Property_CenterForms:
                property = PropertyValueToString(application->GetCenterForms());
                break;

            case ParameterManager::Parameter::Property_CreateListing:
                property = PropertyValueToString(application->GetCreateListingFile());
                break;

            case ParameterManager::Parameter::Property_CreateLog:
                property = PropertyValueToString(application->GetCreateLogFile());
                break;

            case ParameterManager::Parameter::Property_DecimalComma:
                property = PropertyValueToString(application->GetDecimalMarkIsComma());
                break;

            case ParameterManager::Parameter::Property_OperatorID:
                property = PropertyValueToString(application->GetAskOperatorId());
                break;

            case ParameterManager::Parameter::Property_ParadataCollection:
                property = PropertyValueToString(application_properties.GetParadataProperties().GetCollectionType());
                break;

            case ParameterManager::Parameter::Property_Path:
                property = PropertyValueToString(m_pEngineSettings->IsPathOn());
                break;

            case ParameterManager::Parameter::Property_ShowFieldLabels:
                property = PropertyValueToString(application->GetShowFieldLabels());
                break;

            case ParameterManager::Parameter::Property_ShowLabelsInCaseTree:
            case ParameterManager::Parameter::Property_ShowNavigationControls:
            case ParameterManager::Parameter::Property_ShowSkippedFields:
            case ParameterManager::Parameter::Property_WindowTitle:
            {
#ifdef WIN_DESKTOP
                if( parameter == ParameterManager::Parameter::Property_WindowTitle )
                    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_WINDOW_TITLE_QUERY, true, (LPARAM)&property);
#else
                CString parameter_name = GetDisplayName(parameter);
                property = PlatformInterface::GetInstance()->GetApplicationInterface()->GetProperty(parameter_name);
#endif
                break;
            }

            default:
                ASSERT(false);
                throw std::exception();
        }
    }


    // item and field properties
    else if( additional_argument == ParameterManager::ParameterArgument::ItemProperty ||
             additional_argument == ParameterManager::ParameterArgument::FieldProperty )
    {
        ASSERT(symbol_set != nullptr && symbol_set->size() == 1);

        VART* pVarT = VPT(*(symbol_set->begin()));
        const CDictItem* pDictItem = pVarT->GetDictItem();
        CDEField* pField = nullptr;

        if( additional_argument == ParameterManager::ParameterArgument::FieldProperty )
        {
            pField = GetCDEFieldFromVART(pVarT);
            ASSERT(pField != nullptr);
        }

        switch( parameter )
        {
            case ParameterManager::Parameter::Property_AlwaysVisualValue:
                property = PropertyValueToString(pVarT->IsAlwaysVisualValue());
                break;

            case ParameterManager::Parameter::Property_CanEnterNotAppl:
                property = PropertyValueConfirmToString(pVarT->m_iBehavior, CANENTER_NOTAPPL, CANENTER_NOTAPPL_NOCONFIRM);
                break;

            case ParameterManager::Parameter::Property_CanEnterOutOfRange:
                property = PropertyValueConfirmToString(pVarT->m_iBehavior, CANENTER_OUTOFRANGE, CANENTER_OUTOFRANGE_NOCONFIRM);
                break;

            case ParameterManager::Parameter::Property_CapturePosX:
                property = PropertyValueToString((int)pVarT->GetCapturePos().x);
                break;

            case ParameterManager::Parameter::Property_CapturePosY:
                property = PropertyValueToString((int)pVarT->GetCapturePos().y);
                break;

            case ParameterManager::Parameter::Property_DataCaptureType:
            case ParameterManager::Parameter::Property_CaptureType:
            {
                // use the evaluated capture info so that Unspecified is never returned
                property = PropertyValueToString(pVarT->GetEvaluatedCaptureInfo().GetCaptureType());
                break;
            }

            case ParameterManager::Parameter::Property_CaptureDateFormat:
            {
                // use the evaluated capture info
                if( pVarT->GetEvaluatedCaptureInfo().GetCaptureType() == CaptureType::Date )
                    property = pVarT->GetEvaluatedCaptureInfo().GetExtended<DateCaptureInfo>().GetFormat();

                break;
            }

            case ParameterManager::Parameter::Property_ForceOutOfRange:
                property = PropertyValueToString(( pVarT->m_iBehavior & PropertyForceOutOfRangeFlags ) == PropertyForceOutOfRangeFlags);
                break;

            case ParameterManager::Parameter::Property_HideInCaseTree:
                property = PropertyValueToString(pField->IsHiddenInCaseTree());
                break;

            case ParameterManager::Parameter::Property_Keyboard:
#ifdef WIN_DESKTOP
                property = PropertyValueToString((int)m_pEngineDriver->GetKLIDFromHKL(pVarT->GetHKL()));
#endif
                break;

            case ParameterManager::Parameter::Property_Protected:
                property = PropertyValueToString(pField->IsProtected());
                break;

            case ParameterManager::Parameter::Property_ShowExtendedControlTitle:
                property = PropertyValueToString(pVarT->GetShowExtendedControlTitle());
                break;

            case ParameterManager::Parameter::Property_UpperCase:
                property = PropertyValueToString(pField->IsUpperCase());
                break;

            case ParameterManager::Parameter::Property_UseEnterKey:
                property = PropertyValueToString(pField->IsEnterKeyRequired());
                break;

            case ParameterManager::Parameter::Property_ValidationMethod:
                property = PropertyValueValidationMethodToString(pVarT->m_iBehavior);
                break;

            case ParameterManager::Parameter::Property_AllowMultiLine:
                property = PropertyValueToString(pField->AllowMultiLine());
                break;

            case ParameterManager::Parameter::Property_AutoIncrement:
                property = PropertyValueToString(pVarT->IsAutoIncrement());
                break;

            case ParameterManager::Parameter::Property_ContentType:
                property = ToString(pDictItem->GetContentType());
                break;

            case ParameterManager::Parameter::Property_Decimal:
                property = PropertyValueToString((int)pDictItem->GetDecimal());
                break;

            case ParameterManager::Parameter::Property_DecimalChar:
                property = PropertyValueToString(pDictItem->GetDecChar());
                break;

            case ParameterManager::Parameter::Property_Len:
                property = PropertyValueToString((int)pDictItem->GetLen());
                break;

            case ParameterManager::Parameter::Property_Persistent:
                property = PropertyValueToString(pVarT->IsPersistent());
                break;

            case ParameterManager::Parameter::Property_Sequential:
                property = PropertyValueToString(pVarT->IsSequential());
                break;

            case ParameterManager::Parameter::Property_SkipTo:
                property = pField->GetPlusTarget();
                break;

            case ParameterManager::Parameter::Property_UseUnicodeTextBox:
                property = PropertyValueToString(pField->UseUnicodeTextBox());
                break;

            case ParameterManager::Parameter::Property_Verify:
                property = PropertyValueToString(pField->GetVerifyFlag());
                break;

            case ParameterManager::Parameter::Property_ZeroFill:
                property = PropertyValueToString(pDictItem->GetZeroFill());
                break;

            default:
                ASSERT(false);
                throw std::exception();
        }
    }


    // system properties
    else
    {
        ASSERT(additional_argument == ParameterManager::ParameterArgument::SystemProperty);

        switch( parameter )
        {
            case ParameterManager::Parameter::Property_MaxDisplayWidth:
                property = PropertyValueToString((int)Screen::GetMaxDisplayWidth());
                break;

            case ParameterManager::Parameter::Property_MaxDisplayHeight:
                property = PropertyValueToString((int)Screen::GetMaxDisplayHeight());
                break;

            default:
                ASSERT(false);
                throw std::exception();
        }
    }

    return property;
}


double CIntDriver::exgetproperty(int iExpr)
{
    CString value;

    try
    {
        std::set<int> symbol_set;
        ParameterManager::Parameter parameter = GetSetPropertyParser(iExpr, &symbol_set);
        value = GetProperty(parameter, &symbol_set);
    }

    catch(...)
    {
        value = _T("<invalid property>");
    }

    return AssignAlphaValue(value);
}


double CIntDriver::exsetproperty(int iExpr)
{
    double properties_modified = 0;
    bool refresh_screen = false;
    CString parameter_name;

    try
    {
        std::set<int> symbol_set;
        std::variant<double, CString> value;
        ParameterManager::Parameter parameter = GetSetPropertyParser(iExpr, &symbol_set, &value);

        auto get_string_value = [&]() -> const CString&
        {
            if( !std::holds_alternative<CString>(value) )
                throw InvalidValueException(value, true);

            return std::get<CString>(value);
        };

        parameter_name = ParameterManager::GetDisplayName(parameter);
        ParameterManager::ParameterArgument additional_argument = ParameterManager::GetAdditionalArgument(parameter);

        if( additional_argument == ParameterManager::ParameterArgument::ApplicationProperty )
        {
            ASSERT(symbol_set.empty());

            Application* application = m_pEngineDriver->m_pPifFile->GetApplication();
            auto& application_properties = application->GetApplicationProperties();

            properties_modified = 1;

            switch( parameter )
            {
                case ParameterManager::Parameter::Property_AutoAdvanceOnSelection:
                    application->SetAutoAdvanceOnSelection(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_ComboBoxShowOnlyDiscreteValues:
                    application->SetComboBoxShowOnlyDiscreteValues(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_DisplayCodesAlongsideLabels:
                    application->SetDisplayCodesAlongsideLabels(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_NotesDeleteOtherOperators:
                    application->SetEditNotePermissions(EditNotePermissions::DeleteOtherOperators,StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_NotesEditOtherOperators:
                    application->SetEditNotePermissions(EditNotePermissions::EditOtherOperators,StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_PartialSave:
                    application->SetPartialSave(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_AutoPartialSaveMinutes:
                    application->SetAutoPartialSaveMinutes(StringToPropertyValueInt(value));
                    break;

                case ParameterManager::Parameter::Property_ParadataRecordIteratorLoadCases:
                    application_properties.GetParadataProperties().SetRecordIteratorLoadCases(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_ParadataRecordValues:
                    application_properties.GetParadataProperties().SetRecordValues(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_ParadataRecordCoordinates:
                    application_properties.GetParadataProperties().SetRecordCoordinates(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_ParadataDeviceStateIntervalMinutes:
                    application_properties.GetParadataProperties().SetDeviceStateIntervalMinutes(StringToPropertyValueInt(value));
                    break;

                case ParameterManager::Parameter::Property_ParadataGpsLocationIntervalMinutes:
                    application_properties.GetParadataProperties().SetGpsLocationIntervalMinutes(StringToPropertyValueInt(value));
                    Paradata::Logger::SendPortableMessage(Paradata::PortableMessage::UpdateBackgroundCollectionParameters);
                    break;

                case ParameterManager::Parameter::Property_ShowEndCaseDialog:
                    application->SetShowEndCaseMessage(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_ShowErrorMessageNumbers:
                    application->SetShowErrorMessageNumbers(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_ShowRefusals:
                    application->SetShowRefusals(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_SpecialValuesZero:
                    m_pEngineSettings->SetTreatSpecialValuesAsZero(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_UpdateSaveArrayFile:
                    application->SetUpdateSaveArrayFile(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_UseHtmlComponentsInsteadOfNativeVersions:
                    application->GetApplicationProperties().SetUseHtmlComponentsInsteadOfNativeVersions(StringToPropertyValueBool(value));
                    break;

                case ParameterManager::Parameter::Property_ShowLabelsInCaseTree:
                case ParameterManager::Parameter::Property_ShowNavigationControls:
                case ParameterManager::Parameter::Property_ShowSkippedFields:
                {
                    // make sure the value is valid
                    bool boolean_value = StringToPropertyValueBool(value);
#ifdef WIN_DESKTOP
                    UNREFERENCED_PARAMETER(boolean_value);
                    properties_modified = 0;
#else
                    PlatformInterface::GetInstance()->GetApplicationInterface()->SetProperty(
                        parameter_name, PropertyValueToString(boolean_value));
#endif
                    break;
                }

                case ParameterManager::Parameter::Property_WindowTitle:
                {
                    CString window_title = ValueToString(value);
#ifdef WIN_DESKTOP
                    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_WINDOW_TITLE_QUERY, false, (LPARAM)&window_title);
#else
                    PlatformInterface::GetInstance()->GetApplicationInterface()->SetProperty(parameter_name, window_title);
#endif
                    break;
                }

                default:
                    ASSERT(false);
                    throw std::exception();
            }

            if( properties_modified == 1 && Paradata::Logger::IsOpen() )
            {
                m_pParadataDriver->RegisterAndLogEvent(std::make_shared<Paradata::PropertyEvent>(
                    parameter_name, ValueToString(value), true));
            }
        }


        // item and field properties
        else if( additional_argument == ParameterManager::ParameterArgument::ItemProperty ||
                 additional_argument == ParameterManager::ParameterArgument::FieldProperty )
        {
            for( const auto& symbol_index : symbol_set )
            {
                VART* pVarT = VPT(symbol_index);
                CDEField* pField = nullptr;

                if( additional_argument == ParameterManager::ParameterArgument::FieldProperty )
                {
                    pField = GetCDEFieldFromVART(pVarT);
                    ASSERT(pField != nullptr);
                }

                // assume that the action will be taken
                bool success = true;

                switch( parameter )
                {
                    case ParameterManager::Parameter::Property_AlwaysVisualValue:
                        pVarT->SetAlwaysVisualValue(StringToPropertyValueBool(value));
                        break;

                    case ParameterManager::Parameter::Property_CanEnterNotAppl:
                        pVarT->m_iBehavior = StringToPropertyValueConfirm(get_string_value(),
                            pVarT->m_iBehavior, CANENTER_NOTAPPL, CANENTER_NOTAPPL_NOCONFIRM);
                        break;

                    case ParameterManager::Parameter::Property_CanEnterOutOfRange:
                        pVarT->m_iBehavior = StringToPropertyValueConfirm(get_string_value(),
                            pVarT->m_iBehavior, CANENTER_OUTOFRANGE, CANENTER_OUTOFRANGE_NOCONFIRM);
                        break;

                    case ParameterManager::Parameter::Property_CapturePosX:
                        pVarT->SetCapturePos({ StringToPropertyValueInt(value), pVarT->GetCapturePos().y });
                        break;

                    case ParameterManager::Parameter::Property_CapturePosY:
                        pVarT->SetCapturePos({ pVarT->GetCapturePos().x, StringToPropertyValueInt(value) });
                        break;

                    case ParameterManager::Parameter::Property_DataCaptureType:
                    case ParameterManager::Parameter::Property_CaptureType:
                    {
                        CaptureType new_capture_type = StringToPropertyValueCaptureType(get_string_value());
                        CaptureInfo new_capture_info = CaptureInfo(new_capture_type);
                        CaptureInfo valid_capture_info = new_capture_info.MakeValid(*pVarT->GetDictItem(), pVarT->GetCurrentDictValueSet());

                        if( valid_capture_info.GetCaptureType() == new_capture_type )
                        {
                            // only change the capture type if it is different (so type-specific settings like date formats aren't lost)
                            if( pVarT->GetCaptureInfo().GetCaptureType() != new_capture_type )
                                pVarT->SetCaptureInfo(valid_capture_info);
                        }

                        else
                        {
                            success = false;
                        }

                        break;
                    }

                    case ParameterManager::Parameter::Property_CaptureDateFormat:
                    {
                        if( pVarT->GetCaptureInfo().GetCaptureType() == CaptureType::Date )
                        {
                            CaptureInfo new_capture_info = pVarT->GetCaptureInfo();
                            new_capture_info.GetExtended<DateCaptureInfo>().SetFormat(get_string_value());

                            CaptureInfo valid_capture_info = new_capture_info.MakeValid(*pVarT->GetDictItem(), pVarT->GetCurrentDictValueSet());

                            if( new_capture_info == valid_capture_info )
                            {
                                pVarT->SetCaptureInfo(new_capture_info);
                            }

                            else
                            {
                                success = false;
                            }
                        }

                        else
                        {
                            success = false;
                        }

                        break;
                    }

                    case ParameterManager::Parameter::Property_ForceOutOfRange:
                    {
                        if( StringToPropertyValueBool(value) )
                        {
                            pVarT->m_iBehavior |= PropertyForceOutOfRangeFlags;
                        }

                        else
                        {
                            pVarT->m_iBehavior &= ~PropertyForceOutOfRangeFlags;
                        }

                        pVarT->m_iBehavior &= ~CANENTER_SET_VIA_VALIDATION_METHOD;

                        break;
                    }

                    case ParameterManager::Parameter::Property_HideInCaseTree:
                        pField->IsHiddenInCaseTree(StringToPropertyValueBool(value));
                        break;

                    case ParameterManager::Parameter::Property_Keyboard:
#ifdef WIN_DESKTOP
                        pVarT->SetHKL(m_pEngineDriver->LoadKLID(StringToPropertyValueInt(value)));
#endif
                        break;

                    case ParameterManager::Parameter::Property_Protected:
                    {
                        bool bProtected = StringToPropertyValueBool(value);
                        pField->IsProtected(bProtected);
                        pVarT->SetBehavior(bProtected ? AsProtected : pField->IsEnterKeyRequired() ? AsEnter : AsAutoSkip);
                        refresh_screen = true;
                        break;
                    }

                    case ParameterManager::Parameter::Property_ShowExtendedControlTitle:
                    {
                        pVarT->SetShowExtendedControlTitle(StringToPropertyValueBool(value));
                        break;
                    }

                    case ParameterManager::Parameter::Property_UpperCase:
                    {
                        if( pVarT->IsAlpha() )
                        {
                            pField->IsUpperCase(StringToPropertyValueBool(value));
                        }

                        else
                        {
                            success = false;
                        }

                        break;
                    }

                    case ParameterManager::Parameter::Property_UseEnterKey:
                        pField->IsEnterKeyRequired(StringToPropertyValueBool(value));
                        break;

                    case ParameterManager::Parameter::Property_ValidationMethod:
                        pVarT->m_iBehavior = StringToPropertyValueValidationMethod(get_string_value(), pVarT->m_iBehavior);
                        break;

                    default:
                        ASSERT(false);
                        throw std::exception();
                }

                if( success )
                {
                    ++properties_modified;

                    if( Paradata::Logger::IsOpen() )
                    {
                        m_pParadataDriver->RegisterAndLogEvent(std::make_shared<Paradata::PropertyEvent>(
                            parameter_name, ValueToString(value), true, m_pParadataDriver->CreateObject(*pVarT)));
                    }
                }
            }
        }


        // system properties (are not settable)
        else
        {
            ASSERT(additional_argument == ParameterManager::ParameterArgument::SystemProperty);
            throw std::exception();
        }
    }

    catch( const InvalidValueException& exception ) // an invalid value
    {
        issaerror(MessageType::Error, exception.error_number, parameter_name.GetString(), ValueToString(exception.value).GetString());
        properties_modified = DEFAULT;
    }

    catch(...)
    {
        properties_modified = DEFAULT;
    }

    if( refresh_screen )
        frm_capimode(0, 1);

    return properties_modified;
}


void ParadataDriver::LogProperties()
{
    for( const auto& parameter : ParameterManager::GetParametersOfArgument(ParameterManager::ParameterArgument::ApplicationProperty) )
    {
        CString value = m_pIntDriver->GetProperty(parameter);

        if( !value.IsEmpty() )
        {
            RegisterAndLogEvent(std::make_shared<Paradata::PropertyEvent>(
                ParameterManager::GetDisplayName(parameter), value, false));
        }
    }
}


double CIntDriver::exprotect(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    Symbol& symbol = NPT_Ref(va_node.arguments[0]);
    bool protect = ConditionalValueIsTrue(evalexpr(va_node.arguments[1]));

    auto variable_protect_setter = [&](VART* pVarT) -> bool
    {
        CDEField* pField = GetCDEFieldFromVART(pVarT);

        if( pField != nullptr )
        {
            pField->IsProtected(protect);
            pVarT->SetBehavior(protect ? AsProtected : pField->IsEnterKeyRequired() ? AsEnter : AsAutoSkip);

            if( Paradata::Logger::IsOpen() )
            {
                m_pParadataDriver->RegisterAndLogEvent(std::make_unique<Paradata::PropertyEvent>(
                    ParameterManager::GetDisplayName(ParameterManager::Parameter::Property_Protected),
                    PropertyValueToString(protect), true, m_pParadataDriver->CreateObject(*pVarT)));
            }

            return true;
        }

        return false;
    };

    int fields_processed = VariableWorker(GetSymbolTable(), &symbol, variable_protect_setter);

    if( fields_processed > 0 )
        frm_capimode(0, 1);

    return fields_processed;
}
