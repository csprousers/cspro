#include "stdafx.h"
#include "StringWriter.h"
#include "Nodes/TextTemplate.h"


StringWriter::StringWriter(std::string string_writer_name, const EncodeType encode_type, const EngineData& engine_data)
    :   Symbol(std::move(string_writer_name), SymbolType::StringWriter),
        m_engineData(engine_data),
        m_encodeType(encode_type)
{
}


StringWriter::StringWriter(std::string string_writer_name, const Symbol& symbol, const EngineData& engine_data)
    :   Symbol(std::move(string_writer_name), SymbolType::StringWriter),
        m_engineData(engine_data),
        m_encodeType(EncodeType::Default),
        m_output(symbol.GetSymbolIndex())
{
    ASSERT(symbol.IsA(SymbolType::Report));
}


StringWriter::StringWriter(std::string string_writer_name, const EngineData& engine_data)
    :   StringWriter(std::move(string_writer_name), EncodeType::Default, engine_data)
{
}


std::unique_ptr<Symbol> StringWriter::CloneInInitialState() const
{
    return std::unique_ptr<StringWriter>(new StringWriter(*this));
}


void StringWriter::Reset()
{
    if( std::holds_alternative<SharableString>(m_output) )
        std::get<SharableString>(m_output).Reset();
}


void StringWriter::ResetForQuestionText(const EncodeType encode_type)
{
    ASSERT(std::holds_alternative<SharableString>(m_output));

    m_encodeType = encode_type;
    std::get<SharableString>(m_output).Reset();
}


void StringWriter::serialize_subclass(Serializer& ar)
{
    ar.SerializeEnum(m_encodeType);

    if( ar.IsSaving() )
    {
        ar.Write<bool>(std::holds_alternative<int>(m_output));

        if( std::holds_alternative<int>(m_output) )
            ar << std::get<int>(m_output);
    }

    else
    {
        if( ar.Read<bool>() )
            m_output = ar.Read<int>();
    }
}


void StringWriter::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    if( std::holds_alternative<int>(m_output) )
    {
        m_engineData.symbol_table.GetAt(std::get<int>(m_output)).WriteJsonMetadata_subclass(json_writer);
        return;
    }

    json_writer.Write(JK::encoding, ( m_encodeType == EncodeType::Default ) ? "default" :
                                                                              EncodeTypeStrings[static_cast<size_t>(m_encodeType) - 1]);
}


void StringWriter::WriteValueToJson(JsonWriter& json_writer) const
{
    if( std::holds_alternative<int>(m_output) )
    {
        m_engineData.symbol_table.GetAt(std::get<int>(m_output)).WriteValueToJson(json_writer);
        return;
    }

    json_writer.WriteEngineValue(std::get<SharableString>(m_output));
}


void StringWriter::SetValueFromJson(const JsonNode& json_node)
{
    if( std::holds_alternative<int>(m_output) )
    {
        m_engineData.symbol_table.GetAt(std::get<int>(m_output)).SetValueFromJson(json_node);
        return;
    }

    std::get<SharableString>(m_output) = json_node.GetEngineValue<SharableString>();
}
