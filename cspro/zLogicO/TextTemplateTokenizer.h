#pragma once

#include <zLogicO/zLogicO.h>

class LogicSettings;
namespace Logic { class BasicTokenCompiler; }


// --------------------------------------------------------------------------
// TextTemplateToken
// --------------------------------------------------------------------------

struct TextTemplateToken
{
    enum class Type { DirectText, DoubleTilde, TripleTilde, Logic };

    Type type;
    size_t section_line_number_start;
    std::string text;

    static constexpr std::tuple<const char*, const char*> GetDelimiters(Type type);
    static constexpr std::tuple<const char*, const char*> GetEscapedDelimiters(Type type);
};



// --------------------------------------------------------------------------
// TextTemplateTokenizer
// --------------------------------------------------------------------------

class ZLOGICO_API TextTemplateTokenizer
{
public:
    TextTemplateTokenizer(bool allow_logic_escapes);
    virtual ~TextTemplateTokenizer() { }

    bool Tokenize(std::string_view text_template_sv, const LogicSettings& logic_settings);

    const std::vector<TextTemplateToken>& GetTokens() const { return m_tokens; }

    bool IsOnlyDirectTextUsed() const;

    // Returns true if the direct text contains the specified text.
    bool DirectTextContains(char ch) const                  { return DirectTextContainsWorker(ch); }
    bool DirectTextContains(std::string_view text_sv) const { return DirectTextContainsWorker(text_sv); }

    // Replaces the fills and logic with temporary (unused) text, executes the callback function
    // to convert the the direct text, and then replaces the temporary text with the original fills and logic.
    std::string ConvertDirectText(const std::function<void(std::string& direct_text)>& conversion_function) const;

protected:
    virtual void OnErrorUnbalancedEscapes(size_t line_number) = 0;
    virtual void OnErrorTokenNotEnded(const TextTemplateToken& token) = 0;

private:
    template<typename T>
    bool DirectTextContainsWorker(const T& text) const;

private:
    bool m_allowLogicEscapes;
    std::vector<TextTemplateToken> m_tokens;
};



// --------------------------------------------------------------------------
// ErrorReportingTextTemplateTokenizer
// --------------------------------------------------------------------------

class ZLOGICO_API ErrorReportingTextTemplateTokenizer : public TextTemplateTokenizer
{
public:
    ErrorReportingTextTemplateTokenizer(Logic::BasicTokenCompiler& logic_compiler, bool allow_logic_escapes);

protected:
    void OnErrorUnbalancedEscapes(size_t line_number) override;
    void OnErrorTokenNotEnded(const TextTemplateToken& token) override;

private:
    Logic::BasicTokenCompiler& m_compiler;
};



// --------------------------------------------------------------------------
// ErrorSuppressingTextTemplateTokenizer
// --------------------------------------------------------------------------

class ErrorSuppressingTextTemplateTokenizer : public TextTemplateTokenizer
{
public:
    using TextTemplateTokenizer::TextTemplateTokenizer;

protected:
    void OnErrorUnbalancedEscapes(size_t /*line_number*/) override { }
    void OnErrorTokenNotEnded(const TextTemplateToken& /*token*/) override { }
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

constexpr std::tuple<const char*, const char*> TextTemplateToken::GetDelimiters(const TextTemplateToken::Type type)
{
    return ( type == TextTemplateToken::Type::DoubleTilde ) ? std::make_tuple("~~",  "~~") :
           ( type == TextTemplateToken::Type::TripleTilde ) ? std::make_tuple("~~~", "~~~") :
         /*( type == TextTemplateToken::Type::Logic ) */      std::make_tuple("<?",  "?>");
}


constexpr std::tuple<const char*, const char*> TextTemplateToken::GetEscapedDelimiters(const TextTemplateToken::Type type)
{
    // the tilde delimiters are returned as HTML entities so that we do not have to worry about escaping ~ for Markdown
    return ( type == TextTemplateToken::Type::DoubleTilde ) ? std::make_tuple("&#126;&#126;",       "&#126;&#126;") :
           ( type == TextTemplateToken::Type::TripleTilde ) ? std::make_tuple("&#126;&#126;&#126;", "&#126;&#126;&#126;") :
         /*( type == TextTemplateToken::Type::Logic ) */      std::make_tuple("&lt;?",              "?&gt;");
}
