#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/Token.h>
#include <zLogicO/ReservedWordsTable.h>

namespace Logic { struct KeywordDetails; }


struct Logic::KeywordDetails
{
    const char* const name;
    const char* const help_filename;
    TokenCode token_code;
};


namespace Logic::KeywordTable
{
    ZLOGICO_API const ReservedWordsTable<KeywordDetails>& GetKeywords();
    ZLOGICO_API bool IsKeyword(std::string_view text_sv, const KeywordDetails** keyword_details = nullptr);
    ZLOGICO_API std::optional<double> GetKeywordConstant(TokenCode token_code);
    ZLOGICO_API const char* GetKeywordName(TokenCode token_code);
};
