#pragma once

#include <zHtml/zHtml.h>


namespace LocalhostUrl
{
    constexpr const char* LocalhostHost                = "localhost";
    constexpr const char* LocalFileSystemDirectoryName = "lfs";
    constexpr const char* VirtualFileDirectoryName     = "vf";
    constexpr const char* AndroidBaseUrl               = "https://appassets.androidplatform.net/lfs/";
    constexpr std::string_view WasmBaseUrlSuffix_sv    = "wlfs/";

    // attempts to determine the directory from a Localhost URL based on a file path
    ZHTML_API std::string GetDirectoryFromUrl(const std::string& url);
}
