#include "stdafx.h"
#include "LocalFileServer.h"
#include "LocalhostSettings.h"
#include "LocalhostUrl.h"
#include <zToolsO/Utf8Convert.h>
#include <zToolsO/WinSettings.h>
#include <zUtilO/CSProExecutables.h>
#include <external/cpp-httplib/httplib.h>


namespace
{
    constexpr const wchar_t* CSProMapping   = _T("/cspro");
    constexpr time_t CSProMappingGetTimeout = 50000;
}


// --------------------------------------------------------------------------
// LocalFileServer
// --------------------------------------------------------------------------

LocalFileServer::LocalFileServer(NullTerminatedString root_directory)
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

        httplib::Result result = port_open_check.Get(UTF8Convert::WideToUTF8(CSProMapping));

        if( ( result && result->status == 200 ) || !m_server->bind_to_port(LocalhostUrl::LocalhostHost, *preferred_port) )
            preferred_port.reset();
    }

    // if there is no preferred port, or if it was in use, use any port
    m_port = preferred_port.has_value() ? *preferred_port :
                                          m_server->bind_to_any_port(LocalhostUrl::LocalhostHost);

    // set up the base mappings
    m_baseUrl = FormatTextCS2WS(_T("http://%s:%d/"), LocalhostUrl::LocalhostHostWide, m_port);

    // mount the root directory, which will usually be Html::GetDirectory()
    m_server->set_mount_point("/", UTF8Convert::WideToUTF8(root_directory));

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


void LocalFileServer::AddMountPoint(std::wstring unescaped_url, wstring_view directory)
{
    ASSERT(unescaped_url.length() >= 2 && unescaped_url.front() == '/');
    ASSERT(Encoders::ToUri(unescaped_url) == unescaped_url);

    m_server->set_mount_point(UTF8Convert::WideToUTF8(PortableFunctions::PathEnsureTrailingForwardSlash(unescaped_url)),
                              UTF8Convert::WideToUTF8(directory));
}


std::shared_ptr<bool> LocalFileServer::AddMapping(NullTerminatedString unescaped_url, std::string content_type, std::function<std::optional<std::string>()> callback)
{
    ASSERT(unescaped_url.length() >= 2 && unescaped_url.front() == '/');

    std::wstring url_regex = Encoders::ToRegex(unescaped_url);

    auto virtual_file_mapping_active = std::make_shared<bool>(true);

    m_server->Get(UTF8Convert::WideToUTF8(url_regex).c_str(),
        [callback_ = std::move(callback), content_type, virtual_file_mapping_active](const httplib::Request& /*request*/, httplib::Response& response)
        {
            if( *virtual_file_mapping_active )
            {
                std::optional<std::string> content = callback_();

                if( content.has_value() )
                {
                    response.set_content(*content, content_type.c_str());
                    return;
                }
            }

            // 404 on error or if the mapping is no longer active
            response.status = 404;
        });

    return virtual_file_mapping_active;
}


void LocalFileServer::AddMapping(VirtualFileMappingHandler& virtual_file_mapping_handler, NullTerminatedString filename)
{
    ASSERT(filename.empty() || filename.c_str() == PortableFunctions::PathGetFilename(filename));

    std::wstring unescaped_url = FormatTextCS2WS(filename.empty() ? _T("/%s/%d") : _T("/%s/%d/%s"),
                                                 LocalhostUrl::VirtualFileDirectoryName,
                                                 ++m_mappingDirectoryCounter,
                                                 filename.c_str());

    std::wstring url_regex = Encoders::ToRegex(unescaped_url);

    auto virtual_file_mapping_active = std::make_shared<bool>(true);

    m_server->Get(UTF8Convert::WideToUTF8(url_regex).c_str(),
        [&virtual_file_mapping_handler, virtual_file_mapping_active](const httplib::Request& /*request*/, httplib::Response& response)
        {
            if( *virtual_file_mapping_active )
            {
                ASSERT(virtual_file_mapping_active == virtual_file_mapping_handler.m_virtualFileMapping->m_mappingActive);

                if( virtual_file_mapping_handler.ServeContent(&response) )
                    return;
            }

            // 404 on error or if the mapping is no longer active
            response.status = 404;
        });

    std::wstring full_url = m_baseUrl + Encoders::ToUri(wstring_view(unescaped_url).substr(1), false);

    virtual_file_mapping_handler.m_virtualFileMapping.reset(new VirtualFileMapping(std::move(full_url), std::move(virtual_file_mapping_active)));
}


void LocalFileServer::AddMapping(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    const std::wstring unescaped_url = FormatTextCS2WS(_T("/%s/%d/"), LocalhostUrl::VirtualFileDirectoryName, ++m_mappingDirectoryCounter);
    const size_t key_start_pos = unescaped_url.length();

    // match anything in the directory
    const std::wstring url_regex = unescaped_url + _T(".*");

    auto virtual_file_mapping_active = std::make_shared<bool>(true);

    m_server->Get(UTF8Convert::WideToUTF8(url_regex).c_str(),
        [&key_based_virtual_file_mapping_handler, key_start_pos, virtual_file_mapping_active]
        (const httplib::Request& request, httplib::Response& response)
        {
            if( *virtual_file_mapping_active )
            {
                ASSERT(virtual_file_mapping_active == key_based_virtual_file_mapping_handler.m_virtualFileMapping->m_mappingActive);

                std::wstring key = UTF8Convert::UTF8ToWide(request.path);

                if( key.length() > key_start_pos )
                {
                    key = key.substr(key_start_pos);

                    if( key_based_virtual_file_mapping_handler.ServeContent(&response, key) )
                        return;
                }
            }

            // 404 on error or if the mapping is no longer active
            response.status = 404;
        });

    std::wstring full_base_url = m_baseUrl + unescaped_url.substr(1);

    key_based_virtual_file_mapping_handler.m_virtualFileMapping.reset(new VirtualFileMapping(std::move(full_base_url), std::move(virtual_file_mapping_active)));
}


void LocalFileServer::AddCSProMapping()
{
    AddMapping(CSProMapping, UTF8Convert::WideToUTF8(MimeType::Type::Json),
        [ module_filename = CSProExecutables::GetModuleFilename(),
          start_time = GetTimestamp() ]()
        {
            auto json_writer = Json::CreateStringWriter();

            json_writer->BeginObject()
                        .Write(JK::process, module_filename)
                        .Write(JK::startTime, start_time)
                        .EndObject();

            return UTF8Convert::WideToUTF8(json_writer->GetString());
        });
}



// --------------------------------------------------------------------------
// LocalFileServerSetResponse
// --------------------------------------------------------------------------

void LocalFileServerSetResponse(void* response_object, const void* content_data, size_t content_size, const char* content_type)
{
    httplib::Response* response = static_cast<httplib::Response*>(response_object);

    if( content_type != nullptr && *content_type != 0 )
    {
        response->set_content(static_cast<const char*>(content_data), content_size, content_type);
    }

    else
    {
        response->set_content(static_cast<const char*>(content_data), content_size);
    }
}
