#include "stdafx.h"
#include "PortableLocalFileServer.h"
#include <zToolsO/FileIO.h>


struct PortableLocalFileServer::VirtualHtmlFileDetails
{
    std::string directory;
    std::string html_file_path;
    std::function<SharableString()> html_file_callback;
};


PortableLocalFileServer::PortableLocalFileServer(std::string base_url)
    :   m_baseUrl(PortableFunctions::PathEnsureTrailingForwardSlash(PortableFunctions::PathToForwardSlash((std::move(base_url))))),
        m_filePathUrlMappingIndex(SIZE_MAX)
{
}


std::string PortableLocalFileServer::CreateUrl(const std::string& file_path/* = ""*/, const bool add_subdirectories_for_relative_pathing/* = false*/,
                                               const size_t mapping_index_override/* = SIZE_MAX*/)
{
    const size_t mapping_index = ( mapping_index_override == SIZE_MAX ) ? m_mappings.size() :
                                                                          mapping_index_override;

    ASSERT(mapping_index <= m_mappings.size());

    std::string url = PortableFunctions::PathAppendForwardSlashToPath(m_baseUrl, IntToString(mapping_index));

    if( !file_path.empty() )
    {
        auto append_to_url = [&](std::string append_text)
        {
            url = PortableFunctions::PathAppendForwardSlashToPath(url, Encoders::ToUri(std::move(append_text)));
        };

        // because a virtual HTML file may reference other files (using relative paths), we must construct a URL with
        // enough subdirectories so that we can properly process relative paths referencing previous directories
        if( add_subdirectories_for_relative_pathing )
        {
            const std::string directory = PortableFunctions::PathToForwardSlash(PortableFunctions::PathGetDirectory(file_path));

            for( std::string subdirectory_name : SO::SplitString(directory, '/', false, false) )
                append_to_url(std::move(subdirectory_name));
        }

        append_to_url(PortableFunctions::PathGetFilename(file_path));
    }

    return url;
}


void PortableLocalFileServer::CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, const cs::string_sz filename)
{
    // use a dummy filename if necessary
    std::string url = CreateUrl(!filename.empty() ? filename.c_str() : "g");

    Mapping& mapping = m_mappings.emplace_back(Mapping { &virtual_file_mapping_handler, std::make_shared<bool>(true) });
    virtual_file_mapping_handler.m_virtualFileMapping.reset(new VirtualFileMapping(std::move(url), mapping.mapping_active));
}


VirtualFileMapping PortableLocalFileServer::CreateVirtualHtmlFile(const std::string& directory, std::function<SharableString()> html_file_callback)
{
    // create a file path that does not exist in the directory where the virtual HTML file is located
    auto virtual_html_file_details = std::make_unique<VirtualHtmlFileDetails>(VirtualHtmlFileDetails
        {
            directory,
            PortableFunctions::GetUniqueFilePathInDirectory(directory, FileExtensions::HTML),
            std::move(html_file_callback)
        });

    std::string url = CreateUrl(virtual_html_file_details->html_file_path, true);

    Mapping& mapping = m_mappings.emplace_back(Mapping { std::move(virtual_html_file_details), std::make_shared<bool>(true) });
    return VirtualFileMapping(std::move(url), mapping.mapping_active);
}


void PortableLocalFileServer::CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    std::string url = CreateUrl();

    Mapping& mapping = m_mappings.emplace_back(Mapping { &key_based_virtual_file_mapping_handler, std::make_shared<bool>(true) });
    key_based_virtual_file_mapping_handler.m_virtualFileMapping.reset(new VirtualFileMapping(std::move(url), mapping.mapping_active));
}


std::string PortableLocalFileServer::CreateFileUrl(const std::string& file_path)
{
    // because file URLs include the full path of the file, and the mapping is forever active,
    // only a single mapping needs to be created that can be used to serve all files
    std::string url = CreateUrl(file_path, true, m_filePathUrlMappingIndex);

    if( m_filePathUrlMappingIndex == SIZE_MAX )
    {
        m_filePathUrlMappingIndex = m_mappings.size();
        m_mappings.emplace_back(Mapping { file_path, std::make_shared<bool>(true) });
    }

    return url;
}


