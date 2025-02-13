#include "stdafx.h"
#include "IncludesCC.h"
#include "Nodes/GeneralizedFunction.h"


int LogicCompiler::CompileExpression(DataType data_type)
{
    if( data_type == DataType::Numeric )
    {
        return exprlog();
    }

    else if( data_type == DataType::String )
    {
        return CompileStringExpression();
    }

    else
    {
        IssueError(ReturnProgrammingError(MGF::statement_invalid_1));
    }
}


int LogicCompiler::CompileExpressionOrObject(const std::vector<GF::VariableType>& variable_types, const TCHAR* const argument_name/* = _T("unknown")*/)
{
    ASSERT(!variable_types.empty() && argument_name != nullptr);

    auto variable_type_is_allowed = [&](const GF::VariableType variable_type)
    {
        return ( std::find(variable_types.cbegin(), variable_types.cend(), variable_type) != variable_types.cend() );
    };

    auto& gf_value_node = CreateNode<Nodes::GeneralizedFunctionValue>();

    // allow the user to specify the parameter variable type, which will allow for the resolution of ambiguity errors
    // between strings and arrays/objects specified as strings
    if( Tkn == TOKATOP )
    {
        const size_t type_index = NextKeywordOrError({ _T("string"), _T("number"), _T("boolean"), _T("array"), _T("object" )});
        gf_value_node.parameter_variable_type = ( type_index == 1 ) ? GF::VariableType::String :
                                                ( type_index == 2 ) ? GF::VariableType::Number :
                                                ( type_index == 3 ) ? GF::VariableType::Boolean :
                                                ( type_index == 4 ) ? GF::VariableType::Array :
                                                                      GF::VariableType::Object;

        if( !variable_type_is_allowed(gf_value_node.parameter_variable_type) )
        {
            IssueError(MGF::argument_argument_must_be_of_types_94703, argument_name,
                       SO::CreateSingleStringUsingCallback(variable_types, [](const GF::VariableType variable_type) { return ToString(variable_type); }).c_str());
        }

        NextToken();
    }

    // if not explicitly specified, determine what the user is specifying
    else
    {
        // arrays can be clearly identified
        if( variable_type_is_allowed(GF::VariableType::Array) && ( Tkn == TOKARRAY || Tkn == TOKLIST ) && !IsNextToken(TOKLPAREN) )
        {
            gf_value_node.parameter_variable_type = GF::VariableType::Array;
        }

        // as can numbers
        else if( ( variable_type_is_allowed(GF::VariableType::Number) || variable_type_is_allowed(GF::VariableType::Boolean) ) && !IsCurrentTokenString() )
        {
            gf_value_node.parameter_variable_type = GF::VariableType::Number;
        }

        // as can strings
        else if( variable_type_is_allowed(GF::VariableType::String) && IsCurrentTokenString() )
        {
            gf_value_node.parameter_variable_type = GF::VariableType::String;
        }

        else
        {
            IssueError(MGF::argument_type_automatic_casting_not_supported_94707, argument_name, CSPRO_VERSION_NUMBER);
        }
    }


    // String: string expressions
    if( gf_value_node.parameter_variable_type == GF::VariableType::String )
    {
        gf_value_node.argument_variable_type = GF::VariableType::String;
        gf_value_node.argument_expression = CompileStringExpression();
    }


    // Number + Boolean: numeric expressions
    else if( gf_value_node.parameter_variable_type == GF::VariableType::Number ||
             gf_value_node.parameter_variable_type == GF::VariableType::Boolean )
    {
        gf_value_node.argument_variable_type = GF::VariableType::Number;
        gf_value_node.argument_expression = exprlog();
    }


    // Array: accept Array and List objects, or a JSON string
    else if( gf_value_node.parameter_variable_type == GF::VariableType::Array )
    {
        if( ( Tkn == TOKARRAY || Tkn == TOKLIST ) && !IsNextToken(TOKLPAREN) )
        {
            gf_value_node.argument_variable_type = GF::VariableType::Array;
            gf_value_node.argument_expression = Tokstindex;

            NextToken();
        }

        else if( IsCurrentTokenString() )
        {
            gf_value_node.argument_variable_type = GF::VariableType::String;
            gf_value_node.argument_expression = CompileJsonText(
                [&](const JsonNode<wchar_t>& json_node)
                {
                    if( !json_node.IsArray() )
                        IssueError(MGF::argument_argument_must_be_of_types_94703, argument_name, ToString(GF::VariableType::Array));
                });
        }

        else
        {
            IssueError(MGF::argument_argument_must_be_of_types_94703, argument_name,
                       SO::CreateSingleStringUsingCallback(std::vector<SymbolType>({ SymbolType::Array, SymbolType::List, SymbolType::WorkString }),
                                                           [](SymbolType symbol_type) { return ToString(symbol_type); }).c_str());
        }
    }


    // Object: accept a JSON string
    else if( gf_value_node.parameter_variable_type == GF::VariableType::Object )
    {
        if( !IsCurrentTokenString() )
            IssueError(MGF::argument_argument_must_be_of_types_94703, argument_name, ToString(SymbolType::WorkString));

        gf_value_node.argument_variable_type = GF::VariableType::String;
        gf_value_node.argument_expression = CompileJsonText(
            [&](const JsonNode<wchar_t>& json_node)
            {
                if( !json_node.IsObject() )
                    IssueError(MGF::argument_argument_must_be_of_types_94703, argument_name, ToString(GF::VariableType::Object));
            });
    }


    // unknown type
    else
    {
        IssueError(ReturnProgrammingError(MGF::statement_invalid_1));
    }

    return GetProgramIndex(gf_value_node);
}


