#pragma once

#include <zEngineO/zEngineO.h>

class LogicSettings;


struct TextTemplateToken
{
    enum class Type { DirectText, DoubleTilde, TripleTilde, Logic };

    Type type;
    size_t section_line_number_start;
    std::string text;
};


class ZENGINEO_API TextTemplateTokenizer
{
public:
    TextTemplateTokenizer(bool allow_logic_escapes);
    virtual ~TextTemplateTokenizer() { }

    bool Tokenize(std::string_view text_template_sv, const LogicSettings& logic_settings);

    const std::vector<TextTemplateToken>& GetTokens() const { return m_tokens; }

protected:
    virtual void OnErrorUnbalancedEscapes(size_t line_number) = 0;
    virtual void OnErrorTokenNotEnded(const TextTemplateToken& token) = 0;

private:
    bool m_allowLogicEscapes;
    std::vector<TextTemplateToken> m_tokens;
};
