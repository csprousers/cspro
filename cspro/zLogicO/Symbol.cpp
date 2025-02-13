#include "stdafx.h"
#include "Symbol.h"
#include "KeywordTable.h"
#include <zToolsO/Serializer.h>


std::unique_ptr<Symbol> Symbol::CloneInInitialState() const
{
    // if a symbol supports cloning, it should override this method
    return ReturnProgrammingError(nullptr);
}


void Symbol::Reset()
{
    // if a symbol needs resetting, it should override this method
    ASSERT(false);
}


void Symbol::serialize(Serializer& ar)
{
    // the loading of symbols is handled in CEngineDriver::LoadBaseSymbols
    ASSERT(ar.IsSaving());
    ar << m_name;
    ar.SerializeEnum(m_type)
      .SerializeEnum(m_subtype);
    ar << m_symbolIndex;
}


void Symbol::serialize_subclass(Serializer& /*ar*/)
{
    // if a symbol has additional properties to serialize, it should override this method
}


void Symbol::WriteJson(JsonWriter& json_writer, SymbolJsonOutput symbol_json_output/* = SymbolJsonOutput::Metadata*/) const
{
    if( symbol_json_output == SymbolJsonOutput::Value )
    {
        WriteValueToJson(json_writer);
        return;
    }

    json_writer.BeginObject();

    json_writer.Write(JK::name, m_name)
               .Write(JK::type, m_type);

    WriteJsonMetadata_subclass(json_writer);

    if( symbol_json_output == SymbolJsonOutput::MetadataAndValue )
    {
        json_writer.Key(JK::value);
        WriteValueToJson(json_writer);
    }

    json_writer.EndObject();
}


void Symbol::WriteJsonMetadata_subclass(JsonWriter& /*json_writer*/) const
{
    // if a symbol has additional definitional content to write to JSON, it should override this method
}


void Symbol::WriteValueToJson(JsonWriter& json_writer) const
{
    // if a symbol supports writing its value to JSON, it should override this method
    json_writer.WriteNull();
}


void Symbol::UpdateValueFromJson(const JsonNode<wchar_t>& /*json_node*/)
{
    // if a symbol supports updating its value from JSON, it should override this method
    throw NoUpdateValueFromJsonRoutine(_T("No JSON deserialization routine exists for symbols of type: '%s'"), ToString(m_type));
}


const std::map<std::wstring, SymbolType>& Symbol::GetDeclarationTextMap()
{
    static const std::map<std::wstring, SymbolType> symbol_declaration_text_map = []()
    {
        std::map<std::wstring, SymbolType> map;

        for( const auto& [symbol_type, declaration_token] : std::initializer_list<std::tuple<SymbolType, std::variant<TokenCode, const TCHAR*>>>
            {
                { SymbolType::Array,          TokenCode::TOKKWARRAY },
                { SymbolType::Audio,          TokenCode::TOKKWAUDIO },
                // TODO_DISABLED_FOR_CSPRO77{ SymbolType::Dictionary,     TokenCode::TOKKWCASE },
                // TODO_DISABLED_FOR_CSPRO77{ SymbolType::Dictionary,     TokenCode::TOKKWDATASOURCE },
                { SymbolType::Document,       TokenCode::TOKKWDOCUMENT },
                { SymbolType::File,           TokenCode::TOKKWFILE },
                { SymbolType::Geometry,       TokenCode::TOKKWGEOMETRY },
                { SymbolType::HashMap,        TokenCode::TOKKWHASHMAP },
                { SymbolType::Image,          TokenCode::TOKKWIMAGE },
                { SymbolType::List,           TokenCode::TOKKWLIST },
                { SymbolType::Map,            TokenCode::TOKKWMAP },
                { SymbolType::NamedFrequency, TokenCode::TOKKWFREQ },
                { SymbolType::NamedFrequency, _T("Frequency") },
                { SymbolType::Pff,            TokenCode::TOKKWPFF },
                { SymbolType::Relation,       TokenCode::TOKKWRELATION },
                { SymbolType::Report,         _T("Report") },
                { SymbolType::SystemApp,      TokenCode::TOKKWSYSTEMAPP },
                { SymbolType::UserFunction,   TokenCode::TOKKWFUNCTION },
                { SymbolType::ValueSet,       TokenCode::TOKKWVALUESET },
                { SymbolType::WorkString,     TokenCode::TOKALPHA },
                { SymbolType::WorkString,     TokenCode::TOKSTRING },
                { SymbolType::WorkVariable,   TokenCode::TOKNUMERIC },
            } )
        {
            const TCHAR* declaration_name = std::holds_alternative<TokenCode>(declaration_token) ? Logic::KeywordTable::GetKeywordName(std::get<TokenCode>(declaration_token)) :
                                                                                                   std::get<const TCHAR*>(declaration_token);

            ASSERT(declaration_name != nullptr && map.find(declaration_name) == map.cend());

            map.try_emplace(declaration_name, symbol_type);
        }

        return map;
    }();

    return symbol_declaration_text_map;
}
