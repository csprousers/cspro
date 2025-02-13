#pragma once

#include <zListingO/Lister.h>

class CaseItem;
class ExcelWriter;


namespace Listing
{
    class ExcelLister : public Lister
    {
    public:
        ExcelLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, std::shared_ptr<const CaseAccess> case_access);
        ~ExcelLister();

    protected:
        void WriteMessages(const Messages& messages) override;

        void ProcessCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary) override;

        void ProcessCaseSource(const Case* data_case) override;

    private:
        std::unique_ptr<ExcelWriter> m_excelWriter;
        uint32_t m_row;

        std::shared_ptr<const CaseAccess> m_caseAccess;

        bool m_useLevelKey;
        std::vector<const CaseItem*> m_idCaseItems;
        std::vector<std::variant<double, std::wstring>> m_keyValues;

        std::optional<std::wstring> m_inputDataUri;
        std::optional<std::tuple<std::wstring, std::wstring>> m_caseKeyUuid;
    };
}
