#pragma once

#include <zToolsO/zToolsO.h>

#if defined(WIN_DESKTOP) || defined(_CONSOLE)

// a simple way to read from Windows registry keys;
// look at SettingsDb and SimpleDbMap for similar functionality

class CLASS_DECL_ZTOOLSO WinRegistry
{
public:
    WinRegistry();
    ~WinRegistry();

    bool Open(HKEY base_key, NullTerminatedString key, bool create_key_if_not_exists = false);
    void Close();

    bool ReadString(NullTerminatedString value_name, CString* pcsValueData);
    bool ReadString(NullTerminatedString value_name, std::wstring& value_data);
    std::optional<std::wstring> ReadOptionalString(NullTerminatedString value_name);
    bool WriteString(NullTerminatedString value_name, wstring_view value_data);

    bool ReadDWord(NullTerminatedString value_name, DWORD* pdwData);
    bool WriteDWord(NullTerminatedString value_name, DWORD valueData);

private:
    HKEY m_hKey;
};

#endif
