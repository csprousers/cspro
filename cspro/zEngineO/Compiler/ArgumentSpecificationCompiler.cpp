#include "stdafx.h"
#include "IncludesCC.h"
#include "ArgumentSpecificationCompiler.h"


std::tuple<std::unique_ptr<int[]>, int> ArgumentSpecification::CompileArguments(LogicCompiler& logic_compiler) const
{
    auto GetCurrentToken = [&]() -> const Logic::Token& { return logic_compiler.GetCurrentToken(); };

    auto get_first_optional_argument_type = [&]()
    {
        return std::find_if(m_argumentTypes.cbegin(), m_argumentTypes.cend(),
                            [](const auto& argument_type) { return std::holds_alternative<std::optional<DataType>>(argument_type); });
    };

#ifdef _DEBUG
    // make sure that once an optional argument is encountered, that all subsequent arguments are optional
    ASSERT(m_argumentTypes.cend() == get_first_optional_argument_type() ||
           m_argumentTypes.cend() == std::find_if(get_first_optional_argument_type() + 1, m_argumentTypes.cend(),
                                                  [](const auto& argument_type) { return !std::holds_alternative<std::optional<DataType>>(argument_type); }));
#endif

    auto argument_types_itr = m_argumentTypes.cbegin();
    const auto& argument_types_end = m_argumentTypes.cend();

    auto arguments = std::make_unique_for_overwrite<int[]>(m_argumentTypes.size());
    int* arguments_itr = arguments.get();
    const int* arguments_begin = arguments_itr;
    const int* arguments_end = arguments_begin + m_argumentTypes.size();

    auto get_number_arguments_provided = [&]() { return static_cast<int>(arguments_itr - arguments_begin); };
    auto is_argument_required          = [&]() { return std::holds_alternative<DataType>(*argument_types_itr); };

    ASSERT(Tkn == TOKLPAREN || Tkn == TOKCOMMA);

    logic_compiler.NextToken();

    // process each argument
    while( Tkn != TOKRPAREN )
    {
        if( argument_types_itr == argument_types_end )
            logic_compiler.IssueError(MGF::function_call_too_many_arguments_detailed_532, static_cast<int>(m_argumentTypes.size()));

        if( arguments_itr != arguments_begin )
        {
            if( Tkn != TOKCOMMA )
                logic_compiler.IssueError(MGF::function_call_comma_expected_detailed_529, get_number_arguments_provided());

            logic_compiler.NextToken();
        }

        DataType argument_data_type = is_argument_required() ? std::get<DataType>(*argument_types_itr) :
                                                               *std::get<std::optional<DataType>>(*argument_types_itr);
        ASSERT(arguments_itr < arguments_end);
        *arguments_itr = logic_compiler.CompileExpression(argument_data_type);

        ++argument_types_itr;
        ++arguments_itr;
    }

    if( arguments_itr != arguments_end && is_argument_required() )
    {
        size_t number_required_arguments = std::distance(m_argumentTypes.cbegin(), get_first_optional_argument_type());
        logic_compiler.IssueError(MGF::function_call_too_few_arguments_detailed_531, get_number_arguments_provided(), static_cast<int>(number_required_arguments));
    }

    logic_compiler.IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    logic_compiler.NextToken();

    return std::make_tuple(std::move(arguments), get_number_arguments_provided());
}


int ArgumentSpecification::CompileVariableArgumentsNode(LogicCompiler& logic_compiler, FunctionCode function_code) const
{
    auto [arguments, number_arguments_defined] = CompileArguments(logic_compiler);

    auto& va_node = logic_compiler.CreateNode<Nodes::VariableArguments>(function_code, m_argumentTypes.size());

    memcpy(va_node.arguments, arguments.get(), number_arguments_defined * sizeof(arguments[0]));

    for( size_t i = number_arguments_defined; i < m_argumentTypes.size(); ++i )
        va_node.arguments[i] = -1;

    return logic_compiler.GetProgramIndex(va_node);
}


int ArgumentSpecification::CompileVariableArgumentsWithSizeNode(LogicCompiler& logic_compiler, FunctionCode function_code) const
{
    auto [arguments, number_arguments_defined] = CompileArguments(logic_compiler);

    auto& va_with_size_node = logic_compiler.CreateNode<Nodes::VariableArgumentsWithSize>(function_code, m_argumentTypes.size());

    va_with_size_node.number_arguments = number_arguments_defined;
    memcpy(va_with_size_node.arguments, arguments.get(), number_arguments_defined * sizeof(arguments[0]));

    return logic_compiler.GetProgramIndex(va_with_size_node);
}
