#pragma once

#include <zJson/JsonKeys.h>


namespace CSProperty // connection string property
{
    constexpr const char* binaryDataDirectory   = "binaryDataDirectory";    // JsonRepository
    constexpr const char* binaryDataFormat      = JK::binaryDataFormat;     // JsonRepository
    constexpr const char* cache                 = "cache";                  // CacheableCaseWrapperRepository
    constexpr const char* cacheLocally          = "cacheLocally";           // CSWebRepository
    constexpr const char* decimalMark           = JK::decimalMark;          // export writers: CSV, semicolon, tab
    constexpr const char* dictionaryName        = "dictionaryName";         // CSWebRepository
    constexpr const char* dictionaryPath        = "dictionaryPath";         // export writers: CSPro; Data Manager (via cspro:// URIs)
    constexpr const char* encoding              = JK::encoding;             // TextRepository; export writers: CSV, semicolon, tab; SAS (syntax)
    constexpr const char* factorRanges          = "factorRanges";           // export writers: R
    constexpr const char* header                = "header";                 // export writers: CSV, semicolon, tab; Excel
    constexpr const char* jsonFormat            = JK::jsonFormat;           // JsonRepository
    constexpr const char* key                   = JK::key;                  // Data Manager (via cspro:// URIs)
    constexpr const char* mappedSpecialValues   = "mappedSpecialValues";    // export writers: all but CSPro
    constexpr const char* newline               = "newline";                // TextRepository; export writers: CSV, semicolon, tab; SAS (syntax)
    constexpr const char* password              = JK::password;             // CSWebRepository, EncryptedSQLiteRepository
    constexpr const char* record                = JK::record;               // export writers: all
    constexpr const char* username              = JK::username;             // CSWebRepository
    constexpr const char* uuid                  = JK::uuid;                 // Data Manager (via cspro:// URIs)
    constexpr const char* syntaxPath            = "syntaxPath";             // export writers: SAS
    constexpr const char* verbose               = "verbose";                // JsonRepository
    constexpr const char* writeBlankValues      = JK::writeBlankValues;     // JsonRepository
    constexpr const char* writeCodes            = "writeCodes";             // export writers: CSV, semicolon, tab; Excel; R
    constexpr const char* writeFactors          = "writeFactors";           // export writers: R
    constexpr const char* writeLabels           = JK::writeLabels;          // JsonRepository; export writers: CSV, semicolon, tab; Excel
}


namespace CSValue // connection string value
{
    constexpr const char* true_                 = "true";
    constexpr const char* false_                = "false";

    constexpr const char* ANSI                  = "ANSI";
    constexpr const char* codes                 = "codes";
    constexpr const char* comma                 = "comma";
    constexpr const char* compact               = "compact";
    constexpr const char* CRLF                  = "CRLF";
    constexpr const char* dataUrl               = "dataUrl";
    constexpr const char* default_              = "default";
    constexpr const char* disk                  = "disk";
    constexpr const char* embed                 = "embed";
    constexpr const char* native                = "native";
    constexpr const char* names                 = "names";
    constexpr const char* labels                = JK::labels;
    constexpr const char* LF                    = "LF";
    constexpr const char* period                = "period";
    constexpr const char* pretty                = "pretty";
    constexpr const char* suppress              = "suppress";
    constexpr const char* UTF_8                 = "UTF-8";
    constexpr const char* UTF_8_BOM             = "UTF-8-BOM";
}
