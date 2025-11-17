#include "StdAfx.h"
#include "LoggingListBox.h"
#include <zToolsO/File.h>
#include <zUtilO/FileDlg.h>


namespace
{
    constexpr size_t MaxLineLength = 8 * 1024;
}

namespace Action
{
    constexpr WPARAM Clear               = 1;
    constexpr WPARAM AddText             = 2;
    constexpr WPARAM SetHorizontalExtent = 3;
}

namespace TimerCode
{
    constexpr UINT AddText = 1;
    constexpr UINT Scroll  = 2;
}

namespace AddText
{
    constexpr size_t MaxLinesForDirectUpdate = 10;
    constexpr UINT ElapseTimeMilliseconds    = 100;
}

namespace Scroll
{
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
        m_userScrolledManually(false),
        m_addTextMessagePending(false)
{
    // create a fixed-width font
    m_logfont.lfHeight = 18;
    lstrcpyn(m_logfont.lfFaceName, L"Consolas", LF_FACESIZE);
    m_font.CreateFontIndirect(&m_logfont);

    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &m_scrollLinesDelta, 0);
}


BOOL LoggingListBox::Create(DWORD dwStyle, const RECT& rect, CWnd* const pParentWnd, const UINT nID)
{
    dwStyle |= WS_HSCROLL | WS_VSCROLL |
               LBS_EXTENDEDSEL | LBS_OWNERDRAWFIXED | LBS_NODATA | LBS_NOINTEGRALHEIGHT;

    return __super::Create(dwStyle, rect, pParentWnd, nID);
}


void LoggingListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    lpMeasureItemStruct->itemHeight = m_logfont.lfHeight;
}


const std::wstring* LoggingListBox::GetWideLine(const size_t index, const bool line_is_for_displaying)
{
    if( index >= m_lines.size() )
        return ReturnProgrammingError(&SO::Empty_wstring);

    Line& line = m_lines[index];

    if( line.wide_line == nullptr )
        line.wide_line = std::make_unique<std::wstring>(TC::ToWide(line.utf8_line.GetString()));

    if( !line_is_for_displaying || line.wide_line->length() <= MaxLineLength )
        return line.wide_line.get();

    if( line.wide_line_for_display == nullptr )
    {
        line.wide_line_for_display = std::make_unique<std::wstring>(line.wide_line->substr(0, MaxLineLength));
        line.wide_line_for_display->append(L"...[line not fully displayed due to its length]");
    }

    return line.wide_line_for_display.get();
}


void LoggingListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if( lpDrawItemStruct->itemAction != ODA_SELECT && lpDrawItemStruct->itemAction != ODA_DRAWENTIRE )
        return;

    CDC dc;
    dc.Attach(lpDrawItemStruct->hDC);

    // use our fixed-width font
    const HGDIOBJ old_font = dc.SelectObject(m_font);

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

    dc.FillRect(&lpDrawItemStruct->rcItem, CBrush::FromHandle(reinterpret_cast<HBRUSH>(background_color_index + 1)));
    dc.SetBkColor(::GetSysColor(background_color_index));
    dc.SetTextColor(::GetSysColor(text_color_index));

    // draw the text
    const std::wstring* wide_text;

    {
        std::lock_guard<std::mutex> lock(m_linesMutex);
        wide_text = GetWideLine(static_cast<size_t>(lpDrawItemStruct->itemID), true);
    }

    // if the text is longer than the longest measured text, modify the horizonal extent
    if( std::get<0>(m_maxLineLengthAndHorizontalExtent) < wide_text->length() )
    {
        std::get<0>(m_maxLineLengthAndHorizontalExtent) = wide_text->length();

        const CSize size = dc.GetTextExtent(wide_text->c_str(), wide_text->length());

        if( size.cx > std::get<1>(m_maxLineLengthAndHorizontalExtent) )
        {
            std::get<1>(m_maxLineLengthAndHorizontalExtent) = size.cx;
            PostMessage(UWM::UtilF::UpdateLoggingListBox, Action::SetHorizontalExtent);
        }
    }

    // draw the text
    dc.DrawText(wide_text->c_str(), wide_text->length(), &lpDrawItemStruct->rcItem,
                DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);

    // restore the original font
    dc.SelectObject(old_font);

    dc.Detach();
}


void LoggingListBox::OnVScroll(const UINT nSBCode, const UINT nPos, CScrollBar* const pScrollBar)
{
    m_userScrolledManually = true;

    __super::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL LoggingListBox::OnMouseWheel(UINT /*nFlags*/, const short zDelta, CPoint /*pt*/)
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

    SetTimer(TimerCode::Scroll, Scroll::ElapseTimeMilliseconds, nullptr);

    return TRUE;
}


void LoggingListBox::OnTimer(const UINT_PTR nIDEvent)
{
    if( nIDEvent == TimerCode::AddText )
    {
        ASSERT(m_addTextMessagePending);
        KillTimer(TimerCode::AddText);
        ProcessAddText(GetCount());
    }

    else if( nIDEvent == TimerCode::Scroll )
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
            const int max_index = GetCount() - 1;

            if( new_index >= max_index )
            {
                new_index = max_index;
                m_pendingMouseWheelActions = 0;
            }

            else
            {
                --m_pendingMouseWheelActions;
            }

            SetTopIndex(new_index);
        }

        if( m_pendingMouseWheelActions == 0 )
            KillTimer(TimerCode::Scroll);
    }

    else
    {
        __super::OnTimer(nIDEvent);
    }
}