int LogicCompiler::CompileFunctionCall(const int program_index/* = -1*/)
{
    auto& function_call_node = CreateNode<Nodes::FunctionCall>(FunctionCode::FUCALL_CODE);

    function_call_node.next_st = -1;
    function_call_node.expression = program_index;

    if( function_call_node.expression == -1 )
    {
        // TODO: this all needs to be improved at some point;
        // for now, the use of call_tester is similar to the pre-8.0 code
        auto& [call_tester, is_lone_function_call] = get_COMPILER_DLL_TODO_m_loneAlphaFunctionCallTester();
        call_tester = 0;

        function_call_node.expression = exprlog();
    }

    return GetProgramIndex(function_call_node);
}


int LogicCompiler::CompileFunctionsArgumentsFixedN()
{
    // compiling functions with exactly N arguments, with the number of arguments specified
    // in the array in FunctionTable (as number_arguments)

    static const std::set<FunctionCode> functions_with_string_arguments =
    {
        FunctionCode::FNCOMPARENOCASE_CODE
    };

    const FunctionCode function_code = CurrentToken.function_details->code;
    const DataType argument_data_type = ( functions_with_string_arguments.find(function_code) != functions_with_string_arguments.cend() ) ? DataType::String :
                                                                                                                                            DataType::Numeric;
    const int number_arguments = CurrentToken.function_details->number_arguments;
    auto& va_node = CreateVariableSizeNode<Nodes::VariableArguments>(function_code, number_arguments);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    // process each argument
    for( int i = 0; i < number_arguments; ++i )
    {
        if( Tkn == TOKRPAREN )
            IssueError(MGF::function_call_arguments_count_mismatch_detailed_530, i, number_arguments);

        if( i > 0 )
        {
            if( Tkn != TOKCOMMA )
                IssueError(MGF::function_call_comma_expected_detailed_529, i);

            NextToken();
        }

        va_node.arguments[i] = CompileExpression(argument_data_type);
    }

    if( Tkn == TOKCOMMA )
        IssueError(MGF::function_call_too_many_arguments_detailed_532, number_arguments);

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(va_node);
}


