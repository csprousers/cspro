#pragma once

#include <zLogicO/zLogicO.h>


namespace GF // GF = Generalized Function
{
    // do not renumber these values as they are used in bytecode
    enum class VariableType : int { String = 1, Number, Boolean, Array, Object };


    struct Variable
    {
        std::string name;
        std::string description;
        std::vector<VariableType> types;

        ZLOGICO_API static Variable CreateFromJson(const JsonNode& json_node);
        ZLOGICO_API void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;
    };


    struct Parameter
    {
        Variable variable;
        bool required;

        ZLOGICO_API static Parameter CreateFromJson(const JsonNode& json_node);
        ZLOGICO_API void WriteJson(JsonWriter& json_writer) const;
    };


    struct DeprecatedParameter
    {
        std::string name;
        std::vector<VariableType> types;

        ZLOGICO_API static DeprecatedParameter CreateFromJson(const JsonNode& json_node);
        ZLOGICO_API void WriteJson(JsonWriter& json_writer) const;
    };


    struct Function
    {
        std::string namespace_name;
        std::string name;
        std::string description;
        std::vector<Parameter> parameters;
        std::vector<DeprecatedParameter> deprecated_parameters;
        std::vector<Variable> returns;

        ZLOGICO_API static Function CreateFromJson(const JsonNode& json_node);
        ZLOGICO_API void WriteJson(JsonWriter& json_writer) const;
    };
}


ZLOGICO_API const char* ToString(GF::VariableType variable_type);
