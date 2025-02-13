#include "StdAfx.h"
#include "ExtendedControl.h"
#include "CapiControl.h"
#include <zUtilO/CustomFont.h>
#include <engine/VarT.h>
#include <CSEntry/UWM.h>


IMPLEMENT_DYNAMIC(CExtendedControl, CDialog)

BEGIN_MESSAGE_MAP(CExtendedControl, CDialog)
    ON_WM_SYSCOMMAND()
    ON_BN_CLICKED(IDC_BUTTON_NEXT_FIELD, &OnClickedButtonNextField)
    ON_BN_CLICKED(IDC_BUTTON_PREVIOUS_FIELD, &OnClickedButtonPreviousField)
    ON_BN_CLICKED(IDC_BUTTON_SEARCH, &OnClickedButtonSearch)
END_MESSAGE_MAP()


namespace
{
    const int SEARCH_BAR_TOP_BORDER = 5;
    const int SEARCH_BAR_BOTTOM_BORDER = 5;
}


CExtendedControl::CExtendedControl(CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_EXTENDEDCONTROL)
{
    m_pParent = pParent;
    font.CreatePointFont(8 * 10,_T("MS Shell Dlg"));//_T("MS Sans Serif")); GHM 20121119 this wasn't working for cyrillic fonts in drop down and checkbox options
}

CExtendedControl::~CExtendedControl()
{
    // delete m_pCapiControl;
}

BOOL CExtendedControl::PreTranslateMessage(MSG* pMsg)
{
    m_toolTips.RelayEvent(pMsg);

    // If user hits enter inside search edit box then do the search
    if (pMsg->message == WM_KEYDOWN &&
        pMsg->wParam == VK_RETURN &&
        GetFocus() == GetDlgItem(IDC_EDIT_SEARCH_TEXT))
    {
        OnClickedButtonSearch();

        // Set focus on back on field on form so that you can use keys to navigate
        m_pEdit->SetFocus();
        return TRUE; // this doesn't need processing anymore
    }

    return FALSE; // all other cases still need default processing
}




// CExtendedControl message handlers


