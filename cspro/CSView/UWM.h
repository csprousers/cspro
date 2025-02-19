#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::CSView
{
    // the following messages are only used within the project
    const unsigned CloseDocument = UWM::Ranges::ExeStart + 0;

    CHECK_MESSAGE_NUMBERING(CloseDocument, UWM::Ranges::ExeLast)
}
