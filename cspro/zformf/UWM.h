#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Form
{
    const unsigned ShowSourceCode              = UWM::Ranges::FormStart +  0;
    const unsigned PutSourceCode               = UWM::Ranges::FormStart +  1;
    const unsigned RunActiveApplication        = UWM::Ranges::FormStart +  2;
    const unsigned RunActiveApplicationAsBatch = UWM::Ranges::FormStart +  3;
    const unsigned PublishApplication          = UWM::Ranges::FormStart +  4;
    const unsigned PublishAndDeployApplication = UWM::Ranges::FormStart +  5;
    const unsigned GetApplication              = UWM::Ranges::FormStart +  6;
    const unsigned HasLogic                    = UWM::Ranges::FormStart +  7;
    const unsigned HasQuestionText             = UWM::Ranges::FormStart +  8;
    const unsigned IsNameUnique                = UWM::Ranges::FormStart +  9;
    const unsigned UpdateStatusBar             = UWM::Ranges::FormStart + 10;
    const unsigned GetCapiLanguages            = UWM::Ranges::FormStart + 11;
    const unsigned UpdateCapiLanguages         = UWM::Ranges::FormStart + 12;
    const unsigned ShowCapiText                = UWM::Ranges::FormStart + 13;
    const unsigned CapiMacros                  = UWM::Ranges::FormStart + 14;

    CHECK_MESSAGE_NUMBERING(CapiMacros, UWM::Ranges::FormLast)
}
