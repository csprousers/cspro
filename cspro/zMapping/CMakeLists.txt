project(zMapping)

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/geometry.hpp/include
    ${EXTERNAL_DIRECTORY}/variant/include
)

add_library(${PROJECT_NAME}
    CoordinateConverter.cpp
    GeoJson.cpp
    HtmlMapUI.cpp
    MBTilesReader.cpp
    OfflineTileProvider.cpp
    TPKReader.cpp
)
