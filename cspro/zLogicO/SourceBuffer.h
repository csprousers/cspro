#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/BasicToken.h>
#include <zAppO/LogicSettings.h>

namespace Logic { class SourceBuffer; }


class ZLOGICO_API Logic::SourceBuffer
{
    friend class SourceBufferTokenizer;

public:
    SourceBuffer(SharableString buffer);

    const char* GetBuffer() const { return m_buffer->c_str(); }

    static std::vector<BasicToken> Tokenize(const SharableString& buffer, const LogicSettings& logic_settings);

    const std::vector<BasicToken>& Tokenize(const LogicSettings& logic_settings);

    const std::vector<BasicToken>& GetTokens() const;

    size_t GetPositionInBuffer(const BasicToken& basic_token) const;

    void RemoveTokensAfterText(size_t start_position, TokenCode token_code, cs::cref_optional<std::string> end_text = std::nullopt);


    // for adjusting line numbers when using a source buffer that does not exactly match the input buffer
    struct LineAdjuster
    {
        virtual ~LineAdjuster() { }
        virtual size_t GetLineNumber(size_t line_number) = 0;
    };

    void SetLineAdjuster(std::shared_ptr<LineAdjuster> line_adjuster) { m_lineAdjuster = std::move(line_adjuster); }

private:
    SharableString m_buffer;
    std::optional<std::vector<BasicToken>> m_basicTokens;
    std::shared_ptr<LineAdjuster> m_lineAdjuster;
};
