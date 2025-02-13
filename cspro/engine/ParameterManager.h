#pragma once

#include <zLogicO/FunctionTable.h>


namespace ParameterManager
{
    enum class Parameter
    {
        Unassigned = -2,
        Invalid = -1,

        // diagnostics
        Diagnostics_Version,
        Diagnostics_VersionDetailed,
        Diagnostics_ReleaseDate,
        Diagnostics_Beta,
        Diagnostics_Serializer,
        Diagnostics_Md5,

        // settable/gettable properties
        Property_AutoAdvanceOnSelection,
        Property_ComboBoxShowOnlyDiscreteValues,
        Property_DisplayCodesAlongsideLabels,
        Property_NotesDeleteOtherOperators,
        Property_NotesEditOtherOperators,
        Property_PartialSave,
        Property_AutoPartialSaveMinutes,
        Property_ParadataRecordIteratorLoadCases,
        Property_ParadataRecordValues,
        Property_ParadataRecordCoordinates,
        Property_ParadataDeviceStateIntervalMinutes,
        Property_ParadataGpsLocationIntervalMinutes,
        Property_ShowEndCaseDialog,
        Property_ShowErrorMessageNumbers,
        Property_ShowLabelsInCaseTree,
        Property_ShowNavigationControls,
        Property_ShowSkippedFields,
        Property_ShowRefusals,
        Property_SpecialValuesZero,
        Property_UpdateSaveArrayFile,
        Property_UseHtmlComponentsInsteadOfNativeVersions,
        Property_WindowTitle,

        Property_AlwaysVisualValue,
        Property_CanEnterNotAppl,
        Property_CanEnterOutOfRange,
        Property_CapturePosX,
        Property_CapturePosY,
        Property_DataCaptureType, // allow this as it is the name used in the form file (prior to CSPro 7.7)
        Property_CaptureType,
        Property_CaptureDateFormat,
        Property_ForceOutOfRange,
        Property_HideInCaseTree,
        Property_Keyboard,
        Property_Protected,
        Property_ShowExtendedControlTitle,
        Property_UpperCase,
        Property_UseEnterKey,
        Property_ValidationMethod,

        // gettable properties
        Property_AppType,
        Property_CAPI,
        Property_CaseTree,
        Property_CenterForms,
        Property_CreateListing,
        Property_CreateLog,
        Property_DecimalComma,
        Property_OperatorID,
        Property_ParadataCollection,
        Property_Path,
        Property_ShowFieldLabels,

        Property_AllowMultiLine,
        Property_AutoIncrement,
        Property_ContentType,
        Property_Decimal,
        Property_DecimalChar,
        Property_Len,
        Property_Persistent,
        Property_Sequential,
        Property_SkipTo,
        Property_UseUnicodeTextBox,
        Property_Verify,
        Property_ZeroFill,

        // gettable system properties
        Property_MaxDisplayWidth,
        Property_MaxDisplayHeight,
    };

    enum class ParameterArgument
    {
        Unused,
        ApplicationProperty,
        ItemProperty,
        FieldProperty,
        SystemProperty,
    };

    Parameter Parse(FunctionCode function_code, wstring_view parameter_text_sv, int* min_arguments = nullptr, int* max_arguments = nullptr);
    const TCHAR* GetDisplayName(Parameter parameter);
    ParameterArgument GetAdditionalArgument(Parameter parameter);
    std::vector<Parameter> GetParametersOfArgument(ParameterArgument parameter_argument);
}
