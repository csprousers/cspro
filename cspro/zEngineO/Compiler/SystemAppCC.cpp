#include "stdafx.h"
#include "IncludesCC.h"
#include "SystemApp.h"


SystemApp* LogicCompiler::CompileSystemAppDeclaration()
{
    std::wstring system_app_name = CompileNewSymbolName();

    auto system_app = std::make_shared<SystemApp>(std::move(system_app_name));

    m_engineData->AddSymbol(system_app);

    return system_app.get();
}


int LogicCompiler::CompileSystemAppDeclarations()
{
    ASSERT(Tkn == TOKKWSYSTEMAPP);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        const SystemApp* system_app = CompileSystemAppDeclaration();

        AddSymbolResetNode(symbol_reset_node, *system_app);

        NextToken();

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileSystemAppFunctions()
{
    // compiling system_app_name.clear();
    //           system_app_name.setArgument(name[, value]);
    //           system_app_name.getResult(name);
    //           system_app_name.exec([package_name, activity_name])

    FunctionCode function_code = CurrentToken.function_details->code;
    const SystemApp* system_app = assert_cast<const SystemApp*>(CurrentToken.symbol);
    int number_arguments = CurrentToken.function_details->number_arguments;
    Nodes::SymbolVariableArguments& symbol_va_node = CreateSymbolVariableArgumentsNode(function_code, *system_app, number_arguments, -1);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    // all functions with arguments have at least one argument, which is a string
    bool argumentless_exec = ( function_code == FunctionCode::SYSTEMAPPFN_EXEC_CODE && Tkn == TOKRPAREN );

    if( number_arguments > 0 && !argumentless_exec )
    {
        symbol_va_node.arguments[0] = CompileStringExpression();

        if( Tkn == TOKCOMMA && function_code != FunctionCode::SYSTEMAPPFN_GETRESULT_CODE )
        {
            NextToken();
            bool read_string = IsCurrentTokenString();

            // setargument can take a string or number
            if( read_string || function_code != FunctionCode::SYSTEMAPPFN_SETARGUMENT_CODE )
            {
                symbol_va_node.arguments[1] = CompileStringExpression();
            }

            else
            {
                symbol_va_node.arguments[1] = exprlog();
            }

            // set the argument type
            if( function_code == FunctionCode::SYSTEMAPPFN_SETARGUMENT_CODE )
                symbol_va_node.arguments[2] = static_cast<int>(read_string ? DataType::String : DataType::Numeric);
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(symbol_va_node);
}
