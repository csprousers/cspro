#include "StdAfx.h"
#include "LoggingListBox.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/Filedlg.h>
#include <zUtilO/WindowHelpers.h>


namespace Action
{
    constexpr WPARAM Clear               = 1;
    constexpr WPARAM AddText             = 2;
    constexpr WPARAM SetHorizontalExtent = 3;
}

namespace Scroll
{ 
    constexpr UINT_PTR TimerCode          = 1;
    constexpr UINT ElapseTimeMilliseconds = 3;
}


BEGIN_MESSAGE_MAP(LoggingListBox, CListBox)

    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_TIMER()

    ON_WM_CONTEXTMENU()
    ON_WM_KEYDOWN()

    ON_MESSAGE(UWM::UtilF::UpdateLoggingListBox, OnLoggingListBoxUpdate)

    ON_COMMAND(ID_EDIT_COPY, OnCopySelectedLinesToClipboard)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateCopySelectedLinesToClipboard)

    ON_COMMAND(ID_EDIT_CLEAR, OnClearLines)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, OnUpdateClearLines)

    ON_COMMAND(ID_EDIT_SELECT_ALL, OnSelectAllLines)
    ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateSelectAllLines)

    ON_COMMAND(ID_FILE_SAVE, OnSaveLines)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateSaveLines)

END_MESSAGE_MAP()


LoggingListBox::LoggingListBox()
    :   m_logfont{ 0 },
        m_maxLineLengthAndHorizontalExtent(0, 0),
        m_scrollLinesDelta(3),
        m_pendingMouseWheelActions(0),
        m_userScrolledManually(false)
{
    m_logfont.lfHeight = 18;
    lstrcpyn(m_logfont.lfFaceName, _T("Consolas"), LF_FACESIZE);

    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &m_scrollLinesDelta, 0);
}


BOOL LoggingListBox::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
    dwStyle |= WS_HSCROLL | WS_VSCROLL |
               LBS_EXTENDEDSEL | LBS_OWNERDRAWFIXED | LBS_NODATA | LBS_NOINTEGRALHEIGHT;

    return __super::Create(dwStyle, rect, pParentWnd, nID);
}


void LoggingListBox::PreSubclassWindow()
{
    __super::PreSubclassWindow();

    if( m_font.CreateFontIndirect(&m_logfont) )
        SetFont(&m_font, FALSE); 
}


void LoggingListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    lpMeasureItemStruct->itemHeight = m_logfont.lfHeight;
}


void LoggingListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if( lpDrawItemStruct->itemAction != ODA_SELECT && lpDrawItemStruct->itemAction != ODA_DRAWENTIRE )
        return;

    // draw the background and set the text colors
    int background_color_index;
    int text_color_index;

    if( ( lpDrawItemStruct->itemState & ODS_SELECTED ) != 0 )
    {
        background_color_index = COLOR_HIGHLIGHT;
        text_color_index = COLOR_HIGHLIGHTTEXT;
    }

    else
    {
        background_color_index = COLOR_WINDOW;
        text_color_index = COLOR_WINDOWTEXT;
    }

    ::FillRect(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem, reinterpret_cast<HBRUSH>(background_color_index + 1));
    ::SetBkColor(lpDrawItemStruct->hDC, ::GetSysColor(background_color_index));
    ::SetTextColor(lpDrawItemStruct->hDC, ::GetSysColor(text_color_index));

    // draw the text
    const std::wstring* text;

    if( static_cast<size_t>(lpDrawItemStruct->itemID) < m_lines.size() )
    {
        std::lock_guard<std::mutex> lock(m_linesMutex);
        text = m_lines[lpDrawItemStruct->itemID].get();
    }

    else
    {
        ASSERT(false);
        return;
    }

    // if the text is longer than the longest measured text, modify the horizonal extent
    if( std::get<0>(m_maxLineLengthAndHorizontalExtent) < text->length() )
    {
        std::get<0>(m_maxLineLengthAndHorizontalExtent) = text->length();

        SIZE size;
        ::GetTextExtentPoint32(lpDrawItemStruct->hDC, text->c_str(), text->length(), &size);

        if( size.cx > std::get<1>(m_maxLineLengthAndHorizontalExtent) )
        {
            std::get<1>(m_maxLineLengthAndHorizontalExtent) = size.cx;
            PostMessage(UWM::UtilF::UpdateLoggingListBox, Action::SetHorizontalExtent);
        }
    }

    // draw the text
    ::DrawText(lpDrawItemStruct->hDC, text->c_str(), text->length(), &lpDrawItemStruct->rcItem,
               DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
}


