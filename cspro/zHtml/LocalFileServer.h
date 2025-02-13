#pragma once

#include <zHtml/VirtualFileMapping.h>
#include <thread>

namespace httplib { class Server; }


/// <summary>
/// Mini web server that can be used to serve files on the local file system.
/// The server maps a directory so that images and other files can be specified
/// using relative paths.
/// </summary>

class LocalFileServer
{
public:
    LocalFileServer(NullTerminatedString root_directory);
    ~LocalFileServer();

    void Start();
    void Stop();

    // returns the server port
    int GetPort() const { return m_port; }

    // returns the server's base URL, which will look like http://localhost:<port>/
    const std::wstring& GetBaseUrl() const { return m_baseUrl; }

    // adds a mount point, mapping the URL to the directory;
    // the URL should not contain any characters that require escaping
    void AddMountPoint(std::wstring unescaped_url, wstring_view directory);

    // adds a mapping for the URL that will result in the callback being called to return content, or std::nullopt if no content is available;
    // the URL can contain characters that would require escaping, but the argument should not contain the escaped characters;
    // the function returns a boolean that can be set to false to indicate that the callback is no longer active
    std::shared_ptr<bool> AddMapping(NullTerminatedString unescaped_url, std::string content_type, std::function<std::optional<std::string>()> callback);

    // adds a mapping to a virtual file using the filename (if specified) that will result in the
    // VirtualFileMappingHandler's subclass' ServeContent method being called to return content;
    // the filename can contain characters that would require escaping, but the argument should not contain the escaped characters
    void AddMapping(VirtualFileMappingHandler& virtual_file_mapping_handler, NullTerminatedString filename);

    // adds a mapping that will result in all requests originating from the handler's base URL being passed to 
    // the KeyBasedVirtualFileMappingHandler's subclass' ServeContent method to return content;
    // KeyBasedVirtualFileMappingHandler::CreateUrl can be called to create URLs 
    void AddMapping(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler);

private:
    // adds a mapping at http://localhost:<port>/cspro that returns information about what process is
    // running the server; this mapping is used to make sure that two processes don't use the same port
    void AddCSProMapping();

private:
    httplib::Server* m_server;
    std::thread m_thread;
    int m_port;
    std::wstring m_baseUrl;
    int m_mappingDirectoryCounter;
};
