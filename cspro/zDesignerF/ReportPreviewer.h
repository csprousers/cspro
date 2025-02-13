#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zHtml/UriResolver.h>

class LogicSettings;


// ReportPreviewer is used to show a preview of HTML reports

class CLASS_DECL_ZDESIGNERF ReportPreviewer
{
public:
    // CSProException exceptions thrown if the report text does not compile
    ReportPreviewer(wstring_view report_text, const LogicSettings& logic_settings);

    ~ReportPreviewer();

    const std::wstring& GetReportHtml() const { return m_reportHtml; }

    std::wstring GetReportUrl(const std::wstring& report_filename);
    std::unique_ptr<UriResolver> GetReportUriResolver(std::wstring report_filename);

private:
    std::wstring m_reportHtml;

    struct ReportVirtualFileMappingDetails;
    std::unique_ptr<ReportVirtualFileMappingDetails> m_reportVirtualFileMappingDetails;
};
