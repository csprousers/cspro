#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Edit
{
    constexpr unsigned GetLexerLanguage         = UWM::Ranges::EditStart + 0;
    constexpr unsigned RefreshLexer             = UWM::Ranges::EditStart + 1;

    // unlike the above messages, the following messages are only used within the project
    constexpr unsigned UpdateStatusPaneCaretPos = UWM::Ranges::EditStart + 2;
}

// RESOURCE_TODO ... use UWM scheme
constexpr unsigned ZEDIT2O_SEL_CHANGE              = UWM::Ranges::EditStart + 3;
constexpr unsigned ZEDIT2O_LOGIC_REFERENCE         = UWM::Ranges::EditStart + 4;
constexpr unsigned ZEDIT2O_LOGIC_REFERENCE_GOTO    = UWM::Ranges::EditStart + 5;
constexpr unsigned ZEDIT2O_LOGIC_REFERENCE_HELP    = UWM::Ranges::EditStart + 6;
constexpr unsigned ZEDIT2O_LOGIC_AUTO_COMPLETE     = UWM::Ranges::EditStart + 7;
constexpr unsigned ZEDIT2O_LOGIC_INSERT_PROC_NAME  = UWM::Ranges::EditStart + 8;

namespace UWM::Edit
{
    CHECK_MESSAGE_NUMBERING(ZEDIT2O_LOGIC_INSERT_PROC_NAME, UWM::Ranges::EditLast)
}
