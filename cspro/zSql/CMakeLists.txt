project(zSql)

set(SQLITE_DIRECTORY ${EXTERNAL_DIRECTORY}/SQLite)

include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIRECTORY}
)

add_library(${PROJECT_NAME}
    DB.cpp
    Exception.cpp
    SQLiteHelpers.cpp
    Statement.cpp
    Transaction.cpp

    ${SQLITE_DIRECTORY}/sqlite3.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    # use memory for temporary files
    SQLITE_TEMP_STORE=3
)
