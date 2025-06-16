#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

enum class EncodeType : int;


class ZENGINEO_API StringWriter : public Symbol
{
public:
    StringWriter(std::string string_writer_name, EncodeType encode_type, const EngineData& engine_data);
    StringWriter(std::string string_writer_name, const Symbol& symbol, const EngineData& engine_data);
    StringWriter(std::string string_writer_name, const EngineData& engine_data);

    EncodeType GetEncodeType() const { return m_encodeType; }

    const std::variant<SharableString, int>& GetOutput() const { return m_output; }
    std::variant<SharableString, int>& GetOutput()             { return m_output; }

    void ResetForQuestionText(EncodeType encode_type);

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;
    void WriteValueToJson(JsonWriter& json_writer) const override;
    void SetValueFromJson(const JsonNode& json_node) override;

private:
    const EngineData& m_engineData;
    EncodeType m_encodeType;
    std::variant<SharableString, int> m_output; // a string or a symbol index
};
