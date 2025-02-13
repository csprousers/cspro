#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/Token.h>
#include <zLogicO/ReservedWordsTable.h>

namespace Logic
{
    struct KeywordDetails
    {
        const TCHAR* const name;
        const TCHAR* const help_filename;
        TokenCode token_code;
    };

    namespace KeywordTable
    {
        ZLOGICO_API const ReservedWordsTable<KeywordDetails>& GetKeywords();
        ZLOGICO_API bool IsKeyword(wstring_view text_sv, const KeywordDetails** keyword_details = nullptr);
        ZLOGICO_API std::optional<double> GetKeywordConstant(TokenCode token_code);
        ZLOGICO_API const TCHAR* GetKeywordName(TokenCode token_code);
    };
}
