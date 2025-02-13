#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::CaseTree
{
    const unsigned GoTo                         = UWM::Ranges::CaseTreeStart +  0;
    const unsigned RefreshCaseTree              = UWM::Ranges::CaseTreeStart +  1;
    const unsigned DeleteCaseTree               = UWM::Ranges::CaseTreeStart +  2;
    const unsigned CreateCaseTree               = UWM::Ranges::CaseTreeStart +  3;
    const unsigned GoToNode                     = UWM::Ranges::CaseTreeStart +  4;
    const unsigned Page1ChangeShowStatus        = UWM::Ranges::CaseTreeStart +  5;
    const unsigned RestoreEntryRunViewFocus     = UWM::Ranges::CaseTreeStart +  6;
    const unsigned TreeItemWithNothingToDo      = UWM::Ranges::CaseTreeStart +  7;
    const unsigned SelectTreeItem               = UWM::Ranges::CaseTreeStart +  8;
    const unsigned SelectTreeItemUnsuccessful   = UWM::Ranges::CaseTreeStart +  9;
    const unsigned UpdateWindowText             = UWM::Ranges::CaseTreeStart + 10;
    const unsigned CloseAllTreeLayers           = UWM::Ranges::CaseTreeStart + 11;
    const unsigned RestoreTree                  = UWM::Ranges::CaseTreeStart + 12;
    const unsigned CloseCurrentTreeLayer        = UWM::Ranges::CaseTreeStart + 13;
    const unsigned Refresh                      = UWM::Ranges::CaseTreeStart + 14;
    const unsigned UnknownKey                   = UWM::Ranges::CaseTreeStart + 15;
    const unsigned RecalcLayout                 = UWM::Ranges::CaseTreeStart + 16;
    const unsigned ShowWindow                   = UWM::Ranges::CaseTreeStart + 17;

    CHECK_MESSAGE_NUMBERING(ShowWindow, UWM::Ranges::CaseTreeLast)
}
