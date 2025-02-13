project(zToolsO)

include(../WASM/Definitions.cmake)

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
)

add_library(${PROJECT_NAME} 
    rijndael/rijndael-alg-fst.c

    scrypt/insecure_memzero.c
    scrypt/sha256.c

    base64.cpp
    BinaryGen.cpp
    bzlib.c
    CSProException.cpp
    DateTime.cpp
    DelimitedTextCreator.cpp
    DirectoryLister.cpp
    Encoders.cpp
    Encryption.cpp
    ErrorMessageDisplayer.cpp
    FileIO.cpp
    FuzzyWuzzy.cpp
    Hash.cpp
    ImsaStrMem.cpp
    md5.c
    Memctrl.cpp
    NumberToString.cpp
    ObjectTransporter.cpp
    PortableFunctions.cpp
    Screen.cpp
    serializer.cpp
    Special.cpp
    StringOperations.cpp
    TextConverterAnsi.cpp
    Tools.cpp
    Utf8Convert.cpp
    Utf8FileStream.cpp
    uuid.cpp
    VarFuncs.cpp
)
