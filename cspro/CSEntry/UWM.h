#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::CSEntry
{
    const unsigned ChangeEdit                      = UWM::Ranges::ExeStart +  0;
    const unsigned ShowCapi                        = UWM::Ranges::ExeStart +  1;
    const unsigned ControlsSetWindowText           = UWM::Ranges::ExeStart +  2;
    const unsigned UsingOperatorControlledMessages = UWM::Ranges::ExeStart +  3;
    const unsigned PreprocessEngineMessage         = UWM::Ranges::ExeStart +  4;

    // unlike the above messages, the following messages are only used within the project
    const unsigned MoveToField                     = UWM::Ranges::ExeStart +  5;
    const unsigned AdvanceToEnd                    = UWM::Ranges::ExeStart +  6;
    const unsigned EndGroup                        = UWM::Ranges::ExeStart +  7;
    const unsigned EndLevel                        = UWM::Ranges::ExeStart +  8;
    const unsigned NextLevelOccurrence             = UWM::Ranges::ExeStart +  9;
    const unsigned PreviousPage                    = UWM::Ranges::ExeStart + 10;
    const unsigned NextPage                        = UWM::Ranges::ExeStart + 11;
    const unsigned SlashKey                        = UWM::Ranges::ExeStart + 12;
    const unsigned CheatKey                        = UWM::Ranges::ExeStart + 13;
    const unsigned PlusKey                         = UWM::Ranges::ExeStart + 14;
    const unsigned PreviousPersistent              = UWM::Ranges::ExeStart + 15;
    const unsigned InsertAfterOccurrence           = UWM::Ranges::ExeStart + 16;
    const unsigned RefreshSelected                 = UWM::Ranges::ExeStart + 17;
    const unsigned EndCustomMessage                = UWM::Ranges::ExeStart + 18;
    const unsigned AcceleratorKey                  = UWM::Ranges::ExeStart + 19;
    const unsigned RecalcLeftLayout                = UWM::Ranges::ExeStart + 20;
    const unsigned CaseTreeFocus                   = UWM::Ranges::ExeStart + 21;
    const unsigned SimulatedKeyDown                = UWM::Ranges::ExeStart + 22;

    CHECK_MESSAGE_NUMBERING(SimulatedKeyDown, UWM::Ranges::ExeLast)
}