void LoggingListBox::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    m_userScrolledManually = true;

    __super::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL LoggingListBox::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
    m_userScrolledManually = true;

    // without this fix, scrolling with the mouse wheel led to lots of screen flicker, so scroll manually using timer calls
    // https://stackoverflow.com/questions/18846080/mfc-ownerdrawn-listbox-scroll-issue

    // scrolling up
    if( zDelta > 0 )
    {
        --m_pendingMouseWheelActions;
    }

    // scrolling down
    else
    {
        ++m_pendingMouseWheelActions;
    }

    SetTimer(Scroll::TimerCode, Scroll::ElapseTimeMilliseconds, nullptr);

    return TRUE;
}


void LoggingListBox::OnTimer(UINT_PTR nIDEvent)
{
    if( nIDEvent == Scroll::TimerCode )
    {
        // scrolling up
        if( m_pendingMouseWheelActions < 0 )
        {
            int new_index = GetTopIndex() - m_scrollLinesDelta;

            if( new_index <= 0 )
            {
                new_index = 0;
                m_pendingMouseWheelActions = 0;
            }

            else
            {
                ++m_pendingMouseWheelActions;
            }

            SetTopIndex(new_index);
        }

        // scrolling down
        else if( m_pendingMouseWheelActions > 0 )
        {
            int new_index = GetTopIndex() + m_scrollLinesDelta;
            int max_index = GetCount() - 1;

            if( new_index >= max_index )
            {
                max_index = max_index;
                m_pendingMouseWheelActions = 0;
            }

            else
            {
                --m_pendingMouseWheelActions;
            }

            SetTopIndex(new_index);
        }

        if( m_pendingMouseWheelActions == 0 )
            KillTimer(Scroll::TimerCode);
    }

    else
    {
        __super::OnTimer(nIDEvent);
    }
}


void LoggingListBox::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    bool c_pressed = ( nChar == 'C' );

    // Ctrl+C: copy selected items to the clipboard
    // Ctrl+A: select all lines
    if( ( c_pressed || nChar == 'A' ) && GetKeyState(VK_CONTROL) < 0 )
    {
        PostMessage(WM_COMMAND, c_pressed ? ID_EDIT_COPY :
                                            ID_EDIT_SELECT_ALL);
    }

    else
    {
        __super::OnKeyDown(nChar, nRepCnt, nFlags);
    }
}


void LoggingListBox::OnContextMenu(CWnd* pWnd, CPoint pos)
{
    // if invoked using the keyboard, determine where to show the menu
    if( pos.x < 0 )
    {
        ASSERT(pos.x == -1 && pos.y == -1);

        CRect rect;
        GetClientRect(rect);

        // if an item is selected and it is visible, display the menu (y-pos) by the selection;
        // otherwise display it in the center of the window
        int selected_index = GetCurSel();
        std::optional<LONG> y_pos;

        if( selected_index != LB_ERR )
        {
            CRect selected_rect;
            
            if( GetItemRect(selected_index, selected_rect) != LB_ERR &&
                selected_rect.left >= rect.left && selected_rect.right <= rect.right &&
                selected_rect.top >= rect.top && selected_rect.bottom <= rect.bottom )
            {
                ClientToScreen(selected_rect);
                y_pos = ( selected_rect.top + selected_rect.bottom ) / 2;
            }
        }

        ClientToScreen(rect);

        if( !y_pos.has_value() )
            y_pos = ( rect.top + rect.bottom ) / 2;

        // the x-pos of the menu will be 25% of the way into the window
        pos = CPoint(rect.left + rect.Width() / 4, *y_pos);
    }

    // load, update, and show the menu
    CMenu menu;
    menu.LoadMenu(IDR_LOGGING_LIST_BOX);
    ASSERT(menu.GetMenuItemCount() == 1);

    CMenu* popup_menu = menu.GetSubMenu(0);

    AddAdditionalContextMenuItems(*popup_menu);

    WindowHelpers::DoUpdateForMenuItems(popup_menu, this);

    popup_menu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, pWnd);
}


void LoggingListBox::AddAdditionalContextMenuItems(CMenu& /*popup_menu*/)
{
}


