#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Interface
{
    const unsigned SelectLanguage = UWM::Ranges::InterfaceStart + 0;

    // the following message is only used within the project
    const unsigned FocusOnUniverse = UWM::Ranges::InterfaceStart + 1;

    CHECK_MESSAGE_NUMBERING(FocusOnUniverse, UWM::Ranges::InterfaceLast)
}
