#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/Compiler/LogicCompiler.h>

namespace GF { enum class VariableType: int; }


class ZENGINEO_API OptionalNamedArgumentsCompiler
{
public:
    using CompilationType = std::variant<DataType, std::vector<GF::VariableType>, std::function<int()>>;

    OptionalNamedArgumentsCompiler(LogicCompiler& logic_compiler, bool argument_names_are_case_sensitive = false);

    // custom compilation functions need to get the next token because the current token will be the
    // named arguments operator; commas following the custom compilation should not be read
    void AddArgument(std::wstring name, int& program_index, CompilationType type_or_compilation_function);

    // numeric compilation with range checking
    void AddArgumentInteger(std::wstring name, int& program_index, std::optional<int> min_value, std::optional<int> max_value);

    // string compilation with a callback that checks string literals
    void AddArgumentWithStringLiteralCheck(std::wstring name, int& program_index, std::function<void(const std::wstring&)> string_literal_check_callback);

    // string compilation of JSON text
    void AddArgumentJsonText(std::wstring name, int& program_index,
                             std::function<void(const JsonNode<wchar_t>& json_node)> json_node_callback = { });

    // string compilation of PortableColor text
    void AddArgumentPortableColorText(std::wstring name, int& program_index);

    // confirms that nothing has been assigned to the argument associated with the program index
    void ConfirmNotAssigned(int& program_index);

    // the method returns the number of arguments processed
    size_t Compile(bool allow_left_parenthesis_starting_token = false);

private:
    const Logic::Token& GetCurrentToken() const { return m_compiler.GetCurrentToken(); }

private:
    LogicCompiler& m_compiler;
    bool m_argumentNamesAreCaseSensitive;

    std::vector<std::wstring> m_argumentNames;
    std::vector<CompilationType> m_compilationFunctions;
    std::vector<int*> m_programIndices;
};
