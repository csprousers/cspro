#include "stdafx.h"
#include "CaseListingCtrl.h"
#include "CaseIterator.h"
#include "DataRepository.h"
#include "resource.h"
#include "UWM.h"


namespace
{
    constexpr int IconWidth = 16;
    constexpr int IconTextMargin = 2;

    constexpr int InitialColumnWidth = 10;

    constexpr size_t CaseSummariesQueryLimit = 1000;
}


BEGIN_MESSAGE_MAP(CaseListingCtrl, CListCtrl)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
#ifdef _DEBUG
    ON_NOTIFY_REFLECT(LVN_ODCACHEHINT, OnCacheHint)
#endif
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemChanged)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDoubleClick)
    ON_NOTIFY_REFLECT(NM_RCLICK, OnRightClick)
    ON_COMMAND(ID_CONTEXT_MENU, OnContextMenu)
    ON_MESSAGE(UWM::Data::AdjustColumnWidthAndInvalidate, OnAdjustColumnWidthAndInvalidate)
    ON_MESSAGE(UWM::Data::SelectionsChanged, OnSelectionsChanged)
    ON_MESSAGE(UWM::Data::RequeryCaseSummaries, OnRequeryCaseSummaries)
END_MESSAGE_MAP()


CaseListingCtrl::CaseListingCtrl()
    :   m_textColors{ GetSysColor(COLOR_WINDOWTEXT), GetSysColor(COLOR_HIGHLIGHTTEXT) },
        m_backgroundColors{ GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_HIGHLIGHT) },
        m_columnWidths{ InitialColumnWidth, InitialColumnWidth },
        m_adjustColumnWidthAndInvalidateMessagePosted(false),
        m_hAccelerators(LoadAccelerators(zDataODLL.hModule, MAKEINTRESOURCE(IDR_CASE_LISTING))),
        m_selectionsChangedMessagePosted(false),
        m_refreshSelectedCaseSummaries(false)
{
}


CaseListingCtrl::~CaseListingCtrl()
{
}


void CaseListingCtrl::PreSubclassWindow()
{
    ASSERT(( GetStyle() & LVS_REPORT ) != 0);
    ASSERT(( GetStyle() & LVS_OWNERDATA ) != 0);

    // because case labels can be of differing lengths, allow selection of rows even
    // when clicking on whitespace to the right of the key / case label
    SetExtendedStyle(LVS_EX_FULLROWSELECT);

    InsertColumn(0, L"Key", LVCFMT_LEFT, InitialColumnWidth);

    CreateImageList();
}


void CaseListingCtrl::CreateImageList()
{
    constexpr UINT IconResourceIds[] =
    {
        IDI_CASE_COMPLETE,
        IDI_CASE_COMPLETE_VERIFIED,
        IDI_CASE_DELETED,
        IDI_CASE_PARTIAL_ADD,
        IDI_CASE_PARTIAL_MODIFY,
        IDI_CASE_PARTIAL_VERIFY
    };

    if( !m_imageList.Create(IconWidth, IconWidth, ILC_COLOR32, 0, _countof(IconResourceIds)) )
    {
        ASSERT(false);
        return;
    }

    m_imageList.SetBkColor(GetSysColor(COLOR_WINDOW));

    for( const UINT resource_id : IconResourceIds )
    {
        HICON icon = AfxGetApp()->LoadIcon(resource_id);

        if( icon == nullptr )
        {
            ASSERT(false);
            continue;
        }

        m_imageList.Add(icon);
    }

    SetImageList(&m_imageList, TVSIL_NORMAL);
}


int CaseListingCtrl::GetIconIndex(const CaseSummary& case_summary)
{
    if( case_summary.GetDeleted() )
        return 2;

    switch( case_summary.GetPartialSaveMode() )
    {
        case PartialSaveMode::None:   return case_summary.GetVerified() ? 1 : 0;
        case PartialSaveMode::Add:    return 3;
        case PartialSaveMode::Modify: return 4;
        case PartialSaveMode::Verify: return 5;
        default:                      return ReturnProgrammingError(0);
    }
}


