#include "stdafx.h"
#include "IncludesCC.h"
#include "OptionalNamedArgumentsCompiler.h"


OptionalNamedArgumentsCompiler::OptionalNamedArgumentsCompiler(LogicCompiler& logic_compiler, bool argument_names_are_case_sensitive/* = false*/)
    :   m_compiler(logic_compiler),
        m_argumentNamesAreCaseSensitive(argument_names_are_case_sensitive)
{
}


void OptionalNamedArgumentsCompiler::AddArgument(std::wstring name, int& program_index, CompilationType type_or_compilation_function)
{
    ASSERT(program_index == -1);

    ASSERT(!std::holds_alternative<DataType>(type_or_compilation_function) ||
           std::get<DataType>(type_or_compilation_function) == DataType::Numeric ||
           std::get<DataType>(type_or_compilation_function) == DataType::String);

    m_argumentNames.emplace_back(std::move(name));
    m_compilationFunctions.emplace_back(std::move(type_or_compilation_function));
    m_programIndices.emplace_back(&program_index);
}


void OptionalNamedArgumentsCompiler::AddArgumentInteger(std::wstring name, int& program_index, std::optional<int> min_value, std::optional<int> max_value)
{
    ASSERT(min_value.has_value() || max_value.has_value());

    size_t argument_name_index = m_argumentNames.size();

    AddArgument(std::move(name), program_index,
        [this, min_value, max_value, argument_name_index]()
        {
            std::optional<double> constant_value;

            m_compiler.MarkInputBufferToRestartLater();
            m_compiler.NextToken();

            if( Tkn == TOKCTE )
            {
                constant_value = Tokvalue;
            }

            else if( Tkn == TOKMINOP )
            {
                m_compiler.NextToken();

                if( Tkn == TOKCTE )
                    constant_value = -1 * Tokvalue;
            }

            if( constant_value.has_value() )
            {
                m_compiler.NextToken();

                // this check prevents expressions that begin with a constant like := 3 * some_numeric
                if( Tkn == TOKCOMMA || Tkn == TOKRPAREN )
                {
                    m_compiler.ClearMarkedInputBuffer();

                    if( ( LogicCompiler::IsNumericConstantInteger(*constant_value) ) &&
                        ( !min_value.has_value() || *min_value <= static_cast<int>(*constant_value) ) &&
                        ( !max_value.has_value() || *max_value >= static_cast<int>(*constant_value) ) )
                    {
                        return m_compiler.CreateNumericConstantNode(*constant_value);
                    }

                    else if( min_value.has_value() && max_value.has_value() )
                    {
                        m_compiler.IssueError(MGF::argument_integer_out_of_range_94704, m_argumentNames[argument_name_index].c_str(), *min_value, *max_value);
                    }

                    else
                    {
                        m_compiler.IssueError(min_value.has_value() ? MGF::argument_integer_too_low_94705 : MGF::argument_integer_too_high_94706,
                                              m_argumentNames[argument_name_index].c_str(),
                                              min_value.has_value() ? *min_value : *max_value);
                    }
                }
            }

            // fallback compilation if not a constant known at compile-time
            m_compiler.RestartFromMarkedInputBuffer();
            m_compiler.NextToken();

            return m_compiler.exprlog();
        });
}


void OptionalNamedArgumentsCompiler::AddArgumentWithStringLiteralCheck(std::wstring name, int& program_index, std::function<void(const std::wstring&)> string_literal_check_callback)
{
    AddArgument(std::move(name), program_index,
        [this, check_callback = std::move(string_literal_check_callback)]()
        {
            m_compiler.NextToken();
            return m_compiler.CompileStringExpressionWithStringLiteralCheck(check_callback);
        });
}


