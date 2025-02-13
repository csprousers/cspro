#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Capi
{
    // the following messages are only used within the project
    const unsigned FinishSelectDialog  = UWM::Ranges::CapiStart + 0;
    const unsigned RefreshQuestionText = UWM::Ranges::CapiStart + 1;
    const unsigned GetWindowHeight     = UWM::Ranges::CapiStart + 2;
    const unsigned SetWindowHeight     = UWM::Ranges::CapiStart + 3;

    CHECK_MESSAGE_NUMBERING(SetWindowHeight, UWM::Ranges::CapiLast)
}
