#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::UtilF
{
    const unsigned UpdateBatchMeterDlg          = UWM::Ranges::UtilFStart + 0;
    const unsigned RunOnUIThread                = UWM::Ranges::UtilFStart + 1;
    const unsigned GetApplicationShutdownRunner = UWM::Ranges::UtilFStart + 2;
	const unsigned CanAddResourceFolder         = UWM::Ranges::UtilFStart + 3;
	const unsigned CreateResourceFolder         = UWM::Ranges::UtilFStart + 4;
	const unsigned UpdateLoggingListBox         = UWM::Ranges::UtilFStart + 5;
	const unsigned UpdateDialogUI               = UWM::Ranges::UtilFStart + 6;

    // unlike the above messages, the following message is only used within the project
    const unsigned UpdateThreadedProgressDlg    = UWM::Ranges::UtilFStart + 7;

    CHECK_MESSAGE_NUMBERING(UpdateThreadedProgressDlg, UWM::Ranges::UtilFLast)
}