int CExtendedControl::DoModeless(VART* pVarT, const CaptureInfo& capture_info, const ResponseProcessor* response_processor,
    CWnd* pEdit, bool bCommaDecimal)
{
    m_pVarT = pVarT;
    m_captureInfo = capture_info;
    m_captureType = m_captureInfo.GetCaptureType();
    m_pDictItem = pVarT->GetDictItem();
    m_responseProcessor = response_processor;
    m_pEdit = pEdit;
    m_bCommaDecimal = bCommaDecimal;

    ASSERT(response_processor != nullptr || ( m_captureType == CaptureType::Date || m_captureType == CaptureType::NumberPad ));

    if( Create(IDD_EXTENDEDCONTROL, m_pParent) )
    {
        // Hide the search box for the date and number pad
        if( ( m_captureType == CaptureType::Date ) || ( m_captureType == CaptureType::NumberPad ) )
        {
            GetDlgItem(IDC_BUTTON_SEARCH)->ShowWindow(SW_HIDE);
            GetDlgItem(IDC_EDIT_SEARCH_TEXT)->ShowWindow(SW_HIDE);
        }

        // Hide search box and next/prev for numberpad since we have those buttons on num pad
        if( m_captureType == CaptureType::NumberPad )
        {
            GetDlgItem(IDC_BUTTON_SEARCH)->ShowWindow(SW_HIDE);
            GetDlgItem(IDC_EDIT_SEARCH_TEXT)->ShowWindow(SW_HIDE);

            GetDlgItem(IDC_BUTTON_PREVIOUS_FIELD)->ShowWindow(SW_HIDE);
            GetDlgItem(IDC_BUTTON_NEXT_FIELD)->ShowWindow(SW_HIDE);
        }


        HINSTANCE hInstResource = AfxFindResourceHandle(MAKEINTRESOURCE(IDI_FIND), RT_GROUP_ICON);

        HICON findIcon = (HICON)::LoadImage(hInstResource, MAKEINTRESOURCE(IDI_FIND), IMAGE_ICON, 16, 16, LR_SHARED);
        ((CButton*) GetDlgItem(IDC_BUTTON_SEARCH))->SetIcon(findIcon);
        HICON prevIcon = (HICON)::LoadImage(hInstResource, MAKEINTRESOURCE(IDI_PREVIOUS), IMAGE_ICON, 16, 16, LR_SHARED);
        ((CButton*) GetDlgItem(IDC_BUTTON_PREVIOUS_FIELD))->SetIcon(prevIcon);
        HICON nextIcon = (HICON)::LoadImage(hInstResource, MAKEINTRESOURCE(IDI_NEXT), IMAGE_ICON, 16, 16, LR_SHARED);
        ((CButton*) GetDlgItem(IDC_BUTTON_NEXT_FIELD))->SetIcon(nextIcon);
        m_toolTips.Create(this);
        m_toolTips.AddTool(GetDlgItem(IDC_BUTTON_SEARCH), IDS_TOOLTIP_SEARCH);
        m_toolTips.AddTool(GetDlgItem(IDC_EDIT_SEARCH_TEXT), IDS_TOOLTIP_SEARCH);
        m_toolTips.AddTool(GetDlgItem(IDC_BUTTON_PREVIOUS_FIELD), IDS_TOOLTIP_PREV);
        m_toolTips.AddTool(GetDlgItem(IDC_BUTTON_NEXT_FIELD), IDS_TOOLTIP_NEXT);
        m_toolTips.Activate(true);

        m_pCapiControl = new CCapiControl; // it eventually gets deleted within CCapiControl
        m_pCapiControl->LaunchWindow(this);

        CSize controlsSize = m_pCapiControl->GetControlsMinSize();
        m_OrigControlsRect = CRect(CPoint(0,0), controlsSize);
        m_minSearchBarSize = GetSearchBarSize();
        CRect rect = m_OrigControlsRect;

        if( !m_pVarT->GetShowExtendedControlTitle() ) // GHM 20100708 use title functionality added
            SetWindowText(_T(""));

        else if( m_captureType == CaptureType::Date || m_captureType == CaptureType::NumberPad )
            SetWindowText(CaptureInfo::GetCaptureTypeName(m_captureType, true));

        else
            SetWindowText(pVarT->GetDictItem()->GetLabel());

        DoSizing(rect);
        SetWindowPos(NULL,rect.left,rect.top,rect.Width(),rect.Height(),SWP_NOACTIVATE);
        PlaceSearchBarControls();
        m_pCapiControl->LayoutControls();

        CString fieldContents;
        m_pEdit->GetWindowText(fieldContents);
        m_pCapiControl->UpdateSelection(fieldContents);

        ShowWindow(SW_SHOWNORMAL);

        m_pEdit->SetFocus();

        return IDOK;
    }

    return IDCANCEL;
}


void CExtendedControl::UpdateSelection(const CString& keyedText)
{
    m_pCapiControl->UpdateSelection(keyedText);
}

CSize CExtendedControl::GetSearchBarSize()
{
    // No additional controls for number pad
   // if (m_iCaptureType == CAPTURE_TYPE_NUMBERPAD) {
    //  return CSize(0,0);
    //}

    CButton *prevButton = (CButton*) GetDlgItem(IDC_BUTTON_PREVIOUS_FIELD);
    CRect prevButtonRect;
    prevButton->GetWindowRect(prevButtonRect);
    ScreenToClient(prevButtonRect);
    CButton *nextButton = (CButton*) GetDlgItem(IDC_BUTTON_NEXT_FIELD);
    CRect nextButtonRect;
    nextButton->GetWindowRect(nextButtonRect);
    ScreenToClient(nextButtonRect);
    // Don't add space for the search box and button for date
    int searchBarHeight = prevButtonRect.Height() + SEARCH_BAR_TOP_BORDER + SEARCH_BAR_BOTTOM_BORDER;

    if( m_captureType == CaptureType::Date )
        return CSize(nextButtonRect.Width() + prevButtonRect.Width() + 10, searchBarHeight);
    else
        return CSize(nextButtonRect.right - prevButtonRect.right, searchBarHeight);
}

