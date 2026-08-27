project(zJavaScript)

set(QUICKJS_DIRECTORY ${EXTERNAL_DIRECTORY}/QuickJS)

add_library(${PROJECT_NAME}
    ActionInvokerJS.cpp
    Executor.cpp
    QuickJSAccess.cpp
    Value.cpp
    VariablePropertyNameEvaluator.cpp

    ${QUICKJS_DIRECTORY}/cutils.c
    ${QUICKJS_DIRECTORY}/dtoa.c
    ${QUICKJS_DIRECTORY}/libregexp.c
    ${QUICKJS_DIRECTORY}/libunicode.c
    ${QUICKJS_DIRECTORY}/quickjs.c
)

# QuickJS requires compiling code with features from latest C working draft
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:clatest)
endif()

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)
