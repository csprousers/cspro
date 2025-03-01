#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::DataManager
{
    // the following messages are only used within the project
    const unsigned RunStartupActions                          = UWM::Ranges::ExeStart +  0;
    const unsigned UpdateDialogControls                       = UWM::Ranges::ExeStart +  1;
    const unsigned UpdateStatusBarCaseCount                   = UWM::Ranges::ExeStart +  2;
    const unsigned UpdateStatusBarSelectedCaseKeyPosition     = UWM::Ranges::ExeStart +  3;
    const unsigned ToggleFilters                              = UWM::Ranges::ExeStart +  4;
    const unsigned ShowDefaultPage                            = UWM::Ranges::ExeStart +  5;
    const unsigned ProcessConnectionStringParameters          = UWM::Ranges::ExeStart +  6;
    const unsigned ShowSelectedCases                          = UWM::Ranges::ExeStart +  7;
    const unsigned UpdateContentOnCaseListingSettingsChange   = UWM::Ranges::ExeStart +  8;
    const unsigned UpdateContentOnCaseListingSelectionsChange = UWM::Ranges::ExeStart +  9;
    const unsigned ProcessWebViewMessage                      = UWM::Ranges::ExeStart + 10;
    const unsigned TaskEvent                                  = UWM::Ranges::ExeStart + 11;
    const unsigned RunTaskFromCaseListing                     = UWM::Ranges::ExeStart + 12;
    const unsigned CaseConstructionReporterMessage            = UWM::Ranges::ExeStart + 13;

    CHECK_MESSAGE_NUMBERING(RunTaskFromCaseListing, UWM::Ranges::ExeLast)
}
