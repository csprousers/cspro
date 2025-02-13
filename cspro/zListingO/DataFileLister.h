#pragma once

#include <zListingO/Lister.h>

namespace Listing
{
    class DataFileLister : public Lister
    {
    public:
        DataFileLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, bool append, std::shared_ptr<const CaseAccess> case_access);
        ~DataFileLister();

    protected:
        void WriteMessages(const Messages& messages) override;

        void ProcessCaseSource(const Case* data_case) override;

    private:
        void CreateAndInitializeListingDictionary(const CDataDict& source_dictionary);

    private:
        struct ClassVariables;
        std::unique_ptr<ClassVariables> m_cv;

        std::shared_ptr<const CaseAccess> m_caseAccess;
    };
}
