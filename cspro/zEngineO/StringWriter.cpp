#include "stdafx.h"
#include "StringWriter.h"
#include "Nodes/Various.h"


StringWriter::StringWriter(std::string string_writer_name, const Nodes::EncodeType encode_type)
    :   Symbol(std::move(string_writer_name), SymbolType::StringWriter),
        m_encodeType(encode_type)
{
}


StringWriter::StringWriter(std::string string_writer_name)
    :   StringWriter(std::move(string_writer_name), Nodes::EncodeType::Default)
{
}


std::unique_ptr<Symbol> StringWriter::CloneInInitialState() const
{
    return std::unique_ptr<StringWriter>(new StringWriter(*this));
}


void StringWriter::Reset()
{
    // SW_TODO
}


void StringWriter::serialize_subclass(Serializer& ar)
{
    ar.SerializeEnum(m_encodeType);
}


void StringWriter::WriteValueToJson(JsonWriter& json_writer) const
{
    json_writer; // SW_TODO
}
