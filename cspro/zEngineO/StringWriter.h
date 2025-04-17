#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

namespace Nodes { enum class EncodeType : int; }


class ZENGINEO_API StringWriter : public Symbol
{
public:
    StringWriter(std::string string_writer_name, Nodes::EncodeType encode_type);
    StringWriter(std::string string_writer_name);

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteValueToJson(JsonWriter& json_writer) const override;

private:
    Nodes::EncodeType m_encodeType;
};
