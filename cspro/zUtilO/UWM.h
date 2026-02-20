#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::UtilO
{
    constexpr unsigned RunOnUIThread            = UWM::Ranges::UtilOStart + 0;
    constexpr unsigned IsReservedWord           = UWM::Ranges::UtilOStart + 1;
    constexpr unsigned GetCodeText              = UWM::Ranges::UtilOStart + 2;
    constexpr unsigned GetSharedDictionaryConst = UWM::Ranges::UtilOStart + 3;

    // unlike the above messages, the following message is only used within the project
    constexpr unsigned ResizableDlgRestoreSize  = UWM::Ranges::UtilOStart + 4;

    CHECK_MESSAGE_NUMBERING(ResizableDlgRestoreSize, UWM::Ranges::UtilOLast)
}
