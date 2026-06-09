#include "stdafx.h"
#include "GeneralizedFunction.h"

using namespace GF;


CREATE_JSON_KEY(deprecatedParameters)


const char* ToString(const VariableType variable_type)
{
    switch( variable_type)
    {
        case VariableType::String:  return "string";
        case VariableType::Number:  return "number";
        case VariableType::Boolean: return "boolean";
        case VariableType::Array:   return "array";
        case VariableType::Object:  return "object";
        default:                    return ReturnProgrammingError("");
    }
}


CREATE_ENUM_JSON_SERIALIZER(VariableType,
    { VariableType::String,  ToString(VariableType::String) },
    { VariableType::Number,  ToString(VariableType::Number) },
    { VariableType::Boolean, ToString(VariableType::Boolean) },
    { VariableType::Array,   ToString(VariableType::Array) },
    { VariableType::Object,  ToString(VariableType::Object) })


Variable Variable::CreateFromJson(const JsonNode& json_node)
{
    const JsonNode name_node = json_node.Get(JK::name);

    return Variable
    {
        name_node.IsNull() ? std::string() : json_node.Get<std::string>(JK::name),
        json_node.GetOrConstruct<std::string>(JK::description),
        json_node.GetArray(JK::types).GetVector<VariableType>(),
    };
}


void Variable::WriteJson(JsonWriter& json_writer, const bool write_to_new_json_object/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    json_writer.Write(JK::name, name)
               .WriteIfNotBlank(JK::description, description)
               .Write(JK::types, types);

    if( write_to_new_json_object )
        json_writer.EndObject();
}


Parameter Parameter::CreateFromJson(const JsonNode& json_node)
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


DeprecatedParameter DeprecatedParameter::CreateFromJson(const JsonNode& json_node)
{
    return DeprecatedParameter
    {
        json_node.Get<std::string>(JK::name),
        json_node.GetArray(JK::types).GetVector<VariableType>()
    };
}


void DeprecatedParameter::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::name, name)
               .Write(JK::types, types)
               .EndObject();
}


Function Function::CreateFromJson(const JsonNode& json_node)
{
    return Function
    {
        json_node.GetOrConstruct<std::string>(JK::namespace_),
        json_node.Get<std::string>(JK::name),
        json_node.GetOrConstruct<std::string>(JK::description),
        json_node.GetArrayOrEmpty(JK::parameters).GetVector<Parameter>(),
        json_node.GetArrayOrEmpty(JK::deprecatedParameters).GetVector<DeprecatedParameter>(),
        json_node.GetArrayOrEmpty(JK::returns).GetVector<Variable>()
    };
}


void Function::WriteJson(JsonWriter& json_writer, const std::function<void(JsonWriter&)>* const additional_properties_writer/* = nullptr*/) const
{
    json_writer.BeginObject()
               .WriteIfNotBlank(JK::namespace_, namespace_name)
               .Write(JK::name, name);

    if( additional_properties_writer != nullptr )
        (*additional_properties_writer)(json_writer);

    json_writer.WriteIfNotBlank(JK::description, description)
               .WriteIfNotEmpty(JK::parameters, parameters)
               .WriteIfNotEmpty(JK::deprecatedParameters, deprecated_parameters)
               .WriteIfNotEmpty(JK::returns, returns)
               .EndObject();
}
