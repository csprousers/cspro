#include "stdafx.h"
#include "LocalFileServer.h"
#include "LocalhostSettings.h"
#include "LocalhostUrl.h"
#include "PortableLocalFileServer.h"
#include <zToolsO/WinSettings.h>
#include <zUtilO/CSProExecutables.h>
#include <external/cpp-httplib/httplib.h>


namespace
{
    constexpr const char* CSProMapping      = "/cspro";
    constexpr time_t CSProMappingGetTimeout = 50000;
}


// --------------------------------------------------------------------------
// LocalFileServer
// --------------------------------------------------------------------------

LocalFileServer::LocalFileServer(const std::string& root_directory)
    :   m_mappingDirectoryCounter(0)
{
    ASSERT(PortableFunctions::FileIsDirectory(root_directory));

    m_server = new httplib::Server();

    // if the user specifies a preferred port, see if it is in use by another instance
    std::optional<int> preferred_port = LocalhostSettings::GetPreferredPort();

    if( preferred_port.has_value() )
    {
        httplib::Client port_open_check(LocalhostUrl::LocalhostHost, *preferred_port);

        // only try to connect for a short time (50 milliseconds)
        port_open_check.set_connection_timeout(0, CSProMappingGetTimeout);
        port_open_check.set_read_timeout(0, CSProMappingGetTimeout);

        httplib::Result result = port_open_check.Get(CSProMapping);

        if( ( result && result->status == 200 ) || !m_server->bind_to_port(LocalhostUrl::LocalhostHost, *preferred_port) )
            preferred_port.reset();
    }

    // if there is no preferred port, or if it was in use, use any port
    m_port = preferred_port.has_value() ? *preferred_port :
                                          m_server->bind_to_any_port(LocalhostUrl::LocalhostHost);

    // set up the base mappings
    m_baseUrl = FormatText("http://%s:%d/", LocalhostUrl::LocalhostHost, m_port);

    // mount the root directory, which will usually be Html::GetDirectory()
    m_server->set_mount_point("/", root_directory);

    // add the CSPro mapping, which can be used by other instances to see if the port is in use
    AddCSProMapping();
}


LocalFileServer::~LocalFileServer()
{
    Stop();
    delete m_server;
}


void LocalFileServer::Start()
{
    m_thread = std::thread([&]() { m_server->listen_after_bind(); });
}


void LocalFileServer::Stop()
{
    if( m_server->is_running() )
    {
        m_server->stop();
        m_thread.join();
    }
}


void LocalFileServer::AddMountPoint(std::string unescaped_url, const std::string& directory)
{
    ASSERT(unescaped_url.length() >= 2 && unescaped_url.front() == '/');
    ASSERT(Encoders::ToUri(unescaped_url) == unescaped_url);

    m_server->set_mount_point(PortableFunctions::PathEnsureTrailingForwardSlash(std::move(unescaped_url)),
                              directory);
}


std::shared_ptr<bool> LocalFileServer::AddMapping(const cs::string_sz unescaped_url, std::string content_type, std::function<SharableString()> callback)
{
    ASSERT(unescaped_url.length() >= 2 && unescaped_url.front() == '/');

    const std::string url_regex = Encoders::ToRegex(unescaped_url);

    auto virtual_file_mapping_active = std::make_shared<bool>(true);

    m_server->Get(url_regex.c_str(),
        [callback_ = std::move(callback), content_type_ = std::move(content_type), virtual_file_mapping_active]
        (const httplib::Request& /*request*/, httplib::Response& response)
        {
            if( *virtual_file_mapping_active )
            {
                const SharableString content = callback_();

                if( content.IsSet() )
                {
                    response.set_content(content.GetString(), content_type_.c_str());
                    return;
                }
            }

            // 404 on error or if the mapping is no longer active
            response.status = 404;
        });

    return virtual_file_mapping_active;
}


