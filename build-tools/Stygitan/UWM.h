#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Stygitan
{
    // the following messages are only used within the project
    const unsigned UpdateUI             = UWM::Ranges::ExeStart + 0;
    const unsigned ThreadStart          = UWM::Ranges::ExeStart + 1;
    const unsigned ThreadIsActive       = UWM::Ranges::ExeStart + 2;
    const unsigned ThreadPromptCancel   = UWM::Ranges::ExeStart + 3;
    const unsigned ThreadComplete       = UWM::Ranges::ExeStart + 4;

    CHECK_MESSAGE_NUMBERING(ThreadComplete, UWM::Ranges::ExeLast)
}
