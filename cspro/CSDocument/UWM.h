#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::CSDocument
{
    // the following messages are only used within the project
    const unsigned SyncToolbarAndWindows            = UWM::Ranges::ExeStart + 0;
    const unsigned TextEditFrameActivate            = UWM::Ranges::ExeStart + 1;
    const unsigned GetOpenTextSourceEditables       = UWM::Ranges::ExeStart + 2;
    const unsigned GenerateDlgUpdateText            = UWM::Ranges::ExeStart + 3;
    const unsigned GenerateDlgTaskComplete          = UWM::Ranges::ExeStart + 4;
    const unsigned ShowHtmlAndRestoreScrollbarState = UWM::Ranges::ExeStart + 5;

    CHECK_MESSAGE_NUMBERING(ShowHtmlAndRestoreScrollbarState, UWM::Ranges::ExeLast)
}
