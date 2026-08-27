project(zDataO)

add_library(${PROJECT_NAME}
    CacheableCaseWrapperRepository.cpp
    CaseIteratorSettings.cpp
    CSWebBinaryContentReader.cpp
    CSWebCaseResponse.cpp
    CSWebRepository.cpp
    CSWebRepositoryCache.cpp
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
    SQLiteSyncStatusEvaluator.cpp
    SyncBinaryDataUploadManager.cpp
    SyncCaseJsonSerializer.cpp
    SyncCaseSerializer.cpp
    TextRepository.cpp
    TextRepositoryNotesFile.cpp
    TextRepositoryStatusFile.cpp
    WrapperRepository.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIRECTORY}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
