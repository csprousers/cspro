#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Data
{
    // the following messages are only used within the project
    const unsigned AdjustColumnWidthAndInvalidate = UWM::Ranges::DataStart + 0;
    const unsigned SelectionsChanged              = UWM::Ranges::DataStart + 1;
    const unsigned RequeryCaseSummaries           = UWM::Ranges::DataStart + 2;

    CHECK_MESSAGE_NUMBERING(RequeryCaseSummaries, UWM::Ranges::DataLast)
}
