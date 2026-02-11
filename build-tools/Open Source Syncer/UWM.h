#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::OpenSourceSyncer
{
    // the following messages are only used within the project
    constexpr unsigned UpdateUI            = UWM::Ranges::ExeStart + 0;
    constexpr unsigned UpdateStatusBar     = UWM::Ranges::ExeStart + 1;
    constexpr unsigned OperationInitialize = UWM::Ranges::ExeStart + 2;
    constexpr unsigned OperationComplete   = UWM::Ranges::ExeStart + 3;

    CHECK_MESSAGE_NUMBERING(OperationComplete, UWM::Ranges::ExeLast)
}
