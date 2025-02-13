#include "StdAfx.h"
#include "WindowsUserbar.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zUtilO/CustomFont.h>
#include <zUtilO/WindowsWS.h>


namespace Default
{
    constexpr long Height           = 30;
    constexpr long Spacing          = 10;
    constexpr long ButtonHeight     = 23;
    constexpr long ButtonPadding    = 20;
    constexpr long FieldHeight      = 16;
    constexpr long FieldPadding     = 10;

    constexpr const TCHAR* FontName = _T("MS Sans Serif");
    constexpr int FontSize          = 8;
}


WindowsUserbar::WindowsUserbar()
    :   m_wnd(new WindowsUserbarWnd(*this)),
        m_height(Default::Height),
        m_buttonHeight(Default::ButtonHeight),
        m_fieldHeight(Default::FieldHeight),
        m_status(Status::Hidden)
{
    // if the user has defined a particular font, we'll resize the userbar based on this
    UserDefinedFonts* user_defined_fonts = nullptr;
    WindowsDesktopMessage::Send(WM_IMSA_GET_USER_FONTS, &user_defined_fonts);

    if( user_defined_fonts != nullptr && user_defined_fonts->IsFontDefined(UserDefinedFonts::FontType::Userbar) )
    {
        LOGFONT lf;
        user_defined_fonts->GetFont(UserDefinedFonts::FontType::Userbar)->GetLogFont(&lf);

        const long sizing_factor = std::max<long>(0, abs(lf.lfHeight) - Default::FontSize);

        m_height += sizing_factor;
        m_buttonHeight += sizing_factor;
        m_fieldHeight += sizing_factor;
    }

    CRect rect(0, 0, ::GetSystemMetrics(SM_CXSCREEN), m_height);
    m_wnd->Create(_T("STATIC"), nullptr, WS_CHILD, rect, AfxGetApp()->GetMainWnd(), static_cast<UINT>(-1));

    // create the default font
    m_font.CreatePointFont(Default::FontSize * 10, Default::FontName);
}


WindowsUserbar::~WindowsUserbar()
{
    // normally the window will be deleted when handling the WM_NCDESTROY message; however,
    // we will also make WindowsUserbar's destructor hide and remove the userbar
    if( m_wnd != nullptr )
    {
        Hide();
        m_wnd->DestroyWindow();
    }

    ASSERT(m_wnd == nullptr);
}


void WindowsUserbar::Show()
{
    // create the rebar frame if it doesn't already exist
    if( m_rebar == nullptr )
    {
        m_rebar = std::make_unique<CReBar>();
        m_rebar->Create(AfxGetApp()->GetMainWnd());
        m_rebar->AddBar(m_wnd, nullptr, nullptr, RBBS_FIXEDSIZE | RBBS_NOGRIPPER);
    }

    else
    {
        m_rebar->GetReBarCtrl().ShowBand(0, TRUE);
    }

    m_status = Status::Shown;
}


void WindowsUserbar::Hide()
{
    if( m_rebar != nullptr )
        m_rebar->GetReBarCtrl().ShowBand(0, FALSE);

    m_status = Status::Hidden;
}


void WindowsUserbar::Clear()
{
    Hide();
    m_backgroundColor.reset();
    m_items.clear();
}


void WindowsUserbar::Pause(bool pause)
{
    if( pause && m_status == Status::Shown )
    {
        Hide();
        m_status = Status::Paused;
    }

    else if( !pause && m_status == Status::Paused )
    {
        Show();
    }
}


CFont* WindowsUserbar::GetFont()
{
    // allow for the dynamic setting of fonts for the userbar
    UserDefinedFonts* user_defined_fonts = nullptr;
    WindowsDesktopMessage::Send(WM_IMSA_GET_USER_FONTS, &user_defined_fonts);

    if( user_defined_fonts != nullptr && user_defined_fonts->IsFontDefined(UserDefinedFonts::FontType::Userbar) )
        return user_defined_fonts->GetFont(UserDefinedFonts::FontType::Userbar);

    return &m_font;
}