std::string PortableLocalFileServer::CreateUniqueFileUrl(const std::string& file_path)
{
    std::string url = CreateUrl(file_path, true);
    m_mappings.emplace_back(Mapping { file_path, std::make_shared<bool>(true) });
    return url;
}


bool PortableLocalFileServer::GetVirtualFile(const std::string_view path_sv, VirtualFileMappingResponse& response)
{
    const auto [index_sv, key_sv] = SO::GetTextOnEitherSideOfCharacter(path_sv, '/');

    if( index_sv.length() != path_sv.length() )
    {
        const size_t mapping_index = atoi(std::string(index_sv).c_str());

        if( mapping_index < m_mappings.size() )
        {
            Mapping& mapping = m_mappings[mapping_index];

            if( *mapping.mapping_active )
            {
                try
                {
                    ResponseObject response_object
                    {
                        response,
                        key_sv
                    };

                    std::visit([&](auto& handler) { GetVirtualFile(handler, response_object); }, mapping.mapping_details);

                    return true;
                }

                catch( const CSProException& exception )
                {
                    TRACE("Error retrieving virtual file: %s\n", exception.what());
                }
            }
        }
    }

    return false;
}


std::string PortableLocalFileServer::GetFilePathFromUrlCreatedWithSubdirectoriesAddedForRelativePathing(const ResponseObject& response_object)
{
    constexpr bool PrependForwardSlashToPath = !OnWindowsDesktop();

#ifdef _DEBUG
    auto calculate_file_path_using_name_components = [&]()
    {
        const std::string normalized_path = PortableFunctions::PathToForwardSlash(std::string(response_object.key_sv));
        std::string file_path = PrependForwardSlashToPath ? "/" : "";

        for( const std::string_view name_component : SO::SplitString<std::string_view>(normalized_path, '/', false, false) )
            file_path = PortableFunctions::PathAppendForwardSlashToPath(file_path, name_component);

        return file_path;
    };
#endif

    std::string file_path = PrependForwardSlashToPath ? ( "/" +std::string(response_object.key_sv) ) :
                                                        std::string(response_object.key_sv);

    ASSERT(file_path == calculate_file_path_using_name_components());

    return PortableFunctions::MakePathToNativeSlash(file_path);
}


void PortableLocalFileServer::GetVirtualFile(VirtualFileMappingHandler* const handler, ResponseObject& response_object)
{
    handler->ServeContent(response_object.response);
}


void PortableLocalFileServer::GetVirtualFile(const std::shared_ptr<VirtualHtmlFileDetails>& virtual_html_file_details, ResponseObject& response_object)
{
    ASSERT(virtual_html_file_details != nullptr);

    const std::string file_path = GetFilePathFromUrlCreatedWithSubdirectoriesAddedForRelativePathing(response_object);

    // if a virtual HTML file, serve it using the callback
    if( file_path == virtual_html_file_details->html_file_path )
    {
        const SharableString content = virtual_html_file_details->html_file_callback ? virtual_html_file_details->html_file_callback() :
                                                                                       SharableString();

        response_object.response.SetContent(content->data(), content->size(), MimeType::Type::Html);
    }

    // otherwise serve it using the mechanism used for file URLs
    else
    {
        GetVirtualFile(file_path, response_object);
    }
}


void PortableLocalFileServer::GetVirtualFile(KeyBasedVirtualFileMappingHandler* const handler, ResponseObject& response_object)
{
    handler->ServeContent(response_object.response, std::string(response_object.key_sv));
}


void PortableLocalFileServer::GetVirtualFile(const std::string& /*file_path*/, ResponseObject& response_object)
{
    const std::string file_path = GetFilePathFromUrlCreatedWithSubdirectoriesAddedForRelativePathing(response_object);

    response_object.response.SetContent(FileIO::Read(file_path),
                                        ValueOrDefault(MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(file_path))));
}
