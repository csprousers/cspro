#pragma once

#include <zUtilF/LinkCtrls.h>
#include <DataManager/DataSourceCaseListingCtrl.h>


class CaseListingView : public CFormView
{
    DECLARE_DYNCREATE(CaseListingView)

protected:
    CaseListingView(); // create from serialization only

public:
    ~CaseListingView();

    const DataSourceDoc& GetDataSourceDoc() const { return *assert_cast<const DataSourceDoc*>(GetDocument()); }
    DataSourceDoc& GetDataSourceDoc()             { return *assert_cast<DataSourceDoc*>(GetDocument()); }

    DataSourceCaseListingCtrl& GetCaseListingCtrl() { return m_caseListingCtrl; }

    void UpdateCaseStatusComboBox();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    void OnDestroy();

    void OnKeyFilterChange();
    void OnCaseStatusChange();

    LRESULT OnToggleFiltersVisibility(WPARAM wParam, LPARAM lParam);

private:
    void SetUpInitialWidth();
    void SetUpInitialFilters();

    void UpdateSettingsFromKeyFilter();

    void CalculateFilterData();

    void SetFiltersVisibility();

private:
    CEdit m_keyFilterEdit;
    CComboBox m_keyFilterComboBox;
    CComboBox m_caseStatusComboBox;
    MessagePostingLinkCtrl m_toggleFiltersLinkCtrl;
    DataSourceCaseListingCtrl m_caseListingCtrl;

    enum class CaseIterationStartTypeExtended { LessThan, LessThanEquals, GreaterThanEquals, GreaterThan, StartsWith };
    RadioEnumHelper<CaseIterationStartTypeExtended> m_keyFilterTypeRadioEnumHelper;

    RadioEnumHelper<CaseIterationCaseStatus> m_caseStatusRadioEnumHelper;

    std::shared_ptr<ViewableCaseIteratorSettings> m_settings;

    struct FilterData;
    std::unique_ptr<FilterData> m_filterData;
    bool m_applyFilterChanges;
};
