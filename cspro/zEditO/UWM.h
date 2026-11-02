#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Edit
{
    constexpr unsigned GetLexerLanguage = UWM::Ranges::EditStart + 0;
    constexpr unsigned RefreshLexer     = UWM::Ranges::EditStart + 1;
    constexpr unsigned AutoComplete     = UWM::Ranges::EditStart + 2;
    constexpr unsigned InsertProcName   = UWM::Ranges::EditStart + 3;

}

// RESOURCE_TODO ... use UWM scheme
constexpr unsigned ZEDITO_SEL_CHANGE              = UWM::Ranges::EditStart + 4;
constexpr unsigned ZEDITO_LOGIC_REFERENCE         = UWM::Ranges::EditStart + 5;
constexpr unsigned ZEDITO_LOGIC_REFERENCE_GOTO    = UWM::Ranges::EditStart + 6;
constexpr unsigned ZEDITO_LOGIC_REFERENCE_HELP    = UWM::Ranges::EditStart + 7;

namespace UWM::Edit
{
    CHECK_MESSAGE_NUMBERING(ZEDITO_LOGIC_REFERENCE_HELP, UWM::Ranges::EditLast)
}
