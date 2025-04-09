#include "StdAfx.h"
#include "KeyboardLoader.h"
#include <zToolsO/RaiiHelpers.h>


#ifdef WIN_DESKTOP

namespace
{
    constexpr const wchar_t* DefaultKeyboardName      = L"Default Keyboard";
    constexpr const wchar_t* KeyboardNotInstalledName = L"Keyboard Not Installed";
}


unsigned int KeyboardLoader::GetKeyboardId(const unsigned int keyboard_id)
{
    if( keyboard_id != 0 )
    {
        // the keyboard ID is only valid if the keyboard can be looked up successfully
        if( GetHKLFromKlid(keyboard_id) != nullptr )
            return keyboard_id;
    }

    return 0;
}


void KeyboardLoader::Activate(const unsigned int keyboard_id)
{
    if( m_hDefaultKL == nullptr )
    {
        // quit if using the default keyboard layout
        if( keyboard_id == 0 )
            return;

        m_hDefaultKL = GetKeyboardLayout(0);
        m_hCurrentKL = m_hDefaultKL;
    }

    HKL hKL = ( keyboard_id == 0 ) ? m_hDefaultKL :
                                     GetHKLFromKlid(keyboard_id);

    if( hKL == nullptr )
        hKL = m_hDefaultKL;

    if( hKL != m_hCurrentKL )
    {
        m_hCurrentKL = hKL;
        ActivateKeyboardLayout(m_hCurrentKL, 0);
    }
}


std::wstring KeyboardLoader::GetDisplayName(const unsigned int keyboard_id, const bool strip_country_name/* = true*/)
{
    if( keyboard_id == 0 )
        return DefaultKeyboardName;

    const int num_keyboard_layouts = GetKeyboardLayoutList(0, nullptr);
    auto keyboard_layouts = std::make_unique_for_overwrite<HKL[]>(num_keyboard_layouts);
    GetKeyboardLayoutList(num_keyboard_layouts, keyboard_layouts.get());

    const RAII::RunOnDestruction run_on_destruction(
        [hCurrentKL = GetKeyboardLayout(0)]()
        {
            ActivateKeyboardLayout(hCurrentKL, 0);
        });

    HKL hFoundKL = nullptr;
    wchar_t layout_name[KL_NAMELENGTH];

    for( int i = 0; i < num_keyboard_layouts; ++i )
    {
        ActivateKeyboardLayout(keyboard_layouts[i], 0);
        GetKeyboardLayoutName(layout_name);

        if( LayoutNameToKlid(layout_name) == keyboard_id )
        {
            hFoundKL = keyboard_layouts[i];
            break;
        }
    }

    // load the keyboard only if we can't find it in the current list of keyboards
    if( hFoundKL == nullptr )
    {
        const std::wstring hex_layout_name = FormatText(L"%08x", keyboard_id);

        const HKL hKL = LoadKeyboardLayout(hex_layout_name.c_str(), KLF_ACTIVATE);
        GetKeyboardLayoutName(layout_name);
        UnloadKeyboardLayout(hKL);

        if( hex_layout_name != layout_name )
            return KeyboardNotInstalledName;

        hFoundKL = hKL;
    }

    ASSERT(hFoundKL != nullptr);

    return GetDisplayName(hFoundKL, strip_country_name);
}


std::wstring KeyboardLoader::GetDisplayName(const HKL hKL, const bool strip_country_name/* = true*/)
{
    if( hKL == nullptr )
        return DefaultKeyboardName;

    constexpr int DisplayNameBufferSize = 512;
    auto display_name = std::make_unique_for_overwrite<wchar_t[]>(DisplayNameBufferSize);

    const int display_name_length_with_null = GetLocaleInfo(MAKELCID(reinterpret_cast<DWORD>(hKL), SORT_DEFAULT),
                                                            LOCALE_SLANGUAGE, display_name.get(), DisplayNameBufferSize);

    if( display_name_length_with_null == 0 )
        return KeyboardNotInstalledName;

    const std::wstring_view display_name_sv(display_name.get(), display_name_length_with_null - 1);

    // strip the country name; e.g., English (United States) -> English
    if( strip_country_name )
    {
        const size_t parenthesis_pos = display_name_sv.find('(');

        if( parenthesis_pos != std::wstring_view::npos )
            return std::wstring(display_name_sv.substr(0, parenthesis_pos - 1));
    }

    return std::wstring(display_name_sv);
}


std::vector<std::tuple<HKL, std::wstring>> KeyboardLoader::GetKeyboardLayouts(const bool include_default_option, const bool strip_country_name/* = true*/)
{
    std::vector<std::tuple<HKL, std::wstring>> keyboard_layouts;

    if( include_default_option )
        keyboard_layouts.emplace_back(nullptr, DefaultKeyboardName);

    const int num_keyboard_layouts = GetKeyboardLayoutList(0, nullptr);
    auto keyboard_layout_hkls = std::make_unique_for_overwrite<HKL[]>(num_keyboard_layouts);
    GetKeyboardLayoutList(num_keyboard_layouts, keyboard_layout_hkls.get());

    for( int i = 0; i < num_keyboard_layouts; ++i )
    {
        const HKL hKL = keyboard_layout_hkls[i];
        keyboard_layouts.emplace_back(hKL, GetDisplayName(hKL, strip_country_name));
    }

    return keyboard_layouts;
}


unsigned int KeyboardLoader::GetKlidFromHKL(const HKL hKL)
{
    if( hKL == nullptr )
        return 0;

    const RAII::RunOnDestruction run_on_destruction(
        [hCurrentKL = GetKeyboardLayout(0)]()
        {
            ActivateKeyboardLayout(hCurrentKL, 0);
        });

    ActivateKeyboardLayout(hKL, 0);

    wchar_t layout_name[KL_NAMELENGTH];
    GetKeyboardLayoutName(layout_name);

    return LayoutNameToKlid(layout_name);
}


HKL KeyboardLoader::GetHKLFromKlid(const unsigned int keyboard_id)
{
    ASSERT(keyboard_id != 0);

    // set up the mapping for non-default keyboard layouts
    if( m_keyboardIdHklMapping.empty() )
    {
        for( const auto& [hKL, display_name] : KeyboardLoader::GetKeyboardLayouts(false, false) )
            m_keyboardIdHklMapping.try_emplace(GetKlidFromHKL(hKL), hKL);

    }

    const auto& lookup = m_keyboardIdHklMapping.find(keyboard_id);

    return ( lookup != m_keyboardIdHklMapping.cend() ) ? lookup->second :
                                                         nullptr;
}


unsigned int KeyboardLoader::LayoutNameToKlid(const wchar_t* const layout_name)
{
    constexpr size_t LayoutNameLength = KL_NAMELENGTH - 1;
    ASSERT(wcslen(layout_name)  == LayoutNameLength);

    const std::wstring_view layout_name_sv(layout_name, LayoutNameLength);
    unsigned int keyboard_id = 0;

    for( const wchar_t ch : layout_name_sv )
    {
        keyboard_id *= 16;

        keyboard_id += is_digit(ch) ? ( ch - '0' ) :
                                      ( std::tolower(ch) - 'a' + 10 );
    }

    return keyboard_id;
}

#else

// --------------------------------------------------------------------------
// dummy portable implementations
// --------------------------------------------------------------------------

unsigned int KeyboardLoader::GetKeyboardId(const unsigned int keyboard_id) { return keyboard_id; }
void KeyboardLoader::Activate(unsigned int /*keyboard_id*/) { }

#endif
