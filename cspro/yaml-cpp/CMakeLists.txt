project(yaml-cpp)

include(../WASM/Definitions.cmake)

set(YAML_DIRECTORY ${EXTERNAL_DIRECTORY}/yaml-cpp)

include_directories(${PROJECT_NAME} PUBLIC
    ${YAML_DIRECTORY}/include
)

add_library(${PROJECT_NAME}
    ${YAML_DIRECTORY}/src/binary.cpp
    ${YAML_DIRECTORY}/src/convert.cpp
    ${YAML_DIRECTORY}/src/depthguard.cpp
    ${YAML_DIRECTORY}/src/directives.cpp
    ${YAML_DIRECTORY}/src/emit.cpp
    ${YAML_DIRECTORY}/src/emitfromevents.cpp
    ${YAML_DIRECTORY}/src/emitter.cpp
    ${YAML_DIRECTORY}/src/emitterstate.cpp
    ${YAML_DIRECTORY}/src/emitterutils.cpp
    ${YAML_DIRECTORY}/src/exceptions.cpp
    ${YAML_DIRECTORY}/src/exp.cpp
    ${YAML_DIRECTORY}/src/memory.cpp
    ${YAML_DIRECTORY}/src/node.cpp
    ${YAML_DIRECTORY}/src/node_data.cpp
    ${YAML_DIRECTORY}/src/nodebuilder.cpp
    ${YAML_DIRECTORY}/src/nodeevents.cpp
    ${YAML_DIRECTORY}/src/null.cpp
    ${YAML_DIRECTORY}/src/ostream_wrapper.cpp
    ${YAML_DIRECTORY}/src/parse.cpp
    ${YAML_DIRECTORY}/src/parser.cpp
    ${YAML_DIRECTORY}/src/regex_yaml.cpp
    ${YAML_DIRECTORY}/src/scanner.cpp
    ${YAML_DIRECTORY}/src/scanscalar.cpp
    ${YAML_DIRECTORY}/src/scantag.cpp
    ${YAML_DIRECTORY}/src/scantoken.cpp
    ${YAML_DIRECTORY}/src/simplekey.cpp
    ${YAML_DIRECTORY}/src/singledocparser.cpp
    ${YAML_DIRECTORY}/src/stream.cpp
    ${YAML_DIRECTORY}/src/tag.cpp
)
