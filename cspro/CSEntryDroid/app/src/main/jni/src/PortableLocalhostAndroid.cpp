#include <engine/StandardSystemIncludes.h>
#include "PortableLocalhostAndroid.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/MimeType.h>
#include <zHtml/LocalhostUrl.h>


namespace
{
    struct ResponseObject
    {
        JNIEnv* pEnv;
        wstring_view key_sv;
        jbyteArray& content;
        std::wstring& content_type;
    };

    void GetVirtualFile(VirtualFileMappingHandler* handler, ResponseObject& response_object);
    void GetVirtualFile(const std::unique_ptr<VirtualHtmlFileDetails>& virtual_html_file_details, ResponseObject& response_object);
    void GetVirtualFile(KeyBasedVirtualFileMappingHandler* handler, ResponseObject& response_object);
    void GetVirtualFile(const std::wstring& filename, ResponseObject& response_object);
}


// --------------------------------------------------------------------------
// LocalFileServer
// --------------------------------------------------------------------------

LocalFileServer::LocalFileServer()
    :   m_baseUrl(LocalhostUrl::AndroidBaseUrl)
{
}


LocalFileServer& LocalFileServer::GetInstance()
{
    static LocalFileServer local_file_server;
    return local_file_server;
}


// CreateUrl returns a URL that begins with the size of m_mappings (which is used in GetVirtualFile to determine what mapping this is),
// so any calls to this should be made before adding an entry to m_mappings
std::wstring LocalFileServer::CreateUrl(NullTerminatedString filename/* = _T("")*/, bool add_subdirectories_for_relative_pathing/* = false*/,
                                        std::optional<size_t>* filename_url_mapping_index/* = nullptr*/)
{
    size_t mapping_index = m_mappings.size();

    // because filename URLs include the full path of the filename, and the mapping is forever active,
    // only a single mapping needs to be created that can be used to serve all files
    if( filename_url_mapping_index != nullptr )
    {
        if( filename_url_mapping_index->has_value() )
        {
            mapping_index = *(*filename_url_mapping_index);
        }

        else
        {
            *filename_url_mapping_index = mapping_index;
        }
    }

    std::wstring url = PortableFunctions::PathAppendForwardSlashToPath(m_baseUrl, IntToString(mapping_index));

    if( !filename.empty() )
    {
        auto append_to_url = [&](wstring_view append_text_sv)
        {
            url = PortableFunctions::PathAppendForwardSlashToPath(url, Encoders::ToUri(append_text_sv));
        };

        // because a virtual HTML file may reference other files (using relative paths), we must construct a URL with
        // enough subdirectories so that we can properly process relative paths referencing previous directories
        if( add_subdirectories_for_relative_pathing )
        {
            const std::wstring directory = PortableFunctions::PathToForwardSlash(PortableFunctions::PathGetDirectory(filename));

            for( wstring_view subdirectory_name_sv : SO::SplitString<wstring_view>(directory, '/', false, false) )
                append_to_url(Encoders::ToUri(subdirectory_name_sv));
        }

        append_to_url(PortableFunctions::PathGetFilename(filename));
    }

    return url;
}


void LocalFileServer::CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, NullTerminatedString filename)
{
    // use a dummy filename if necessary
    std::wstring url = CreateUrl(!filename.empty() ? filename.c_str() : _T("g"));

    Mapping& mapping = m_mappings.emplace_back(Mapping { &virtual_file_mapping_handler, std::make_shared<bool>(true) });
    virtual_file_mapping_handler.m_virtualFileMapping.reset(new VirtualFileMapping(std::move(url), mapping.mapping_active));
}


VirtualFileMapping LocalFileServer::CreateVirtualHtmlFile(const std::wstring& directory, std::function<std::string()> html_file_callback)
{
    // create a filename that does not exist in the directory where the virtual HTML filename is located
    auto virtual_html_file_details = std::make_unique<VirtualHtmlFileDetails>(VirtualHtmlFileDetails
        {
            directory,
            PortableFunctions::GetUniqueFilenameInDirectory(directory, FileExtensions::HTML),
            std::move(html_file_callback)
        });

    std::wstring url = CreateUrl(virtual_html_file_details->html_file_filename, true);

    Mapping& mapping = m_mappings.emplace_back(Mapping { std::move(virtual_html_file_details), std::make_shared<bool>(true) });
    return VirtualFileMapping(std::move(url), mapping.mapping_active);
}


void LocalFileServer::CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    std::wstring url = CreateUrl();

    Mapping& mapping = m_mappings.emplace_back(Mapping { &key_based_virtual_file_mapping_handler, std::make_shared<bool>(true) });
    key_based_virtual_file_mapping_handler.m_virtualFileMapping.reset(new VirtualFileMapping(std::move(url), mapping.mapping_active));
}


std::wstring LocalFileServer::CreateFilenameUrl(std::wstring filename)
{
    const bool first_filename_url_mapping = !m_filenameUrlMappingIndex.has_value();

    std::wstring url = CreateUrl(filename, true, &m_filenameUrlMappingIndex);

    if( first_filename_url_mapping )
        m_mappings.emplace_back(Mapping { std::move(filename), std::make_shared<bool>(true) });

    return url;
}


