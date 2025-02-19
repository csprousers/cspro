#include "StdAfx.h"
#include "WasmController.h"
#include "WasmLocalFileServer.h"


constexpr size_t WasmThreadStackSize = 10 * 1024 * 1024;


WasmController* WasmController::m_instance = nullptr;


WasmController::WasmController(std::string base_url_for_local_file_server)
    :   m_getVirtualFileThreadId(0)
{
    if( m_instance != nullptr )
        throw CSProException("Only one instance of the WasmController can be used at a time.");

    m_localFileServer = std::make_unique<WasmLocalFileServer>(std::move(base_url_for_local_file_server));

    m_instance = this;
}


WasmController::~WasmController()
{
    ASSERT(m_instance == this);
    m_instance = nullptr;
}


WasmController& WasmController::GetInstance()
{
    if( m_instance == nullptr )
        throw CSProException("You must create an instance of the WasmController.");

    return *m_instance;
}


void WasmController::DoPostMessageToMainWindow(const std::string& message)
{
    ASSERT(OnMainThread());

    EM_ASM({
        postMessage(UTF8ToString($0));
    }, message.c_str());
}


void WasmController::DoPostMessageToMainWindow(const int message_index)
{
    ASSERT(OnMainThread());

    WasmController& controller = GetInstance();

    controller.DoPostMessageToMainWindow(controller.m_stringThreadCacher.Retrieve(message_index));
}


void WasmController::PrepareVirtualFile(WasmController& controller, std::string lfs_path)
{
    ASSERT(OnMainThread());

    // create a thread for getting virtual files
    if( controller.m_getVirtualFileThreadId == 0 )
        controller.m_getVirtualFileThreadId = emscripten_malloc_wasm_worker(WasmThreadStackSize);

    const int lfs_path_index = controller.m_stringThreadCacher.Store(std::move(lfs_path));

    emscripten_wasm_worker_post_function_vi(controller.m_getVirtualFileThreadId, DoPrepareVirtualFile, lfs_path_index);
}


void WasmController::DoPrepareVirtualFile(const int lfs_path_index)
{
    ASSERT(OnWorkerThread());

    WasmController& controller = GetInstance();

    const std::string lfs_path = controller.m_stringThreadCacher.Retrieve(lfs_path_index);

    const int lfs_index = static_cast<WasmLocalFileServer&>(*controller.m_localFileServer).RetrieveVirtualFile(lfs_path);

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::action, "CSWasm.virtualFilePrepared")
                .Write("lfsPath", lfs_path)
                .Write("lfsIndex", lfs_index)
                .EndObject();

    controller.PostMessageToMainWindow(json_writer->GetString());
}


emscripten::val WasmController::GetVirtualFile(const WasmController& controller, const int lfs_index)
{
    ASSERT(OnMainThread());

    return static_cast<WasmLocalFileServer&>(*controller.m_localFileServer).GetVirtualFile(lfs_index);
}
