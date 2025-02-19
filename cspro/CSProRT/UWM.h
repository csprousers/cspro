#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::CSProRT
{
    // the following messages are only used within the project
    const unsigned CloseRuntime = UWM::Ranges::ExeStart + 0;

    CHECK_MESSAGE_NUMBERING(CloseRuntime, UWM::Ranges::ExeLast)
}
