#include "StdAfx.h"
#include "DynamicMenuBuilder.h"


DynamicMenuBuilder::DynamicMenuBuilder(CMenu& popup_menu, const unsigned placeholder_menu_item_id)
    :   m_popupMenu(popup_menu),
        m_lastAdditionWasASeparator(true)
{
    ASSERT(m_popupMenu.GetMenuItemID(0) == placeholder_menu_item_id);
    m_popupMenu.RemoveMenu(0, MF_BYPOSITION);
}


DynamicMenuBuilder& DynamicMenuBuilder::AddOption(const unsigned id, const wchar_t* const text)
{
    m_popupMenu.AppendMenu(MF_STRING, id, text);
    m_lastAdditionWasASeparator = false;

    return *this;
}


template<typename T>
DynamicMenuBuilder& DynamicMenuBuilder::AddSubmenu(const wchar_t* const text, const std::vector<std::tuple<unsigned, T>>& options)
{
    CMenu submenu;
    submenu.CreatePopupMenu();

    for( const auto& [option_id, option_text] : options )
        submenu.AppendMenu(MF_STRING, option_id, UTF8_TODO::EnsureWide(option_text).c_str());

    m_popupMenu.AppendMenu(MF_STRING | MF_POPUP, reinterpret_cast<UINT>(submenu.Detach()), text);
    m_lastAdditionWasASeparator = false;

    return *this;
}

template CLASS_DECL_ZUTILF DynamicMenuBuilder& DynamicMenuBuilder::AddSubmenu(const wchar_t* text, const std::vector<std::tuple<unsigned, std::wstring>>& options);
template CLASS_DECL_ZUTILF DynamicMenuBuilder& DynamicMenuBuilder::AddSubmenu(const wchar_t* text, const std::vector<std::tuple<unsigned, std::string>>& options);


DynamicMenuBuilder& DynamicMenuBuilder::AddSeparator()
{
    if( !m_lastAdditionWasASeparator )
    {
        m_popupMenu.AppendMenu(MF_SEPARATOR);
        m_lastAdditionWasASeparator = true;
    }

    return *this;
}


DynamicMenuBuilder& DynamicMenuBuilder::AddOptions(CMenu* const menu)
{
    AddOptions(menu, m_popupMenu);
    return *this;
}


void DynamicMenuBuilder::AddOptions(const CMenu* const source_menu, CMenu& destination_menu)
{
    if( source_menu == nullptr )
        return;

    const int item_count = source_menu->GetMenuItemCount();
    CString menu_text;

    for( int i = 0; i < item_count; ++i )
    {
        const unsigned id = source_menu->GetMenuItemID(i);

        // separator
        if( id == 0 )
        {
            destination_menu.AppendMenu(MF_SEPARATOR);
            continue;
        }

        source_menu->GetMenuString(i, menu_text, MF_BYPOSITION);

        // submenu
        if( id == static_cast<unsigned>(-1) )
        {
            CMenu submenu;
            submenu.CreatePopupMenu();

            AddOptions(source_menu->GetSubMenu(i), submenu);

            destination_menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(submenu.Detach()), menu_text);
        }

        // regular item
        else
        {
#ifdef _DEBUG
            const int state = source_menu ->GetMenuState(i, MF_BYPOSITION);
            ASSERT(( state & MF_CHECKED ) == 0);
            ASSERT(( state & MF_DISABLED ) == 0);
            ASSERT(( state & MF_GRAYED ) == 0);
            ASSERT(( state & MF_POPUP ) == 0);
            ASSERT(( state & MF_SEPARATOR ) == 0);
#endif
            destination_menu.AppendMenu(MF_STRING, id, menu_text);
        }
    }
}


std::wstring DynamicMenuBuilder::GetMenuText(const unsigned id)
{
    CString text;
    text.FormatMessage(id);

    const int newline_pos = text.Find('\n', 0);
    ASSERT(newline_pos >= 0);

    return std::wstring(text.GetString() + newline_pos + 1,
                        text.GetLength() - newline_pos - 1);
}
