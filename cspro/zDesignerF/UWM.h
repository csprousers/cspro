#pragma once

#include <zUtilO/UWMRanges.h>
#include <zDesignerF/FrameType.h>
#include <zDesignerF/ViewType.h>


namespace UWM::Designer
{
    const unsigned HideToolbar                = UWM::Ranges::DesignerStart +  0;
    const unsigned ShowToolbar                = UWM::Ranges::DesignerStart +  1;
    const unsigned SelectTab                  = UWM::Ranges::DesignerStart +  2;
    const unsigned SwitchView                 = UWM::Ranges::DesignerStart +  3;
    const unsigned GoToLogicError             = UWM::Ranges::DesignerStart +  4;
    const unsigned GetMessageTextSource       = UWM::Ranges::DesignerStart +  5;
    const unsigned GetApplication             = UWM::Ranges::DesignerStart +  6;
    const unsigned GetFormFileOrDictionary    = UWM::Ranges::DesignerStart +  7;
    const unsigned DisplayErrorMessage        = UWM::Ranges::DesignerStart +  8;
    const unsigned RedrawPropertyGrid         = UWM::Ranges::DesignerStart +  9;
    const unsigned GetDesignerIcon            = UWM::Ranges::DesignerStart + 10;
    const unsigned FindOpenTextSourceEditable = UWM::Ranges::DesignerStart + 11;
    const unsigned EditReportProperties       = UWM::Ranges::DesignerStart + 12;
    const unsigned GetDictionaryType          = UWM::Ranges::DesignerStart + 13;
    const unsigned TreeSelectionChanged       = UWM::Ranges::DesignerStart + 14;
    const unsigned GetCurrentLanguageName     = UWM::Ranges::DesignerStart + 15;

    // unlike the above messages, the following messages are only used within the project
    const unsigned TabViewContainerTabChange  = UWM::Ranges::DesignerStart + 16;

    CHECK_MESSAGE_NUMBERING(TabViewContainerTabChange, UWM::Ranges::DesignerLast)
}
