project(zJavaScript)

include(../WASM/Definitions.cmake)

set(QUICKJS_DIRECTORY ${EXTERNAL_DIRECTORY}/QuickJS)

add_library(${PROJECT_NAME}
    ActionInvokerJS.cpp
    Executor.cpp
    QuickJSAccess.cpp
    Value.cpp

    ${QUICKJS_DIRECTORY}/cutils.c
    ${QUICKJS_DIRECTORY}/libbf.c
    ${QUICKJS_DIRECTORY}/libregexp.c
    ${QUICKJS_DIRECTORY}/libunicode.c
    ${QUICKJS_DIRECTORY}/quickjs.c
)
