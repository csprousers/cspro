#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/VirtualFileMapping.h>


class ZHTML_API PortableLocalhost
{
public:
    // documented in SharedHtmlLocalFileServer::CreateVirtualFile
    static void CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, NullTerminatedString filename = _T(""));

    // documented in SharedHtmlLocalFileServer::CreateVirtualHtmlFile
    static VirtualFileMapping CreateVirtualHtmlFile(wstring_view directory, std::function<std::string()> callback);

    // documented in SharedHtmlLocalFileServer::CreateVirtualDirectory
    static void CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler);

    static std::wstring CreateFilenameUrl(NullTerminatedStringView filename);
};
