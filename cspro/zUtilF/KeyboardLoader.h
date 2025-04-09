#pragma once

#include <zUtilF/zUtilF.h>


class CLASS_DECL_ZUTILF KeyboardLoader
{
public:
    // Returns the keyboard ID if valid, returning 0 otherwise.
    unsigned int GetKeyboardId(unsigned int keyboard_id);

    // Activates the specified keyboard.
    void Activate(unsigned int keyboard_id);

    // Returns a name describing the keyboard layout.
    static std::wstring GetDisplayName(unsigned int keyboard_id, bool strip_country_name = true);
    static std::wstring GetDisplayName(HKL hKL, bool strip_country_name = true);

    // Returns the system's keyboard layouts, returning the HKL and display name.
    static std::vector<std::tuple<HKL, std::wstring>> GetKeyboardLayouts(bool include_default_option, bool strip_country_name = true);

    // Returns the keyboard ID for the given HKL.
    static unsigned int GetKlidFromHKL(HKL hKL);

private:
    // Returns the HKL for the given keyboard ID, or null if no match exists on the system.
    HKL GetHKLFromKlid(unsigned int keyboard_id);

    // Converts the layout name, represented as a hex value, to a number.
    static unsigned int LayoutNameToKlid(const wchar_t* layout_name);

private:
    std::map<unsigned int, HKL> m_keyboardIdHklMapping;
    HKL m_hDefaultKL = nullptr;
    HKL m_hCurrentKL = nullptr;
};
