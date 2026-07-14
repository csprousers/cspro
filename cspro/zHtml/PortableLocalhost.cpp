#include "stdafx.h"
#include "PortableLocalhost.h"
#include "PortableLocalFileServer.h"

#if defined(WIN_DESKTOP)
    #include "SharedHtmlLocalFileServer.h"
    #include <zUtilF/ApplicationShutdownRunner.h>
#elif defined(ANDROID)
    #include <CSEntryDroid/app/src/main/jni/AndroidLocalFileServer.h>
#elif defined(WASM)
    #include <WASM/WasmController.h>
#endif


// --------------------------------------------------------------------------
// PortableLocalhost::GetPortableLocalFileServer implementations
// --------------------------------------------------------------------------

#if defined(WIN_DESKTOP) && !defined(USE_PORTABLE_LOCAL_FILE_SERVER)

auto& PortableLocalhost::GetPortableLocalFileServer()
{
    ASSERT(ApplicationShutdownRunner::Get() != nullptr);
    static SharedHtmlLocalFileServer local_file_server;
    return local_file_server;
}


#elif defined(WIN_DESKTOP)

namespace
{
    // WindowsPortableLocalFileServer simulates the PortableLocalhost handling in the Android and WASM environments
    class WindowsPortableLocalFileServer : public KeyBasedVirtualFileMappingHandler
    {
    public:
        WindowsPortableLocalFileServer()
        {
            ASSERT(ApplicationShutdownRunner::Get() != nullptr);
            SharedHtmlLocalFileServer local_file_server;

            local_file_server.CreateVirtualDirectory(*this);

            m_portableLocalFileServer = std::make_unique<PortableLocalFileServer>(m_virtualFileMapping->GetUrl());
        }

        PortableLocalFileServer& GetPortableLocalFileServer()
        {
            return *m_portableLocalFileServer;
        }

        bool ServeContent(VirtualFileMappingResponse& response, const std::string& key) override
        {
            return m_portableLocalFileServer->GetVirtualFile(key, response);
        }

    private:
        std::unique_ptr<PortableLocalFileServer> m_portableLocalFileServer;
    };
}


auto& PortableLocalhost::GetPortableLocalFileServer()
{
    static WindowsPortableLocalFileServer windows_portable_local_file_server;
    return windows_portable_local_file_server.GetPortableLocalFileServer();
}


#elif defined(ANDROID)

auto& PortableLocalhost::GetPortableLocalFileServer()
{
    return AndroidLocalFileServer::GetInstance();
}


#elif defined(WASM)

auto& PortableLocalhost::GetPortableLocalFileServer()
{
    return WasmController::GetInstance().GetLocalFileServer();
}


#elif defined(_CONSOLE)

auto& PortableLocalhost::GetPortableLocalFileServer()
{
    static PortableLocalFileServer local_file_server("console");
    return local_file_server;
}

#endif



// --------------------------------------------------------------------------
// PortableLocalhost
// --------------------------------------------------------------------------

void PortableLocalhost::CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, const cs::string_sz filename/* = ""*/)
{
    GetPortableLocalFileServer().CreateVirtualFile(virtual_file_mapping_handler, filename);
}


VirtualFileMapping PortableLocalhost::CreateVirtualHtmlFile(const std::string& directory, std::function<SharableString()> callback)
{
    return GetPortableLocalFileServer().CreateVirtualHtmlFile(directory, std::move(callback));
}


void PortableLocalhost::CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    GetPortableLocalFileServer().CreateVirtualDirectory(key_based_virtual_file_mapping_handler);
}


std::string PortableLocalhost::CreateFileUrl(const std::string& file_path)
{
    return GetPortableLocalFileServer().CreateFileUrl(file_path);
}


std::string PortableLocalhost::CreateUniqueFileUrl(const std::string& file_path)
{
    return GetPortableLocalFileServer().CreateUniqueFileUrl(file_path);
}



// --------------------------------------------------------------------------
// VirtualFileMappingHandler::ServeContent implementation (Console)
// --------------------------------------------------------------------------

#if defined(_CONSOLE)

void VirtualFileMappingResponse::SetContent(const void* const /*content_data*/, const size_t /*content_size*/, const std::string& /*content_type*/)
{
    ASSERT(false);
}

#endif
