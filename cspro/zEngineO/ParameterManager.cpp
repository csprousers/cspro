#include "stdafx.h"
#include "ParameterManager.h"

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
        std::variant<const char*, const wchar_t*> parameter_text;
        int min_arguments;
        int max_arguments;
        ParameterArgument additional_argument;
    };


    const FMapping FunctionMappings[] =
    {
        { FunctionCode::FNDIAGNOSTICS_CODE, Parameter::Diagnostics_Version,             Parameter::Diagnostics_Md5 },
        { FunctionCode::FNSETPROPERTY_CODE, Parameter::Property_AutoAdvanceOnSelection, Parameter::Property_ValidationMethod },
        { FunctionCode::FNGETPROPERTY_CODE, Parameter::Property_AutoAdvanceOnSelection, Parameter::Property_MaxDisplayHeight },
    };


    const PMapping ParameterMappings[] =
    {
        // diagnostics
        { "version",          0, 0, ParameterArgument::Unused },
        { "version_detailed", 0, 0, ParameterArgument::Unused },
        { "releasedate",      0, 0, ParameterArgument::Unused },
        { "beta",             0, 0, ParameterArgument::Unused },
        { "serializer",       0, 0, ParameterArgument::Unused },
        { "md5",              1, 1, ParameterArgument::Unused },

        // settable/gettable properties
#define AddApplicationProperty(property_name) { property_name, 0, 0, ParameterArgument::ApplicationProperty }
#define AddItemProperty(property_name)        { property_name, 0, 0, ParameterArgument::ItemProperty }
#define AddFieldProperty(property_name)       { property_name, 0, 0, ParameterArgument::FieldProperty }
#define AddSystemProperty(property_name)      { property_name, 0, 0, ParameterArgument::SystemProperty }

        AddApplicationProperty("AutoAdvanceOnSelection"),
        AddApplicationProperty("ComboBoxShowOnlyDiscreteValues"),
        AddApplicationProperty("DisplayCodesAlongsideLabels"),
        AddApplicationProperty("NotesDeleteOtherOperators"),
        AddApplicationProperty("NotesEditOtherOperators"),
        AddApplicationProperty("PartialSave"),
        AddApplicationProperty("AutoPartialSaveMinutes"),
        AddApplicationProperty("ParadataRecordIteratorLoadCases"),
        AddApplicationProperty("ParadataRecordValues"),
        AddApplicationProperty("ParadataRecordCoordinates"),
        AddApplicationProperty("ParadataDeviceStateMinutes"),
        AddApplicationProperty("ParadataGpsLocationMinutes"),
        AddApplicationProperty("ShowEndCaseDialog"),
        AddApplicationProperty("ShowErrorMessageNumbers"),
        AddApplicationProperty("ShowLabelsInCaseTree"),
        AddApplicationProperty("ShowNavigationControls"),
        AddApplicationProperty("ShowSkippedFields"),
        AddApplicationProperty("ShowRefusals"),
        AddApplicationProperty("SpecialValuesZero"),
        AddApplicationProperty("UpdateSaveArrayFile"),
        AddApplicationProperty(JK::useHtmlComponentsInsteadOfNativeVersions),
        AddApplicationProperty("WindowTitle"),

        AddFieldProperty("AlwaysVisualValue"),
        AddFieldProperty("CanEnterNotAppl"),
        AddFieldProperty("CanEnterOutOfRange"),
        AddFieldProperty("CapturePosX"),
        AddFieldProperty("CapturePosY"),
        AddFieldProperty("DataCaptureType"),
        AddFieldProperty(CMD_CAPTURE_TYPE),
        AddFieldProperty(CMD_CAPTURE_TYPE_DATE),
        AddFieldProperty(FRM_CMD_FORCEORANGE),
        AddFieldProperty(FRM_CMD_HIDE_IN_CASETREE),
        AddFieldProperty(FRM_CMD_KEYBOARD_ID),
        AddFieldProperty(FRM_CMD_PROTECTED),
        AddFieldProperty("ShowExtendedControlTitle"),
        AddFieldProperty(FRM_CMD_UPPERCASE),
        AddFieldProperty(FRM_CMD_ENTERKEY),
        AddFieldProperty(FRM_CMD_VALIDATION_METHOD),

        // gettable properties
        AddApplicationProperty("AppType"),
        AddApplicationProperty("CAPI"),
        AddApplicationProperty("CaseTree"),
        AddApplicationProperty("CenterForms"),
        AddApplicationProperty("CreateListing"),
        AddApplicationProperty("CreateLog"),
        AddApplicationProperty("DecimalComma"),
        AddApplicationProperty("OperatorID"),
        AddApplicationProperty("ParadataCollection"),
        AddApplicationProperty("Path"),
        AddApplicationProperty("ShowFieldLabels"),

        AddFieldProperty(FRM_CMD_ALLOWMULTILINE),
        AddFieldProperty(FRM_CMD_AUTOINCREMENT),
        AddItemProperty("DataType"),
        AddItemProperty("Decimal"),
        AddItemProperty("DecimalChar"),
        AddItemProperty("Len"),
        AddFieldProperty(FRM_CMD_PERSISTENT),
        AddFieldProperty(FRM_CMD_SEQUENTIAL),
        AddFieldProperty(FRM_CMD_SKIPTO),
        AddFieldProperty(FRM_CMD_USEUNICODETEXTBOX),
        AddFieldProperty(FRM_CMD_VERIFY),
        AddItemProperty("ZeroFill"),

        // gettable system properties
        AddSystemProperty("MaxDisplayWidth"),
        AddSystemProperty("MaxDisplayHeight"),
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


Parameter ParameterManager::Parse(const FunctionCode function_code, const std::string_view parameter_text_sv,
                                  int* const min_arguments/* = nullptr*/, int* const max_arguments/* = nullptr*/)
{
    const PMapping* mapping;
    const PMapping* last_mapping;

    GetFirstLastParameterMapping(function_code, mapping, last_mapping);

    // find the parameter
    for( ; mapping <= last_mapping; ++mapping )
    {
        if( std::holds_alternative<const char*>(mapping->parameter_text)
            ? SO::EqualsNoCase(std::get<const char*>(mapping->parameter_text), parameter_text_sv)
            : SO::EqualsNoCase(std::get<const wchar_t*>(mapping->parameter_text), parameter_text_sv) )
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


const char* ParameterManager::GetDisplayName(const Parameter parameter)
{
    // restore the commented-out line when all the parameters are non-wide
    // return ParameterMappings[static_cast<size_t>(parameter)].parameter_text;
    const PMapping& mapping = ParameterMappings[static_cast<size_t>(parameter)];
    return std::holds_alternative<const char*>(mapping.parameter_text)
        ? std::get<const char*>(mapping.parameter_text)
        : UTF8_TODO::Create_Reference(std::get<const wchar_t*>(mapping.parameter_text)).c_str();
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
