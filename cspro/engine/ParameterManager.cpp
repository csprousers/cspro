#include "StandardSystemIncludes.h"
#include "ParameterManager.h"
#include <zJson/JsonKeys.h>
#include <zUtilO/Interapp.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zFormO/FormFile.h>

using namespace ParameterManager;


namespace
{
    struct FMapping
    {
        FunctionCode function_code;
        Parameter starting_parameter;
        Parameter end_parameter;
    };


    struct PMapping
    {
        const TCHAR* const parameter_text;
        int min_arguments;
        int max_arguments;
        ParameterArgument additional_argument;
    };


    const FMapping FunctionMappings[] =
    {
        { FNDIAGNOSTICS_CODE, Parameter::Diagnostics_Version,             Parameter::Diagnostics_Md5 },
        { FNSETPROPERTY_CODE, Parameter::Property_AutoAdvanceOnSelection, Parameter::Property_ValidationMethod },
        { FNGETPROPERTY_CODE, Parameter::Property_AutoAdvanceOnSelection, Parameter::Property_MaxDisplayHeight },
    };


    const PMapping ParameterMappings[] =
    {
        // diagnostics
        { _T("version"),          0, 0, ParameterArgument::Unused },
        { _T("version_detailed"), 0, 0, ParameterArgument::Unused },
        { _T("releasedate"),      0, 0, ParameterArgument::Unused },
        { _T("beta"),             0, 0, ParameterArgument::Unused },
        { _T("serializer"),       0, 0, ParameterArgument::Unused },
        { _T("md5"),              1, 1, ParameterArgument::Unused },

        // settable/gettable properties
#define AddApplicationProperty(property_name) { property_name, 0, 0, ParameterArgument::ApplicationProperty }
#define AddItemProperty(property_name)        { property_name, 0, 0, ParameterArgument::ItemProperty }
#define AddFieldProperty(property_name)       { property_name, 0, 0, ParameterArgument::FieldProperty }
#define AddSystemProperty(property_name)      { property_name, 0, 0, ParameterArgument::SystemProperty }

        AddApplicationProperty(_T("AutoAdvanceOnSelection")),
        AddApplicationProperty(_T("ComboBoxShowOnlyDiscreteValues")),
        AddApplicationProperty(_T("DisplayCodesAlongsideLabels")),
        AddApplicationProperty(_T("NotesDeleteOtherOperators")),
        AddApplicationProperty(_T("NotesEditOtherOperators")),
        AddApplicationProperty(_T("PartialSave")),
        AddApplicationProperty(_T("AutoPartialSaveMinutes")),
        AddApplicationProperty(_T("ParadataRecordIteratorLoadCases")),
        AddApplicationProperty(_T("ParadataRecordValues")),
        AddApplicationProperty(_T("ParadataRecordCoordinates")),
        AddApplicationProperty(_T("ParadataDeviceStateMinutes")),
        AddApplicationProperty(_T("ParadataGpsLocationMinutes")),
        AddApplicationProperty(_T("ShowEndCaseDialog")),
        AddApplicationProperty(_T("ShowErrorMessageNumbers")),
        AddApplicationProperty(_T("ShowLabelsInCaseTree")),
        AddApplicationProperty(_T("ShowNavigationControls")),
        AddApplicationProperty(_T("ShowSkippedFields")),
        AddApplicationProperty(_T("ShowRefusals")),
        AddApplicationProperty(_T("SpecialValuesZero")),
        AddApplicationProperty(_T("UpdateSaveArrayFile")),
        AddApplicationProperty(JK::useHtmlComponentsInsteadOfNativeVersions),
        AddApplicationProperty(_T("WindowTitle")),

        AddFieldProperty(_T("AlwaysVisualValue")),
        AddFieldProperty(_T("CanEnterNotAppl")),
        AddFieldProperty(_T("CanEnterOutOfRange")),
        AddFieldProperty(_T("CapturePosX")),
        AddFieldProperty(_T("CapturePosY")),
        AddFieldProperty(_T("DataCaptureType")),
        AddFieldProperty(CMD_CAPTURE_TYPE),
        AddFieldProperty(CMD_CAPTURE_TYPE_DATE),
        AddFieldProperty(FRM_CMD_FORCEORANGE),
        AddFieldProperty(FRM_CMD_HIDE_IN_CASETREE),
        AddFieldProperty(FRM_CMD_KEYBOARD_ID),
        AddFieldProperty(FRM_CMD_PROTECTED),
        AddFieldProperty(_T("ShowExtendedControlTitle")),
        AddFieldProperty(FRM_CMD_UPPERCASE),
        AddFieldProperty(FRM_CMD_ENTERKEY),
        AddFieldProperty(FRM_CMD_VALIDATION_METHOD),

        // gettable properties
        AddApplicationProperty(_T("AppType")),
        AddApplicationProperty(_T("CAPI")),
        AddApplicationProperty(_T("CaseTree")),
        AddApplicationProperty(_T("CenterForms")),
        AddApplicationProperty(_T("CreateListing")),
        AddApplicationProperty(_T("CreateLog")),
        AddApplicationProperty(_T("DecimalComma")),
        AddApplicationProperty(_T("OperatorID")),
        AddApplicationProperty(_T("ParadataCollection")),
        AddApplicationProperty(_T("Path")),
        AddApplicationProperty(_T("ShowFieldLabels")),

        AddFieldProperty(FRM_CMD_ALLOWMULTILINE),
        AddFieldProperty(FRM_CMD_AUTOINCREMENT),
        AddItemProperty(_T("DataType")),
        AddItemProperty(_T("Decimal")),
        AddItemProperty(_T("DecimalChar")),
        AddItemProperty(_T("Len")),
        AddFieldProperty(FRM_CMD_PERSISTENT),
        AddFieldProperty(FRM_CMD_SEQUENTIAL),
        AddFieldProperty(FRM_CMD_SKIPTO),
        AddFieldProperty(FRM_CMD_USEUNICODETEXTBOX),
        AddFieldProperty(FRM_CMD_VERIFY),
        AddItemProperty(_T("ZeroFill")),

        // gettable system properties
        AddSystemProperty(_T("MaxDisplayWidth")),
        AddSystemProperty(_T("MaxDisplayHeight")),
    };

