#include "stdafx.h"
#include "WorkVariable.h"
#include <zJavaScript/Executor.h>


// --------------------------------------------------------------------------
// WorkVariable
// --------------------------------------------------------------------------

namespace
{
    constexpr double DefaultValue = 0;
}


WorkVariable::WorkVariable(std::string variable_name)
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


void WorkVariable::SetValueFromJson(const JsonNode& json_node)
{
    m_value = json_node.GetEngineValue<double>();
}


JavaScript::Value WorkVariable::GetJavaScriptValue(JavaScript::Executor& executor) const
{
    return executor.CreateEngineValue(m_value);
}


void WorkVariable::SetValueFromJavaScript(JavaScript::Executor& executor, const JavaScript::Value& js_value)
{
    m_value = executor.ConvertEngineValue<double>(js_value);
}
