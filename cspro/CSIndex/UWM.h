#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::CSIndex
{
    // the following message is only used within the project
    constexpr unsigned UpdateDialogUI = UWM::Ranges::ExeStart + 0;

    CHECK_MESSAGE_NUMBERING(UpdateDialogUI, UWM::Ranges::ExeLast)
}
