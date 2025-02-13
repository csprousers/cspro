#include "stdafx.h"
#include "SymbolType.h"


// --------------------------------------------------------------------------
// SymbolType
// --------------------------------------------------------------------------

namespace
{
    struct SymbolString
    {
        const TCHAR* const for_to_string;
        const TCHAR* const for_json;

        SymbolString(const TCHAR* text_for_to_string, const TCHAR* text_for_json = nullptr)
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
            { SymbolType::Pre80Dictionary, { _T("Dictionary")                     } },
            { SymbolType::Pre80Flow,       { _T("Flow")                           } },
            { SymbolType::Section,         { _T("Section")                        } },
            { SymbolType::Variable,        { _T("Variable")                       } },
            { SymbolType::WorkVariable,    { _T("WorkVariable"),  _T("numeric")   } },
            { SymbolType::Form,            { _T("Form")                           } },
            { SymbolType::Application,     { _T("Application")                    } },
            { SymbolType::UserFunction,    { _T("UserFunction"),  _T("function")  } },
            { SymbolType::Array,           { _T("Array")                          } },
            { SymbolType::Group,           { _T("Group")                          } },
            { SymbolType::ValueSet,        { _T("ValueSet")                       } },
            { SymbolType::Relation,        { _T("Relation")                       } },
            { SymbolType::File,            { _T("File")                           } },
            { SymbolType::List,            { _T("List")                           } },
            { SymbolType::Block,           { _T("Block")                          } },
            { SymbolType::Crosstab,        { _T("Crosstab")                       } },
            { SymbolType::Map,             { _T("Map")                            } },
            { SymbolType::Pff,             { _T("Pff")                            } },
            { SymbolType::SystemApp,       { _T("SystemApp")                      } },
            { SymbolType::Audio,           { _T("Audio")                          } },
            { SymbolType::HashMap,         { _T("HashMap")                        } },
            { SymbolType::NamedFrequency,  { _T("Freq")                           } },
            { SymbolType::WorkString,      { _T("String"),        _T("string")    } },
            { SymbolType::Dictionary,      { _T("Dictionary")                     } },
            { SymbolType::Record,          { _T("Record")                         } },
            { SymbolType::Image,           { _T("Image")                          } },
            { SymbolType::Document,        { _T("Document")                       } },
            { SymbolType::Geometry,        { _T("Geometry")                       } },
            { SymbolType::Flow,            { _T("Flow")                           } },
            { SymbolType::Report,          { _T("Report")                         } },
            { SymbolType::Item,            { _T("Item")                           } },
            { SymbolType::None,            { _T("None")                           } },
        };

        return symbol_strings;
    };
}


const TCHAR* ToString(SymbolType symbol_type)
{
    const std::map<SymbolType, SymbolString>& symbol_strings = GetSymbolStrings();
    const auto& lookup = symbol_strings.find(symbol_type);

    return ( lookup != symbol_strings.cend() ) ? lookup->second.for_to_string :
                                                 _T("Unknown");
}


void JsonSerializer<SymbolType>::WriteJson(JsonWriter& json_writer, SymbolType value)
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

const TCHAR* ToString(SymbolSubType symbol_subtype)
{
    return ( symbol_subtype == SymbolSubType::NoType )              ? _T("NoType")              :
           ( symbol_subtype == SymbolSubType::Input )               ? _T("Input")               :
           ( symbol_subtype == SymbolSubType::Output )              ? _T("Output")              :
           ( symbol_subtype == SymbolSubType::Work )                ? _T("Work")                :
           ( symbol_subtype == SymbolSubType::External )            ? _T("External")            :
           ( symbol_subtype == SymbolSubType::Primary )             ? _T("Primary")             :
           ( symbol_subtype == SymbolSubType::Secondary )           ? _T("Secondary")           :
           ( symbol_subtype == SymbolSubType::DynamicValueSet )     ? _T("DynamicValueSet")     :
           ( symbol_subtype == SymbolSubType::ValueSetListWrapper ) ? _T("ValueSetListWrapper") :
           ( symbol_subtype == SymbolSubType::WorkAlpha )           ? _T("WorkAlpha")           :
                                                                      _T("Unknown");
}


void JsonSerializer<SymbolSubType>::WriteJson(JsonWriter& json_writer, SymbolSubType value)
{
    json_writer.Write(( value == SymbolSubType::WorkAlpha ) ? _T("alpha") :
                                                              SO::TitleToCamelCase(ToString(value)).c_str());
}