    static_assert(_countof(ParameterMappings) == ( static_cast<size_t>(Parameter::Property_MaxDisplayHeight) + 1 ));


    void GetFirstLastParameterMapping(const FunctionCode function_code, const PMapping*& first_mapping, const PMapping*& last_mapping)
    {
        // find the function code
        for( int i = 0; i < _countof(FunctionMappings); ++i )
        {
            if( FunctionMappings[i].function_code == function_code )
            {
                first_mapping = ParameterMappings + static_cast<int>(FunctionMappings[i].starting_parameter);
                last_mapping = ParameterMappings + static_cast<int>(FunctionMappings[i].end_parameter);
                return;
            }
        }

        ASSERT(false);
    }
}


Parameter ParameterManager::Parse(const FunctionCode function_code, wstring_view parameter_text_sv,
                                  int* const min_arguments/* = nullptr*/, int* const max_arguments/* = nullptr*/)
{
    const PMapping* mapping;
    const PMapping* last_mapping;

    GetFirstLastParameterMapping(function_code, mapping, last_mapping);

    // find the parameter
    for( ; mapping <= last_mapping; ++mapping )
    {
        if( SO::EqualsNoCase(mapping->parameter_text, parameter_text_sv) )
        {
            if( min_arguments != nullptr )
                *min_arguments = mapping->min_arguments;

            if( max_arguments != nullptr )
                *max_arguments = mapping->max_arguments;

            return static_cast<Parameter>(mapping - ParameterMappings);
        }
    }

    return Parameter::Invalid;
}


const TCHAR* ParameterManager::GetDisplayName(const Parameter parameter)
{
    return ParameterMappings[static_cast<size_t>(parameter)].parameter_text;
}


ParameterArgument ParameterManager::GetAdditionalArgument(const Parameter parameter)
{
    return ParameterMappings[static_cast<size_t>(parameter)].additional_argument;
}


std::vector<Parameter> ParameterManager::GetParametersOfArgument(const ParameterArgument parameter_argument)
{
    std::vector<Parameter> parameters;

    for( size_t i = 0; i < _countof(ParameterMappings); ++i )
    {
        if( ParameterMappings[i].additional_argument == parameter_argument )
            parameters.emplace_back(static_cast<Parameter>(i));
    }

    return parameters;
}
