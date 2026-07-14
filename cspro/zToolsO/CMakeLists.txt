project(zToolsO)

set(BZIP2_DIRECTORY ${EXTERNAL_DIRECTORY}/bzip2)
set(MD4C_DIRECTORY ${EXTERNAL_DIRECTORY}/md4c)
set(RIJNDAEL_DIRECTORY rijndael)
set(SCRYPT_DIRECTORY ${EXTERNAL_DIRECTORY}/scrypt)

include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIRECTORY}
)

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
    Screen.cpp
    serializer.cpp
    SharableString.cpp
    Special.cpp
    StringOperations.cpp
    TextConverterAnsi.cpp
    TextEncoding.cpp
    Tools.cpp
    UniqueId.cpp
    Utf8.cpp
    Utf8Convert.cpp
    Utf8FileStream.cpp
    uuid.cpp
    VarFuncs.cpp

    ${BZIP2_DIRECTORY}/blocksort.c
    ${BZIP2_DIRECTORY}/bzlib.c
    ${BZIP2_DIRECTORY}/compress.c
    ${BZIP2_DIRECTORY}/crctable.c
    ${BZIP2_DIRECTORY}/decompress.c
    ${BZIP2_DIRECTORY}/huffman.c
    ${BZIP2_DIRECTORY}/randtable.c

    ${MD4C_DIRECTORY}/entity.c

    ${RIJNDAEL_DIRECTORY}/rijndael-alg-fst.c

    ${SCRYPT_DIRECTORY}/insecure_memzero.c
    ${SCRYPT_DIRECTORY}/sha256.c
)
