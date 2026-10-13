#pragma once

#include <zUtilO/UWMRanges.h>
#include <zDesignerF/FrameType.h>
#include <zDesignerF/ViewType.h>


namespace UWM::Designer
{
    constexpr unsigned HideToolbar                     = UWM::Ranges::DesignerStart +  0;
    constexpr unsigned ShowToolbar                     = UWM::Ranges::DesignerStart +  1;
    constexpr unsigned SelectTab                       = UWM::Ranges::DesignerStart +  2;
    constexpr unsigned SwitchView                      = UWM::Ranges::DesignerStart +  3;
    constexpr unsigned GoToLogicError                  = UWM::Ranges::DesignerStart +  4;
    constexpr unsigned GetApplicationBeingLoaded       = UWM::Ranges::DesignerStart +  5;
    constexpr unsigned GetApplication                  = UWM::Ranges::DesignerStart +  6;
    constexpr unsigned GetFormFileOrDictionary         = UWM::Ranges::DesignerStart +  7;
    constexpr unsigned DisplayErrorMessage             = UWM::Ranges::DesignerStart +  8;
    constexpr unsigned RedrawPropertyGrid              = UWM::Ranges::DesignerStart +  9;
    constexpr unsigned GetDesignerIcon                 = UWM::Ranges::DesignerStart + 10;
    constexpr unsigned FindOpenTextSourceEditable      = UWM::Ranges::DesignerStart + 11;
    constexpr unsigned ShowFileProperties              = UWM::Ranges::DesignerStart + 12;
    constexpr unsigned GetDictionaryType               = UWM::Ranges::DesignerStart + 13;
    constexpr unsigned TreeSelectionChanged            = UWM::Ranges::DesignerStart + 14;
    constexpr unsigned GetCurrentLanguageName          = UWM::Ranges::DesignerStart + 15;
    constexpr unsigned CanCodeFileCompilationBeSkipped = UWM::Ranges::DesignerStart + 16;
    constexpr unsigned SetCodeFileSuccessfullyCompiled = UWM::Ranges::DesignerStart + 17;
    constexpr unsigned TokenizeLogic_V0                = UWM::Ranges::DesignerStart + 18;
    constexpr unsigned CreateCapiLogicCompiler         = UWM::Ranges::DesignerStart + 19;
    constexpr unsigned TEXT_ENCODING_TODO_UseNewSettings = UWM::Ranges::DesignerStart + 20;

    CHECK_MESSAGE_NUMBERING(TEXT_ENCODING_TODO_UseNewSettings, UWM::Ranges::DesignerLast)
}
