#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Capi
{
    const unsigned GetWindowHeight     = UWM::Ranges::CapiStart + 0;
    const unsigned SetWindowHeight     = UWM::Ranges::CapiStart + 1;
    const unsigned AdjustCapturePos    = UWM::Ranges::CapiStart + 2;

    // unlike the above messages, the following messages are only used within the project
    const unsigned FinishSelectDialog  = UWM::Ranges::CapiStart + 3;
    const unsigned RefreshQuestionText = UWM::Ranges::CapiStart + 4;

    CHECK_MESSAGE_NUMBERING(RefreshQuestionText, UWM::Ranges::CapiLast)
}
