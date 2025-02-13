#pragma once

#include <zListingO/Lister.h>


namespace Listing
{
    class ProcessSummaryFormatter;

    class TextLister : public Lister
    {
    public:
        TextLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, bool append, const PFF& pff);
        ~TextLister();

        void WriteHeader(const std::vector<HeaderAttribute>& header_attributes) override;

    protected:
        void WriteMessages(const Messages& messages) override;

        bool IssueMultipleLevelMessagesTogether() const override { return false; }

        void ProcessCaseSource(const Case* data_case) override;

        void WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries) override;

        void WriteWarningAboutApplicationErrors(const std::wstring& application_errors_filename) override;

        void UpdateProcessSummary() override;

        void WriteFooter() override;

        std::optional<std::tuple<bool, ListingType, void*>> GetFrequencyPrinter() override;

    private:
        std::unique_ptr<CStdioFileUnicode> m_file;
        size_t m_listingWidth;
        size_t m_wrapMessages;
        bool m_messagesAreFromCase;

        bool m_writeProcessSummaryAndMessages;
        std::optional<ULONGLONG> m_endTimePosition;
        std::unique_ptr<ProcessSummaryFormatter> m_processSummaryFormatter;
    };
}
