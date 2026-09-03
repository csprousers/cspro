project(zToolsO)

set(RIJNDAEL_DIR rijndael)
set(SCRYPT_DIR ${EXTERNAL_DIR}/scrypt)

add_library(${PROJECT_NAME}
    base64.cpp
    BinaryGen.cpp
    DateTime.cpp
    DebugLogging.cpp
    DelimitedTextCreator.cpp
    DirectoryLister.cpp
    Encoders.cpp
    Encryption.cpp
    ErrorMessageDisplayer.cpp
    ExceptionHolder.cpp
    File.cpp
    FileIO.cpp
    FuzzyWuzzy.cpp
    Hash.cpp
    ImsaStrMem.cpp
    IniFile.cpp
    InterfaceString.cpp
    md5.c
    MemoryStream.cpp
    NewlineSubstitutor.cpp
    NumberToString.cpp
    ObjectTransporter.cpp
    Path.cpp
    PortableFunctions.cpp
    $<$<BOOL:${WIN32}>:SafePath.cpp>
    Screen.cpp
    serializer.cpp
    SharableString.cpp
    Special.cpp
    StringOperations.cpp
    TextConverterAnsi.cpp
    TextEncoding.cpp
    TextFile.cpp
    Tools.cpp
    UniqueId.cpp
    Utf8.cpp
    Utf8Convert.cpp
    Utf8FileStream.cpp
    uuid.cpp
    VarFuncs.cpp
    $<$<BOOL:${WIN32}>:WinRegistry.cpp>

    ${RIJNDAEL_DIR}/rijndael-alg-fst.c

    ${SCRYPT_DIR}/insecure_memzero.c
    ${SCRYPT_DIR}/sha256.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIR}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h>"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # bzip2
    ${THIRD_PARTY_LIB_DIR}/libbzip2${THIRD_PARTY_LIB_EXT}
)
