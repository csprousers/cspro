#include "stdafx.h"
#include "LocalhostUrl.h"
#include <regex>


std::wstring LocalhostUrl::GetDirectoryFromUrl(const std::wstring& url)
{
    std::wcmatch matches;

    auto process_regex_match = [&]()
    {
        constexpr size_t AllDirectoriesIndex = OnWindowsDesktop() ? 2 : 1;
        ASSERT(matches.size() == ( AllDirectoriesIndex + 1 ));

#ifdef WIN_DESKTOP
        // the first match is the volume
        std::wstring volume = matches.str(1);
        ASSERT(!volume.empty() && volume.find(PATH_CHAR) == std::wstring::npos);

        if( volume.back() != ':' )
            volume.push_back(':');

#else
        std::wstring volume = _T("/");
#endif

        std::wstring all_directories = Encoders::FromPercentEncoding(matches.str(AllDirectoriesIndex));

        // on Android, the URL will come in percent-encoded twice
        if constexpr(OnAndroid())
            all_directories = Encoders::FromPercentEncoding(all_directories);

        ASSERT(all_directories == PortableFunctions::PathToForwardSlash(all_directories));

        return PortableFunctions::PathAppendToPath(volume, PortableFunctions::PathToNativeSlash(std::move(all_directories)));
    };


#ifdef WIN_DESKTOP

    // 1a) check if this is a Localhost URL
    //     e.g.:  http://localhost:50505/lfs/C/Action%20Invoker/.CS2.html
    //     (look at LocalFileServer::LocalFileServer and SharedHtmlLocalFileServer::Impl::GetFilenameUrl to see how filename URLs are created)
    static std::wregex lfs_regex([]() { return FormatTextCS2WS(LR"(^http:\/\/%s:\d+\/%s\/([^\/]*)\/(.*)\/.*$)", LocalhostHostWide, LocalFileSystemDirectoryName); }());

    if( std::regex_match(url.c_str(), matches, lfs_regex) )
        return process_regex_match();

    // 1b) check if this is a directory created as part of a virtual file mapping such as DocumentVirtualFileMappingHandler
    //     e.g.: http://localhost:50505/vf/1/C:/Action%20Invoker/my-html.html
    //     (look at LocalFileServer::AddMapping to see how key-based URLs are created)
    static std::wregex vf_regex([]() { return FormatTextCS2WS(LR"(^http:\/\/%s:\d+\/%s\/\d+\/([^\/]*)\/(.*)\/.*$)", LocalhostHostWide, LocalhostUrl::VirtualFileDirectoryName); }());

    if( std::regex_match(url.c_str(), matches, vf_regex) )
    {
        // because the virtual file mapping could be for something other than a directory, only return the value if it is a proper directory
        std::wstring path = process_regex_match();

        if( PortableFunctions::FileIsDirectory(path) )
            return path;
    }

#elif defined(ANDROID)

    // 1a) check if this is a Localhost URL
    //     e.g.:  https://appassets.androidplatform.net/lfs/1/storage/emulated/.../csentry/Action%20Invoker/.CS2.html
    //     (look at LocalFileServer::CreateUrl to see how filename URLs are created)
    static std::wregex lfs_regex([]() { return FormatTextCS2WS(LR"(^%s\d+\/(.*)\/.*$)", Encoders::ToRegex(AndroidBaseUrl).c_str()); }());

    if( std::regex_match(url.c_str(), matches, lfs_regex) )
        return process_regex_match();

#endif

    // 2) if the URL could not be decoded above, see if this is a file URL
    std::optional<std::wstring> path_from_file_url = Encoders::FromFileUrl(url);

    if( path_from_file_url.has_value() )
        return PortableFunctions::PathGetDirectory(*path_from_file_url);

    // 3) there was no ability to get the directory
    return std::wstring();
}
