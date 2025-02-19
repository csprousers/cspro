#pragma once

#include <zListingO/Lister.h>

class HtmlWriter;
namespace FileIO { class TextFile; }
namespace Listing { class HtmlLister; class ProcessSummaryHTMLFormatter; }
    

class Listing::HtmlLister : public Lister
{
public:
    HtmlLister(std::shared_ptr<ProcessSummary> process_summary, const std::string& file_path, bool append, const PFF& pff);
    ~HtmlLister();

    void WriteHeader(const std::vector<HeaderAttribute>& header_attributes) override;

protected:
    void WriteMessages(const Messages& messages) override;

    void ProcessCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary) override;

    void ProcessCaseSource(const Case* data_case) override;

    void WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries) override;

    void WriteWarningAboutApplicationErrors(const std::string& application_errors_path) override;

    void UpdateProcessSummary() override;

    void WriteFooter() override;

private:
    void WriteUpdatesToProcessMessageTable();

    void MoveToHtmlEndTag() const;

private:
    std::unique_ptr<FileIO::TextFile> m_textFile;
    std::unique_ptr<HtmlWriter> m_htmlWriter;
    bool m_hasData;

    std::optional<std::string> m_inputDataUri;
    std::optional<std::tuple<std::string, std::string>> m_caseKeyUuid;

    bool m_writeProcessSummaryAndMessages;
    std::optional<int64_t> m_endTimePosition;
    std::optional<int64_t> m_startProcessMessagePosition;
    bool m_isProcessMessageComplete;
    std::unique_ptr<ProcessSummaryHTMLFormatter> m_processSummaryFormatter;

    bool m_isMultiLevel;
};
