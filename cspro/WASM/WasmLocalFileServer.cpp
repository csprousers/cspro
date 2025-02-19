#include "StdAfx.h"
#include "WasmLocalFileServer.h"
#include <zToolsO/Encoders.h>
#include <zHtml/LocalhostUrl.h>


struct WasmLocalFileServer::VirtualFile
{
    std::shared_ptr<const std::vector<std::byte>> content;
    std::string content_type;
    bool retrieved = false;
};


// --------------------------------------------------------------------------
// WasmLocalFileServer
// --------------------------------------------------------------------------

WasmLocalFileServer::WasmLocalFileServer(std::string base_url)
    :   PortableLocalFileServer(Path::Combine(std::move(base_url), LocalhostUrl::WasmBaseUrlSuffix_sv)),
        m_nextIndex(1)
{
}


WasmLocalFileServer::~WasmLocalFileServer()
{
}


int WasmLocalFileServer::RetrieveVirtualFile(const std::string& path)
{
    ASSERT(WasmController::OnWorkerThread());

    try
    {
        if( !SO::StartsWith(path, LocalhostUrl::WasmBaseUrlSuffix_sv) )
            throw CSProException("The path '%s' is not valid.", path.c_str());

        // the path URL will arrive percent-encoded
        const std::string evaluated_path = Encoders::FromPercentEncoding(std::string_view(path).substr(LocalhostUrl::WasmBaseUrlSuffix_sv.length()));
        ASSERT(path.empty() || path.front() != '/');

        auto virtual_file = std::make_unique<VirtualFile>();
        VirtualFileMappingResponse vfm_response(virtual_file.get());

        if( !PortableLocalFileServer::GetVirtualFile(evaluated_path, vfm_response) )
            throw CSProException("The evaluated path '%s' could not be handled by the local file server.", evaluated_path.c_str());

        ASSERT(virtual_file->content != nullptr);

        // store the content
        std::lock_guard<std::mutex> lock(m_mutex);

        if( m_virtualFiles.size() == MaxVirtualFilesSize )
            ClearRetrievedVirtualFiles();

        ASSERT(m_virtualFiles.size() < MaxVirtualFilesSize);

        TRACE("Virtual file '%s' with size %d stored at index %d", evaluated_path.c_str(),
                                                                   static_cast<int>(virtual_file->content->size()),
                                                                   m_nextIndex);

        m_virtualFiles.try_emplace(m_nextIndex, std::move(virtual_file));

        return m_nextIndex++;
    }

    catch( const CSProException& exception )
    {
        TRACE("Exception: %s\n", exception.what());
    }

    return -1;
}


emscripten::val WasmLocalFileServer::GetVirtualFile(const int index)
{
    ASSERT(WasmController::OnMainThread());

    if( index == -1 )
        return emscripten::val::undefined();

    std::lock_guard<std::mutex> lock(m_mutex);

    const auto& lookup = m_virtualFiles.find(index);

    if( lookup == m_virtualFiles.cend() )
    {
        ASSERT(false);
        return emscripten::val::undefined();
    }

    VirtualFile& virtual_file = *lookup->second;
    virtual_file.retrieved = true;

    auto js_virtuaL_file = emscripten::val::object();

    js_virtuaL_file.set("content", emscripten::val(emscripten::typed_memory_view(virtual_file.content->size(), reinterpret_cast<const char*>(virtual_file.content->data()))));

    if( !virtual_file.content_type.empty() )
        js_virtuaL_file.set("contentType", emscripten::val::u8string(virtual_file.content_type.c_str()));

    return js_virtuaL_file;
}


void WasmLocalFileServer::ClearRetrievedVirtualFiles()
{
    ASSERT(m_virtualFiles.size() == MaxVirtualFilesSize);

    // first remove any retrieved files
    auto virtual_file_itr = m_virtualFiles.begin();

    while( virtual_file_itr != m_virtualFiles.end() )
    {
        if( virtual_file_itr->second->retrieved )
        {
            virtual_file_itr = m_virtualFiles.erase(virtual_file_itr);
        }

        else
        {
           ++virtual_file_itr;
        }
    }

    // if there are still no slots for the next virtual file, remove the oldest one
    if( m_virtualFiles.size() == MaxVirtualFilesSize )
        m_virtualFiles.erase(m_virtualFiles.begin());
}



// --------------------------------------------------------------------------
// VirtualFileMappingHandler::ServeContent
// --------------------------------------------------------------------------

void VirtualFileMappingResponse::SetContent(const void* const content_data, const size_t content_size, const cs::string_sz content_type)
{
    ASSERT(content_data != nullptr);
    const std::byte* const bytes = reinterpret_cast<const std::byte*>(content_data);

    SetContent(std::make_unique<const std::vector<std::byte>>(bytes, bytes + content_size),
               content_type);
}


void VirtualFileMappingResponse::SetContent(std::shared_ptr<const std::vector<std::byte>> content, const cs::string_sz content_type)
{
    WasmLocalFileServer::VirtualFile& virtual_file = *static_cast<WasmLocalFileServer::VirtualFile*>(m_responseObject);
    ASSERT(!virtual_file.retrieved);

    virtual_file.content = std::move(content);
    virtual_file.content_type = content_type.c_str();
}