void OptionalNamedArgumentsCompiler::AddArgumentJsonText(std::wstring name, int& program_index,
                                                         std::function<void(const JsonNode<wchar_t>& json_node)> json_node_callback/* = { }*/)
{
    AddArgument(std::move(name), program_index,
        [&, lambda_json_node_callback = std::move(json_node_callback)]()
        {
            m_compiler.NextToken();
            return m_compiler.CompileJsonText(lambda_json_node_callback);
        });
}


void OptionalNamedArgumentsCompiler::AddArgumentPortableColorText(std::wstring name, int& program_index)
{
    AddArgument(std::move(name), program_index,
        [&]()
        {
            m_compiler.NextToken();
            return m_compiler.CompilePortableColorText();
        });
}


void OptionalNamedArgumentsCompiler::ConfirmNotAssigned(int& program_index)
{
    auto get_vector_index = [&]() -> size_t
    {
        const auto& program_index_lookup = std::find_if(m_programIndices.cbegin(), m_programIndices.cend(),
                                                        [&](const int* pi) { return ( pi == &program_index ); });
        ASSERT(program_index_lookup != m_programIndices.cend());

        return std::distance(m_programIndices.cbegin(), program_index_lookup);
    };

    ASSERT(get_vector_index() < m_programIndices.size());

    if( program_index != -1 )
        m_compiler.IssueError(MGF::argument_duplicate_named_argument_94701, m_argumentNames[get_vector_index()].c_str());
}


size_t OptionalNamedArgumentsCompiler::Compile(bool allow_left_parenthesis_starting_token/* = false*/)
{
    size_t arguments_read = 0;

    auto is_valid_separation_token = [&]()
    {
        if( allow_left_parenthesis_starting_token )
        {
            allow_left_parenthesis_starting_token = false;

            if( Tkn == TOKLPAREN )
                return true;
        }

        return ( Tkn == TOKCOMMA );
    };

    while( is_valid_separation_token() )
    {
        // quit if the named arguments operator is not used
        if( !m_compiler.IsNextTokenNamedArgument() )
            return arguments_read;

        size_t argument_index = m_compiler.NextKeywordOrError(m_argumentNames) - 1;

        if( m_argumentNamesAreCaseSensitive )
        {
            wstring_view argument_name_as_specified_sv = m_compiler.GetCurrentBasicToken()->GetTextSV();

            if( !SO::Equals(argument_name_as_specified_sv, m_argumentNames[argument_index]) )
                m_compiler.IssueError(MGF::argument_invalid_case_94700, std::wstring(argument_name_as_specified_sv).c_str(), m_argumentNames[argument_index].c_str());
        }

        int& program_index = *m_programIndices[argument_index];

        // the argument cannot be supplied more than once
        ConfirmNotAssigned(program_index);

        // read the named arguments operator and then compile the argument
        m_compiler.NextToken();
        ASSERT(Tkn == TokenCode::TOKNAMEDARGOP);

        const auto& compilation_function = m_compilationFunctions[argument_index];

        // compile generic numeric/string arguments
        if( std::holds_alternative<DataType>(compilation_function) )
        {
            m_compiler.NextToken();

            if( std::get<DataType>(compilation_function) != m_compiler.GetCurrentTokenDataType() )
            {
                m_compiler.IssueError(MGF::argument_invalid_type_94702, m_argumentNames[argument_index].c_str(),
                                                                        ToString(std::get<DataType>(compilation_function)));
            }

            program_index = m_compiler.CompileExpression(std::get<DataType>(compilation_function));
        }

        // compile generalized function variable types
        else if( std::holds_alternative<std::vector<GF::VariableType>>(compilation_function) )
        {
            m_compiler.NextToken();

            program_index = m_compiler.CompileExpressionOrObject(std::get<std::vector<GF::VariableType>>(compilation_function), m_argumentNames[argument_index].c_str());
        }

        // or use a custom compilation function
        else
        {
            ASSERT(std::holds_alternative<std::function<int()>>(compilation_function));

            program_index = std::get<std::function<int()>>(compilation_function)();
        }

        ++arguments_read;
    }

    return arguments_read;
}
