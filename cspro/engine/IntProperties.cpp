#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "Engine.h"
#include "VariableIterator.h"
#include <zEngineO/ParameterManager.h>
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/Screen.h>
#include <zUtilF/KeyboardLoader.h>
#include <zAppO/Application.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zDictO/DDClass.h>
#include <zFormO/FormFile.h>
#include <zBridgeO/NPff.h>
#include <zParadataO/Logger.h>


namespace
{
    struct InvalidValueException : public CSProException
    {
        InvalidValueException(const std::variant<double, std::string>& value_, const bool type_error = false)
            :   CSProException("Invalid value"),
                value(value_),
                error_number(type_error ? 1109 : 1107)
        {
        }

        std::variant<double, std::string> value;
        int error_number;
    };

    std::string ValueToString(const std::variant<double, std::string>& value)
    {
        if( std::holds_alternative<double>(value) )
            return DoubleToString(std::get<double>(value));

        return std::get<std::string>(value);
    }
}


ParameterManager::Parameter CIntDriver::GetSetPropertyParser(const int program_index, std::set<int>& symbol_set,
                                                             std::variant<double, std::string>* const out_value/* = nullptr*/)
{
    const auto& various_node = GetNode<FNVARIOUS_NODE>(program_index);
    const bool set_function = ( various_node.fn_code == FunctionCode::FNSETPROPERTY_CODE );
    const int* const arguments = various_node.fn_expr;
    size_t argument_counter = set_function ? 3 : 1;

    if( arguments[argument_counter] == -1 )
        --argument_counter;

    // process the value
    if( set_function )
    {
        ASSERT(out_value != nullptr);

        const DataType data_type = static_cast<DataType>(arguments[argument_counter - 1]);
        *out_value = EvaluateVariant<std::string>(data_type, arguments[argument_counter]);

        if( data_type == DataType::String )
            SO::MakeTrim(std::get<std::string>(*out_value));

        argument_counter -= 2;
    }


    // process the property
    const SharableString property = EvaluateSharableString(arguments[argument_counter--]);

    const ParameterManager::Parameter parameter = ParameterManager::Parse(FunctionCode::FNGETPROPERTY_CODE, *property);

    if( parameter == ParameterManager::Parameter::Invalid )
    {
        issaerror(MessageType::Error, 1100, property->c_str());
        throw std::exception();
    }

    if( set_function && ParameterManager::Parse(FunctionCode::FNSETPROPERTY_CODE, *property) == ParameterManager::Parameter::Invalid )
    {
        issaerror(MessageType::Error, 1102, property->c_str());
        throw std::exception();
    }


    // process the symbol
    Symbol* symbol = nullptr;

    if( argument_counter == 0 )
        symbol = NPT(arguments[0]);

    const ParameterManager::ParameterArgument additional_argument = ParameterManager::GetAdditionalArgument(parameter);

    if( bool application_property = ( additional_argument == ParameterManager::ParameterArgument::ApplicationProperty );
        application_property || additional_argument == ParameterManager::ParameterArgument::SystemProperty )
    {
        if( symbol != nullptr )
        {
            issaerror(MessageType::Error, 1106, application_property ? "application" : "system", property->c_str());
            throw std::exception();
        }
    }

    // an item or field property
    else
    {
        if( symbol == nullptr )
        {
            issaerror(MessageType::Error, 1105, property->c_str());
            throw std::exception();
        }

        const bool item_property = ( additional_argument == ParameterManager::ParameterArgument::ItemProperty );

        auto get_set_item_populator = [&](VART& vart)
        {
            if( vart.GetDictItem() != nullptr ) // don't add working variables
            {
                if( item_property || GetCDEFieldFromVART(&vart) != nullptr )
                {
                    symbol_set.insert(vart.GetSymbolIndex());
                    return 1;
                }
            }

            return 0;
        };

        ForeachVariable(GetSymbolTable(), *symbol, get_set_item_populator);

        if( !set_function && symbol_set.size() != 1 )
        {
            issaerror(MessageType::Error, item_property ? 1103 : 1104, property->c_str());
            throw std::exception();
        }
    }

    return parameter;
}


