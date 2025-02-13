#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>


class ZENGINEO_API WorkVariable : public Symbol
{
private:
    WorkVariable(const WorkVariable& work_variable);

public:
    WorkVariable(std::wstring variable_name);

    double GetValue() const   { return m_value; }
    double* GetValueAddress() { return &m_value; }

    void SetValue(double value) { m_value = value; }

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void WriteValueToJson(JsonWriter& json_writer) const override;
    void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

private:
    double m_value;
};
