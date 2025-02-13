#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

class Serializer;
class FrequencyDriver;


class ZENGINEO_API NamedFrequency : public Symbol
{
private:
    NamedFrequency(const NamedFrequency& named_frequency);

public:
    NamedFrequency(std::wstring frequency_name);

    size_t GetFrequencyIndex() const     { return *m_frequencyIndex; }
    void SetFrequencyIndex(size_t index) { m_frequencyIndex = index; }

    bool IsFunctionParameter() const { return !m_frequencyIndex.has_value(); }

    void SetFrequencyDriver(FrequencyDriver* frequency_driver) { m_frequencyDriver = frequency_driver; }

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

public:
	void WriteValueToJson(JsonWriter& json_writer) const override;

private:
    std::optional<size_t> m_frequencyIndex;

    FrequencyDriver* m_frequencyDriver;
};
