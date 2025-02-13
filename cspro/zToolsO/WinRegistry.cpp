#include "StdAfx.h"
#include "WinRegistry.h"


WinRegistry::WinRegistry()
    :   m_hKey(nullptr)
{
}


WinRegistry::~WinRegistry()
{
    Close();
}


bool WinRegistry::Open(HKEY base_key, NullTerminatedString key, bool create_key_if_not_exists/* = false*/)
{
    Close();

    if( create_key_if_not_exists )
    {
        RegCreateKey(base_key, key.c_str(), &m_hKey);
    }

    else
    {
        RegOpenKey(base_key, key.c_str(), &m_hKey);
    }

    return ( m_hKey != nullptr );
}


void WinRegistry::Close()
{
    if( m_hKey != nullptr )
    {
        RegCloseKey(m_hKey);
        m_hKey = nullptr;
    }
}


bool WinRegistry::ReadString(NullTerminatedString value_name, CString* pcsValueData)
{
    if( m_hKey != nullptr )
    {
        const int BufferSize = 500;
        TCHAR szBuff[BufferSize];
        DWORD dwType = 0;
        DWORD dwKeyLen = sizeof(szBuff);

        LSTATUS lStatus = RegQueryValueEx(m_hKey, value_name.c_str(), nullptr, &dwType, reinterpret_cast<LPBYTE>(szBuff), &dwKeyLen);
        ASSERT(lStatus != ERROR_MORE_DATA);

        if( lStatus == ERROR_SUCCESS )
        {
            if( dwKeyLen == 0 )
            {
                pcsValueData->Empty();
                return true;
            }

            else if( dwType == REG_SZ )
            {
                *pcsValueData = szBuff;
                return true;
            }

            else if( dwType == REG_EXPAND_SZ )
            {
                TCHAR szExpandedBuff[BufferSize];
                int characters_used = ExpandEnvironmentStrings(szBuff, szExpandedBuff, BufferSize);

                if( characters_used > 0 && characters_used <= BufferSize )
                {
                    *pcsValueData = szExpandedBuff;
                    return true;
                }
            }
        }
    }

    return false;
}


bool WinRegistry::ReadString(NullTerminatedString value_name, std::wstring& value_data)
{
    CString cstring_value_data;

    if( ReadString(value_name.c_str(), &cstring_value_data) )
    {
        value_data = CS2WS(cstring_value_data);
        return true;
    }

    return false;
}


std::optional<std::wstring> WinRegistry::ReadOptionalString(NullTerminatedString value_name)
{
    std::wstring value;

    return ReadString(value_name, value) ? std::make_optional(std::move(value)) :
                                           std::nullopt;
}


bool WinRegistry::WriteString(NullTerminatedString value_name, wstring_view value_data)
{
    return ( m_hKey != nullptr &&
             RegSetKeyValue(m_hKey, nullptr, value_name.c_str(), REG_SZ, value_data.data(), value_data.length() * sizeof(TCHAR)) == ERROR_SUCCESS );
}


bool WinRegistry::ReadDWord(NullTerminatedString value_name, DWORD* pdwData)
{
    if( m_hKey != nullptr )
    {
        DWORD dwType = 0;
        DWORD dwKeyLen = sizeof(*pdwData);

        LSTATUS lStatus = RegQueryValueEx(m_hKey, value_name.c_str(), nullptr, &dwType, reinterpret_cast<LPBYTE>(pdwData), &dwKeyLen);
        return ( lStatus == ERROR_SUCCESS && dwType == REG_DWORD );
    }

    return false;
}


bool WinRegistry::WriteDWord(NullTerminatedString value_name, DWORD valueData)
{
    if( m_hKey != nullptr )
    {
        LSTATUS lStatus = RegSetKeyValue(m_hKey, nullptr, value_name.c_str(), REG_DWORD, reinterpret_cast<const BYTE*>(&valueData), sizeof(valueData));
        return ( lStatus  == ERROR_SUCCESS );
    }

    return false;
}