std::string PropertyValueToString(const bool value)
{
    return value ? UTF8_TODO::GetUtf8(CSPRO_ARG_YES) :
                   UTF8_TODO::GetUtf8(CSPRO_ARG_NO);
}


bool StringToPropertyValueBool(const std::variant<double, std::string>& value)
{
    if( std::holds_alternative<double>(value) )
    {
        return ( std::get<double>(value) != 0 );
    }

    else if( SO::EqualsNoCase(std::get<std::string>(value), CSPRO_ARG_YES) )
    {
        return true;
    }

    else if( SO::EqualsNoCase(std::get<std::string>(value), CSPRO_ARG_NO) )
    {
        return false;
    }

    else
    {
        throw InvalidValueException(value);
    }
}


std::string PropertyValueToString(const int value)
{
    return IntToString(value);
}


int StringToPropertyValueInt(const std::variant<double, std::string>& value)
{
    if( std::holds_alternative<double>(value) )
    {
        return static_cast<int>(std::get<double>(value));
    }

    else
    {
        try
        {
            return std::stoi(std::get<std::string>(value));
        }

        catch(...)
        {
            throw InvalidValueException(value);
        }
    }
}


std::string PropertyValueToString(const unsigned int value)
{
    return IntToString(value);
}


unsigned int StringToPropertyValueUnsignedInt(const std::variant<double, std::string>& value)
{
    if( std::holds_alternative<double>(value) )
    {
        return static_cast<unsigned int>(std::get<double>(value));
    }

    else
    {
        try
        {
            return std::stoul(std::get<std::string>(value));
        }

        catch(...)
        {
            throw InvalidValueException(value);
        }
    }
}


constexpr int PropertyForceOutOfRangeFlags = CANENTER_NOTAPPL | CANENTER_OUTOFRANGE;
constexpr int PropertyValidationMethodFlags = PropertyForceOutOfRangeFlags | CANENTER_SET_VIA_VALIDATION_METHOD;
constexpr int PropertyValidationMethodNoConfirmFlags =  CANENTER_NOTAPPL_NOCONFIRM | CANENTER_OUTOFRANGE_NOCONFIRM;

constexpr const char* const ARG_DEFAULT = "Default";
constexpr const char* const ARG_CUSTOM  = "Custom";


std::string PropertyValueConfirmToString(const TCHAR iBehavior, const TCHAR iOn, const TCHAR iOnNoConfirm)
{
    if( ( iBehavior & iOn ) != 0 )
    {
        return ( ( iBehavior & iOnNoConfirm ) != 0 ) ? UTF8_TODO::GetUtf8(CSPRO_ARG_NOCONFIRM) :
                                                       UTF8_TODO::GetUtf8(CSPRO_ARG_CONFIRM);
    }

    else
    {
        return UTF8_TODO::GetUtf8(CSPRO_ARG_NO);
    }
}


TCHAR StringToPropertyValueConfirm(const std::string& value, const TCHAR iBehavior, const TCHAR iOn, const TCHAR iOnNoConfirm)
{
    TCHAR new_behavior;

    if( SO::EqualsNoCase(value, CSPRO_ARG_CONFIRM) )
    {
        new_behavior = ( iBehavior | iOn & ~iOnNoConfirm );
    }

    else if( SO::EqualsNoCase(value, CSPRO_ARG_NOCONFIRM) )
    {
        new_behavior = ( iBehavior | iOn | iOnNoConfirm );
    }

    else if( SO::EqualsNoCase(value, CSPRO_ARG_NO) )
    {
        new_behavior = ( iBehavior & ~iOn );
    }

    else
    {
        throw InvalidValueException(value);
    }

    return ( new_behavior & ~CANENTER_SET_VIA_VALIDATION_METHOD );
}