// Layout and size the search edit box, search button, prev and next
// buttons to fit width of the window and be at the bottom of the window.
void CExtendedControl::PlaceSearchBarControls()
{
    // No additional controls for number pad
    //if (m_iCaptureType == CAPTURE_TYPE_NUMBERPAD) {
    //  return;
    //}

    CRect rect;
    GetClientRect(rect);

    // Get positions of all the controls from the resources as a starting point.
    CButton *prevButton = (CButton*) GetDlgItem(IDC_BUTTON_PREVIOUS_FIELD);
    CRect prevButtonRect;
    prevButton->GetWindowRect(prevButtonRect);
    ScreenToClient(prevButtonRect);
    CButton *nextButton = (CButton*) GetDlgItem(IDC_BUTTON_NEXT_FIELD);
    CRect nextButtonRect;
    nextButton->GetWindowRect(nextButtonRect);
    ScreenToClient(nextButtonRect);
    CButton *searchButton = (CButton*) GetDlgItem(IDC_BUTTON_SEARCH);
    CRect searchButtonRect;
    searchButton->GetWindowRect(searchButtonRect);
    ScreenToClient(searchButtonRect);
    CEdit *searchTextEdit = (CEdit*) GetDlgItem(IDC_EDIT_SEARCH_TEXT);
    CRect searchEditRect;
    searchTextEdit->GetWindowRect(searchEditRect);
    ScreenToClient(searchEditRect);

    // Set top of controls so that bottom will be aligned to bottom of window
    // minus border.
    int searchBarTop = rect.bottom - m_minSearchBarSize.cy + SEARCH_BAR_TOP_BORDER;

    // Align next button with bottom right corner of window
    nextButtonRect.OffsetRect(- nextButtonRect.left + rect.Width() - nextButtonRect.Width() - 1,0);
    nextButton->SetWindowPos(NULL, nextButtonRect.left, searchBarTop, nextButtonRect.Width(), nextButtonRect.Height(), SWP_NOZORDER);

    // Place search button just to left of next button
    searchButtonRect.OffsetRect(-searchButtonRect.left + nextButtonRect.left - searchButtonRect.Width() - 10,0);
    searchButton->SetWindowPos(NULL, searchButtonRect.left, searchBarTop, searchButtonRect.Width(), searchButtonRect.Height(), SWP_NOZORDER);

    // Make search edit span available space between prev and search buttons
    searchEditRect.right = searchButtonRect.left - 5;
    searchTextEdit->SetWindowPos(NULL, searchEditRect.left, searchBarTop, searchEditRect.Width(), searchEditRect.Height(), SWP_NOZORDER);

    // Align prev button with bottom left of window
    prevButton->SetWindowPos(NULL, prevButtonRect.left, searchBarTop, prevButtonRect.Width(), prevButtonRect.Height(), SWP_NOZORDER);

    // Hide search edit box by default
    searchTextEdit->ShowWindow(SW_HIDE);
}

