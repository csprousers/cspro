#pragma once

#include <zListingO/zListingO.h>
#include <zMessageO/MessageType.h>
#include <zUtilO/ProcessSummary.h>

class Case;
class CaseAccess;
class ConnectionString;
class CStdioFileUnicode;
struct MessageSummary;
class PFF;


namespace Listing
{
    struct HeaderAttribute;

    enum class ListingType { Text, Csv, DataFile, Excel, Html, Json, Null };


    class ZLISTINGO_API Lister
    {
        template<typename T> friend class ListerWriteFile;

    protected:
        Lister(std::shared_ptr<ProcessSummary> process_summary);

        struct MessageDetails
        {
            MessageType type;
            int number;
        };

        struct Message
        {
            std::wstring level_key;
            std::optional<MessageDetails> details;
            std::wstring text;
        };

        struct Messages
        {
            std::wstring source;            
            std::vector<Message> messages;
        };

    public:
        static std::unique_ptr<Lister> Create(std::shared_ptr<ProcessSummary> process_summary, const PFF& pff,
                                              bool append, std::shared_ptr<const CaseAccess> case_access);

        virtual ~Lister();

        void SetUpdateProcessSummaryWithMessageNumbers(bool update) { m_updateProcessSummaryWithMessageNumbers = update; }

        void UpdateCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary);

        void SetMessageSource(std::wstring message_source);
        void SetMessageSource(const Case& data_case, std::wstring level_key = std::wstring());

        void Write(MessageType message_type, int message_number, std::wstring message_text);

        virtual void WriteHeader(const std::vector<HeaderAttribute>& /*header_attributes*/) { }

        void Finalize(const PFF& pff, const std::vector<std::vector<MessageSummary>>& message_summary_sets);

        std::optional<std::tuple<ListingType, void*>> GetFrequencyPrinterFeatures();

        static void View(NullTerminatedString listing_filename);

    protected:
        virtual void WriteMessages(const Messages& messages) = 0;

        virtual bool IssueMultipleLevelMessagesTogether() const { return true; }

        virtual void ProcessCaseSourceDetails(const ConnectionString& /*connection_string*/, const CDataDict& /*dictionary*/) { }

        virtual void ProcessCaseSource(const Case* /*data_case*/) { }

        virtual void WriteMessageSummaries(const std::vector<MessageSummary>& /*message_summaries*/) { }

        virtual void WriteWarningAboutApplicationErrors(const std::wstring& /*application_errors_filename*/) { }

        virtual void UpdateProcessSummary() { }

        virtual void WriteFooter() { }

        // the bool should indicate whether or not the messages should be written before the frequencies are printed
        virtual std::optional<std::tuple<bool, ListingType, void*>> GetFrequencyPrinter() { return std::nullopt; }

        /// <summary>Counts the number of messages, returning them in a tuple (errors, warnings, user messages, total).</summary>
        std::tuple<size_t, size_t, size_t, size_t> CountMessages() const;

    private:
        void WriteAndResetCachedMessages();

        void WriteLineForWriteFile(std::wstring text);
        
    protected:
        std::shared_ptr<ProcessSummary> m_processSummary;

    private:
        Messages m_messages;
        const Case* m_currentCaseToBeProcessed;
        double m_currentCasePositionInRepository;
        std::wstring m_currentCaseLevelKey;
        bool m_updateProcessSummaryWithMessageNumbers;
    };
}
