#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Stygitan
{
    // the following messages are only used within the project
    const unsigned AppActivated = UWM::Ranges::ExeStart + 0;
    const unsigned UpdateUI     = UWM::Ranges::ExeStart + 1;

    CHECK_MESSAGE_NUMBERING(AppActivated, UWM::Ranges::ExeLast)
}
