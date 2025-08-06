#include "StdAfx.h"
#include "CaseListingView.h"
#include "DataSourceSettings.h"
#include <zUtilO/DynamicLayoutControlResizer.h>


IMPLEMENT_DYNCREATE(CaseListingView, CFormView)

BEGIN_MESSAGE_MAP(CaseListingView, CFormView)
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_EN_CHANGE(IDC_KEY_FILTER, OnKeyFilterChange)
    ON_CBN_SELCHANGE(IDC_KEY_FILTER_TYPE, OnKeyFilterChange)
    ON_CBN_SELCHANGE(IDC_CASE_STATUS, OnCaseStatusChange)
    ON_MESSAGE(UWM::DataManager::ToggleFiltersVisibility, OnToggleFiltersVisibility)
END_MESSAGE_MAP()


CaseListingView::CaseListingView()
    :   CFormView(IDD_CASE_LISTING),
        m_toggleFiltersLinkCtrl(this, UWM::DataManager::ToggleFiltersVisibility),
        m_keyFilterTypeRadioEnumHelper({ CaseIterationStartTypeExtended::LessThan,
                                         CaseIterationStartTypeExtended::LessThanEquals,
                                         CaseIterationStartTypeExtended::StartsWith,
                                         CaseIterationStartTypeExtended::GreaterThanEquals,
                                         CaseIterationStartTypeExtended::GreaterThan }),
        m_caseStatusRadioEnumHelper({ CaseIterationCaseStatus::All,
                                      CaseIterationCaseStatus::NotDeletedOnly,
                                      CaseIterationCaseStatus::PartialsOnly,
                                      CaseIterationCaseStatus::DuplicatesOnly }),
        m_applyFilterChanges(false)
{
    static_assert(static_cast<int>(CaseIterationStartTypeExtended::GreaterThan) == static_cast<int>(CaseIterationStartType::GreaterThan));
    static_assert(static_cast<int>(CaseIterationStartTypeExtended::LessThanEquals) == static_cast<int>(CaseIterationStartType::LessThanEquals));
    static_assert(static_cast<int>(CaseIterationStartTypeExtended::GreaterThanEquals) == static_cast<int>(CaseIterationStartType::GreaterThanEquals));
    static_assert(static_cast<int>(CaseIterationStartTypeExtended::GreaterThan) == static_cast<int>(CaseIterationStartType::GreaterThan));
}


CaseListingView::~CaseListingView()
{
}


void CaseListingView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    DataSourceDoc& data_source_doc = GetDataSourceDoc();
    m_settings = data_source_doc.GetSettings<ViewableCaseIteratorSettings>();

    SetUpInitialWidth();
    SetUpInitialFilters();

    m_caseListingCtrl.Initialize(data_source_doc.GetSharedDataRepository(), m_settings);

    m_applyFilterChanges = true;
}


void CaseListingView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_KEY_FILTER, m_keyFilterEdit);
    DDX_Control(pDX, IDC_KEY_FILTER_TYPE, m_keyFilterComboBox);
    DDX_Control(pDX, IDC_CASE_STATUS, m_caseStatusComboBox);
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


void CaseListingView::OnSize(const UINT nType, const int cx, const int cy)
{
    if( m_dynamicLayoutControlResizer == nullptr )
    {
        m_dynamicLayoutControlResizer = std::make_unique<DynamicLayoutControlResizer>(*this);

        m_dynamicLayoutControlResizer->Add(IDC_CASE_FILTERS, SizingDirection::X)
                                      .Add(&m_keyFilterEdit, SizingDirection::X)
                                      .Add(&m_keyFilterComboBox, SizingDirection::X)
                                      .Add(&m_caseStatusComboBox, SizingDirection::X)
                                      .Add(&m_toggleFiltersLinkCtrl, MovingDirection::X)
                                      .Add(&m_caseListingCtrl, SizingDirection::XY);
    }

    __super::OnSize(nType, cx, cy);

    m_dynamicLayoutControlResizer->OnSize(cx, cy);
}


void CaseListingView::SetUpInitialWidth()
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

    // a width of 190 is sufficient to show a case key of length 1 along with the filters
    constexpr int MinCalculatedWidth = 190;
    splitter_wnd.SetColumnInfo(0, std::max(*case_listing_width, MinCalculatedWidth), 0);

    splitter_wnd.RecalcLayout();
}


void CaseListingView::SetUpInitialFilters()
{
    ASSERT(m_settings != nullptr);

    // limit the key filter to the length of the key
    const DataSourceDoc& data_source_doc = GetDataSourceDoc();
    m_keyFilterEdit.SetLimitText(data_source_doc.GetDictionary().GetKeyLength());

    // update the key filter and key filter type
    CaseIterationStartTypeExtended start_type_extended = CaseIterationStartTypeExtended::StartsWith;
    const CaseIteratorParameters* const start_parameters = m_settings->GetParameters();

    if( start_parameters != nullptr )
    {
        if( start_parameters->key_prefix.has_value() )
        {
            WindowsUtf8::SetText(m_keyFilterEdit, *start_parameters->key_prefix);
        }

        else if( std::holds_alternative<std::string>(start_parameters->first_key_or_position) )
        {
            WindowsUtf8::SetText(m_keyFilterEdit, std::get<std::string>(start_parameters->first_key_or_position));
            start_type_extended = static_cast<CaseIterationStartTypeExtended>(start_parameters->start_type);
        }

        else
        {
            ASSERT(false);
        }
    }

    m_keyFilterComboBox.SetCurSel(m_keyFilterTypeRadioEnumHelper.ToForm(start_type_extended));

    // update the case status
    UpdateCaseStatusComboBox();

    // hide the filters if the settings indicate that they should not be initially shown
    if( !m_settings->GetViewFilters() )
        SetFiltersVisibility();
}


