#include "stdafx.h"
#include "PortableLocalhost.h"
#include "SharedHtmlLocalFileServer.h"
#include <zUtilF/ApplicationShutdownRunner.h>


namespace
{
    bool CanUseLocallyDeclaredSharedHtmlLocalFileServer()
    {
        return ( ApplicationShutdownRunner::Get() != nullptr );
    }
}


void PortableLocalhost::CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, NullTerminatedString filename/* = _T("")*/)
{
    ASSERT(CanUseLocallyDeclaredSharedHtmlLocalFileServer());
    SharedHtmlLocalFileServer file_server;
    file_server.CreateVirtualFile(virtual_file_mapping_handler, filename);
}


VirtualFileMapping PortableLocalhost::CreateVirtualHtmlFile(wstring_view directory, std::function<std::string()> callback)
{
    ASSERT(CanUseLocallyDeclaredSharedHtmlLocalFileServer());
    SharedHtmlLocalFileServer file_server;
    return file_server.CreateVirtualHtmlFile(directory, std::move(callback));
}


void PortableLocalhost::CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    ASSERT(CanUseLocallyDeclaredSharedHtmlLocalFileServer());
    SharedHtmlLocalFileServer file_server;
    file_server.CreateVirtualDirectory(key_based_virtual_file_mapping_handler);
}


std::wstring PortableLocalhost::CreateFilenameUrl(NullTerminatedStringView filename)
{
    ASSERT(CanUseLocallyDeclaredSharedHtmlLocalFileServer());
    SharedHtmlLocalFileServer file_server;
    return file_server.GetFilenameUrl(filename);
}
