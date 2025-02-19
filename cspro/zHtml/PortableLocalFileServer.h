#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/VirtualFileMapping.h>


class ZHTML_API PortableLocalFileServer
{
private:
    struct VirtualHtmlFileDetails;

    using MappingType = std::variant<VirtualFileMappingHandler*,
                                     std::shared_ptr<VirtualHtmlFileDetails>,
                                     KeyBasedVirtualFileMappingHandler*,
                                     std::string>;
    struct Mapping
    {
        MappingType mapping_details;
        std::shared_ptr<bool> mapping_active;
    };

public:
    struct ResponseObject
    {
        VirtualFileMappingResponse& response;
        std::string_view key_sv;
    };

public:
    PortableLocalFileServer(std::string base_url);

    // PortableLocalhost implementations
    void CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, cs::string_sz filename);
    VirtualFileMapping CreateVirtualHtmlFile(const std::string& directory, std::function<SharableString()> callback);
    void CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler);
    std::string CreateFileUrl(const std::string& file_path);
    std::string CreateUniqueFileUrl(const std::string& file_path);

    // Returns true if the virtual file was successfully retrieved.
    bool GetVirtualFile(std::string_view path_sv, VirtualFileMappingResponse& response);

private:
    // Returns a URL that begins with the size of m_mappings (or mapping_index_override),
    // which is used in GetVirtualFile to determine what mapping this is,
    // so any calls to this should be made before adding an entry to m_mappings.
    std::string CreateUrl(const std::string& file_path = "", bool add_subdirectories_for_relative_pathing = false,
                          size_t mapping_index_override = SIZE_MAX);

    static std::string GetFilePathFromUrlCreatedWithSubdirectoriesAddedForRelativePathing(const ResponseObject& response_object);

    void GetVirtualFile(VirtualFileMappingHandler* handler, ResponseObject& response_object);
    void GetVirtualFile(const std::shared_ptr<VirtualHtmlFileDetails>& virtual_html_file_details, ResponseObject& response_object);
    void GetVirtualFile(KeyBasedVirtualFileMappingHandler* handler, ResponseObject& response_object);
    void GetVirtualFile(const std::string& file_path, ResponseObject& response_object);    

private:
    std::string m_baseUrl;
    std::vector<Mapping> m_mappings;
    size_t m_filePathUrlMappingIndex;
};
