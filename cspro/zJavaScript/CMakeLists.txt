project(zJavaScript)

set(QUICKJS_DIR ${THIRD_PARTY_SOURCES_DIR}/quickjs)

add_library(${PROJECT_NAME}
    ActionInvokerJS.cpp
    Executor.cpp
    QuickJSAccess.cpp
    Value.cpp
    VariablePropertyNameEvaluator.cpp

    ${QUICKJS_DIR}/cutils.c
    ${QUICKJS_DIR}/dtoa.c
    ${QUICKJS_DIR}/libregexp.c
    ${QUICKJS_DIR}/libunicode.c
    ${QUICKJS_DIR}/quickjs.c
)

# QuickJS requires compiling code with features from latest C working draft
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:clatest)
endif()

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)
