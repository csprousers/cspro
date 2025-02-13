#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::TextView
{
    const unsigned Search      = UWM::Ranges::ExeStart + 0;
    const unsigned SearchClose = UWM::Ranges::ExeStart + 1;
    const unsigned WaitCancel  = UWM::Ranges::ExeStart + 2;
    const unsigned ToggleRuler = UWM::Ranges::ExeStart + 3;

    CHECK_MESSAGE_NUMBERING(ToggleRuler, UWM::Ranges::ExeLast)
}
