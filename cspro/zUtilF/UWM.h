#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::UtilF
{
    constexpr unsigned UpdateBatchMeterDlg          = UWM::Ranges::UtilFStart + 0;
    constexpr unsigned GetApplicationShutdownRunner = UWM::Ranges::UtilFStart + 1;
    constexpr unsigned CanAddResources              = UWM::Ranges::UtilFStart + 2;
    constexpr unsigned CopyToResourceDirectory      = UWM::Ranges::UtilFStart + 3;
    constexpr unsigned UpdateLoggingListBox         = UWM::Ranges::UtilFStart + 4;
    constexpr unsigned UpdateDialogUI               = UWM::Ranges::UtilFStart + 5;

    // unlike the above messages, the following message is only used within the project
    constexpr unsigned UpdateThreadedProgressDlg    = UWM::Ranges::UtilFStart + 6;

    CHECK_MESSAGE_NUMBERING(UpdateThreadedProgressDlg, UWM::Ranges::UtilFLast)
}