template<typename T>
std::optional<size_t> WindowsUserbar::GetItemIndex(T id_or_hwnd) const
{
    for( size_t i = 0; i < m_items.size(); ++i )
    {
        const WindowsUserbarItem& item = m_items[i];
        bool matches;

        if constexpr(std::is_same_v<T, int>)
        {
            matches = ( item.id == id_or_hwnd );
        }

        else
        {
            matches = ( item.window != nullptr && item.window.get() == m_wnd->FromHandlePermanent(id_or_hwnd) );
        }

        if( matches )
            return i;
    }

    return std::nullopt;
}


template<typename T>
WindowsUserbarItem* WindowsUserbar::GetItem(T id_or_hwnd)
{
    const std::optional<size_t> item_index = GetItemIndex(id_or_hwnd);

    return item_index.has_value() ? &m_items[*item_index] :
                                    nullptr;
}


void WindowsUserbar::MoveItems(size_t starting_index, int shift_amount)
{
    for( size_t i = starting_index; i < m_items.size(); ++i )
    {
        WindowsUserbarItem& item = m_items[i];

        item.rect.left += shift_amount;
        item.rect.right += shift_amount;

        if( item.window != nullptr )
            item.window->SetWindowPos(nullptr, item.rect.left, item.rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    m_wnd->RedrawWindow();
}


template<typename CF>
int WindowsUserbar::AddItem(WindowsUserbarItem::Type item_type, std::optional<Action> action, CF callback_function)
{
    const int id = m_items.empty() ? StartingId : ( m_items.back().id + 1 );
    const long x = Default::Spacing + ( m_items.empty() ? 0 : m_items.back().rect.right );

    WindowsUserbarItem& item = m_items.emplace_back(
        WindowsUserbarItem
        {
            item_type,
            id,
            std::move(action)
        });

    item.window.reset(callback_function(item, x));

    return item.id;
}


int WindowsUserbar::AddButton(std::wstring text, std::optional<Action> action)
{
    NewlineSubstitutor::MakeNewlineToSpace(text);

    return AddItem(WindowsUserbarItem::Type::Button, std::move(action),
        [&](WindowsUserbarItem& item, long x)
        {
            CFont* font = GetFont();

            CClientDC dc(m_wnd);
            dc.SelectObject(font);
            CSize text_extent = dc.GetTextExtent(text.c_str());

            const long y = ( m_height - m_buttonHeight ) / 2;
            item.rect = CRect(x, y, x + text_extent.cx + Default::ButtonPadding, y + m_buttonHeight);

            CButton* button_window = new CButton;
            button_window->Create(text.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, item.rect, m_wnd, (UINT)-1);
            button_window->SetFont(font);

            return button_window;
        });
}


namespace
{
    // a subclass that traps enter keystrokes and sends the userbar a message informing it of such keystrokes
    class FieldEdit : public CEdit
    {
    protected:
        DECLARE_MESSAGE_MAP()

        void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    };


    BEGIN_MESSAGE_MAP(FieldEdit, CEdit)
        ON_WM_KEYUP()
    END_MESSAGE_MAP()


    void FieldEdit::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        if( nChar == VK_RETURN )
        {
            GetParent()->SendMessage(UWM::Engine::UserbarEditReturnKey, reinterpret_cast<WPARAM>(m_hWnd));
        }

        else
        {
            CEdit::OnKeyUp(nChar, nRepCnt, nFlags);
        }
    }
}


int WindowsUserbar::AddField(std::wstring text, std::optional<Action> action)
{
    NewlineSubstitutor::MakeNewlineToSpace(text);

    return AddItem(WindowsUserbarItem::Type::Field, std::move(action),
        [&](WindowsUserbarItem& item, long x)
        {
            CFont* font = GetFont();

            CClientDC dc(m_wnd);
            dc.SelectObject(font);
            CSize text_extent = dc.GetTextExtent(text.c_str());

            const long y = ( m_height - m_fieldHeight ) / 2;
            item.rect = CRect(x, y, x + text_extent.cx + Default::FieldPadding, y + m_fieldHeight);

            CEdit* edit_window = new FieldEdit;
            edit_window->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, item.rect, m_wnd, (UINT)-1);
            edit_window->SetFont(font);

            // an empty string can be used to set the length, but we won't actually fill the box with spaces
            if( !SO::IsBlank(text) )
                edit_window->SetWindowText(text.c_str());

            return edit_window;
        });
}


