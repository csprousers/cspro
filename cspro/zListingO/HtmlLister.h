#pragma once

#include <zListingO/Lister.h>


namespace Listing
{
    class ProcessSummaryHTMLFormatter;

    class HtmlLister : public Lister
    {
    public:
        HtmlLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, bool append, const PFF& pff);
        ~HtmlLister();

        void WriteHeader(const std::vector<HeaderAttribute>& header_attributes) override;

    protected:
        void WriteMessages(const Messages& messages) override;

        void ProcessCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary) override;

        void ProcessCaseSource(const Case* data_case) override;

        void WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries) override;

        void WriteWarningAboutApplicationErrors(const std::wstring& application_errors_filename) override;

        void UpdateProcessSummary() override;

        void WriteFooter() override;

    private:
        void WriteUpdatesToProcessMessageTable();

        void MoveToHtmlEndTag() const;

    private:
        std::unique_ptr<CStdioFileUnicode> m_file;
        bool m_hasData;

        std::optional<std::wstring> m_inputDataUri;
        std::optional<std::tuple<std::wstring, std::wstring>> m_caseKeyUuid;

        bool m_writeProcessSummaryAndMessages;
        std::optional<ULONGLONG> m_endTimePosition;
        std::optional<ULONGLONG> m_startProcessMessagePosition;
        bool m_isProcessMessageComplete;
        std::unique_ptr<ProcessSummaryHTMLFormatter> m_processSummaryFormatter;

        bool m_isMultiLevel;
    };
}