LRESULT LoggingListBox::OnLoggingListBoxUpdate(WPARAM wParam, LPARAM /*lParam*/)
{
    // these actions are handled using messages so that a thread using logging
    // can continue without waiting for any list box UI thread to end
    if( wParam == Action::AddText )
    {
        auto auto_scroll = [&]()
        {
            // scroll automatically only if the user hasn't manually scroled
            if( !m_userScrolledManually )
                SetTopIndex(GetCount() - 1);
        };

        int strings_to_add = m_lines.size() - GetCount();

        // if there are multiple strings to add, suspend UI updates temporarily
        if( strings_to_add > 1 )
        {
            SetRedraw(FALSE);

            while( strings_to_add-- > 0 )
                AddString(nullptr);

            auto_scroll();

            SetRedraw(TRUE);

            Invalidate();
        }

        else if( strings_to_add == 1 )
        {
            AddString(nullptr);
            auto_scroll();
        }
    }

    else if( wParam == Action::Clear )
    {
        ResetContent();
        m_maxLineLengthAndHorizontalExtent = { 0, 0 };
        m_pendingMouseWheelActions = 0;
        m_userScrolledManually = false;
    }

    else if( wParam == Action::SetHorizontalExtent )
    {
        SetHorizontalExtent(std::get<1>(m_maxLineLengthAndHorizontalExtent));
    }

    return 1;
}


void LoggingListBox::Clear()
{
    // clear the lines
    {
        std::lock_guard<std::mutex> lock(m_linesMutex);
        m_lines.clear();
    }

    PostMessage(UWM::UtilF::UpdateLoggingListBox, Action::Clear);
}


void LoggingListBox::AddText(std::wstring text)
{
    // make sure text with newlines are displayed on separate lines
    if( text.find_last_of(_T("\r\n")) != std::wstring::npos )
    {
        SO::ForeachLine(text, true,
            [&](wstring_view line)
            {
                AddText(line);
                return true;
            });

        return;
    }

    // add the line
    {
        std::lock_guard<std::mutex> lock(m_linesMutex);
        m_lines.emplace_back(std::make_unique<std::wstring>(std::move(text)));
    }

    PostMessage(UWM::UtilF::UpdateLoggingListBox, Action::AddText);
}


void LoggingListBox::OnCopySelectedLinesToClipboard()
{
    size_t selected_count = GetSelCount();
    auto selected_indices = std::make_unique<int[]>(selected_count);
    GetSelItems(selected_count, selected_indices.get());

    std::wstring clipboard_text;

    // combine the lines into a single line
    {
        std::lock_guard<std::mutex> lock(m_linesMutex);

        for( size_t i = 0; i < selected_count; ++i )
        {
            size_t index = selected_indices[i];

            if( index < m_lines.size() )
            {
                clipboard_text.append(*m_lines[index]);
                clipboard_text.push_back('\n');
            }
        }
    }

    WinClipboard::PutText(this, clipboard_text);
}


void LoggingListBox::OnUpdateCopySelectedLinesToClipboard(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetSelCount() > 0);
}


void LoggingListBox::OnClearLines()
{
    Clear();
}

void LoggingListBox::OnUpdateClearLines(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetCount() > 0);
}


void LoggingListBox::OnSelectAllLines()
{
    SelItemRange(TRUE, 0, GetCount());
}


void LoggingListBox::OnUpdateSelectAllLines(CCmdUI* pCmdUI)
{
    int count = GetCount();
    pCmdUI->Enable(count > 0 && count != GetSelCount());
}


void LoggingListBox::OnSaveLines()
{
    CIMSAFileDialog file_dlg(FALSE, _T("txt, *"), nullptr, OFN_HIDEREADONLY, _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));
    file_dlg.m_ofn.lpstrTitle = _T("Save Log Lines");

    if( file_dlg.DoModal() != IDOK )
        return;

    std::wstring text;

    // combine the lines into a single line
    {
        std::lock_guard<std::mutex> lock(m_linesMutex);

        for( const std::unique_ptr<std::wstring>& line : m_lines )
        {
            text.append(*line);
            text.push_back('\n');
        }
    }

    // save the text
    try
    {
        FileIO::WriteText(file_dlg.GetPathName(), text, true);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void LoggingListBox::OnUpdateSaveLines(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetCount() != 0);
}