std::string PropertyValueValidationMethodToString(const TCHAR iBehavior)
{
    // some checks when both are turned on
    if( ( iBehavior & PropertyValidationMethodFlags ) == PropertyValidationMethodFlags )
    {
        if( ( iBehavior & PropertyValidationMethodNoConfirmFlags ) == PropertyValidationMethodNoConfirmFlags  )
            return UTF8_TODO::GetUtf8(CSPRO_ARG_NOCONFIRM);

        if( ( iBehavior & PropertyValidationMethodNoConfirmFlags ) == 0 )
            return UTF8_TODO::GetUtf8(CSPRO_ARG_CONFIRM);
    }

    // 'Default' if none are turned on
    if( ( iBehavior & PropertyValidationMethodFlags ) == 0 )
        return ARG_DEFAULT;

    // 'Custom' if only one is turned or when a mismatch of the confirmation flags
    return ARG_CUSTOM;
}


TCHAR StringToPropertyValueValidationMethod(const std::string& value, const TCHAR iBehavior)
{
    if( SO::EqualsNoCase(value, CSPRO_ARG_NOCONFIRM) )
    {
        return ( iBehavior | PropertyValidationMethodFlags | PropertyValidationMethodNoConfirmFlags );
    }

    else if( SO::EqualsNoCase(value, CSPRO_ARG_CONFIRM) )
    {
        return ( ( iBehavior | PropertyValidationMethodFlags ) & ~PropertyValidationMethodNoConfirmFlags );
    }

    else if( SO::EqualsNoCase(value, ARG_DEFAULT) )
    {
        return ( iBehavior & ~( PropertyValidationMethodFlags | PropertyValidationMethodNoConfirmFlags ) );
    }

    else
    {
        throw InvalidValueException(value);
    }
}


std::string PropertyValueToString(const CaptureType capture_type)
{
    return CaptureInfo::GetCaptureTypeName(capture_type);
}


CaptureType StringToPropertyValueCaptureType(const std::string& value)
{
    const std::optional<CaptureType> capture_type = CaptureInfo::GetCaptureTypeFromSerializableName(value);

    if( capture_type.has_value() )
    {
        return *capture_type;
    }

    else
    {
        throw InvalidValueException(value);
    }
}


std::string PropertyValueToString(const CaseTreeType case_tree_type)
{
    switch( case_tree_type )
    {
        case CaseTreeType::Always:      return "Always";
        case CaseTreeType::MobileOnly:  return "Mobile";
        case CaseTreeType::DesktopOnly: return "Desktop";
        case CaseTreeType::Never:
        default:                        return "Never";
    }
}


std::string PropertyValueToString(const ParadataProperties::CollectionType collection_type)
{
    return ( collection_type == ParadataProperties::CollectionType::AllEvents )  ? "AllEvents" :
           ( collection_type == ParadataProperties::CollectionType::SomeEvents ) ? "SomeEvents" :
                                                                                   UTF8_TODO::GetUtf8(CSPRO_ARG_NO);
}


template<typename T> std::string PropertyValueToString(T); // this will prevent any automatic casts


std::string CIntDriver::GetProperty(const ParameterManager::Parameter parameter, std::set<int>* const symbol_set/* = nullptr*/)
{
    std::string property;

    const ParameterManager::ParameterArgument additional_argument = ParameterManager::GetAdditionalArgument(parameter);

    // application properties
    if( additional_argument == ParameterManager::ParameterArgument::ApplicationProperty )
    {
        ASSERT(symbol_set == nullptr || symbol_set->size() == 0);

        const Application* const application = m_pEngineDriver->m_pPifFile->GetApplication();
        const ApplicationProperties& application_properties = application->GetApplicationProperties();

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
                property = PropertyValueToString(m_engineData->engine_settings.GetTreatSpecialValuesAsZero());
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
                    property = "DataEntry";
                }

                else
                {
                    property = ToString(application->GetEngineAppType());
                    property.front() = static_cast<char>(std::toupper(property.front()));
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
                    WindowsDesktopMessage::Send(WM_IMSA_WINDOW_TITLE_QUERY, true, &property);
#else
                const std::string parameter_name = GetDisplayName(parameter);
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

        VART* const pVarT = VPT(*(symbol_set->begin()));
        const CDictItem* const pDictItem = pVarT->GetDictItem();
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
                property = PropertyValueToString(static_cast<int>(pVarT->GetCapturePos().x));
                break;

            case ParameterManager::Parameter::Property_CapturePosY:
                property = PropertyValueToString(static_cast<int>(pVarT->GetCapturePos().y));
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
                property = PropertyValueToString(pVarT->GetKeyboardLayoutId());
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
                property = PropertyValueToString(static_cast<int>(pDictItem->GetDecimal()));
                break;

            case ParameterManager::Parameter::Property_DecimalChar:
                property = PropertyValueToString(pDictItem->GetDecChar());
                break;

            case ParameterManager::Parameter::Property_Len:
                property = PropertyValueToString(static_cast<int>(pDictItem->GetLen()));
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
                property = PropertyValueToString(static_cast<int>(Screen::GetMaxDisplayWidth()));
                break;

            case ParameterManager::Parameter::Property_MaxDisplayHeight:
                property = PropertyValueToString(static_cast<int>(Screen::GetMaxDisplayHeight()));
                break;

            default:
                ASSERT(false);
                throw std::exception();
        }
    }

    return property;
}


