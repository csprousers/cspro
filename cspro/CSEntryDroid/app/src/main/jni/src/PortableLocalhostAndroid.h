#pragma once

#include <zToolsO/NullTerminatedString.h>
#include <zHtml/PortableLocalhost.h>
#include <jni.h>


struct VirtualHtmlFileDetails
{
    std::wstring directory;
    std::wstring html_file_filename;
    std::function<std::string()> html_file_callback;
};


class LocalFileServer
{
private:
    using MappingType = std::variant<VirtualFileMappingHandler*,
                                     std::unique_ptr<VirtualHtmlFileDetails>,
                                     KeyBasedVirtualFileMappingHandler*,
                                     std::wstring>;
    struct Mapping
    {
        MappingType mapping_details;
        std::shared_ptr<bool> mapping_active;
    };

private:
    LocalFileServer();

public:
    static LocalFileServer& GetInstance();

    void CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, NullTerminatedString filename);
    VirtualFileMapping CreateVirtualHtmlFile(const std::wstring& directory, std::function<std::string()> html_file_callback);
    void CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler);
    std::wstring CreateFilenameUrl(std::wstring filename);

    bool GetVirtualFile(JNIEnv* pEnv, wstring_view path_sv, jbyteArray& content, std::wstring& content_type);

private:
    std::wstring CreateUrl(NullTerminatedString filename = _T(""), bool add_subdirectories_for_relative_pathing = false,
                           std::optional<size_t>* filename_url_mapping_index = nullptr);

private:
    const std::wstring m_baseUrl;
    std::vector<Mapping> m_mappings;
    std::optional<size_t> m_filenameUrlMappingIndex;
};
