#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/VirtualFileMapping.h>


// --------------------------------------------------------------------------
// FileSystemVirtualFileMappingHandler
//
// a subclass of KeyBasedVirtualFileMappingHandler that serves any files
// on the file system
//
// an optional feature allows for the addition of virtual file mapping
// handlers with a specific path that will be served separately from the
// other files on the file system;
//
// this added virtual file mapping handler does not need to be mapped
// separately because this class will be mapped and only the
// VirtualFileMappingHandler::ServeContent override will be called, not
// VirtualFileMappingHandler::GetUrl
// --------------------------------------------------------------------------

class ZHTML_API FileSystemVirtualFileMappingHandler : public KeyBasedVirtualFileMappingHandler
{
public:
    FileSystemVirtualFileMappingHandler() { }
    FileSystemVirtualFileMappingHandler(std::wstring file_path, std::shared_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler);

    void RegisterSpecialHandler(std::wstring file_path, std::shared_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler);

    std::wstring CreateUrlForPath(const std::wstring& file_path) const;

private:
    VirtualFileMappingHandler* GetSpecialHandler(const std::wstring& file_path);

    bool ServeContent(void* response_object, const std::wstring& key) override;

    bool ServeFileSystemContent(void* response_object, const std::wstring& file_path) const;

private:
    std::map<std::wstring, std::shared_ptr<VirtualFileMappingHandler>> m_specialHandlers;
};

