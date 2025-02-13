#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::CSPro
{
    constexpr unsigned UpdateApplicationExternalities   = UWM::Ranges::ExeStart + 0;
    constexpr unsigned CreateUniqueName                 = UWM::Ranges::ExeStart + 1;
    constexpr unsigned SetExternalApplicationProperties = UWM::Ranges::ExeStart + 2;

    CHECK_MESSAGE_NUMBERING(SetExternalApplicationProperties, UWM::Ranges::ExeLast)
}