void LoggingListBox::OnKeyDown(const UINT nChar, const UINT nRepCnt, const UINT nFlags)
{
    const bool c_pressed = ( nChar == 'C' );

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


void LoggingListBox::OnContextMenu(CWnd* const pWnd, CPoint pos)
{
    // if invoked using the keyboard, determine where to show the menu
    if( pos.x < 0 )
    {
        ASSERT(pos.x == -1 && pos.y == -1);

        CRect rect;
        GetClientRect(rect);

        // if an item is selected and it is visible, display the menu (y-pos) by the selection;
        // otherwise display it in the center of the window
        const int selected_index = GetCurSel();
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

    CMenu* const popup_menu = menu.GetSubMenu(0);

    AddAdditionalContextMenuItems(*popup_menu);

    WindowHelpers::DoUpdateForMenuItems(popup_menu, this);

    popup_menu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, pWnd);
}


void LoggingListBox::AddAdditionalContextMenuItems(CMenu& /*popup_menu*/)
{
}


LRESULT LoggingListBox::OnLoggingListBoxUpdate(const WPARAM wParam, LPARAM /*lParam*/)
{
    // these actions are handled using messages so that a thread using logging
    // can continue without waiting for any list box UI thread to end
    if( wParam == Action::AddText )
    {
        ASSERT(m_addTextMessagePending);

        const int current_lines = GetCount();

        // when there are many lines to add, add the text using a timer so that
        // the UI thread is not blocked by constantly processing messages to add text
        if( ( m_lines.size() - current_lines ) > AddText::MaxLinesForDirectUpdate )
        {
            SetTimer(TimerCode::AddText, AddText::ElapseTimeMilliseconds, nullptr);
        }

        else
        {
            ProcessAddText(current_lines);
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


void LoggingListBox::ProcessAddText(const int current_lines)
{
    ASSERT(m_addTextMessagePending);

    int strings_to_add = m_lines.size() - current_lines;

    m_addTextMessagePending = false;

    auto auto_scroll = [&]()
    {
        // scroll automatically only if the user hasn't manually scroled
        if( !m_userScrolledManually )
            SetTopIndex(GetCount() - 1);
    };

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


void LoggingListBox::Clear()
{
    // clear the lines
    {
        std::lock_guard<std::mutex> lock(m_linesMutex);
        m_lines.clear();
    }

    PostMessage(UWM::UtilF::UpdateLoggingListBox, Action::Clear);
}


void LoggingListBox::AddText(SharableString text)
{
    // add the line
    {
        std::lock_guard<std::mutex> lock(m_linesMutex);

        // make sure text with newlines are displayed on separate lines
        if( SO::ContainsNewlineCharacter(*text) )
        {
            SO::ForeachLine(*text, true,
                [&](const std::string_view line_sv)
                {
                    m_lines.emplace_back(Line { line_sv, nullptr, nullptr });
                });
        }

        else
        {
            m_lines.emplace_back(Line { std::move(text), nullptr, nullptr });
        }
    }

    if( !m_addTextMessagePending )
    {
        m_addTextMessagePending = true;
        PostMessage(UWM::UtilF::UpdateLoggingListBox, Action::AddText);
    }
}


void LoggingListBox::OnCopySelectedLinesToClipboard()
{
    const size_t selected_count = GetSelCount();
    auto selected_indices = std::make_unique_for_overwrite<int[]>(selected_count);
    GetSelItems(selected_count, selected_indices.get());

    std::wstring clipboard_text;

    // combine the lines into a single line
    {
        std::lock_guard<std::mutex> lock(m_linesMutex);

        for( size_t i = 0; i < selected_count; ++i )
        {
            const std::wstring* const wide_line = GetWideLine(selected_indices[i], false);

            clipboard_text.append(*wide_line);
            clipboard_text.push_back('\n');
        }
    }

    WinClipboard::PutText(this, clipboard_text);
}


void LoggingListBox::OnUpdateCopySelectedLinesToClipboard(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(GetSelCount() > 0);
}


void LoggingListBox::OnClearLines()
{
    Clear();
}


void LoggingListBox::OnUpdateClearLines(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(GetCount() > 0);
}


void LoggingListBox::OnSelectAllLines()
{
    SelItemRange(TRUE, 0, GetCount());
}


void LoggingListBox::OnUpdateSelectAllLines(CCmdUI* const pCmdUI)
{
    const int count = GetCount();
    pCmdUI->Enable(count > 0 && count != GetSelCount());
}


void LoggingListBox::OnSaveLines()
{
    SaveFileDlg save_file_dlg(0, FileExtensions::Text, nullptr, FileFilters::Text, this);
    save_file_dlg.SetTitle(L"Save Log Lines")
                 .DisableExtensionCheck();

    if( save_file_dlg.DoModal() != IDOK )
        return;

    // only lock the mutex to get pointers to all the lines
    std::unique_ptr<const std::string*[]> line_pointers;
    const std::string** line_pointers_end;

    {
        std::lock_guard<std::mutex> lock(m_linesMutex);

        line_pointers = std::make_unique_for_overwrite<const std::string*[]>(m_lines.size());
        line_pointers_end = line_pointers.get();

        for( const Line& line : m_lines )
        {
            *line_pointers_end = &line.utf8_line.GetString();
            ++line_pointers_end;
        }
    }

    // write the lines
    try
    {
        FileIO::TextFile text_file;
        text_file.OpenForWritingCreate(save_file_dlg.GetFilePath());

        for( const std::string** line_pointers_itr = line_pointers.get(); line_pointers_itr != line_pointers_end; ++line_pointers_itr )
            text_file.WriteLine(**line_pointers_itr);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void LoggingListBox::OnUpdateSaveLines(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(GetCount() != 0);
}
