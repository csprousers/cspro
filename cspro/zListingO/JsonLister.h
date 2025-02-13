#pragma once

#include <zListingO/Lister.h>

class CaseItem;
class JsonFileWriter;


namespace Listing
{
    class JsonLister : public Lister
    {
    public:
        JsonLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, std::shared_ptr<const CaseAccess> case_access);
        ~JsonLister();

        void WriteHeader(const std::vector<HeaderAttribute>& header_attributes) override;

    protected:
        void WriteMessages(const Messages& messages) override;
        void ProcessCaseSource(const Case* data_case) override;
        void WriteMessageSummaries(const std::vector<MessageSummary>& message_summaries) override;
        void WriteFooter() override;
        std::optional<std::tuple<bool, ListingType, void*>> GetFrequencyPrinter() override;

    private:
        void WriteFrequencies();

        void EndListingArrayAndStartSummaryObjectIfNecessary();

    private:
        std::unique_ptr<JsonFileWriter> m_jsonWriter;
        bool m_mustEndListingArray;
        bool m_mustEndMessageSummariesArray;

        std::shared_ptr<const CaseAccess> m_caseAccess;

        std::vector<const CaseItem*> m_idCaseItems;
        std::unique_ptr<std::vector<std::wstring>> m_levelNames;

        std::vector<std::tuple<const CaseItem*, std::variant<double, std::wstring>>> m_keyValues;

        double m_startTimestamp;

        std::vector<std::string> m_jsonFrequencyTexts;
    };
}
