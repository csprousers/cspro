#include "StdAfx.h"
#include "CaseListingView.h"
#include "DataSourceSettings.h"


IMPLEMENT_DYNCREATE(CaseListingView, CFormView)

BEGIN_MESSAGE_MAP(CaseListingView, CFormView)
    ON_WM_DESTROY()
    ON_MESSAGE(UWM::DataManager::ToggleFilters, OnToggleFilters)
END_MESSAGE_MAP()


CaseListingView::CaseListingView()
    :   CFormView(IDD_CASE_LISTING),
        m_toggleFiltersLinkCtrl(this, UWM::DataManager::ToggleFilters)
{
}


void CaseListingView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    SetInitialWidth();

    DataSourceDoc& data_source_doc = GetDataSourceDoc();

    m_caseListingCtrl.Initialize(data_source_doc.GetSharedDataRepository(),
                                 data_source_doc.GetSettings<ViewableCaseIteratorSettings>());
}


void CaseListingView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_CASES, m_caseListingCtrl);
    DDX_Control(pDX, IDC_TOGGLE_FILTERS, m_toggleFiltersLinkCtrl);
}


void CaseListingView::OnDestroy()
{
    // update the case listing width to restore when the data source is opened again
    const std::shared_ptr<DataSourceSettings> data_source_settings = GetDataSourceDoc().GetSettings<DataSourceSettings>();

    CRect rect;
    GetClientRect(rect);
    data_source_settings->SetCaseListingWidth(rect.Width());

    __super::OnDestroy();
}


void CaseListingView::SetInitialWidth()
{
    // since this is hosted in a splitter, set the initial width based on the length of the case key
    CSplitterWnd& splitter_wnd = *assert_cast<CSplitterWnd*>(GetParent());
    ASSERT(splitter_wnd.GetPane(0, 0) == this);

    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    const std::shared_ptr<const DataSourceSettings> data_source_settings = data_source_doc.GetSettings<DataSourceSettings>();
    std::optional<int> case_listing_width = data_source_settings->GetCaseListingWidth();

    // if the case listing width was not saved from a previous session, approximate it based on the length of the case key
    if( !case_listing_width.has_value() )
    {
        constexpr int Margin = 20;
        case_listing_width = Margin + GetDC()->GetOutputTextExtent(L"A", 1).cx * data_source_doc.GetDictionary().GetKeyLength();
    }

    constexpr int MinCalculatedWidth = 120;
    splitter_wnd.SetColumnInfo(0, std::max(*case_listing_width, MinCalculatedWidth), 0);

    splitter_wnd.RecalcLayout();
}


LRESULT CaseListingView::OnToggleFilters(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_toggleFiltersLinkCtrl.SetWindowText(L"DATA_TODO toggle filters");
    return 1;
}
