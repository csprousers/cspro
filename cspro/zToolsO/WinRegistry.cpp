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


bool WinRegistry::Open(const HKEY base_key, const wchar_t* const key, const bool create_key_if_not_exists/* = false*/)
{
    Close();

    if( create_key_if_not_exists )
    {
        RegCreateKey(base_key, key, &m_hKey);
    }

    else
    {
        RegOpenKey(base_key, key, &m_hKey);
    }

    return ( m_hKey != nullptr );
}


bool WinRegistry::Open(const HKEY base_key, const std::string_view key_sv, const bool create_key_if_not_exists/* = false*/)
{
    return Open(base_key, TC::ToWide(key_sv).c_str(), create_key_if_not_exists);
}


void WinRegistry::Close()
{
    if( m_hKey != nullptr )
    {
        RegCloseKey(m_hKey);
        m_hKey = nullptr;
    }
}


bool WinRegistry::ReadString(const wchar_t* const value_name, CString* const pcsValueData)
{
    if( m_hKey != nullptr )
    {
        constexpr int BufferSize = 500;
        TCHAR szBuff[BufferSize];
        DWORD dwType = 0;
        DWORD dwKeyLen = sizeof(szBuff);

        LSTATUS lStatus = RegQueryValueEx(m_hKey, value_name, nullptr, &dwType, reinterpret_cast<LPBYTE>(szBuff), &dwKeyLen);
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
                const int characters_used = ExpandEnvironmentStrings(szBuff, szExpandedBuff, BufferSize);

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


bool WinRegistry::ReadString(const wchar_t* const value_name, std::wstring& value_data)
{
    CString cstring_value_data;

    if( ReadString(value_name, &cstring_value_data) )
    {
        value_data = CS2WS(cstring_value_data);
        return true;
    }

    return false;
}


bool WinRegistry::ReadString(const std::string_view value_name_sv, std::string& value_data)
{
    CString cstring_value_data;

    if( ReadString(TC::ToWide(value_name_sv).c_str(), &cstring_value_data) )
    {
        value_data = UTF8_TODO::GetUtf8(cstring_value_data);
        return true;
    }

    return false;
}


std::optional<std::string> WinRegistry::ReadOptionalString(const std::string_view value_name_sv)
{
    std::string value;

    if( ReadString(value_name_sv, value) )
        return value;

    return std::nullopt;
}


bool WinRegistry::WriteString(const wchar_t* const value_name, const wstring_view value_data_sv)
{
    return ( m_hKey != nullptr &&
             RegSetKeyValue(m_hKey, nullptr, value_name, REG_SZ, value_data_sv.data(), value_data_sv.length() * sizeof(TCHAR)) == ERROR_SUCCESS );
}


bool WinRegistry::WriteString(const std::string_view value_name_sv, const std::string_view value_data_sv)
{
    return WriteString(TC::ToWide(value_name_sv).c_str(), TC::ToWide(value_data_sv));
}


bool WinRegistry::ReadDWord(const wchar_t* const value_name, DWORD* const pdwData)
{
    if( m_hKey != nullptr )
    {
        DWORD dwType = 0;
        DWORD dwKeyLen = sizeof(*pdwData);

        LSTATUS lStatus = RegQueryValueEx(m_hKey, value_name, nullptr, &dwType, reinterpret_cast<LPBYTE>(pdwData), &dwKeyLen);
        return ( lStatus == ERROR_SUCCESS && dwType == REG_DWORD );
    }

    return false;
}


bool WinRegistry::ReadDWord(const std::string_view value_name_sv, DWORD* const pdwData)
{
    return ReadDWord(TC::ToWide(value_name_sv).c_str(), pdwData);
}


bool WinRegistry::WriteDWord(const wchar_t* const value_name, const DWORD valueData)
{
    if( m_hKey != nullptr )
    {
        LSTATUS lStatus = RegSetKeyValue(m_hKey, nullptr, value_name, REG_DWORD, reinterpret_cast<const BYTE*>(&valueData), sizeof(valueData));
        return ( lStatus  == ERROR_SUCCESS );
    }

    return false;
}
