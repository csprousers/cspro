#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Mapping
{
    // the following messages are only used within the project
    const unsigned ExecuteJavaScript = UWM::Ranges::MappingStart + 0;
    const unsigned SaveSnapshot      = UWM::Ranges::MappingStart + 1;

    CHECK_MESSAGE_NUMBERING(SaveSnapshot, UWM::Ranges::MappingLast)
}
