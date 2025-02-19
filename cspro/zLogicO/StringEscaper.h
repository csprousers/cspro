#pragma once

#include <zLogicO/zLogicO.h>

namespace Logic { class StringEscaper; }


class ZLOGICO_API Logic::StringEscaper
{
public:
    StringEscaper(bool escape_string_literals);

    std::string EscapeString(std::string text, bool use_verbatim_string_literals = false) const;
    std::string EscapeStringWithSplitNewlines(std::string text, bool use_verbatim_string_literals = false) const;

private:
    std::string EscapeStringUsingVerbatimStringLiterals(std::string text) const;
    std::string EscapeStringForOldLogic(std::string text) const;

private:
    bool m_escapeStringLiterals;
};
