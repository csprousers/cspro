project(zSql)

add_compile_definitions(SQLITE_TEMP_STORE=3) # use memory for temp files

set(SQLITE_DIRECTORY ${EXTERNAL_DIRECTORY}/SQLite)

include_directories(${PROJECT_NAME} PUBLIC
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
