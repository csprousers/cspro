#pragma once

#include <zHtml/PortableLocalFileServer.h>


class WasmLocalFileServer : public PortableLocalFileServer
{
    // the maximum number of virtual files to keep in memory
    static constexpr size_t MaxVirtualFilesSize = 5;

public:
    struct VirtualFile;

    WasmLocalFileServer(std::string base_url);
    ~WasmLocalFileServer();

    // returns the index of the stored virtual file, or -1 on error
    int RetrieveVirtualFile(const std::string& path);

    // returns the virtual file associated with the index
    emscripten::val GetVirtualFile(int index);

private:
    // removes virtual files that have been retrieved, or unretrieved ones if necessary,
    // to result in m_virtualFiles.size() < MaxVirtualFilesSize
    void ClearRetrievedVirtualFiles();

private:
    std::mutex m_mutex;
    std::map<int, std::unique_ptr<VirtualFile>> m_virtualFiles;
    int m_nextIndex;
};
