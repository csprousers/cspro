#pragma once

#include <zToolsO/CaseInsensitiveComparer.h>


class LexerProperties
{
public:
    struct Properties
    {
        std::map<char, COLORREF> colors;
        std::vector<std::string> keywords;
        std::unique_ptr<std::map<std::string, const char*, cs::case_insensitive_less>> logic_tooltips;
    };

    static const Properties& GetProperties(int lexer_language);

    static const std::map<char, COLORREF>& GetColors(int lexer_language)
    {
        return GetProperties(lexer_language).colors;
    }

    static const std::vector<std::string>& GetKeywords(int lexer_language)
    {
        return GetProperties(lexer_language).keywords;
    }

    static const std::map<std::string, const char*, cs::case_insensitive_less>* GetLogicTooltips(int lexer_language)
    {
        return GetProperties(lexer_language).logic_tooltips.get();
    }

private:
    static std::vector<std::tuple<int, COLORREF>> GetColorsWorker(int lexer_language);
    static std::vector<std::tuple<int, COLORREF>> GetExternalLanguageColorsWorker(int lexer_language);

    static void GetKeywordsAndLogicTooltipsWorker(Properties& properties, int lexer_language);

private:
    static std::map<int, Properties> m_propertiesMap;
};
