#pragma once

#include <zLogicO/zLogicO.h>

template<typename CharType> class JsonNode;
class JsonWriter;


namespace GF // GF = Generalized Function
{
    // do not renumber these values as they are used in bytecode
    enum class VariableType : int { String = 1, Number, Boolean, Array, Object };


    struct Variable
    {
        std::wstring name;
        std::wstring description;
        std::vector<VariableType> types;

        ZLOGICO_API static Variable CreateFromJson(const JsonNode<wchar_t>& json_node);
        ZLOGICO_API void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;
    };


    struct Parameter
    {
        Variable variable;
        bool required;

        ZLOGICO_API static Parameter CreateFromJson(const JsonNode<wchar_t>& json_node);
        ZLOGICO_API void WriteJson(JsonWriter& json_writer) const;
    };


    struct Function
    {
        std::wstring namespace_name;
        std::wstring name;
        std::wstring description;
        std::vector<Parameter> parameters;
        std::vector<Variable> returns;

        ZLOGICO_API static Function CreateFromJson(const JsonNode<wchar_t>& json_node);
        ZLOGICO_API void WriteJson(JsonWriter& json_writer) const;
    };
}


ZLOGICO_API const TCHAR* ToString(GF::VariableType variable_type);
