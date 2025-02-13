#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Table
{
    const unsigned ShowSourceCode           = UWM::Ranges::TableStart +  0;
    const unsigned PutSourceCode            = UWM::Ranges::TableStart +  1;
    const unsigned RunActiveApplication     = UWM::Ranges::TableStart +  2;
    const unsigned IsNameUnique             = UWM::Ranges::TableStart +  3;
    const unsigned UpdateTree               = UWM::Ranges::TableStart +  4;
    const unsigned ReconcileLinkObj         = UWM::Ranges::TableStart +  5;
    const unsigned PutTallyProc             = UWM::Ranges::TableStart +  6;
    const unsigned DeleteLogic              = UWM::Ranges::TableStart +  7;
    const unsigned CheckSyntax              = UWM::Ranges::TableStart +  8;
    const unsigned RenameProc               = UWM::Ranges::TableStart +  9;
    const unsigned ReplaceLevelProcForLevel = UWM::Ranges::TableStart + 10;
    const unsigned TableGridUpdated         = UWM::Ranges::TableStart + 11;

    // unlike the above messages, the following messages are only used within the project
    const unsigned Update                   = UWM::Ranges::TableStart + 12;
    const unsigned Zoom                     = UWM::Ranges::TableStart + 13;

    CHECK_MESSAGE_NUMBERING(Zoom, UWM::Ranges::TableLast)
}
