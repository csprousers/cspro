#pragma once

#include <zJson/JsonKeys.h>


namespace CSProperty // connection string property
{
    constexpr const TCHAR* binaryDataDirectory  = _T("binaryDataDirectory");    // JsonRepository
    constexpr const TCHAR* binaryDataFormat     = JK::binaryDataFormat;         // JsonRepository
    constexpr const TCHAR* cache                = _T("cache");                  // CacheableCaseWrapperRepository
    constexpr const TCHAR* decimalMark          = JK::decimalMark;              // export writers: CSV, semicolon, tab
    constexpr const TCHAR* dictionaryPath       = _T("dictionaryPath");         // export writers: CSPro
    constexpr const TCHAR* encoding             = JK::encoding;                 // export writers: CSV, semicolon, tab; SAS (syntax)
    constexpr const TCHAR* factorRanges         = _T("factorRanges");           // export writers: R
    constexpr const TCHAR* header               = _T("header");                 // export writers: CSV, semicolon, tab; Excel
    constexpr const TCHAR* jsonFormat           = JK::jsonFormat;               // JsonRepository
    constexpr const TCHAR* mappedSpecialValues  = _T("mappedSpecialValues");    // export writers: all but CSPro
    constexpr const TCHAR* password             = _T("password");               // EncryptedSQLiteRepository
    constexpr const TCHAR* record               = JK::record;                   // export writers: all
    constexpr const TCHAR* syntaxPath           = _T("syntaxPath");             // export writers: SAS
    constexpr const TCHAR* verbose              = _T("verbose");                // JsonRepository
    constexpr const TCHAR* writeBlankValues     = JK::writeBlankValues;         // JsonRepository
    constexpr const TCHAR* writeCodes           = _T("writeCodes");             // export writers: CSV, semicolon, tab; Excel; R
    constexpr const TCHAR* writeFactors         = _T("writeFactors");           // export writers: R
    constexpr const TCHAR* writeLabels          = JK::writeLabels;              // JsonRepository; export writers: CSV, semicolon, tab; Excel
}


namespace CSValue // connection string value
{
    constexpr const TCHAR* true_                = _T("true");
    constexpr const TCHAR* false_               = _T("false");

    constexpr const TCHAR* ANSI                 = _T("ANSI");
    constexpr const TCHAR* codes                = _T("codes");
    constexpr const TCHAR* comma                = _T("comma");
    constexpr const TCHAR* compact              = _T("compact");
    constexpr const TCHAR* dataUrl              = _T("dataUrl");
    constexpr const TCHAR* default_             = _T("default");
    constexpr const TCHAR* disk                 = _T("disk");
    constexpr const TCHAR* embed                = _T("embed");
    constexpr const TCHAR* native               = _T("native");
    constexpr const TCHAR* names                = _T("names");
    constexpr const TCHAR* labels               = JK::labels;
    constexpr const TCHAR* period               = _T("period");
    constexpr const TCHAR* pretty               = _T("pretty");
    constexpr const TCHAR* suppress             = _T("suppress");
    constexpr const TCHAR* UTF_8                = _T("UTF-8");
    constexpr const TCHAR* UTF_8_BOM            = _T("UTF-8-BOM");
}
