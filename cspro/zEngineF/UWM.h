#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Engine
{
    // the following message is only used within the project
    const unsigned UserbarEditReturnKey = UWM::Ranges::EngineStart + 0;

    CHECK_MESSAGE_NUMBERING(UserbarEditReturnKey, UWM::Ranges::EngineLast)
}
