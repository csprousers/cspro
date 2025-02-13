#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Freq
{
    const unsigned GetUniverseAndWeight = UWM::Ranges::FreqStart + 0;

    CHECK_MESSAGE_NUMBERING(GetUniverseAndWeight, UWM::Ranges::FreqLast)
}
