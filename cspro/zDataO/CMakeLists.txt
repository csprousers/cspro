project(zDataO)

include(../WASM/Definitions.cmake)

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
)

add_library(${PROJECT_NAME}
    CacheableCaseWrapperRepository.cpp
    CaseIteratorSettings.cpp
    CSWebBinaryContentReader.cpp
    CSWebRepository.cpp
    DataRepository.cpp
    DataRepositoryDefines.cpp
    DataRepositoryHelpers.cpp
    DictionarySource.cpp
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
    SyncBinaryDataUploadManager.cpp
    SyncCaseJsonSerializer.cpp
    SyncCaseSerializer.cpp
    TextRepository.cpp
    TextRepositoryNotesFile.cpp
    TextRepositoryStatusFile.cpp
    WrapperRepository.cpp
)
