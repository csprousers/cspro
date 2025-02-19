#pragma once

#include <zListingO/Lister.h>

namespace FileIO { class TextFile; }
namespace Listing { class TextLister; class ProcessSummaryFormatter; }


class Listing::TextLister : public Lister
{
public:
    TextLister(std::shared_ptr<ProcessSummary> process_summary, const std::string& file_path, bool append, const PFF& pff);
    ~TextLister();

    void WriteHeader(const std::vector<HeaderAttribute>& header_attributes) override;

protected:
    void WriteMessages(const Messages& messages) override;

    bool IssueMultipleLevelMessagesTogether() const override { return false; }

    void ProcessCaseSource(const Case* data_case) override;

    void WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries) override;

    void WriteWarningAboutApplicationErrors(const std::string& application_errors_path) override;

    void UpdateProcessSummary() override;

    void WriteFooter() override;

    std::optional<std::tuple<bool, ListingType, void*>> GetFrequencyPrinter() override;

private:
    std::unique_ptr<FileIO::TextFile> m_textFile;
    size_t m_listingWidth;
    size_t m_wrapMessages;
    bool m_messagesAreFromCase;

    bool m_writeProcessSummaryAndMessages;
    std::optional<int64_t> m_endTimePosition;
    std::unique_ptr<ProcessSummaryFormatter> m_processSummaryFormatter;
};
