#pragma once

#include <zLogicO/zLogicO.h>


namespace Logic
{
    class ZLOGICO_API StringEscaper
    {
    public:
        StringEscaper(bool escape_string_literals);

        std::wstring EscapeString(std::wstring text, bool use_verbatim_string_literals = false) const;
        std::wstring EscapeStringWithSplitNewlines(std::wstring text, bool use_verbatim_string_literals = false) const;

    private:
        std::wstring EscapeStringUsingVerbatimStringLiterals(std::wstring text) const;
        std::wstring EscapeStringForOldLogic(std::wstring text) const;

    private:
        bool m_escapeStringLiterals;
    };
}