void CaseListingCtrl::Initialize(std::shared_ptr<DataRepository> data_depository,
                                 std::shared_ptr<const ViewableCaseIteratorSettings> viewable_case_iterator_setting)
{
    m_dataRepository = std::move(data_depository);
    m_viewableCaseIteratorSettings = std::move(viewable_case_iterator_setting);
    ASSERT(m_dataRepository != nullptr && m_viewableCaseIteratorSettings != nullptr);

    UpdateCaseListing();
}


void CaseListingCtrl::UpdateCaseListing(const bool change_is_only_visual/* = false*/)
{
    if( !m_adjustColumnWidthAndInvalidateMessagePosted )
    {
        PostMessage(UWM::Data::AdjustColumnWidthAndInvalidate);
        m_adjustColumnWidthAndInvalidateMessagePosted = true;
    }

    if( !change_is_only_visual )
    {
        m_refreshSelectedCaseSummaries = true;

        PostMessage(UWM::Data::RequeryCaseSummaries);
    }
}


BOOL CaseListingCtrl::PreTranslateMessage(MSG* const pMsg)
{
    if( m_hAccelerators != nullptr && ::TranslateAccelerator(m_hWnd, m_hAccelerators, pMsg) )
    {
        return TRUE;
    }

    else if( pMsg->message == WM_KEYDOWN )
    {
        // allow case selection using the Enter key
        if( pMsg->wParam == VK_RETURN )
        {
            OnCaseListingDoubleClickAndReturn();
            return TRUE;
        }

        // potentially allow the Delete key to be used to delete cases
        else if( pMsg->wParam == VK_DELETE )
        {
            if( OnCaseListingDeleteKey() )
                return TRUE;
        }
    }

    return __super::PreTranslateMessage(pMsg);
}


void CaseListingCtrl::OnCustomDraw(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    NMLVCUSTOMDRAW* const pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

    if( m_adjustColumnWidthAndInvalidateMessagePosted )
    {
        // skipping drawing until the message is handled, which will
        // invalidate the control and everything will be redrawn
        *pResult = CDRF_SKIPDEFAULT;
    }

    else if( pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT )
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }

    else if( pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
    {
        CDC* const pDC = CDC::FromHandle(pLVCD->nmcd.hdc);

        const int item_index = static_cast<int>(pLVCD->nmcd.dwItemSpec);
        const int color_index = (( GetItemState(item_index, LVIS_SELECTED) & LVIS_SELECTED ) != 0 ) ? 1 : 0;

        int icon_index;
        std::shared_ptr<const std::wstring> text;

        try
        {
            CaseSummaryWithMeasuredText& case_summary_with_measured_text = GetCaseSummaryWithMeasuredText(item_index);
            ASSERT(case_summary_with_measured_text.case_summary != nullptr);

            icon_index = GetIconIndex(*case_summary_with_measured_text.case_summary);

            const bool view_case_key = ( m_viewableCaseIteratorSettings->GetViewCaseKey() ||
                                         case_summary_with_measured_text.case_summary->GetCaseLabel().empty() );

            // if not already done, get the wide text and measure it to potentially adjust the column width
            if( !case_summary_with_measured_text.last_displayed_text.has_value() ||
                view_case_key != std::get<0>(*case_summary_with_measured_text.last_displayed_text) )
            {
                text = std::make_unique<std::wstring>(TC::ToWide(view_case_key ? case_summary_with_measured_text.case_summary->GetSingleLineKey() :
                                                                                 case_summary_with_measured_text.case_summary->GetSingleLineCaseLabel()));
                ASSERT(!text->empty());

                CRect text_rect;
                pDC->DrawText(text->c_str(), text->length(), &text_rect, DT_CALCRECT);

                case_summary_with_measured_text.last_displayed_text.emplace(view_case_key, text, text_rect.Width());
            }

            ASSERT(std::get<2>(*case_summary_with_measured_text.last_displayed_text) > 0);

            int& current_column_width = GetCurrentColumnWidth();

            if( std::get<2>(*case_summary_with_measured_text.last_displayed_text) > current_column_width )
            {
                current_column_width = std::get<2>(*case_summary_with_measured_text.last_displayed_text);

                PostMessage(UWM::Data::AdjustColumnWidthAndInvalidate);
                m_adjustColumnWidthAndInvalidateMessagePosted = true;

                // when adjusting column widths, skip the drawing stage because the
                // entire control will be redrawn upon invalidation
                *pResult = CDRF_SKIPDEFAULT;

                return;
            }

            text = std::get<1>(*case_summary_with_measured_text.last_displayed_text);
        }

        catch( const CSProException& exception )
        {
            icon_index = -1;
            text = std::make_unique<std::wstring>(TC::ToWide(FormatText("<< %s >>", exception.what())));
        }

        ASSERT(text != nullptr);

        CRect rect;
        GetItemRect(item_index, &rect, LVIR_BOUNDS);

        // fill the backgroud
        pDC->FillSolidRect(&rect, m_backgroundColors[color_index]);

        // draw the icon
        m_imageList.Draw(pDC, icon_index, CPoint(rect.left, rect.top), ILD_TRANSPARENT);

        // draw the text
        CRect text_rect = rect;
        text_rect.left += IconWidth + IconTextMargin;

        pLVCD->clrText = m_textColors[color_index];

        ASSERT(text != nullptr);
        pDC->DrawText(text->c_str(), text->length(), &text_rect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

        *pResult = CDRF_SKIPDEFAULT;
    }

    else
    {
        *pResult = CDRF_DODEFAULT;
    }
}


void CaseListingCtrl::OnCacheHint(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    const NMLVCACHEHINT* const cache_hint = reinterpret_cast<const NMLVCACHEHINT*>(pNMHDR);
    ASSERT(( cache_hint->iTo - cache_hint->iFrom ) < CaseSummariesQueryLimit);
    *pResult = 0;
}


void CaseListingCtrl::OnItemChanged(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    NMLISTVIEW* const pNMLV = reinterpret_cast<NMLISTVIEW*>(pNMHDR);

    if( ( pNMLV->uChanged & LVIF_STATE ) != 0 && !m_selectionsChangedMessagePosted )
    {
        PostMessage(UWM::Data::SelectionsChanged);
        m_selectionsChangedMessagePosted = true;
    }

    *pResult = 0;
}


void CaseListingCtrl::OnDoubleClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
    OnCaseListingDoubleClickAndReturn();
}


