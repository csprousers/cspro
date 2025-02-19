project(zSql)

include(../WASM/Definitions.cmake)

add_compile_definitions(SQLITE_TEMP_STORE=3) # use memory for temp files

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
)

add_library(${PROJECT_NAME}
    DB.cpp
    Exception.cpp
    SQLiteHelpers.cpp
    Statement.cpp
    Transaction.cpp

    ${EXTERNAL_DIRECTORY}/SQLite/sqlite3.c
)
