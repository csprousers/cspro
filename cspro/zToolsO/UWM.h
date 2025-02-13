#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::ToolsO
{
    constexpr unsigned DisplayErrorMessage  = UWM::Ranges::ToolsOStart + 0;
    constexpr unsigned GetObjectTransporter = UWM::Ranges::ToolsOStart + 1;

    CHECK_MESSAGE_NUMBERING(GetObjectTransporter, UWM::Ranges::ToolsOLast)
}
