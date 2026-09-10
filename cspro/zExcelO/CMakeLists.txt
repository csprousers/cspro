project(zExcelO)

add_library(${PROJECT_NAME}
    ExcelWriter.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # libxlsxwriter
    ${THIRD_PARTY_PREBUILT_LIB_DIR}/libxlsxwriter${THIRD_PARTY_LIB_EXT}

    # zlib
    ${THIRD_PARTY_PREBUILT_LIB_DIR}/libz${THIRD_PARTY_LIB_EXT}
)
