#pragma once

#include <zLogicO/zLogicO.h>
#include <zJson/JsonSerializer.h>


// --------------------------------------------------------------------------
// SymbolType
// --------------------------------------------------------------------------

enum class SymbolType : int
{
    None                       = 255,

    Pre80Dictionary            =   0,
    Pre80Flow                  =   7,
    Section                    =  12,
    // (removed) DictGroup     =  13, // dictionary group, no longer used
    Variable                   =  14,
    WorkVariable               =  16,
    Form                       =  17,
    Application                =  21,
    UserFunction               =  22,
    Array                      =  23, // prior to CSPro 7.2, this number was for Crosstab
    Index_Unused               =  25, // secondary index, no longer used
    // (removed) Record_Unused =  26,
    Group                      =  27, // (flow group)
    // (removed) ValueLabel    =  28,
    // (removed) OldValueSet   =  29,
    ValueSet                   =  30,
    Relation                   =  31,
    File                       =  32,
    List                       =  33,
    Block                      =  34,
    Crosstab                   =  35,
    Map                        =  36,
    Pff                        =  37,
    SystemApp                  =  38,
    Audio                      =  39,
    HashMap                    =  40,
    NamedFrequency             =  41,
    WorkString                 =  42,
    Dictionary                 =  43,
    Record                     =  44,
    // Item                    =  45, // ENGINECR_TODO
    Image                      =  46,
    Document                   =  47,
    Geometry                   =  48,
    Flow                       =  49,
    Report                     =  50,
    Item                       =  51,
    Unknown
};

ZLOGICO_API const TCHAR* ToString(SymbolType symbol_type);

template<> struct ZLOGICO_API JsonSerializer<SymbolType>
{
    static void WriteJson(JsonWriter& json_writer, SymbolType value);
};


// --------------------------------------------------------------------------
// SymbolSubType
// --------------------------------------------------------------------------

enum class SymbolSubType
{
    NoType              = 255,

    Input               =   0,
    Output              =   1,
    Work                =   2,
    External            =   3,

    Primary             =   4,
    Secondary           =   5,

    DynamicValueSet     =  31,
    ValueSetListWrapper =  32,

    WorkAlpha           =  51,
};

ZLOGICO_API const TCHAR* ToString(SymbolSubType symbol_subtype);

template<> struct ZLOGICO_API JsonSerializer<SymbolSubType>
{
    static void WriteJson(JsonWriter& json_writer, SymbolSubType value);
};
