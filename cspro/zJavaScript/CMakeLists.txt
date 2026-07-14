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
