#pragma once


// a class to build dynamic Windows menus

class DynamicMenuBuilder
{
public:
    DynamicMenuBuilder(CMenu& popup_menu, unsigned placeholder_menu_item_id);

    void AddOption(unsigned id, const TCHAR* text);
    void AddOption(unsigned id, const std::wstring& text)                      { AddOption(id, text.c_str()); }
    void AddOption(const std::tuple<unsigned, std::wstring>& id_and_menu_text) { AddOption(std::get<0>(id_and_menu_text), std::get<1>(id_and_menu_text)); }

    void AddSubmenu(const TCHAR* text, const std::vector<std::tuple<unsigned, std::wstring>>& options);

    void AddSeparator();

    // returns the menu text from the resource file
    std::wstring GetMenuText(unsigned id);

    std::tuple<unsigned, std::wstring> GetIdAndMenuText(unsigned id) { return { id, GetMenuText(id) }; }

private:
    CMenu& m_popupMenu;
    bool m_lastAdditionWasASeparator;
};



inline DynamicMenuBuilder::DynamicMenuBuilder(CMenu& popup_menu, unsigned placeholder_menu_item_id)
    :   m_popupMenu(popup_menu),
        m_lastAdditionWasASeparator(true)
{
    ASSERT(m_popupMenu.GetMenuItemID(0) == placeholder_menu_item_id);
    m_popupMenu.RemoveMenu(0, MF_BYPOSITION);
}


inline void DynamicMenuBuilder::AddOption(unsigned id, const TCHAR* text)
{
    m_popupMenu.AppendMenu(MF_STRING, id, text);
    m_lastAdditionWasASeparator = false;
}


inline void DynamicMenuBuilder::AddSubmenu(const TCHAR* text, const std::vector<std::tuple<unsigned, std::wstring>>& options)
{
    CMenu submenu;
    submenu.CreatePopupMenu();

    for( const auto& [option_id, option_text] : options )
        submenu.AppendMenu(MF_STRING, option_id, option_text.c_str());

    m_popupMenu.AppendMenu(MF_STRING | MF_POPUP, reinterpret_cast<UINT>(submenu.Detach()), text);
    m_lastAdditionWasASeparator = false;
}


inline void DynamicMenuBuilder::AddSeparator()
{
    if( !m_lastAdditionWasASeparator )
    {
        m_popupMenu.AppendMenu(MF_SEPARATOR);
        m_lastAdditionWasASeparator = true;
    }
}


inline std::wstring DynamicMenuBuilder::GetMenuText(unsigned id)
{
    CString text;
    text.FormatMessage(id);

    const int newline_pos = text.Find('\n', 0);
    ASSERT(newline_pos >= 0);

    return std::wstring(text.GetString() + newline_pos + 1, text.GetLength() - newline_pos - 1);
}
