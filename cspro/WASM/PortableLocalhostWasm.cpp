#include <engine/StandardSystemIncludes.h>
#include <zHtml/PortableLocalhost.h>


// --------------------------------------------------------------------------
// PortableLocalhost
// --------------------------------------------------------------------------

void PortableLocalhost::CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, NullTerminatedString filename/* = _T("")*/)
{
    throw CSProException("WASM_TODO -- Not implemented: CreateVirtualFile");
}


VirtualFileMapping PortableLocalhost::CreateVirtualHtmlFile(wstring_view directory, std::function<std::string()> callback)
{
    throw CSProException("WASM_TODO -- Not implemented: CreateVirtualHtmlFile");
}


void PortableLocalhost::CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    throw CSProException("WASM_TODO -- Not implemented: CreateVirtualDirectory");
}


std::wstring PortableLocalhost::CreateFilenameUrl(NullTerminatedStringView filename)
{
    throw CSProException("WASM_TODO -- Not implemented: CreateFilenameUrl");
}



// --------------------------------------------------------------------------
// LocalFileServerSetResponse
// --------------------------------------------------------------------------

void LocalFileServerSetResponse(void* response_object_ptr, const void* content_data, size_t content_size, const char* content_type)
{
    throw CSProException("WASM_TODO -- Not implemented: LocalFileServerSetResponse");
}