int WindowsUserbar::AddText(std::wstring text)
{
    NewlineSubstitutor::MakeNewlineToSpace(text);

    return AddItem(WindowsUserbarItem::Type::Text, std::nullopt,
        [&](WindowsUserbarItem& item, long x)
        {
            CFont* font = GetFont();

            CClientDC dc(m_wnd);
            dc.SelectObject(font);
            CSize text_extent = dc.GetTextExtent(text.c_str());

            const long y = ( m_height - text_extent.cy ) / 2;
            item.rect = CRect(x, y, x + text_extent.cx, y + text_extent.cy);

            CStatic* static_window = new CStatic();
            static_window->Create(text.c_str(), WS_CHILD | WS_VISIBLE, item.rect, m_wnd);
            static_window->SetFont(font);

            return static_window;
        });
}


int WindowsUserbar::AddSpacing(int spacing)
{
    return AddItem(WindowsUserbarItem::Type::Spacing, std::nullopt,
        [&](WindowsUserbarItem& item, long x)
        {
            item.rect = CRect(x, 0, x + std::max(0, spacing), 0); // the y coordinates don't matter

            return nullptr;
        });
}


std::optional<std::wstring> WindowsUserbar::GetFieldText(int id)
{
    const WindowsUserbarItem* item = GetItem(id);

    if( item == nullptr || item->type != WindowsUserbarItem::Type::Field )
        return std::nullopt;

    return WindowsWS::GetWindowText(*item->window);
}



bool WindowsUserbar::SetColor(COLORREF color, std::optional<int> id)
{
    // setting the value for the entire userbar
    if( !id.has_value() )
    {
        m_backgroundColor = std::make_unique<CBrush>(color);
        m_wnd->RedrawWindow();
        return true;
    }

    // otherwise the user is changing trying to change the color of a text string
    else
    {
        WindowsUserbarItem* item = GetItem(*id);

        if( item != nullptr && item->type == WindowsUserbarItem::Type::Text )
        {
            item->color = color;
            item->window->RedrawWindow();
            return true;
        }
    }

    return false;
}


bool WindowsUserbar::Modify(int id, std::optional<std::wstring> text, std::optional<Action> action, std::optional<int> spacing)
{
    const std::optional<size_t> item_index = GetItemIndex(id);

    if( !item_index.has_value() )
        return false;

    WindowsUserbarItem& item = m_items[*item_index];
    bool success = true;
    std::optional<int> spacing_shift_amount;

    // modifying the text
    if( text.has_value() )
    {
        if( item.window == nullptr )
        {
            success = false;
        }

        else
        {
            NewlineSubstitutor::MakeNewlineToSpace(*text);

            // buttons and text have to be resized based on the new text size
            if( item.type == WindowsUserbarItem::Type::Button || item.type == WindowsUserbarItem::Type::Text )
            {
                CClientDC dc(m_wnd);
                dc.SelectObject(item.window->GetFont());
                CSize text_extent = dc.GetTextExtent(text->c_str());

                const int new_width = text_extent.cx + ( ( item.type == WindowsUserbarItem::Type::Button ) ? Default::ButtonPadding : 0 );
                spacing_shift_amount = new_width - item.rect.Width();

                item.window->SetWindowPos(nullptr, 0, 0, new_width, item.rect.Height(), SWP_NOMOVE | SWP_NOZORDER);
            }

            item.window->SetWindowText(text->c_str());
        }
    }

    // modifying the action
    if( action.has_value() )
    {
        if( item.type == WindowsUserbarItem::Type::Button || item.type == WindowsUserbarItem::Type::Field )
        {
            item.action = std::move(action);
        }

        else
        {
            success = false;
        }
    }

    // modifying the spacing
    if( spacing.has_value() )
    {
        if( item.type == WindowsUserbarItem::Type::Spacing )
        {
            spacing_shift_amount = std::max(0, *spacing) - item.rect.Width();
        }

        else
        {
            success = false;
        }
    }

    if( spacing_shift_amount.has_value() )
    {
        item.rect.right += *spacing_shift_amount;
        MoveItems(*item_index + 1, *spacing_shift_amount);
    }

    return success;
}


