#include "stdafx.h"
#include "SymbolType.h"


// --------------------------------------------------------------------------
// SymbolType
// --------------------------------------------------------------------------

namespace
{
    struct SymbolString
    {
        const char* for_to_string;
        const char* for_json;

        SymbolString(const char* const text_for_to_string, const char* const text_for_json = nullptr)
            :   for_to_string(text_for_to_string),
                for_json(( text_for_json != nullptr ) ? text_for_json : text_for_to_string)
        {
            ASSERT(for_to_string != nullptr && for_json != nullptr);
        }
    };


    const std::map<SymbolType, SymbolString>& GetSymbolStrings()
    {
        static const std::map<SymbolType, SymbolString> symbol_strings =
        {
            { SymbolType::Pre80Dictionary,  { "Dictionary"                  } },
            { SymbolType::Pre80Flow,        { "Flow"                        } },
            { SymbolType::Section,          { "Section"                     } },
            { SymbolType::Variable,         { "Variable"                    } },
            { SymbolType::WorkVariable,     { "WorkVariable",   "numeric"   } },
            { SymbolType::Form,             { "Form"                        } },
            { SymbolType::Application,      { "Application"                 } },
            { SymbolType::UserFunction,     { "UserFunction",   "function"  } },
            { SymbolType::Array,            { "Array"                       } },
            { SymbolType::Group,            { "Group"                       } },
            { SymbolType::ValueSet,         { "ValueSet"                    } },
            { SymbolType::Relation,         { "Relation"                    } },
            { SymbolType::File,             { "File"                        } },
            { SymbolType::List,             { "List"                        } },
            { SymbolType::Block,            { "Block"                       } },
            { SymbolType::Crosstab,         { "Crosstab"                    } },
            { SymbolType::Map,              { "Map"                         } },
            { SymbolType::Pff,              { "Pff"                         } },
            { SymbolType::SystemApp,        { "SystemApp"                   } },
            { SymbolType::Audio,            { "Audio"                       } },
            { SymbolType::HashMap,          { "HashMap"                     } },
            { SymbolType::NamedFrequency,   { "Freq"                        } },
            { SymbolType::WorkString,       { "String",         "string"    } },
            { SymbolType::Dictionary,       { "Dictionary"                  } },
            { SymbolType::Record,           { "Record"                      } },
            { SymbolType::Image,            { "Image"                       } },
            { SymbolType::Document,         { "Document"                    } },
            { SymbolType::Geometry,         { "Geometry"                    } },
            { SymbolType::Flow,             { "Flow"                        } },
            { SymbolType::Report,           { "Report"                      } },
            { SymbolType::Item,             { "Item"                        } },
            { SymbolType::StringWriter,     { "StringWriter"                } },
            { SymbolType::None,             { "None"                        } },
        };

        return symbol_strings;
    };
}


const char* ToString(const SymbolType symbol_type, const char* SymbolString::* const string_value)
{
    const std::map<SymbolType, SymbolString>& symbol_strings = GetSymbolStrings();
    const auto& lookup = symbol_strings.find(symbol_type);

    return ( lookup != symbol_strings.cend() ) ? (lookup->second).*string_value :
                                                 "Unknown";
}


const char* ToString(const SymbolType symbol_type)
{
    return ToString(symbol_type, &SymbolString::for_to_string);
}


const char* ToDisplayString(const SymbolType symbol_type)
{
    return ToString(symbol_type, &SymbolString::for_json);
}


void JsonSerializer<SymbolType>::WriteJson(JsonWriter& json_writer, const SymbolType value)
{
    const std::map<SymbolType, SymbolString>& symbol_strings = GetSymbolStrings();
    const auto& lookup = symbol_strings.find(value);

    if( lookup != symbol_strings.cend() )
    {
        json_writer.Write(lookup->second.for_json);
    }

    else
    {
        ASSERT(false);
        json_writer.WriteNull();
    }
}



// --------------------------------------------------------------------------
// SymbolSubType
// --------------------------------------------------------------------------

const char* ToString(const SymbolSubType symbol_subtype)
{
    return ( symbol_subtype == SymbolSubType::NoType )              ? "NoType"              :
           ( symbol_subtype == SymbolSubType::Input )               ? "Input"               :
           ( symbol_subtype == SymbolSubType::Output )              ? "Output"              :
           ( symbol_subtype == SymbolSubType::Work )                ? "Work"                :
           ( symbol_subtype == SymbolSubType::External )            ? "External"            :
           ( symbol_subtype == SymbolSubType::Primary )             ? "Primary"             :
           ( symbol_subtype == SymbolSubType::Secondary )           ? "Secondary"           :
           ( symbol_subtype == SymbolSubType::DynamicValueSet )     ? "DynamicValueSet"     :
           ( symbol_subtype == SymbolSubType::ValueSetListWrapper ) ? "ValueSetListWrapper" :
           ( symbol_subtype == SymbolSubType::WorkAlpha )           ? "WorkAlpha"           :
                                                                      "Unknown";
}


void JsonSerializer<SymbolSubType>::WriteJson(JsonWriter& json_writer, const SymbolSubType value)
{
    if( value == SymbolSubType::WorkAlpha )
    {
        json_writer.Write("alpha");
    }

    else
    {
        json_writer.Write(SO::TitleToCamelCase(ToString(value)));
    }
}
