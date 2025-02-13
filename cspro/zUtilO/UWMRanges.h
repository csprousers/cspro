#pragma once


namespace UWM::Ranges
{
    // zCapiO
    constexpr unsigned CapiStart       = WM_APP + 200;
    constexpr unsigned CapiLast        = CapiStart + 10;

    // zCaseTreeF
    constexpr unsigned CaseTreeStart   = CapiLast + 1;
    constexpr unsigned CaseTreeLast    = CaseTreeStart + 20;

    // zDesignerF
    constexpr unsigned DesignerStart   = CaseTreeLast + 1;
    constexpr unsigned DesignerLast    = DesignerStart + 20;

    // zDictF
    constexpr unsigned DictionaryStart = DesignerLast + 1;
    constexpr unsigned DictionaryLast  = DictionaryStart + 20;

    // zEdit2O
    constexpr unsigned EditStart       = DictionaryLast + 1;
    constexpr unsigned EditLast        = EditStart + 10;

    // zEngineF
    constexpr unsigned EngineStart     = EditLast + 1;
    constexpr unsigned EngineLast      = EngineStart + 10;

    // zFormF
    constexpr unsigned FormStart       = EngineLast + 1;
    constexpr unsigned FormLast        = FormStart + 30;

    // zFreqO
    constexpr unsigned FreqStart       = FormLast + 1;
    constexpr unsigned FreqLast        = FreqStart + 5;

    // zHtml
    constexpr unsigned HtmlStart       = FreqLast + 1;
    constexpr unsigned HtmlLast        = HtmlStart + 10;

    // zInterfaceF
    constexpr unsigned InterfaceStart  = HtmlLast + 1;
    constexpr unsigned InterfaceLast   = InterfaceStart + 10;

    // zMapping
    constexpr unsigned MappingStart    = InterfaceLast + 1;
    constexpr unsigned MappingLast     = MappingStart + 5;

    // zOrderF
    constexpr unsigned OrderStart      = MappingLast + 1;
    constexpr unsigned OrderLast       = OrderStart + 10;

    // zSyncF
    constexpr unsigned SyncStart       = OrderLast + 1;
    constexpr unsigned SyncLast        = SyncStart + 5;

    // zTableF
    constexpr unsigned TableStart      = SyncLast + 1;
    constexpr unsigned TableLast       = TableStart + 20;

    // zToolsO
    constexpr unsigned ToolsOStart     = TableLast + 1;
    constexpr unsigned ToolsOLast      = ToolsOStart + 5;

    // zUtilF
    constexpr unsigned UtilFStart      = ToolsOLast + 1;
    constexpr unsigned UtilFLast       = UtilFStart + 10;

    // zUtilO
    constexpr unsigned UtilOStart      = UtilFLast + 1;
    constexpr unsigned UtilOLast       = UtilOStart + 5;

    // zUToolO
    constexpr unsigned UToolStart      = UtilOLast + 1;
    constexpr unsigned UToolLast       = UToolStart + 5;

    // executables
    constexpr unsigned ExeStart        = UToolLast + 1;
    constexpr unsigned ExeLast         = ExeStart + 30;


#ifdef _DEBUG
    #define CHECK_MESSAGE_NUMBERING(last_used_message, last_available_message) \
            inline void MessageNumberingCheck() { static_assert(last_used_message <= last_available_message); }
#else
    #define CHECK_MESSAGE_NUMBERING(last_used_message, last_available_message)
#endif
}
