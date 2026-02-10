#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::OpenSourceSyncer
{
    // the following messages are only used within the project
    constexpr unsigned OperationComplete = UWM::Ranges::ExeStart + 0;

    CHECK_MESSAGE_NUMBERING(OperationComplete, UWM::Ranges::ExeLast)
}