bool WindowsUserbar::Remove(int id)
{
    const std::optional<size_t> item_index = GetItemIndex(id);

    if( !item_index.has_value() )
        return false;

    const int item_width = m_items[*item_index].rect.Width();

    m_items.erase(m_items.begin() + *item_index);

    MoveItems(*item_index, -1 * ( item_width + Default::Spacing ));

    return true;
}


LRESULT WindowsUserbarWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    // set the userbar's background color
    if( message == WM_PAINT )
    {
        if( m_userbar.m_backgroundColor != nullptr )
        {
            PAINTSTRUCT ps;
            RECT rect;
            CDC* pDC = BeginPaint(&ps);
            GetWindowRect(&rect);
            ScreenToClient(&rect);
            pDC->FillRect(&rect, m_userbar.m_backgroundColor.get());
            EndPaint(&ps);
            return 0;
        }
    }


    // set the foreground and background colors of text strings
    else if( message == WM_CTLCOLORSTATIC )
    {
        WindowsUserbarItem* item = m_userbar.GetItem(reinterpret_cast<HWND>(lParam));

        if( item != nullptr && ( item->color.has_value() || m_userbar.m_backgroundColor != nullptr ) )
        {
            HDC hDC = reinterpret_cast<HDC>(wParam);
            SetBkMode(hDC, TRANSPARENT);

            if( item->color.has_value() )
                SetTextColor(hDC, *item->color);

            if( m_userbar.m_backgroundColor == nullptr )
            {
                m_userbar.m_backgroundColor = std::make_unique<CBrush>(GetSysColor(COLOR_BTNFACE)); // for some reason COLOR_BTNFACE seems to work
                Invalidate(); // we will have to repaint the background
            }

            return reinterpret_cast<LRESULT>(m_userbar.m_backgroundColor->GetSafeHandle());
        }
    }


    // process a user having pressed enter in a text field
    else if( message == UWM::Engine::UserbarEditReturnKey )
    {
        WindowsUserbarItem* item = m_userbar.GetItem(reinterpret_cast<HWND>(wParam));

        if( item != nullptr )
        {
            m_userbar.m_lastActivatedItemId = item->id;

            if( item->action.has_value() )
                WindowsDesktopMessage::Post(WM_IMSA_USERBAR_UPDATE, &(*item->action));
        }
    }


    // handle a button click
    else if( message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED )
    {
        WindowsUserbarItem* item = m_userbar.GetItem(reinterpret_cast<HWND>(lParam));

        if( item != nullptr )
        {
            m_userbar.m_lastActivatedItemId = item->id;

            // reset the style to address a problem with black borders persisting after a button click
            static_cast<CButton&>(*item->window).SetButtonStyle(BS_PUSHBUTTON);

            if( item->action.has_value() )
                WindowsDesktopMessage::Post(WM_IMSA_USERBAR_UPDATE, &(*item->action));
        }
    }


    // mark the window as having been deleted
    else if( message == WM_NCDESTROY )
    {
        m_userbar.m_wnd = nullptr;

        DefWindowProc(message, wParam, lParam);
        delete this;

        return 0;
    }


    // default handler
    return DefWindowProc(message, wParam, lParam);
}
