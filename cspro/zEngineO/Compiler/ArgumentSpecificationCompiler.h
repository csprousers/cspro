#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/FunctionTable.h>

class LogicCompiler;


class ZENGINEO_API ArgumentSpecification
{
public:
    using ArgumentType = std::variant<DataType, std::optional<DataType>>;

    ArgumentSpecification(std::vector<ArgumentType> argument_types)
        :   m_argumentTypes(std::move(argument_types))
    {
    }

    const std::vector<ArgumentType>& GetArgumentTypes() const { return m_argumentTypes; }

    std::tuple<std::unique_ptr<int[]>, int> CompileArguments(LogicCompiler& logic_compiler) const;

    // returns the program index of a Nodes::VariableArguments node;
    // all arguments are defined, with optional arguments not provided set to -1
    int CompileVariableArgumentsNode(LogicCompiler& logic_compiler, FunctionCode function_code) const;

    // returns the program index of a Nodes::VariableArgumentsWithSize node;
    // the node's number_arguments value specifies how many arguments were provided
    int CompileVariableArgumentsWithSizeNode(LogicCompiler& logic_compiler, FunctionCode function_code) const;

private:
     const std::vector<ArgumentType> m_argumentTypes;
};
