#include "stdafx.h"
#include "WorkVariable.h"


// --------------------------------------------------------------------------
// WorkVariable
// --------------------------------------------------------------------------

namespace
{
    constexpr double DefaultValue = 0;
}


WorkVariable::WorkVariable(std::wstring variable_name)
    :   Symbol(std::move(variable_name), SymbolType::WorkVariable),
        m_value(DefaultValue)
{
}


WorkVariable::WorkVariable(const WorkVariable& work_variable)
    :   Symbol(work_variable),
        m_value(DefaultValue)
{
}


std::unique_ptr<Symbol> WorkVariable::CloneInInitialState() const
{
    return std::unique_ptr<WorkVariable>(new WorkVariable(*this));
}


void WorkVariable::Reset()
{
    m_value = DefaultValue;
}


void WorkVariable::WriteValueToJson(JsonWriter& json_writer) const
{
    json_writer.WriteEngineValue(m_value);
}


void WorkVariable::UpdateValueFromJson(const JsonNode<wchar_t>& json_node)
{
    SetValue(json_node.GetEngineValue<double>());
}
