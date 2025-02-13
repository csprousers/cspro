#pragma once

#include <zListingO/Lister.h>

class CaseItem;


namespace Listing
{
    class CsvLister : public Lister
    {
    public:
        CsvLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, bool append, std::shared_ptr<const CaseAccess> case_access);
        ~CsvLister();

    protected:
        void WriteMessages(const Messages& messages) override;

        void ProcessCaseSource(const Case* data_case) override;

    private:
        std::unique_ptr<CStdioFileUnicode> m_file;

        std::shared_ptr<const CaseAccess> m_caseAccess;

        bool m_useLevelKey;
        std::vector<const CaseItem*> m_idCaseItems;
        std::unique_ptr<TCHAR[]> m_keyColumnsText;
    };
}