void CExtendedControl::DoSizing(CRect & rect)
{
    CString windowText;
    GetWindowText(windowText);
    CDC* pDC = GetDC();
    int titleWidth = pDC->GetTextExtent(windowText).cx + 5; // some extra space for the window edges
    ReleaseDC(pDC);
    if (titleWidth > rect.Width()) {
        // try to make window big enough to cover title but don't make it more than 50% bigger so that
        // we avoid a super long window.
        int widthIncrease = titleWidth - rect.Width();
        widthIncrease =std::min(widthIncrease, rect.Width()/2);
        rect.right += widthIncrease;
    }

    CRect controlRect = rect;

    if( m_captureType != CaptureType::NumberPad )
    {
        // Make sure window is wide enough for the search controls ( we don't show search controls for number pad)
        if( rect.Width() < m_minSearchBarSize.cx )
            rect.right += ( m_minSearchBarSize.cx - rect.Width() );

        controlRect = rect;

        rect.bottom += m_minSearchBarSize.cy;
    }

    // add space for window borders to get full Window rect
    CalcWindowRect(&rect,CWnd::adjustBorder);

    m_pCapiControl->ShowScrollBar(SB_BOTH,FALSE);
    m_pCapiControl->SetWindowPos(NULL,0, 0,controlRect.Width(),controlRect.Height(),SWP_NOACTIVATE);

    // we will position the box to the right of the field and centered vertically
    // it may get moved elsewhere
    CRect fieldRect;
    m_pEdit->GetWindowRect(&fieldRect);

    CRect frameWindowRect;
    m_pEdit->GetParent()->GetWindowRect(&frameWindowRect);

    const int spacing = 25; // how much space to leave in between the field and the responses window

    // there is a chance that the field is not in the frame window; in this case we will display modify
    // the field window so that the control window doesn't get scrunched up against one side of the screen
    if( frameWindowRect.right < fieldRect.left )
        fieldRect.left = fieldRect.right = frameWindowRect.right - spacing;

    if( frameWindowRect.left > fieldRect.right )
        fieldRect.left = fieldRect.right = frameWindowRect.left + spacing;

    if( frameWindowRect.bottom < fieldRect.top )
        fieldRect.bottom = fieldRect.top = frameWindowRect.bottom - spacing;

    if( frameWindowRect.top > fieldRect.bottom )
        fieldRect.bottom = fieldRect.top = frameWindowRect.top + spacing;

    // see where to place the box (depending on where the field is relative to the frame window)

    int responsesWidth = rect.Width() + spacing;
    int responsesHeight = rect.Height() + spacing;

    int SpaceOnLeft = fieldRect.left - frameWindowRect.left;
    int SpaceOnRight = frameWindowRect.right - fieldRect.right;
    int SpaceOnTop = fieldRect.top - frameWindowRect.top;
    int SpaceOnBottom = frameWindowRect.bottom - fieldRect.bottom;

    int newX, newY;

    // 20110502, using the setcapturepos function a user can force the window to show up at a certain part of the screen
    const POINT& origin_point = m_pVarT->GetCapturePos();

    if( ( origin_point.x >= 0 ) && ( ( origin_point.x + spacing ) < frameWindowRect.Width() ) &&
        ( origin_point.y >= 0 ) && ( ( origin_point.y + spacing ) < frameWindowRect.Height() ) )
    {
        newX = frameWindowRect.left + origin_point.x;
        newY = frameWindowRect.top + origin_point.y;

        // check for cases when scroll bars will need to be added
        int scrollWidth = GetSystemMetrics(SM_CXVSCROLL);
        int scrollHeight = GetSystemMetrics(SM_CYHSCROLL);

        SIZE scrollSize;
        scrollSize.cx = controlRect.Width();
        scrollSize.cy = controlRect.Height();

        int initialWidth = rect.Width();
        int initialHeight = rect.Height();

        bool fitsHorizontally = ( newX + responsesWidth + scrollWidth ) < frameWindowRect.right;
        bool fitsVertically = ( newY + responsesHeight + scrollHeight ) < frameWindowRect.bottom;

        if( fitsHorizontally && !fitsVertically )
        {
            rect.left = newX;
            rect.right = newX + initialWidth + scrollWidth;

            rect.top = newY;
            rect.bottom = frameWindowRect.bottom - spacing;

            int heightChanged = initialHeight - rect.Height();

            controlRect.right += scrollWidth;
            m_pCapiControl->SetWindowPos(NULL,0,0,controlRect.Width(),controlRect.Height() - heightChanged,SWP_NOMOVE);

            scrollSize.cy -= scrollHeight;
            m_pCapiControl->SetScrollSizes(MM_TEXT,scrollSize);
            m_pCapiControl->ShowScrollBar(SB_HORZ,FALSE); // add only a vertical scroll bar
        }

        else if( !fitsHorizontally && fitsVertically )
        {
            rect.left = newX;
            rect.right = frameWindowRect.right - spacing;

            rect.top = newY;
            rect.bottom = newY + initialHeight + scrollWidth;

            int widthChanged = initialWidth - rect.Width();

            controlRect.bottom += scrollHeight;
            m_pCapiControl->SetWindowPos(NULL,0,0,controlRect.Width() - widthChanged,controlRect.Height(),SWP_NOMOVE);

            scrollSize.cx -= scrollWidth;
            m_pCapiControl->SetScrollSizes(MM_TEXT,scrollSize);
            m_pCapiControl->ShowScrollBar(SB_VERT,FALSE); // add only a horizontal scroll bar
        }

        else if( !fitsHorizontally && !fitsVertically )
        {
            rect.left = newX;
            rect.right = frameWindowRect.right - spacing;

            rect.top = newY;
            rect.bottom = frameWindowRect.bottom - spacing;

            int widthChanged = initialWidth - rect.Width();
            int heightChanged = initialHeight - rect.Height();

            m_pCapiControl->SetWindowPos(NULL,0,0,controlRect.Width() - widthChanged,controlRect.Height() - heightChanged,SWP_NOMOVE);

            m_pCapiControl->SetScrollSizes(MM_TEXT,scrollSize);
        }
    }


    else if( SpaceOnRight > responsesWidth && responsesHeight < frameWindowRect.Height() ) // right
    {
        newX = fieldRect.right + spacing;
        newY =std::max( ( fieldRect.top + fieldRect.Height() / 2 ) - ( rect.Height() / 2 ),
                    ( frameWindowRect.top + spacing ) );
    }

    else if( SpaceOnLeft > responsesWidth && responsesHeight < frameWindowRect.Height() ) // left
    {
        newX = fieldRect.left - spacing - rect.Width();
        newY =std::max( ( fieldRect.top + fieldRect.Height() / 2 ) - ( rect.Height() / 2 ),
                    ( frameWindowRect.top + spacing ) );
    }

    else if( SpaceOnTop > responsesHeight && responsesWidth < frameWindowRect.Width() ) // top
    {
        newX =std::max( ( fieldRect.left + fieldRect.Width() / 2 ) - ( rect.Width() / 2 ),
                    ( frameWindowRect.left + spacing ) );
        newY = fieldRect.top - spacing - rect.Height();
    }

    else if( SpaceOnBottom > responsesHeight && responsesWidth < frameWindowRect.Width() )
    {
        newX =std::max( ( fieldRect.left + fieldRect.Width() / 2 ) - ( rect.Width() / 2 ),
                    ( frameWindowRect.left + spacing ) );
        newY = fieldRect.bottom + spacing;
    }

    else // there still isn't a place for it so we will have to resize the responses window
    {
        int scrollWidth = GetSystemMetrics(SM_CXVSCROLL);
        int scrollHeight = GetSystemMetrics(SM_CYHSCROLL);

        bool fitsHorizontally =std::max(SpaceOnLeft,SpaceOnRight) > ( responsesWidth + scrollWidth );
        bool fitsVertically =std::max(SpaceOnTop,SpaceOnBottom) > ( responsesHeight + scrollHeight );

        SIZE scrollSize;
        scrollSize.cx = controlRect.Width();
        scrollSize.cy = controlRect.Height();

        int initialWidth = rect.Width();
        int initialHeight = rect.Height();

        if( fitsHorizontally )
        {
            responsesWidth += scrollWidth;
            rect.right += scrollWidth;

            if( SpaceOnRight > responsesWidth )
                newX = fieldRect.right + spacing;

            else
                newX = fieldRect.left - spacing - rect.Width();

            rect.top = newY = frameWindowRect.top + spacing;
            rect.bottom = frameWindowRect.bottom - spacing;

            int heightChanged = initialHeight - rect.Height();

            controlRect.right += scrollWidth;
            m_pCapiControl->SetWindowPos(NULL,0,0,controlRect.Width(),controlRect.Height() - heightChanged,SWP_NOMOVE);

            scrollSize.cy -= scrollHeight;
            m_pCapiControl->SetScrollSizes(MM_TEXT,scrollSize);
            m_pCapiControl->ShowScrollBar(SB_HORZ,FALSE); // add only a vertical scroll bar
        }

        else if( fitsVertically )
        {
            responsesHeight += scrollHeight;
            rect.bottom += scrollHeight;

            if( SpaceOnTop > responsesHeight )
                newY = fieldRect.top - spacing - rect.Height();

            else
                newY = fieldRect.bottom + spacing;

            rect.left = newX = frameWindowRect.left + spacing;
            rect.right = frameWindowRect.right - spacing;

            int widthChanged = initialWidth - rect.Width();

            controlRect.bottom += scrollHeight;
            m_pCapiControl->SetWindowPos(NULL,0,0,controlRect.Width() - widthChanged,controlRect.Height(),SWP_NOMOVE);

            scrollSize.cx -= scrollWidth;
            m_pCapiControl->SetScrollSizes(MM_TEXT,scrollSize);
            m_pCapiControl->ShowScrollBar(SB_VERT,FALSE); // add only a horizontal scroll bar
        }

        else // the control needs to shrunken in both directions
        {
            double xFill =std::max(SpaceOnLeft,SpaceOnRight) / double( responsesWidth + scrollWidth );
            double yFill =std::max(SpaceOnTop,SpaceOnBottom) / double( responsesHeight + scrollHeight );

            bool prioritizeX = xFill > yFill; // we will put the control to the left/right of the field

            if( prioritizeX )
            {
                if( SpaceOnRight > SpaceOnLeft )
                {
                    rect.left = newX = fieldRect.right + spacing;
                    rect.right = frameWindowRect.right - spacing;
                }
                else
                {
                    rect.left = newX = frameWindowRect.left + spacing;
                    rect.right = fieldRect.right - spacing;
                }

                // for extraordinarily large fields that don't fit on a single screen, don't pointlessly
                // make the window so large just to get close to the right side of the field
                if( rect.Width() > initialWidth )
                    rect.right = rect.left + initialWidth;

                rect.top = newY = frameWindowRect.top + spacing;
                rect.bottom = frameWindowRect.bottom - spacing;
            }

            else // prioritizeY
            {
                if( SpaceOnTop > SpaceOnBottom )
                {
                    rect.top = newY = frameWindowRect.top + spacing;
                    rect.bottom = fieldRect.top - spacing;
                }
                else
                {
                    rect.top = newY = fieldRect.bottom + spacing;
                    rect.bottom = frameWindowRect.bottom - spacing;
                }

                rect.left = newX = frameWindowRect.left + spacing;
                rect.right = frameWindowRect.right - spacing;
            }

            int widthChanged = initialWidth - rect.Width();
            int heightChanged = initialHeight - rect.Height();

            m_pCapiControl->SetWindowPos(NULL,0,0,controlRect.Width() - widthChanged,controlRect.Height() - heightChanged,SWP_NOMOVE);

            m_pCapiControl->SetScrollSizes(MM_TEXT,scrollSize);
        }
    }

    rect.MoveToXY(newX,newY);
}



