#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zHtml/UriResolver.h>

class LogicSettings;
struct ReportToken;


// --------------------------------------------------------------------------
// ReportPreviewer is used to show a preview of HTML / Markdown reports.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF ReportPreviewer
{
public:
    // CSProException exceptions thrown if the report text does not compile
    ReportPreviewer(std::string report_file_path, std::string_view report_text_sv,
                    const LogicSettings& logic_settings, const char* action = "previewing");
    ~ReportPreviewer();

    SharableString GetReportHtml() const { return m_reportHtml; }

    std::string GetReportUrl();
    std::unique_ptr<UriResolver> GetReportUriResolver();

private:
    std::string CreateHtmlForHtml(const std::vector<ReportToken>& report_tokens) const;
    std::string CreateHtmlForMarkdown(const std::vector<ReportToken>& report_tokens) const;

private:
    class DesignerReportTokenizer;
    struct ReportVirtualFileMappingDetails;

    std::string m_reportFilePath;
    int m_lexerLanguage;
    SharableString m_reportHtml;
    std::unique_ptr<ReportVirtualFileMappingDetails> m_reportVirtualFileMappingDetails;
};
