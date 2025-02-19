#pragma once

#include <zListingO/zListingO.h>
#include <zMessageO/MessageType.h>
#include <zUtilO/ProcessSummary.h>

class Case;
class CaseAccess;
class ConnectionString;
struct MessageSummary;
class PFF;

namespace Listing
{
    struct HeaderAttribute;
    class Lister;
    class ListerWriteFile;

    enum class ListingType { Text, Csv, DataFile, Excel, Html, Json, Null };
}


class ZLISTINGO_API Listing::Lister
{
    friend ListerWriteFile;

protected:
    struct MessageDetails
    {
        MessageType type;
        int number;
    };

    struct Message
    {
        std::string level_key;
        std::optional<MessageDetails> details;
        SharableString text;
    };

    struct Messages
    {
        std::string source;
        std::vector<Message> messages;
    };

    Lister(std::shared_ptr<ProcessSummary> process_summary);

public:
    static std::unique_ptr<Lister> Create(std::shared_ptr<ProcessSummary> process_summary, const PFF& pff,
                                          bool append, std::shared_ptr<const CaseAccess> case_access);

    virtual ~Lister();

    void SetUpdateProcessSummaryWithMessageNumbers(bool update) { m_updateProcessSummaryWithMessageNumbers = update; }

    void UpdateCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary);

    // Sets the new message source, returning the existing source.
    std::string SetMessageSource(std::string message_source);
    void SetMessageSource(const Case& data_case, std::string level_key = std::string());

    void Write(MessageType message_type, int message_number, SharableString message_text);

    virtual void WriteHeader(const std::vector<HeaderAttribute>& /*header_attributes*/) { }

    void Finalize(const PFF& pff, const std::vector<std::vector<MessageSummary>>& message_summary_sets);

    std::optional<std::tuple<ListingType, void*>> GetFrequencyPrinterFeatures();

    static void View(const std::string& listing_file_path);

protected:
    virtual void WriteMessages(const Messages& messages) = 0;

    virtual bool IssueMultipleLevelMessagesTogether() const { return true; }

    virtual void ProcessCaseSourceDetails(const ConnectionString& /*connection_string*/, const CDataDict& /*dictionary*/) { }

    virtual void ProcessCaseSource(const Case* /*data_case*/) { }

    virtual void WriteMessageSummaries(const std::vector<MessageSummary>& /*message_summaries*/) { }

    virtual void WriteWarningAboutApplicationErrors(const std::string& /*application_errors_path*/) { }

    virtual void UpdateProcessSummary() { }

    virtual void WriteFooter() { }

    // the bool should indicate whether or not the messages should be written before the frequencies are printed
    virtual std::optional<std::tuple<bool, ListingType, void*>> GetFrequencyPrinter() { return std::nullopt; }

    // Counts the number of messages, returning them in a tuple (errors, warnings, user messages, total).
    std::tuple<size_t, size_t, size_t, size_t> CountMessages() const;

    static const std::string& GetMessageTypeText(const std::optional<MessageDetails>& message_details);

private:
    void WriteAndResetCachedMessages();

    void WriteLineForWriteFile(SharableString text);

    static ListingType GetListingType(const std::string& listing_file_path);

protected:
    std::shared_ptr<ProcessSummary> m_processSummary;

private:
    Messages m_messages;
    const Case* m_currentCaseToBeProcessed;
    double m_currentCasePositionInRepository;
    std::string m_currentCaseLevelKey;
    bool m_updateProcessSummaryWithMessageNumbers;
};
