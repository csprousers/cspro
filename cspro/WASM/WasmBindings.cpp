#include "StdAfx.h"
#include "WasmController.h"
#include <emscripten/bind.h>

using namespace emscripten;


EMSCRIPTEN_BINDINGS(WasmBindings)
{
    // WasmController
    class_<WasmController>("CSWasm")
        .constructor<std::string>()
        .class_function("PrepareVirtualFile", &WasmController::PrepareVirtualFile)
        .class_function("GetVirtualFile", &WasmController::GetVirtualFile)
        ;
}