double CIntDriver::ex_getproperty(const int program_index)
{
    try
    {
        std::set<int> symbol_set;
        const ParameterManager::Parameter parameter = GetSetPropertyParser(program_index, symbol_set);
        return AssignString(GetProperty(parameter, &symbol_set));
    }

    catch(...)
    {
        return AssignString("<invalid property>");
    }
}


double CIntDriver::ex_setproperty(const int program_index)
{
    std::optional<size_t> properties_modified = 0;
    bool refresh_screen = false;
    std::string parameter_name;

    try
    {
        std::set<int> symbol_set;
        std::variant<double, std::string> value;
        const ParameterManager::Parameter parameter = GetSetPropertyParser(program_index, symbol_set, &value);

        auto get_string_value = [&]() -> const std::string&
        {
            if( !std::holds_alternative<std::string>(value) )
                throw InvalidValueException(value, true);

            return std::get<std::string>(value);
        };

        parameter_name = ParameterManager::GetDisplayName(parameter);
        const ParameterManager::ParameterArgument additional_argument = ParameterManager::GetAdditionalArgument(parameter);

        if( additional_argument == ParameterManager::ParameterArgument::ApplicationProperty )
        {
            ASSERT(symbol_set.empty());

            Application* const application = m_pEngineDriver->m_pPifFile->GetApplication();
            ApplicationProperties& application_properties = application->GetApplicationProperties();

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
                    m_engineData->engine_settings.SetTreatSpecialValuesAsZero(StringToPropertyValueBool(value));
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
                    const bool boolean_value = StringToPropertyValueBool(value);
#ifdef WIN_DESKTOP
                    UNREFERENCED_PARAMETER(boolean_value);
                    properties_modified = 0;
#else
                    PlatformInterface::GetInstance()->GetApplicationInterface()->SetProperty(
                        parameter_name, PropertyValueToString(boolean_value)
                    );
#endif
                    break;
                }

                case ParameterManager::Parameter::Property_WindowTitle:
                {
                    const std::string window_title = ValueToString(value);
#ifdef WIN_DESKTOP
                    WindowsDesktopMessage::Send(WM_IMSA_WINDOW_TITLE_QUERY, false, &window_title);
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
                m_paradataDriver->RegisterAndLogEvent(std::make_unique<Paradata::PropertyEvent>(
                    parameter_name,
                    ValueToString(value),
                    true
                ));
            }
        }


        // item and field properties
        else if( additional_argument == ParameterManager::ParameterArgument::ItemProperty ||
                 additional_argument == ParameterManager::ParameterArgument::FieldProperty )
        {
            for( const int symbol_index : symbol_set )
            {
                VART* const pVarT = VPT(symbol_index);
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
                        const CaptureType new_capture_type = StringToPropertyValueCaptureType(get_string_value());
                        const CaptureInfo new_capture_info = CaptureInfo(new_capture_type);
                        CaptureInfo valid_capture_info = new_capture_info.MakeValid(*pVarT->GetDictItem(), pVarT->GetCurrentDictValueSet());

                        if( valid_capture_info.GetCaptureType() == new_capture_type )
                        {
                            // only change the capture type if it is different (so type-specific settings like date formats aren't lost)
                            if( pVarT->GetCaptureInfo().GetCaptureType() != new_capture_type )
                                pVarT->SetCaptureInfo(std::move(valid_capture_info));
                        }

                        else
                        {
                            success = false;
                        }

                        break;
                    }

                    case ParameterManager::Parameter::Property_CaptureDateFormat:
                    {
                        success = ( pVarT->GetCaptureInfo().GetCaptureType() == CaptureType::Date );

                        if( success )
                        {
                            CaptureInfo new_capture_info = pVarT->GetCaptureInfo();
                            new_capture_info.GetExtended<DateCaptureInfo>().SetFormat(get_string_value());

                            const CaptureInfo valid_capture_info = new_capture_info.MakeValid(*pVarT->GetDictItem(), pVarT->GetCurrentDictValueSet());

                            if( new_capture_info == valid_capture_info )
                            {
                                pVarT->SetCaptureInfo(std::move(new_capture_info));
                            }

                            else
                            {
                                success = false;
                            }
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
                        pVarT->SetKeyboardLayoutId(m_keyboardLoader->GetKeyboardId(StringToPropertyValueUnsignedInt(value)));
                        break;

                    case ParameterManager::Parameter::Property_Protected:
                    {
                        const bool protect = StringToPropertyValueBool(value);
                        pField->IsProtected(protect);
                        pVarT->SetBehavior(protect ? AsProtected : pField->IsEnterKeyRequired() ? AsEnter : AsAutoSkip);
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
                    ++*properties_modified;

                    if( Paradata::Logger::IsOpen() )
                    {
                        m_paradataDriver->RegisterAndLogEvent(std::make_unique<Paradata::PropertyEvent>(
                            parameter_name,
                            ValueToString(value),
                            true,
                            m_paradataDriver->CreateObject(*pVarT)
                        ));
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
        issaerror(MessageType::Error, exception.error_number, parameter_name.c_str(), ValueToString(exception.value).c_str());
        properties_modified.reset();
    }

    catch(...)
    {
        properties_modified.reset();
    }

    if( refresh_screen )
        frm_capimode(0, 1);

    if( !properties_modified.has_value() )
        return DEFAULT;

    return static_cast<double>(*properties_modified);
}


void EngineParadataDriver::LogProperties()
{
    for( const ParameterManager::Parameter& parameter : ParameterManager::GetParametersOfArgument(ParameterManager::ParameterArgument::ApplicationProperty) )
    {
        std::string value = m_pIntDriver->GetProperty(parameter);

        if( !value.empty() )
        {
            RegisterAndLogEvent(std::make_unique<Paradata::PropertyEvent>(
                ParameterManager::GetDisplayName(parameter),
                std::move(value),
                false
            ));
        }
    }
}


double CIntDriver::ex_protect(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    Symbol& symbol = NPT_Ref(va_node.arguments[0]);
    const bool protect = EvaluateConditional(va_node.arguments[1]);

    auto variable_protect_setter = [&](VART& vart)
    {
        CDEField* const pField = GetCDEFieldFromVART(&vart);

        if( pField == nullptr )
            return 0;

        pField->IsProtected(protect);
        vart.SetBehavior(protect ? AsProtected : pField->IsEnterKeyRequired() ? AsEnter : AsAutoSkip);

        if( Paradata::Logger::IsOpen() )
        {
            m_paradataDriver->RegisterAndLogEvent(std::make_unique<Paradata::PropertyEvent>(
                ParameterManager::GetDisplayName(ParameterManager::Parameter::Property_Protected),
                PropertyValueToString(protect),
                true,
                m_paradataDriver->CreateObject(vart)
            ));
        }

        return 1;
    };

    const size_t fields_processed = ForeachVariable(GetSymbolTable(), symbol, variable_protect_setter);

    if( fields_processed != 0 )
        frm_capimode(0, 1);

    return static_cast<double>(fields_processed);
}