void CaseListingView::UpdateSettingsFromKeyFilter()
{
    ASSERT(m_settings != nullptr && m_settings->GetViewFilters());

    std::string key_prefix = WindowsUtf8::GetText(m_keyFilterEdit);
    std::optional<CaseIteratorParameters> parameters;

    if( !key_prefix.empty() )
    {
        const CaseIterationStartTypeExtended start_type_extended = m_keyFilterTypeRadioEnumHelper.FromForm(m_keyFilterComboBox.GetCurSel());

        if( start_type_extended == CaseIterationStartTypeExtended::StartsWith )
        {
            parameters = CaseIteratorParameters::CreateForKeyPrefix(std::move(key_prefix));
        }

        else
        {
            parameters = CaseIteratorParameters::CreateForKey(static_cast<CaseIterationStartType>(start_type_extended), std::move(key_prefix));
        }
    }

    m_settings->SetParameters(std::move(parameters));
}


void CaseListingView::OnKeyFilterChange()
{
    if( m_applyFilterChanges )
    {
        UpdateSettingsFromKeyFilter();
        m_caseListingCtrl.UpdateCaseListingAsync();
    }
}


void CaseListingView::OnCaseStatusChange()
{
    ASSERT(m_settings != nullptr);

    if( m_applyFilterChanges )
    {
        m_settings->SetStatus(m_caseStatusRadioEnumHelper.FromForm(m_caseStatusComboBox.GetCurSel()));
        m_caseListingCtrl.UpdateCaseListingAsync();
    }
}


void CaseListingView::UpdateCaseStatusComboBox()
{
    ASSERT(m_settings != nullptr);
    m_caseStatusComboBox.SetCurSel(m_caseStatusRadioEnumHelper.ToForm(m_settings->GetStatus()));
}


struct CaseListingView::FilterData
{
    int filters_height;
    std::vector<HWND> filter_controls;
    std::vector<std::tuple<HWND, bool>> non_filter_controls; // true = control should be sized and moved
};


void CaseListingView::CalculateFilterData()
{
    ASSERT(m_filterData == nullptr);
    m_filterData = std::make_unique<FilterData>();

    int filter_group_top = -1;
    int cases_label_top = -1;

    for( HWND hWnd = ::GetWindow(m_hWnd, GW_CHILD); hWnd != nullptr; hWnd = ::GetNextWindow(hWnd, GW_HWNDNEXT) )
    {
        auto get_top = [&]()
        {
            CRect rect;
            ::GetWindowRect(hWnd, &rect);
            return rect.top;
        };

        switch( ::GetDlgCtrlID(hWnd) )
        {
            case IDC_CASES_LABEL:
                cases_label_top = get_top();
                m_filterData->non_filter_controls.emplace_back(hWnd, false);
                break;

            case IDC_TOGGLE_FILTERS:
                m_filterData->non_filter_controls.emplace_back(hWnd, false);
                break;

            case IDC_CASES:
                m_filterData->non_filter_controls.emplace_back(hWnd, true);
                break;

            case IDC_CASE_FILTERS:
                filter_group_top = get_top();
                [[fallthrough]];

            default:
                m_filterData->filter_controls.emplace_back(hWnd);
                break;
        }
    }

    ASSERT(filter_group_top != -1 && filter_group_top < cases_label_top);

    m_filterData->filters_height = cases_label_top - filter_group_top;
}


void CaseListingView::SetFiltersVisibility()
{
    ASSERT(m_settings != nullptr);
    ASSERT(m_dynamicLayoutControlResizer != nullptr);

    if( m_filterData == nullptr )
        CalculateFilterData();

    int filter_controls_show_command;
    int non_filter_controls_height_adjustment;
    CRect rect;

    if( m_settings->GetViewFilters() )
    {
        filter_controls_show_command = SW_SHOW;
        non_filter_controls_height_adjustment = m_filterData->filters_height;

        m_toggleFiltersLinkCtrl.SetWindowText(L"Hide Filters");

        UpdateSettingsFromKeyFilter();
        m_caseListingCtrl.UpdateCaseListingAsync();
    }

    else
    {
        filter_controls_show_command = SW_HIDE;
        non_filter_controls_height_adjustment = -1 * m_filterData->filters_height;

        m_toggleFiltersLinkCtrl.SetWindowText(L"Show Filters");

        // when no filters are shown, we won't apply the filters
        if( m_settings->GetParameters() != nullptr )
        {
            m_settings->SetParameters(std::nullopt);
            m_caseListingCtrl.UpdateCaseListingAsync();
        }
    }

    for( const HWND hWnd : m_filterData->filter_controls )
        ::ShowWindow(hWnd, filter_controls_show_command);

    for( const auto& [hWnd, size_control] : m_filterData->non_filter_controls )
    {
        ::GetWindowRect(hWnd, &rect);
        ScreenToClient(rect);

        rect.top += non_filter_controls_height_adjustment;

        if( !size_control )
            rect.bottom += non_filter_controls_height_adjustment;

        m_dynamicLayoutControlResizer->SetWindowPos(hWnd, rect);
    }
}


LRESULT CaseListingView::OnToggleFiltersVisibility(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_settings != nullptr);
    ASSERT(m_settings->GetViewFilters() == static_cast<bool>(m_keyFilterEdit.IsWindowVisible()));

    m_settings->SetViewFilters(!m_settings->GetViewFilters());
    SetFiltersVisibility();

    return 1;
}
