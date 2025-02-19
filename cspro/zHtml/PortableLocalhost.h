#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/VirtualFileMapping.h>


#ifndef WIN_DESKTOP
#define USE_PORTABLE_LOCAL_FILE_SERVER
#endif


class ZHTML_API PortableLocalhost
{
public:
    // documented in SharedHtmlLocalFileServer::CreateVirtualFile
    static void CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, cs::string_sz filename = "");

    // documented in SharedHtmlLocalFileServer::CreateVirtualHtmlFile
    static VirtualFileMapping CreateVirtualHtmlFile(const std::string& directory, std::function<SharableString()> callback);

    // documented in SharedHtmlLocalFileServer::CreateVirtualDirectory
    static void CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler);

    // documented in SharedHtmlLocalFileServer::CreateFileUrl
    static std::string CreateFileUrl(const std::string& file_path);

    // documented in SharedHtmlLocalFileServer::CreateUniqueFileUrl
    static std::string CreateUniqueFileUrl(const std::string& file_path);

private:
    static auto& GetPortableLocalFileServer();
};
