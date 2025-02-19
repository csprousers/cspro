#pragma once

#include <zToolsO/ThreadCacher.h>
#include <emscripten/val.h>
#include <emscripten/wasm_worker.h>

class PortableLocalFileServer;


class WasmController
{
public:
    WasmController(std::string base_url_for_local_file_server);
    ~WasmController();

    static WasmController& GetInstance();

    static bool OnMainThread()   { return ( emscripten_wasm_worker_self_id() == 0 ); }
    static bool OnWorkerThread() { return !OnMainThread(); }


    // --------------------------------------------------------------------------
    // message handling
    // --------------------------------------------------------------------------

    // posts a message to the main window;
    // if called from a worker thread, the message will be cached and then
    // posted by the main thread
    template<typename T>
    void PostMessageToMainWindow(T&& message);


    // --------------------------------------------------------------------------
    // virtual file handling
    // --------------------------------------------------------------------------

    PortableLocalFileServer& GetLocalFileServer() { return *m_localFileServer; }

    // indicates that the virtual file should be retrieved;
    // the thread that prepares the virtual file will post a message when the virtual file is ready
    static void PrepareVirtualFile(WasmController& controller, std::string lfs_path);

    // retrieves a virtual file previously prepared using PrepareVirtualFile
    static emscripten::val GetVirtualFile(const WasmController& controller, int lfs_index);


private:
    void DoPostMessageToMainWindow(const std::string& message);
    static void DoPostMessageToMainWindow(int message_index);

    static void DoPrepareVirtualFile(int lfs_path_index);

private:
    static WasmController* m_instance;

    ThreadCacher<std::string> m_stringThreadCacher;

    std::unique_ptr<PortableLocalFileServer> m_localFileServer; // non-null
    emscripten_wasm_worker_t m_getVirtualFileThreadId;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
void WasmController::PostMessageToMainWindow(T&& message)
{
    if( OnMainThread() )
    {
        DoPostMessageToMainWindow(message);
    }

    else
    {
        const int message_index = m_stringThreadCacher.Store(std::forward<T>(message));
        emscripten_wasm_worker_post_function_vi(0, DoPostMessageToMainWindow, message_index);
    }
}
