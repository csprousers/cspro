#pragma once

#include <zToolsO/zToolsO.h>

#ifdef WIN32

// a simple way to read from Windows registry keys;
// look at SettingsDb and SimpleDbMap for similar functionality

class CLASS_DECL_ZTOOLSO WinRegistry
{
public:
    WinRegistry();
    ~WinRegistry();

    bool Open(HKEY base_key, const wchar_t* key, bool create_key_if_not_exists = false);
    bool Open(HKEY base_key, std::string_view key_sv, bool create_key_if_not_exists = false);
    void Close();

    bool ReadString(const wchar_t* value_name, CString* pcsValueData);
    bool ReadString(const wchar_t* value_name, std::wstring& value_data);
    bool ReadString(std::string_view value_name_sv, std::string& value_data);
    std::optional<std::string> ReadOptionalString(std::string_view value_name_sv);
    bool WriteString(const wchar_t* value_name, wstring_view value_data_sv);
    bool WriteString(std::string_view value_name_sv, std::string_view value_data_sv);

    bool ReadDWord(const wchar_t* value_name, DWORD* pdwData);
    bool ReadDWord(std::string_view value_name_sv, DWORD* pdwData);
    bool WriteDWord(const wchar_t* value_name, DWORD valueData);

private:
    HKEY m_hKey;
};

#endif
