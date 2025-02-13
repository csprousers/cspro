#pragma once

#include <zHtml/zHtml.h>


namespace LocalhostUrl
{
    constexpr const char* LocalhostHost                 = "localhost";
    constexpr const TCHAR* LocalhostHostWide            = _T("localhost");
    constexpr const TCHAR* LocalFileSystemDirectoryName = _T("lfs");
    constexpr const TCHAR* VirtualFileDirectoryName     = _T("vf");
    constexpr const TCHAR* AndroidBaseUrl               = _T("https://appassets.androidplatform.net/lfs/");

    // attempts to determine the directory from a Localhost URL based on a filename
    ZHTML_API std::wstring GetDirectoryFromUrl(const std::wstring& url);
}