bool LocalFileServer::GetVirtualFile(JNIEnv* pEnv, wstring_view path_sv, jbyteArray& content, std::wstring& content_type)
{
    const size_t slash_pos = path_sv.find('/');

    if( slash_pos != wstring_view::npos )
    {
        const size_t mapping_index = _ttoi(std::wstring(path_sv.substr(0, slash_pos)).c_str());

        if( mapping_index < m_mappings.size() )
        {
            Mapping& mapping = m_mappings[mapping_index];

            if( *mapping.mapping_active )
            {
                try
                {
                    ResponseObject response_object
                    {
                        pEnv,
                        path_sv.substr(slash_pos + 1),
                        content,
                        content_type
                    };

                    std::visit([&](auto& handler) { ::GetVirtualFile(handler, response_object); }, mapping.mapping_details);

                    return true;
                }

                catch(...)
                {
                }
            }
        }
    }

    return false;
}


// --------------------------------------------------------------------------
// PortableLocalhost
// --------------------------------------------------------------------------

void PortableLocalhost::CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, NullTerminatedString filename/* = _T("")*/)
{
    LocalFileServer::GetInstance().CreateVirtualFile(virtual_file_mapping_handler, filename);
}


VirtualFileMapping PortableLocalhost::CreateVirtualHtmlFile(wstring_view directory, std::function<std::string()> callback)
{
    return LocalFileServer::GetInstance().CreateVirtualHtmlFile(directory, std::move(callback));
}


void PortableLocalhost::CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    LocalFileServer::GetInstance().CreateVirtualDirectory(key_based_virtual_file_mapping_handler);
}


std::wstring PortableLocalhost::CreateFilenameUrl(NullTerminatedStringView filename)
{
    return LocalFileServer::GetInstance().CreateFilenameUrl(filename);
}



// --------------------------------------------------------------------------
// LocalFileServerSetResponse + GetVirtualFile implementations
// --------------------------------------------------------------------------

void LocalFileServerSetResponse(void* response_object_ptr, const void* content_data, size_t content_size, const char* content_type)
{
    ResponseObject& response_object = *static_cast<ResponseObject*>(response_object_ptr);

    response_object.content = response_object.pEnv->NewByteArray(content_size);
    response_object.pEnv->SetByteArrayRegion(response_object.content, 0, content_size, (jbyte*)content_data);

    if( content_type != nullptr && *content_type != 0 )
        response_object.content_type = UTF8Convert::UTF8ToWide(content_type);
}


namespace
{
    std::wstring GetFilenameFromUrlCreatedWithSubdirectoriesAddedForRelativePathing(const ResponseObject& response_object)
    {
#ifdef _DEBUG
        auto calculate_filename_using_name_components = [&]()
        {
            const std::wstring normalized_path = PortableFunctions::PathToForwardSlash<std::wstring>(response_object.key_sv);
            std::wstring filename = _T("/");

            for( wstring_view name_component : SO::SplitString<wstring_view>(normalized_path, '/', false, false) )
                filename = PortableFunctions::PathAppendForwardSlashToPath(filename, name_component);

            return filename;
        };
#endif

        std::wstring filename = _T("/") + std::wstring(response_object.key_sv);
        ASSERT(filename == calculate_filename_using_name_components());

        return filename;
    }


    void GetVirtualFile(VirtualFileMappingHandler* handler, ResponseObject& response_object)
    {
        handler->ServeContent(&response_object);
    }


    void GetVirtualFile(const std::unique_ptr<VirtualHtmlFileDetails>& virtual_html_file_details, ResponseObject& response_object)
    {
        ASSERT(virtual_html_file_details != nullptr);

        const std::wstring filename = GetFilenameFromUrlCreatedWithSubdirectoriesAddedForRelativePathing(response_object);

        // if the virtual HTML file, serve it using the callback
        if( filename == virtual_html_file_details->html_file_filename )
        {
            std::string content = virtual_html_file_details->html_file_callback ? virtual_html_file_details->html_file_callback() : std::string();
            LocalFileServerSetResponse(&response_object, content.data(), content.size(), MimeType::Type::Html);
        }

        // otherwise serve it using the mechanism used for file URLs
        else
        {
            GetVirtualFile(filename, response_object);
        }
    }


    void GetVirtualFile(KeyBasedVirtualFileMappingHandler* handler, ResponseObject& response_object)
    {
        handler->ServeContent(&response_object, response_object.key_sv);
    }


    void GetVirtualFile(const std::wstring& /*filename*/, ResponseObject& response_object)
    {
        const std::wstring filename = GetFilenameFromUrlCreatedWithSubdirectoriesAddedForRelativePathing(response_object);

        std::unique_ptr<const std::vector<std::byte>> content = FileIO::Read(filename);

        LocalFileServerSetResponse(&response_object, content->data(), content->size(), nullptr);

        response_object.content_type = ValueOrDefault(MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(filename)));
    }
}