void CaseListingCtrl::OnRightClick(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    NMITEMACTIVATE* const pNMItemActivate = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);

    OnCaseListingContextMenu(pNMItemActivate->ptAction);

    *pResult = FALSE;
}


void CaseListingCtrl::OnContextMenu()
{
    CRect rect;
    GetClientRect(&rect);

    // the x position will be the smaller of 2/3 of the width of the window, or the column width
    const int x = std::min(rect.Width() * 2 / 3, GetCurrentColumnWidth());
    int y;

    // when an item is selected, the y position will be that of the first selected item
    POSITION pos = GetFirstSelectedItemPosition();

    if( pos != nullptr )
    {
        const int item_index = GetNextSelectedItem(pos);
        GetItemRect(item_index, &rect, LVIR_LABEL);
        y = rect.bottom;
    }

    // otherwise it will be towards the top of the window
    else
    {
        constexpr LONG Margin = 10;
        ASSERT(rect.top == 0);
        y = std::min(Margin, rect.bottom);
    }

    OnCaseListingContextMenu(CPoint(x, y));
}


const std::vector<std::shared_ptr<const CaseSummary>>& CaseListingCtrl::GetSelectedCaseSummaries()
{
    if( m_refreshSelectedCaseSummaries )
    {
        m_selectedCaseSummaries.clear();

        POSITION pos = GetFirstSelectedItemPosition();

        while( pos != nullptr )
        {
            const int item_index = GetNextSelectedItem(pos);

            try
            {
                m_selectedCaseSummaries.emplace_back(GetCaseSummaryWithMeasuredText(item_index).case_summary);
            }
            catch(...) { ASSERT(false); }
        }

        m_refreshSelectedCaseSummaries = false;
    }

    return m_selectedCaseSummaries;
}


int& CaseListingCtrl::GetCurrentColumnWidth()
{
    return m_columnWidths[m_viewableCaseIteratorSettings->GetViewCaseKey() ? 0 : 1];
}


