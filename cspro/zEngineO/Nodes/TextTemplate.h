#pragma once

#include <zLogicO/FunctionTable.h>

namespace Nodes { struct TextTemplate; }


enum class EncodeType : int { Default, Html, Csv, PercentEncoding, Uri, UriComponent, Slashes, JsonString, Markdown };

constexpr const char* EncodeTypeStrings[] = { "HTML", "CSV", "PercentEncoding", "URI", "URIComponent", "Slashes", "JsonString", "Markdown" };


struct Nodes::TextTemplate
{
    enum class Type : int { DirectText = 1, TextFill, Write };

    FunctionCode function_code;
    int symbol_index;
    Type type;
    int expression;
    int encode_text; // 0 = false, 1 = true
};
