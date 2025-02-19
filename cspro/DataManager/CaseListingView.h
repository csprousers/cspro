#pragma once

#include <zUtilF/LinkCtrls.h>
#include <DataManager/DataSourceCaseListingCtrl.h>


class CaseListingView : public CFormView
{
    DECLARE_DYNCREATE(CaseListingView)

protected:
    CaseListingView(); // create from serialization only

public:
    const DataSourceDoc& GetDataSourceDoc() const { return *assert_cast<const DataSourceDoc*>(GetDocument()); }
    DataSourceDoc& GetDataSourceDoc()             { return *assert_cast<DataSourceDoc*>(GetDocument()); }

    DataSourceCaseListingCtrl& GetCaseListingCtrl() { return m_caseListingCtrl; }

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    void OnDestroy();

    LRESULT OnToggleFilters(WPARAM wParam, LPARAM lParam);

private:
    void SetInitialWidth();

private:
    DataSourceCaseListingCtrl m_caseListingCtrl;
    MessagePostingLinkCtrl m_toggleFiltersLinkCtrl;
};
