#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zHtml/UriResolver.h>

class LogicSettings;


// ReportPreviewer is used to show a preview of HTML reports

class CLASS_DECL_ZDESIGNERF ReportPreviewer
{
public:
    // CSProException exceptions thrown if the report text does not compile
    ReportPreviewer(std::string_view report_text_sv, const LogicSettings& logic_settings);
    ~ReportPreviewer();

    std::string GetReportUrl(const std::string& report_file_path);
    std::unique_ptr<UriResolver> GetReportUriResolver(std::string report_file_path);

private:
    SharableString m_reportHtml;

    struct ReportVirtualFileMappingDetails;
    std::unique_ptr<ReportVirtualFileMappingDetails> m_reportVirtualFileMappingDetails;
};
