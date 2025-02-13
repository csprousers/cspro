project(zDataO)

include(../WASM/Definitions.cmake)

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/jsoncons
)

add_library(${PROJECT_NAME}
    CacheableCaseWrapperRepository.cpp
    DataRepository.cpp
    DataRepositoryHelpers.cpp
    EncryptedSQLiteRepository.cpp
    EncryptedSQLiteRepositoryPasswordManager.cpp
    ExportWriterRepository.cpp
    IndexableTextRepository.cpp
    JsonRepository.cpp
    MemoryRepository.cpp
    NullRepository.cpp
    ParadataWrapperRepository.cpp
    SQLiteBinaryItemSerializer.cpp
    SQLiteBlobQuestionnaireSerializer.cpp
    SQLiteDictionarySchemaGenerator.cpp
    SQLiteDictionarySchemaReconciler.cpp
    SQLiteQuestionnaireSerializer.cpp
    SQLiteRepository.cpp
    SQLiteRepositoryCaseLoader.cpp
    SQLiteRepositoryIterators.cpp
    TextRepository.cpp
    TextRepositoryNotesFile.cpp
    TextRepositoryStatusFile.cpp
)
