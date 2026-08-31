project(zYaml)

set(YAML_DIR ${EXTERNAL_DIR}/yaml-cpp)

add_library(${PROJECT_NAME}
    ${YAML_DIR}/src/binary.cpp
    ${YAML_DIR}/src/convert.cpp
    ${YAML_DIR}/src/depthguard.cpp
    ${YAML_DIR}/src/directives.cpp
    ${YAML_DIR}/src/emit.cpp
    ${YAML_DIR}/src/emitfromevents.cpp
    ${YAML_DIR}/src/emitter.cpp
    ${YAML_DIR}/src/emitterstate.cpp
    ${YAML_DIR}/src/emitterutils.cpp
    ${YAML_DIR}/src/exceptions.cpp
    ${YAML_DIR}/src/exp.cpp
    ${YAML_DIR}/src/memory.cpp
    ${YAML_DIR}/src/node.cpp
    ${YAML_DIR}/src/node_data.cpp
    ${YAML_DIR}/src/nodebuilder.cpp
    ${YAML_DIR}/src/nodeevents.cpp
    ${YAML_DIR}/src/null.cpp
    ${YAML_DIR}/src/ostream_wrapper.cpp
    ${YAML_DIR}/src/parse.cpp
    ${YAML_DIR}/src/parser.cpp
    ${YAML_DIR}/src/regex_yaml.cpp
    ${YAML_DIR}/src/scanner.cpp
    ${YAML_DIR}/src/scanscalar.cpp
    ${YAML_DIR}/src/scantag.cpp
    ${YAML_DIR}/src/scantoken.cpp
    ${YAML_DIR}/src/simplekey.cpp
    ${YAML_DIR}/src/singledocparser.cpp
    ${YAML_DIR}/src/stream.cpp
    ${YAML_DIR}/src/tag.cpp
)

if(CSPRO_BUILD_CONSOLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        YAML_CPP_STATIC_DEFINE
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${YAML_DIR}/include
)