void LocalFileServer::AddMapping(VirtualFileMappingHandler& virtual_file_mapping_handler, const cs::string_sz filename)
{
    ASSERT(filename.empty() || filename.c_str() == PortableFunctions::PathGetFilename(filename.c_str()));

    const std::string unescaped_url = FormatText(filename.empty() ? "/%s/%d" : "/%s/%d/%s",
                                                 LocalhostUrl::VirtualFileDirectoryName,
                                                 ++m_mappingDirectoryCounter,
                                                 filename.c_str());

    const std::string url_regex = Encoders::ToRegex(unescaped_url);

    auto virtual_file_mapping_active = std::make_shared<bool>(true);

    m_server->Get(url_regex.c_str(),
        [&virtual_file_mapping_handler, virtual_file_mapping_active]
        (const httplib::Request& /*request*/, httplib::Response& response)
        {
            if( *virtual_file_mapping_active )
            {
                ASSERT(virtual_file_mapping_active == virtual_file_mapping_handler.m_virtualFileMapping->m_mappingActive);

                VirtualFileMappingResponse vfm_response(&response);

                if( virtual_file_mapping_handler.ServeContent(vfm_response) )
                    return;
            }

            // 404 on error or if the mapping is no longer active
            response.status = 404;
        });

    std::string full_url = m_baseUrl + Encoders::ToUri(std::string_view(unescaped_url).substr(1), false);

    virtual_file_mapping_handler.m_virtualFileMapping.reset(new VirtualFileMapping(std::move(full_url), std::move(virtual_file_mapping_active)));
}


void LocalFileServer::AddMapping(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    const std::string unescaped_url = FormatText("/%s/%d/", LocalhostUrl::VirtualFileDirectoryName, ++m_mappingDirectoryCounter);
    const size_t key_start_pos = unescaped_url.length();

    // match anything in the directory
    const std::string url_regex = unescaped_url + ".*";

    auto virtual_file_mapping_active = std::make_shared<bool>(true);

    m_server->Get(url_regex.c_str(),
        [&key_based_virtual_file_mapping_handler, key_start_pos, virtual_file_mapping_active]
        (const httplib::Request& request, httplib::Response& response)
        {
            if( *virtual_file_mapping_active )
            {
                ASSERT(virtual_file_mapping_active == key_based_virtual_file_mapping_handler.m_virtualFileMapping->m_mappingActive);

                if( request.path.length() > key_start_pos )
                {
                    VirtualFileMappingResponse vfm_response(&response);
                    const std::string key = request.path.substr(key_start_pos);

                    if( key_based_virtual_file_mapping_handler.ServeContent(vfm_response, key) )
                        return;
                }
            }

            // 404 on error or if the mapping is no longer active
            response.status = 404;
        });

    std::string full_base_url = SO::Concatenate(m_baseUrl, std::string_view(unescaped_url).substr(1));

    key_based_virtual_file_mapping_handler.m_virtualFileMapping.reset(new VirtualFileMapping(std::move(full_base_url), std::move(virtual_file_mapping_active)));
}


void LocalFileServer::AddCSProMapping()
{
    AddMapping(CSProMapping, MimeType::Type::Json,
        [ module_file_path = CSProExecutables::GetModuleFilePath(),
          start_time = GetTimestamp() ]()
        {
            const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

            json_writer->BeginObject()
                        .Write(JK::process, module_file_path)
                        .Write(JK::startTime, start_time)
                        .EndObject();

            return json_writer->ReleaseString();
        });
}



// --------------------------------------------------------------------------
// VirtualFileMappingHandler::ServeContent
// --------------------------------------------------------------------------

void VirtualFileMappingResponse::SetContent(const void* const content_data, const size_t content_size, const cs::string_sz content_type)
{
    httplib::Response* const response = static_cast<httplib::Response*>(m_responseObject);

    if( !content_type.empty() )
    {
        response->set_content(static_cast<const char*>(content_data), content_size, content_type.c_str());
    }

    else
    {
        response->set_content(static_cast<const char*>(content_data), content_size);
    }
}
