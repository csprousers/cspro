#pragma once

#include <zToolsO/CaseInsensitiveComparer.h>


struct LexerStyle
{
    static constexpr COLORREF NoOverride = 0xFF000000;

    COLORREF foreground_color;
    COLORREF background_color = NoOverride;
    bool bold = false;
    bool italic = false;
};


class LexerProperties
{
public:
    struct Properties
    {
        std::map<char, LexerStyle> styles;
        std::vector<std::string> keywords;
        std::unique_ptr<std::map<std::string, const char*, cs::case_insensitive_less>> logic_tooltips;
    };

    static const Properties& GetProperties(int lexer_language);

    static const std::map<char, LexerStyle>& GetStyles(int lexer_language);
    static const std::vector<std::string>& GetKeywords(int lexer_language);
    static const std::map<std::string, const char*, cs::case_insensitive_less>* GetLogicTooltips(int lexer_language);

private:
    static std::vector<std::tuple<int, LexerStyle>> GetStylesWorker(int lexer_language);
    static std::vector<std::tuple<int, LexerStyle>> GetExternalLanguageStylesWorker(int lexer_language);
    static std::vector<std::tuple<int, LexerStyle>> GetMarkdownStylesWorker();

    static void GetKeywordsAndLogicTooltipsWorker(Properties& properties, int lexer_language);

private:
    static std::map<int, Properties> m_propertiesMap;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline const std::map<char, LexerStyle>& LexerProperties::GetStyles(const int lexer_language)
{
    return GetProperties(lexer_language).styles;
}


inline const std::vector<std::string>& LexerProperties::GetKeywords(const int lexer_language)
{
    return GetProperties(lexer_language).keywords;
}


inline const std::map<std::string, const char*, cs::case_insensitive_less>* LexerProperties::GetLogicTooltips(const int lexer_language)
{
    return GetProperties(lexer_language).logic_tooltips.get();
}