void CExtendedControl::RefreshPosition()
{
    CRect rect = m_OrigControlsRect;
    DoSizing(rect);
    SetWindowPos(NULL,rect.left,rect.top,rect.Width(),rect.Height(),SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    PlaceSearchBarControls();
    m_pCapiControl->LayoutControls();
}

CFont* CExtendedControl::GetControlFont(bool font_for_number_pad/* = false*/)
{
    // GHM 20100621 to allow for the dynamic setting of fonts for controls
    UserDefinedFonts::FontType font_type = font_for_number_pad ?
        UserDefinedFonts::FontType::NumberPad : UserDefinedFonts::FontType::ValueSets;

    UserDefinedFonts* pUserFonts = nullptr;
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);

    if( pUserFonts != nullptr && pUserFonts->IsFontDefined(font_type) ) // user has defined a particular font
        return pUserFonts->GetFont(font_type);

    else
        return &font;
}


void CExtendedControl::OnSysCommand(UINT nID,LPARAM lParam)
{
    UINT command = nID & 0xFFF0;

    if( command == SC_CLOSE )
        AfxGetApp()->GetMainWnd()->PostMessage(WM_SYSCOMMAND,nID,lParam);

    else CDialog::OnSysCommand(nID, lParam);
}

void CExtendedControl::OnClickedButtonNextField()
{
    m_pParent->PostMessage(UWM::CSEntry::ChangeEdit, VK_RIGHT, (LPARAM) m_pEdit);
}


void CExtendedControl::OnClickedButtonPreviousField()
{
    m_pParent->PostMessage(UWM::CSEntry::ChangeEdit, VK_LEFT, (LPARAM) m_pEdit);
}


void CExtendedControl::OnClickedButtonSearch()
{
    auto search_edit_text = (CEdit*)GetDlgItem(IDC_EDIT_SEARCH_TEXT);
    if (search_edit_text->IsWindowVisible()) {
        CString search_string;
        search_edit_text->GetWindowText(search_string);
        m_pCapiControl->Filter(search_string);
    }
    else {
        search_edit_text->ShowWindow(SW_NORMAL);
        search_edit_text->SetFocus();
    }
}
