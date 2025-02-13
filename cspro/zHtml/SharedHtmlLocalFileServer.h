#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/VirtualFileMapping.h>


class ZHTML_API SharedHtmlLocalFileServer
{
public:
    SharedHtmlLocalFileServer(std::wstring project_root = std::wstring());
    ~SharedHtmlLocalFileServer();

    // returns a fully evaluated URL, appending the supplied URL to the project root's URL (if applicable);
    // the resultant URL will look like http://localhost:<port>/<project root>/<url_from_project_root>
    std::wstring GetProjectUrl(wstring_view url_from_project_root_sv) const;

    // returns a fully evaluated URL that can be used to retrieve the file;
    // if the filename's volume has not already been mapped, it will be added;
    // the resultant URL will look like http://localhost:<port>/lfs/<volume>/<path-to-file-on-volume>
    std::wstring GetFilenameUrl(NullTerminatedStringView filename);

    // creates a virtual file mapping that will find an unused filename with the extension .html in the directory;
    // the callback is used to get the HTML contents associated with that filename;
    // the mapping will last as long as the returned object lives
    VirtualFileMapping CreateVirtualHtmlFile(wstring_view directory_sv, std::function<std::string()> callback);

    // registers a virtual file mapping handler to a filename (if specified) that will be put in a unique directory;
    // the handler's ServeContent is used to get the content associated with that filename;
    // the mapping will last as long as the handler lives;
    // the resultant URL will look like http://localhost:<port>/vf/<unique id>/<optional filename>
    void CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, NullTerminatedString filename = _T(""));

    // registers a virtual file mapping handler that will process URLs from a unique directory;
    // the handler's ServeContent is used to get content associated with a key that is part of the URL;
    // the mapping will last as long as the handler lives;
    // the resultant URLs will look like http://localhost:<port>/vf/<unique id>/<key>
    void CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler);

private:
    std::wstring m_projectRoot;

    class Impl;
    static std::unique_ptr<Impl> m_sharedImpl;

    Impl* m_impl;
    bool m_usingSharedImpl;
};
