#include "stdafx.h"
#include "GeneralizedFunction.h"

using namespace GF;


const TCHAR* ToString(VariableType variable_type)
{
    switch( variable_type)
    {
        case VariableType::String:  return _T("string");
        case VariableType::Number:  return _T("number");
        case VariableType::Boolean: return _T("boolean");
        case VariableType::Array:   return _T("array");
        case VariableType::Object:  return _T("object");
        default:                    return ReturnProgrammingError(_T(""));
    }
}


CREATE_ENUM_JSON_SERIALIZER(VariableType,
    { VariableType::String,  ToString(VariableType::String) },
    { VariableType::Number,  ToString(VariableType::Number) },
    { VariableType::Boolean, ToString(VariableType::Boolean) },
    { VariableType::Array,   ToString(VariableType::Array) },
    { VariableType::Object,  ToString(VariableType::Object) })


Variable Variable::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    const auto& name_node = json_node.Get(JK::name);

    return Variable
    {
        name_node.IsNull() ? std::wstring() : json_node.Get<std::wstring>(JK::name),
        json_node.GetOrDefault<std::wstring>(JK::description, SO::EmptyString),
        json_node.GetArray(JK::types).GetVector<VariableType>(),
    };
}


void Variable::WriteJson(JsonWriter& json_writer, bool write_to_new_json_object/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    json_writer.Write(JK::name, name)
               .WriteIfNotBlank(JK::description, description)
               .Write(JK::types, types);

    if( write_to_new_json_object )
        json_writer.EndObject();
}


Parameter Parameter::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    return Parameter
    {
        json_node.Get<Variable>(),
        json_node.GetOrDefault(JK::required, true)
    };
}


void Parameter::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    variable.WriteJson(json_writer, false);
    json_writer.Write(JK::required, required);

    json_writer.EndObject();
}


Function Function::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    return Function
    {
        json_node.GetOrDefault<std::wstring>(JK::namespace_, SO::EmptyString),
        json_node.Get<std::wstring>(JK::name),
        json_node.GetOrDefault<std::wstring>(JK::description, SO::EmptyString),
        json_node.GetArrayOrEmpty(JK::parameters).GetVector<Parameter>(),
        json_node.GetArrayOrEmpty(JK::returns).GetVector<Variable>()
    };
}


void Function::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .WriteIfNotBlank(JK::namespace_, namespace_name)
               .Write(JK::name, name)
               .WriteIfNotBlank(JK::description, description)
               .WriteIfNotEmpty(JK::parameters, parameters)
               .WriteIfNotEmpty(JK::returns, returns)
               .EndObject();
}
