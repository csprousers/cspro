#pragma once

#include <zEngineO/zEngineO.h>

class LogicSettings;


struct ReportToken
{
    enum class Type { ReportText, DoubleTilde, TripleTilde, Logic };

    Type type;
    size_t section_line_number_start;
    std::wstring text;
};


class ZENGINEO_API ReportTokenizer
{
public:
    virtual ~ReportTokenizer() { }

    bool Tokenize(wstring_view report_text_sv, const LogicSettings& logic_settings);

    const std::vector<ReportToken>& GetReportTokens() const { return m_reportTokens; }

protected:
    virtual void OnErrorUnbalancedEscapes(size_t line_number) = 0;
    virtual void OnErrorTokenNotEnded(const ReportToken& report_token) = 0;

private:
    std::vector<ReportToken> m_reportTokens;
};
