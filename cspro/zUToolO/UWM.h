#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::UTool
{
    // the following messages are only used within the project
    const unsigned OX_APP_AFTERFLOAT_MSG = UWM::Ranges::UToolStart + 0;
    const unsigned ACTIVATEVIEWBAR       = UWM::Ranges::UToolStart + 1;

    CHECK_MESSAGE_NUMBERING(ACTIVATEVIEWBAR, UWM::Ranges::UToolLast)
}
