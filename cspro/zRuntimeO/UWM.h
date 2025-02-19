#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Runtime
{
    // the following messages are only used within the project
    const unsigned RunUiThreadAction = UWM::Ranges::RuntimeStart + 0;
    const unsigned ProcessMessages   = UWM::Ranges::RuntimeStart + 1;
    const unsigned AllRuntimesClosed = UWM::Ranges::RuntimeStart + 2;

    CHECK_MESSAGE_NUMBERING(AllRuntimesClosed, UWM::Ranges::RuntimeLast)
}
