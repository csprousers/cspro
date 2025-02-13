#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/BasicToken.h>
#include <zAppO/LogicSettings.h>


namespace Logic
{
    class ZLOGICO_API SourceBuffer
    {
        friend class SourceBufferTokenizer;

    public:
        SourceBuffer(std::wstring buffer);
        SourceBuffer(const TCHAR* buffer, bool create_copy_of_buffer);

        const TCHAR* GetBuffer() const
        {
            return std::holds_alternative<std::wstring>(m_buffer) ? std::get<std::wstring>(m_buffer).c_str() :
                                                                    std::get<const TCHAR*>(m_buffer);
        }

        const std::vector<BasicToken>& Tokenize(const LogicSettings& logic_settings);

        const std::vector<BasicToken>& GetTokens() const;

        size_t GetPositionInBuffer(const BasicToken& basic_token) const;

        void RemoveTokensAfterText(size_t start_position, TokenCode token_code, std::optional<wstring_view> end_text_sv = std::nullopt);


        // for adjusting line numbers when using a source buffer that does not exactly match the input buffer
        struct LineAdjuster
        {
            virtual ~LineAdjuster() { }
            virtual size_t GetLineNumber(size_t line_number) = 0;
        };

        void SetLineAdjuster(std::shared_ptr<LineAdjuster> line_adjuster) { m_lineAdjuster = std::move(line_adjuster); }

    private:
        std::variant<std::wstring, const TCHAR*> m_buffer;
        std::unique_ptr<std::vector<BasicToken>> m_basicTokens;
        std::shared_ptr<LineAdjuster> m_lineAdjuster;
    };
}
