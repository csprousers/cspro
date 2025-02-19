#pragma once

#include <CSDocument/DocSetComponent.h>

class PFF;
class ProductionSyncer;


class DataManagerApp : public CWinAppEx
{
public:
    DataManagerApp();

    using CaseDocData = std::tuple<std::shared_ptr<const CDataDict>,
                                   std::shared_ptr<const CaseAccess>,
                                   ConnectionString,
                                   std::shared_ptr<const Case>,
                                   UINT>;

    void OpenCaseDoc(CaseDocData case_doc_data);

    std::unique_ptr<CaseDocData> ReleaseCaseDocData() { return std::move(m_caseDocData); }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;
    int ExitInstance() override;

    void OnAppAbout();

private:
    // Returns monostate or a non-null object if Data Manager should be shown.
    std::variant<std::monostate, std::unique_ptr<ProductionSyncer>> ProcessProductionSyncs(const std::vector<std::wstring>& file_paths);

private:
    CMultiDocTemplate* m_caseDocTemplate;
    std::unique_ptr<CaseDocData> m_caseDocData;
    std::shared_ptr<const PFF> m_pff;
};
