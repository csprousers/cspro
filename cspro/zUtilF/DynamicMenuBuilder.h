#pragma once

#include <zUtilF/zUtilF.h>


// a class to build dynamic Windows menus

class CLASS_DECL_ZUTILF DynamicMenuBuilder
{
public:
    DynamicMenuBuilder(CMenu& popup_menu, unsigned placeholder_menu_item_id);

    DynamicMenuBuilder& AddOption(unsigned id, const wchar_t* text);
    DynamicMenuBuilder& AddOption(unsigned id, const std::wstring& text)                      { return AddOption(id, text.c_str()); }
    DynamicMenuBuilder& AddOption(const std::tuple<unsigned, std::wstring>& id_and_menu_text) { return AddOption(std::get<0>(id_and_menu_text), std::get<1>(id_and_menu_text)); }

    template<typename T>
    DynamicMenuBuilder& AddSubmenu(const wchar_t* text, const std::vector<std::tuple<unsigned, T>>& options);

    DynamicMenuBuilder& AddSeparator();

    DynamicMenuBuilder& AddOptions(CMenu* menu);

    // returns the menu text from the resource file
    std::wstring GetMenuText(unsigned id);

    std::tuple<unsigned, std::wstring> GetIdAndMenuText(unsigned id) { return { id, GetMenuText(id) }; }

private:
    static void AddOptions(const CMenu* source_menu, CMenu& destination_menu);

private:
    CMenu& m_popupMenu;
    bool m_lastAdditionWasASeparator;
};