LRESULT CaseListingCtrl::OnAdjustColumnWidthAndInvalidate(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_adjustColumnWidthAndInvalidateMessagePosted);
    m_adjustColumnWidthAndInvalidateMessagePosted = false;

    const int full_column_width = IconWidth + IconTextMargin + GetCurrentColumnWidth();
    SetColumnWidth(0, full_column_width);

    Invalidate();

    return 1;
}


LRESULT CaseListingCtrl::OnSelectionsChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_selectionsChangedMessagePosted);
    m_selectionsChangedMessagePosted = false;

    m_refreshSelectedCaseSummaries = true;
    OnCaseListingSelectionsChanged();

    return 1;
}


LRESULT CaseListingCtrl::OnRequeryCaseSummaries(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_numberCases.reset();
    m_caseSummariesWithMeasuredTexts.clear();

    try
    {
        m_numberCases = m_dataRepository->GetNumberCases(m_viewableCaseIteratorSettings->GetStatus(),
                                                         m_viewableCaseIteratorSettings->GetParameters());
        OnCaseListingCaseSummariesQueried(*m_numberCases);
    }

    catch( const CSProException& exception )
    {
        m_numberCases.reset();
        OnCaseListingCaseSummariesQueried(exception.what());
    }

    SetItemCountEx(m_numberCases.value_or(0), 0);

    return 1;
}


CaseListingCtrl::CaseSummaryWithMeasuredText& CaseListingCtrl::GetCaseSummaryWithMeasuredText(const int index)
{
    ASSERT(m_numberCases.has_value());

    // case summaries will be read in blocks
    const int block_start_index = index / CaseSummariesQueryLimit * CaseSummariesQueryLimit;
    const size_t block_offset = index - block_start_index;

    // check if the item is in the cache
    auto lookup = m_caseSummariesWithMeasuredTexts.find(block_start_index);

    // if not in the cache, query the repository, reading case summaries in blocks
    if( lookup == m_caseSummariesWithMeasuredTexts.cend() )
    {
        const size_t number_cases_to_query = std::min(*m_numberCases, CaseSummariesQueryLimit);

        const std::unique_ptr<CaseIterator> case_summary_iterator = m_dataRepository->CreateIterator(CaseIterationContent::CaseSummary,
                                                                                                     m_viewableCaseIteratorSettings->GetStatus(),
                                                                                                     m_viewableCaseIteratorSettings->GetMethod(),
                                                                                                     m_viewableCaseIteratorSettings->GetOrder(),
                                                                                                     m_viewableCaseIteratorSettings->GetParameters(),
                                                                                                     block_start_index,
                                                                                                     number_cases_to_query);

        std::vector<CaseSummaryWithMeasuredText> case_summaries_with_measured_texts;
        case_summaries_with_measured_texts.reserve(number_cases_to_query);

        while( true )
        {
            auto case_summary = std::make_unique<CaseSummary>();

            if( !case_summary_iterator->NextCaseSummary(*case_summary) )
                break;

            case_summaries_with_measured_texts.emplace_back(CaseSummaryWithMeasuredText { std::move(case_summary) });
        }

        ASSERT(case_summaries_with_measured_texts.size() <= number_cases_to_query);

        // cache the summaries
        lookup = m_caseSummariesWithMeasuredTexts.try_emplace(block_start_index, std::move(case_summaries_with_measured_texts)).first;
    }

    ASSERT(lookup != m_caseSummariesWithMeasuredTexts.cend());

    if( block_offset < lookup->second.size() )
        return lookup->second.at(block_offset);

    ASSERT(false);
    throw CSProException("The case summary at position '%d' could not be found.", index);
}


void CaseListingCtrl::OnCaseListingCaseSummariesQueried(std::variant<size_t, const char*> /*number_cases_or_exception*/)
{
}


void CaseListingCtrl::OnCaseListingSelectionsChanged()
{
}


void CaseListingCtrl::OnCaseListingDoubleClickAndReturn()
{
}


void CaseListingCtrl::OnCaseListingContextMenu(CPoint /*point*/)
{
}


bool CaseListingCtrl::OnCaseListingDeleteKey()
{
    return false;
}
