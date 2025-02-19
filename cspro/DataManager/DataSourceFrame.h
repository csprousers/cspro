#pragma once

#include <DataManager/CaseHoldingFrame.h>
#include <DataManager/DataSourceDoc.h>
#include <zUtilO/CSProExecutables.h>

class CaseListingView;
class DataSourceView;


class DataSourceFrame : public CaseHoldingFrame
{
    DECLARE_DYNCREATE(DataSourceFrame)

protected:
    DataSourceFrame(); // create from serialization only

public:
    DataSourceDoc& GetDataSourceDoc() { return *assert_cast<DataSourceDoc*>(GetActiveDocument()); }

    bool IsDataSourceReadOnly();
    bool IsDataSourceReadWrite() { return !IsDataSourceReadOnly(); }

protected:
    CaseHoldingDoc& GetCaseHoldingDoc() override;
    HtmlView& GetHtmlView() override;

    std::unique_ptr<CaseProvider> CreateCaseProvider() override;
    std::unique_ptr<CaseProvider> CreateCaseProvider(const std::vector<std::shared_ptr<const CaseSummary>>& selected_case_summaries);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

    void ActivateFrame(int nCmdShow) override;
    void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);

    LRESULT OnUpdateStatusBarCaseCount(WPARAM wParam, LPARAM lParam);
    LRESULT OnUpdateStatusBarSelectedCaseKeyPosition(WPARAM wParam, LPARAM lParam);

    LRESULT OnShowDefaultPage(WPARAM wParam, LPARAM lParam);
    LRESULT OnProcessConnectionStringParameters(WPARAM wParam, LPARAM lParam);

    LRESULT OnShowSelectedCases(WPARAM wParam, LPARAM lParam);
    LRESULT OnRunTaskFromCaseListing(WPARAM wParam, LPARAM lParam);

    // File menu
    void OnFileRefresh();

    void OnFileSynchronize();

    void OnFileSaveData();

    // Listing menu
    void OnListingMethod(UINT nID);
    void OnUpdateListingMethod(CCmdUI* pCmdUI);
    void OnListingToggleMethod();

    void OnListingStatus(UINT nID);
    void OnUpdateListingStatus(CCmdUI* pCmdUI);

    void OnListingOrder(UINT nID);
    void OnUpdateListingOrder(CCmdUI* pCmdUI);

    void OnListingCaseKeyLabel(UINT nID);
    void OnUpdateListingCaseKeyLabel(CCmdUI* pCmdUI);
    void OnListingToggleCaseKeyLabel();

    // View menu
    void OnViewCasePage(UINT nID);

    // View Options menu
    void OnViewOptionsLanguage(UINT nID);

    // Data menu
    void OnDataToggleReadOnly();
    void OnUpdateDataToggleReadOnly(CCmdUI* pCmdUI);

    void OnUpdateDataSourceIsReadWriteWithCaseSelected(CCmdUI* pCmdUI);

    void OnDataDeleteCase();

    // Tools menu
    void OnExportData();
    void OnTabulateFrequencies();

private:
    std::shared_ptr<Case> GetCaseForReading();
    std::shared_ptr<const Case> LoadCase(double position_in_repository);
    std::shared_ptr<const Case> LoadCaseByKey(const std::string& key);
    std::shared_ptr<const Case> LoadCaseByUuid(const std::string& uuid);
    std::shared_ptr<const Case> LoadCaseByUuidOrKey(const std::string& uuid, const std::string& key);

    void ShowCase(std::shared_ptr<const Case> data_case, bool select_in_case_listing);

    void Refresh(bool show_data_summary);

    template<typename T>
    T& GetSettings();

    void UpdateCaseListing(bool change_is_only_visual);

    bool ToggleReadOnly();

    std::vector<DataRepositoryUniqueCaseIdentifer> GetUniqueCaseIdentifiers(const std::vector<std::shared_ptr<const CaseSummary>>& selected_case_summaries,
                                                                            bool use_position_in_repository_for_first_case_summary);

    void OpenDataInTool(CSProExecutables::Program program);

private:
    CSplitterWnd m_splitterWnd;
    CaseListingView* m_caseListingView;
    HtmlView* m_htmlView;

    std::tuple<std::shared_ptr<DataSourceSettings>,
               std::shared_ptr<ViewableCaseIteratorSettings>> m_settings;

    constexpr static size_t CachedCasesMaxSize = 100;
    std::vector<std::shared_ptr<const Case>> m_cachedCases;
};