int LogicCompiler::CompileFunctionsArgumentsVaryingN()
{
    // compiling functions with N or more arguments, with the number of arguments specified
    // in the array in FunctionTable (as number_arguments)

    // a lot of legacy functions are still compiled in this way, even though the number
    // of arguments is fixed, so the bytecode matches that expected by the interpreter;
    // other functions compiled like that are noted with: NUMBER_ARGUMENTS_NOT_NEEDED_LEGACY_NODE

    static const std::set<FunctionCode> functions_with_string_arguments =
    {
        FunctionCode::FNCOMPARE_CODE,       FunctionCode::FNCONCAT_CODE,            FunctionCode::FNDIRCREATE_CODE,     FunctionCode::FNDIRDELETE_CODE,
        FunctionCode::FNDIREXIST_CODE,      FunctionCode::FNGETRECORD_CODE,         FunctionCode::FNISCHECKED_CODE,     FunctionCode::FNPOS_CODE,
        FunctionCode::FNPOSCHAR_CODE,       FunctionCode::FNREGEXMATCH_CODE,        FunctionCode::FNREPLACE_CODE,       FunctionCode::FNSETBLUETOOTHNAME_CODE,
        FunctionCode::FNSETLANGUAGE_CODE,   FunctionCode::FNSETOPERATORID_CODE,     FunctionCode::FNSETVALUESETS_CODE,  FunctionCode::FNSTARTSWITH_CODE,
        FunctionCode::FNSTRIP_CODE,         FunctionCode::FNTOLOWER_CODE,           FunctionCode::FNTONUMB_CODE,        FunctionCode::FNTOUPPER_CODE,
        FunctionCode::FNTR_CODE,            FunctionCode::FNVIEW_CODE
    };

    const FunctionCode function_code = CurrentToken.function_details->code;
    const DataType argument_data_type = ( functions_with_string_arguments.find(function_code) != functions_with_string_arguments.cend() ) ? DataType::String :
                                                                                                                                            DataType::Numeric;
    size_t number_arguments_min;
    size_t number_arguments_max;
    std::vector<int> arguments;

    if( CurrentToken.function_details->number_arguments >= 0 )
    {
        ASSERT(CurrentToken.function_details->number_arguments != 0); // use FunctionCompilationType::ArgumentsFixedN instead

        number_arguments_min = CurrentToken.function_details->number_arguments;
        number_arguments_max = CurrentToken.function_details->number_arguments;
    }

    else
    {
        // negative values mean that N or more arguments are allowed
        number_arguments_min = -1 * CurrentToken.function_details->number_arguments;
        number_arguments_max = std::numeric_limits<size_t>::max();
    }

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    // process each argument
    NextToken();

    while( Tkn != TOKRPAREN )
    {
        if( arguments.size() >= number_arguments_max )
            IssueError(MGF::function_call_too_many_arguments_detailed_532, static_cast<int>(number_arguments_max));

        if( !arguments.empty() )
        {
            if( Tkn != TOKCOMMA )
                IssueError(MGF::function_call_comma_expected_detailed_529, static_cast<int>(arguments.size()));

            NextToken();
        }

        arguments.emplace_back(CompileExpression(argument_data_type));
    }

    // make sure the number of arguments is valid
    ASSERT(arguments.size() <= number_arguments_max);

    if( arguments.size() < number_arguments_min )
        IssueError(MGF::function_call_too_few_arguments_detailed_531, static_cast<int>(arguments.size()), static_cast<int>(number_arguments_min));

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    auto& va_with_size_node = CreateVariableSizeNode<Nodes::VariableArgumentsWithSize>(function_code, arguments.size());

    va_with_size_node.number_arguments = arguments.size();
    memcpy(va_with_size_node.arguments, arguments.data(), arguments.size() * sizeof(arguments[0]));

    return GetProgramIndex(va_with_size_node);
}


int LogicCompiler::CompileFunctionsArgumentSpecification()
{
    // compiling functions with argument types specified in following map, with the boolean value
    // indicating what kind of node to create:
    // - true  -> Nodes::VariableArgumentsWithSize
    // - false -> Nodes::VariableArguments

    static const std::map<FunctionCode, std::tuple<bool, ArgumentSpecification>> argument_specifications =
    {
        { FunctionCode::FNDATEADD_CODE,         {  true, ArgumentSpecification({ DataType::Numeric, DataType::Numeric, std::make_optional(DataType::String) }) } },
        { FunctionCode::FNDATEDIFF_CODE,        {  true, ArgumentSpecification({ DataType::Numeric, DataType::Numeric, std::make_optional(DataType::String) }) } },
        { FunctionCode::FNDECOMPRESS_CODE,      {  true, ArgumentSpecification({ DataType::String, std::make_optional(DataType::String) }) } },
        { FunctionCode::FNEDIT_CODE,            { false, ArgumentSpecification({ DataType::String, DataType::Numeric }) } },
        { FunctionCode::FNSYSDATE_CODE,         {  true, ArgumentSpecification({ std::make_optional(DataType::String), std::make_optional(DataType::Numeric) }) } },
        { FunctionCode::FNSYSTIME_CODE,         {  true, ArgumentSpecification({ std::make_optional(DataType::String), std::make_optional(DataType::Numeric) }) } },
        { FunctionCode::FNSYSPARM_CODE,         {  true, ArgumentSpecification({ std::make_optional(DataType::String) }) } },
    };

    FunctionCode function_code = CurrentToken.function_details->code;
    const auto& argument_specification_lookup = argument_specifications.find(function_code);

    if( argument_specification_lookup == argument_specifications.cend() )
        IssueError(ReturnProgrammingError(MGF::statement_invalid_1));

    const auto& [create_node_with_size, argument_specification] = argument_specification_lookup->second;
    ASSERT(CurrentToken.function_details->number_arguments == static_cast<int>(argument_specification.GetArgumentTypes().size()));

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    return create_node_with_size ? argument_specification.CompileVariableArgumentsWithSizeNode(*this, function_code) :
                                   argument_specification.CompileVariableArgumentsNode(*this, function_code);
}


int LogicCompiler::CompileFunctionsRemovedFromLanguage()
{
    // any features removed from the language can be compiled here; the error message should be specified
    // in the array in FunctionTable, with the message number coming here as number_arguments
    IssueError(CurrentToken.function_details->number_arguments);
}
