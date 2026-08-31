project(zSql)

set(SQLITE_DIR ${EXTERNAL_DIR}/SQLite)

add_library(${PROJECT_NAME}
    DB.cpp
    Exception.cpp
    SQLiteHelpers.cpp
    Statement.cpp
    Transaction.cpp

    ${SQLITE_DIR}/sqlite3.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    # use memory for temporary files
    SQLITE_TEMP_STORE=3
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIR}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)
