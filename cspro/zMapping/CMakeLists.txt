project(zMapping)

add_library(${PROJECT_NAME}
    CoordinateConverter.cpp
    GeoJson.cpp
    HtmlMapUI.cpp
    MBTilesReader.cpp
    OfflineTileProvider.cpp
    TPKReader.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIR}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
