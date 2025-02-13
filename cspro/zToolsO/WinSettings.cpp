#include "StdAfx.h"
#include "WinSettings.h"


namespace
{
    const HKEY BaseKey = HKEY_CURRENT_USER;
    constexpr const TCHAR* SettingsKey = _T("Software\\U.S. Census Bureau\\CSPro Settings");

    constexpr const TCHAR* TypeNames[] =
    {
        _T("View Names in Tree"),
        _T("Append Labels to Names"),

        _T("Last Application Directory"),

        _T("Listing Filename Extension"),
        _T("Frequency Filename Extension"),

        _T("Add Image to Resource Folder"),

        _T("Trace Window: Always on Top"),

        _T("Logic Settings"),
        _T("Logic Editor Zoom Level"),

        _T("Code Folding"),
        _T("Deprecation Warnings"),
        _T("Word Wrap"),

        _T("String Encoder: Split Newlines"),
        _T("String Encoder: Use Verbatim String Literals"),
        _T("String Encoder: Escape JSON Forward Slashes"),

        _T("Localhost: Start Automatically"),
        _T("Localhost: Preferred Port"),
        _T("Localhost: Automatically Mapped Drives"),
    };

    static_assert(_countof(TypeNames) == ( static_cast<size_t>(WinSettings::Type::LocalhostAutomaticallyMappedDrives) + 1 ));
}


WinSettings::WinSettings()
{
    m_winRegistry.Open(BaseKey, SettingsKey, true);
}


WinSettings& WinSettings::GetInstance()
{
    static WinSettings win_settings;
    return win_settings;
}


WinSettings::~WinSettings()
{
    // save any settings that were modified
    for( const KeyType& key : m_savedSettings )
    {
        const TCHAR* key_text = GetKeyText(key);
        const std::variant<std::wstring, DWORD>& value = m_loadedSettings[key];

        if( std::holds_alternative<std::wstring>(value) )
        {
            m_winRegistry.WriteString(key_text, std::get<std::wstring>(value));
        }

        else if( std::holds_alternative<DWORD>(value) )
        {
            m_winRegistry.WriteDWord(key_text, std::get<DWORD>(value));
        }
    }

    m_winRegistry.Close();
}


const TCHAR* WinSettings::GetKeyText(const KeyType& key)
{
    return std::holds_alternative<Type>(key) ? TypeNames[static_cast<size_t>(std::get<Type>(key))] :
                                               std::get<std::wstring>(key).c_str();
}


template<typename T>
T WinSettings::ReadWorker(KeyType key, T* default_value)
{
    WinSettings& instance = GetInstance();

    // check if the setting has already been loaded
    const auto& loaded_settings_lookup = instance.m_loadedSettings.find(key);

    if( loaded_settings_lookup != instance.m_loadedSettings.cend() )
    {
        if constexpr(std::is_same_v<T, CString>)
        {
            return WS2CS(std::get<std::wstring>(loaded_settings_lookup->second));
        }

        else
        {
            return std::get<T>(loaded_settings_lookup->second);
        }
    }

    // if not, look it up in the registry
    T value;
    bool value_found;
    const TCHAR* key_text = GetKeyText(key);

    if constexpr(std::is_same_v<T, std::wstring>)
    {
        CString cstring_value;
        value_found = instance.m_winRegistry.ReadString(key_text, &cstring_value);
        value = CS2WS(cstring_value);
    }

    else if constexpr(std::is_same_v<T, CString>)
    {
        value_found = instance.m_winRegistry.ReadString(key_text, &value);
    }

    else
    {
        value_found = instance.m_winRegistry.ReadDWord(key_text, &value);
    }

    if( value_found )
    {
        if constexpr(std::is_same_v<T, CString>)
        {
            instance.m_loadedSettings.try_emplace(std::move(key), CS2WS(value));
        }

        else
        {
            instance.m_loadedSettings.try_emplace(std::move(key), value);
        }
    }

    else if( default_value != nullptr )
    {
        value = std::move(*default_value);
    }

    return value;
}


template<typename T>
void WinSettings::Write(KeyType key, const T& value)
{
    WinSettings& instance = GetInstance();

    // only save out the setting if it changed
    const auto& loaded_settings_lookup = instance.m_loadedSettings.find(key);

    if constexpr(std::is_same_v<T, CString>)
    {
        if( loaded_settings_lookup != instance.m_loadedSettings.cend() && SO::Equals(std::get<std::wstring>(loaded_settings_lookup->second), value) )
            return;
    }

    else
    {
        if( loaded_settings_lookup != instance.m_loadedSettings.cend() && std::get<T>(loaded_settings_lookup->second) == value )
            return;
    }

    instance.m_savedSettings.insert(key);

    if constexpr(std::is_same_v<T, CString>)
    {
        instance.m_loadedSettings[key] = CS2WS(value);
    }

    else
    {
        instance.m_loadedSettings[key] = value;
    }
}


template CLASS_DECL_ZTOOLSO std::wstring WinSettings::ReadWorker<std::wstring>(KeyType key, std::wstring* default_value);
template CLASS_DECL_ZTOOLSO CString WinSettings::ReadWorker<CString>(KeyType key, CString* default_value);
template CLASS_DECL_ZTOOLSO DWORD WinSettings::ReadWorker<DWORD>(KeyType key, DWORD* default_value);
template CLASS_DECL_ZTOOLSO void WinSettings::Write<std::wstring>(KeyType key, const std::wstring& value);
template CLASS_DECL_ZTOOLSO void WinSettings::Write<CString>(KeyType key, const CString& value);
template CLASS_DECL_ZTOOLSO void WinSettings::Write<DWORD>(KeyType key, const DWORD& value);
