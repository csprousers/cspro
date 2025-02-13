#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Order
{
    const unsigned ShowSourceCode       = UWM::Ranges::OrderStart + 0;
    const unsigned PutSourceCode        = UWM::Ranges::OrderStart + 1;
    const unsigned RunActiveApplication = UWM::Ranges::OrderStart + 2;
    const unsigned HasLogic             = UWM::Ranges::OrderStart + 3;

    CHECK_MESSAGE_NUMBERING(HasLogic, UWM::Ranges::OrderLast)
}
