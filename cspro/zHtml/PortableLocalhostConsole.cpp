#include "stdafx.h"
#include "PortableLocalhost.h"


// --------------------------------------------------------------------------
// PortableLocalhost
// --------------------------------------------------------------------------

void PortableLocalhost::CreateVirtualFile(VirtualFileMappingHandler& /*virtual_file_mapping_handler*/, NullTerminatedString /*filename = _T("")*/)
{
    ASSERT(false);
}


void PortableLocalhost::CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& /*key_based_virtual_file_mapping_handler*/)
{
    ASSERT(false);
}


class LocalFileServer // dummy class to access VirtualFileMapping's private constructor
{
public:
    static VirtualFileMapping CreateVirtualFileMapping() { return VirtualFileMapping(std::wstring(), std::make_shared<bool>(true)); }
};


VirtualFileMapping PortableLocalhost::CreateVirtualHtmlFile(wstring_view /*directory*/, std::function<std::string()> /*callback*/)
{
    ASSERT(false);
    return LocalFileServer::CreateVirtualFileMapping();
}


std::wstring PortableLocalhost::CreateFilenameUrl(NullTerminatedStringView filename)
{
    ASSERT(false);
    return std::wstring();
}



// --------------------------------------------------------------------------
// LocalFileServerSetResponse
// --------------------------------------------------------------------------

void LocalFileServerSetResponse(void* /*response_object*/, const void* /*content_data*/, size_t /*content_size*/, const char* /*content_type*/)
{
    ASSERT(false);
}
