#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

enum class EncodeType : int;


class ZENGINEO_API StringWriter : public Symbol
{
public:
    StringWriter(std::string string_writer_name, EncodeType encode_type);
    StringWriter(std::string string_writer_name, const Symbol& symbol);
    StringWriter(std::string string_writer_name);

    EncodeType GetEncodeType() const { return m_encodeType; }

    const std::variant<SharableString, int>& GetOutput() const { return m_output; }
    std::variant<SharableString, int>& GetOutput()             { return m_output; }

    void ResetForQuestionText(EncodeType encode_type);

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteValueToJson(JsonWriter& json_writer) const override;

private:
    EncodeType m_encodeType;
    std::variant<SharableString, int> m_output; // a string or a symbol index
};
