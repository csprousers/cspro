#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::CSCode
{
    // the following messages are only used within the project
    const unsigned CodeFrameActivate       = UWM::Ranges::ExeStart + 0;
    const unsigned SyncCodeFrameSplitter   = UWM::Ranges::ExeStart + 1;
    const unsigned SetStatusBarFileType    = UWM::Ranges::ExeStart + 2;
    const unsigned SyncToolbar             = UWM::Ranges::ExeStart + 3;
    const unsigned GetCodeTextForDoc       = UWM::Ranges::ExeStart + 4;
    const unsigned StartLocalhost          = UWM::Ranges::ExeStart + 5;
    const unsigned RunOperationComplete    = UWM::Ranges::ExeStart + 6;
    const unsigned ActiveDocChanged        = UWM::Ranges::ExeStart + 7;
    const unsigned ModelessDialogDestroyed = UWM::Ranges::ExeStart + 8;

    CHECK_MESSAGE_NUMBERING(ModelessDialogDestroyed, UWM::Ranges::ExeLast)
}
