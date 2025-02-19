#include "stdafx.h"
#include "IncludesCC.h"
#include "JavaScriptProcessor.h"
#include "UserFunction.h"
#include "UserFunctionArgumentChecker.h"


int LogicCompiler::CompileJavaScriptFunctions()
{
    auto& va_node = CreateVariableSizeNode<Nodes::VariableArguments>(CurrentToken.function_details->code,
                                                                     CurrentToken.function_details->number_arguments);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    // JS.eval(script)
    if( va_node.function_code == FunctionCode::JSFN_EVAL_CODE )
    {
        // arguments[0]: bytecode_index
        // arguments[1]: script_expression
        va_node.arguments[0] = -1;

        va_node.arguments[1] = CompileStringExpressionWithStringLiteralCheck(
            [&](std::string script)
            {
                try
                {
                    EngineJavaScriptProcessor& javascript_processor = m_engineData->GetJavaScriptProcessor();
                    va_node.arguments[0] = javascript_processor.CompileScript(std::move(script), JavaScript::ModuleType::Global);
                }

                catch( const CSProException& exception )
                {
                    IssueError(MGF::JavaScript_compilation_error_100463, exception.what());
                }
            });
    }

    // JS.invoke(function_name[, arguments]))
    else if( va_node.function_code == FunctionCode::JSFN_INVOKE_CODE )
    {
        // arguments[0]: function_name_expression
        // arguments[1]: arguments (list node)
        va_node.arguments[0] = CompileStringExpression();

        std::vector<int> arguments;

        while( Tkn == TOKCOMMA )
        {
            int symbol_type_or_index;
            int expression_or_symbol_subscript_compilation;
            std::tie(symbol_type_or_index, expression_or_symbol_subscript_compilation) = CompileJavaScriptConvertableValue();

            arguments.emplace_back(symbol_type_or_index);
            arguments.emplace_back(expression_or_symbol_subscript_compilation);
        }

        va_node.arguments[1] = CreateListNode(arguments);
    }

    // JS.getValueJson(name)
    // JS.hasValue(name)
    else if( va_node.function_code == FunctionCode::JSFN_GETVALUEJSON_CODE ||
             va_node.function_code == FunctionCode::JSFN_HASVALUE_CODE )
    {
        // arguments[0]: name_expression
        va_node.arguments[0] = CompileStringExpression();
    }

    // JS.setValueFromJson(name, json)
    else if( va_node.function_code == FunctionCode::JSFN_SETVALUEFROMJSON_CODE )
    {
        // arguments[0]: name_expression
        // arguments[1]: json_expression
        va_node.arguments[0] = CompileStringExpression();

        IssueErrorOnTokenMismatch(TOKCOMMA, 528);
        NextToken();

        va_node.arguments[1] = CompileJsonText();
    }

    // JS.getValue(name, value)
    else if( va_node.function_code == FunctionCode::JSFN_GETVALUE_CODE )
    {
        // arguments[0]: name_expression
        // arguments[1]: symbol_type_or_index (< 0 if symbol type)
        // arguments[2]: value_destination_reference_or_symbol_subscript_compilation
        va_node.arguments[0] = CompileStringExpression();

        IssueErrorOnTokenMismatch(TOKCOMMA, 528);

        const std::optional<SymbolType> next_token_symbol_type = GetNextTokenSymbolType();
        NextToken();

        // the value can be assigned to an entire symbol...
        if( next_token_symbol_type.has_value() )
        {
            const Symbol& symbol = NPT_Ref(Tokstindex);

            // make sure that a conversion routine exists for the symbol
            if( *next_token_symbol_type == SymbolType::UserFunction ||
                !EngineJavaScriptProcessor::IsSymbolTypeAllowedAsArgument(*next_token_symbol_type) )
            {
                IssueError(MGF::JavaScript_symbol_conversion_invalid_100465,
                           symbol.GetName().c_str(), ToString(symbol.GetType()));
            }

            va_node.arguments[1] = symbol.GetSymbolIndex();
            va_node.arguments[2] = CurrentToken.symbol_subscript_compilation;

            NextToken();
        }

        // ...or a numeric/string symbol element
        else
        {
            const DataType value_data_type = GetCurrentTokenDataType();
            va_node.arguments[1] = -1 * static_cast<int>(IsNumeric(value_data_type) ? SymbolType::WorkVariable : SymbolType::WorkString);

            va_node.arguments[2] = CompileDestinationVariable(GetCurrentTokenDataType());
        }
    }

    // JS.setValue(name, value)
    else if( va_node.function_code == FunctionCode::JSFN_SETVALUE_CODE )
    {
        // arguments[0]: name_expression
        // arguments[1]: symbol_type_or_index
        // arguments[2]: expression_or_symbol_subscript_compilation
        va_node.arguments[0] = CompileStringExpression();
        std::tie(va_node.arguments[1], va_node.arguments[2]) = CompileJavaScriptConvertableValue();
    }

    // invalid function
    else
    {
        throw ProgrammingErrorException();
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(va_node);
}


std::tuple<int, int> LogicCompiler::CompileJavaScriptConvertableValue()
{
    // add a symbol...
    const std::optional<SymbolType> next_token_symbol_type = GetNextTokenSymbolType();

    if( next_token_symbol_type.has_value() )
    {
        NextToken();

        Symbol& symbol = NPT_Ref(Tokstindex);

        // make sure that a conversion routine exists for the symbol
        if( !EngineJavaScriptProcessor::IsSymbolTypeAllowedAsArgument(*next_token_symbol_type) )
        {
            IssueError(MGF::JavaScript_symbol_conversion_invalid_100465,
                       symbol.GetName().c_str(), ToString(symbol.GetType()));
        }

        // make sure the symbol has permissions to be used as part of a function call
        UserFunctionArgumentChecker::MarkSymbolAsDynamicallyBoundToFunctionParameter(symbol);

        // for callback functions, make sure all parameters can be converted
        if( symbol.IsA(SymbolType::UserFunction) )
        {
            const UserFunction& user_function = assert_cast<const UserFunction&>(symbol);
            const UserFunctionArgumentChecker argument_checker(user_function);
            const std::optional<size_t> invalid_parameter_index =
                argument_checker.FindFirstInvalidParameter(EngineJavaScriptProcessor::SymbolTypesAllowedAsArguments, false);

            if( invalid_parameter_index.has_value() )
            {
                const Symbol& parameter_symbol = user_function.GetParameterSymbol(*invalid_parameter_index);

                IssueError(MGF::JavaScript_function_symbol_conversion_invalid_100466,
                           ToString(parameter_symbol.GetType()), parameter_symbol.GetName().c_str(), user_function.GetName().c_str());
            }
        }

        std::tuple<int, int> result(symbol.GetSymbolIndex(), CurrentToken.symbol_subscript_compilation);

        NextToken();

        return result;
    }

    // ...or an expression
    else
    {
        NextToken();

        const DataType value_data_type = GetCurrentTokenDataType();

        return std::make_tuple(-1 * static_cast<int>(IsNumeric(value_data_type) ? SymbolType::WorkVariable : SymbolType::WorkString),
                               CompileExpression(value_data_type));
    }
}
