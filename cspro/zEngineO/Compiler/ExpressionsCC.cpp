#include "stdafx.h"
#include "IncludesCC.h"


int LogicCompiler::exprlog()
{
    int p1 = expror();

    while( Tkn == TOKEQUOP ) // EQUIVALENCE operator
    {
        NextToken();
        int p2 = expror();

        p1 = CreateOperatorNode(FunctionCode::EQU_CODE, p1, p2);
    }

    return p1;
}


int LogicCompiler::expror()
{
    int p1 = termlog();

    while( Tkn == TOKOROP ) // OR operator
    {
        NextToken();
        int p2 = termlog();

        p1 = CreateOperatorNode(FunctionCode::OR_CODE, p1, p2);
    }

    return p1;
}


int LogicCompiler::termlog()
{
    int p1 = factlog();

    while( Tkn == TOKANDOP ) // AND operator
    {
        NextToken();
        int p2 = factlog();

        p1 = CreateOperatorNode(FunctionCode::AND_CODE, p1, p2);
    }

    return p1;
}


int LogicCompiler::factlog()
{
    bool using_not = ( Tkn == TOKNOTOP );

    if( using_not )
        NextToken();

    const DataType value_data_type = GetCurrentTokenDataType();

    int p1 = expr();

    if( Tkn == TOKIN )
    {
        NextToken();
        const int p2 = CompileInNodes(value_data_type);
        p1 = CreateInNode(value_data_type, p1, p2);
    }

    else if( Tkn == TOKHAS ) // 20120429
    {
        return CompileHas_COMPILER_DLL_TODO(p1);
    }

    else if( IsRelationalOperator(Tkn) ) // relational operator
    {
        const TokenCode oper = Tkn;

        NextToken();
        const int p2 = expr();

        p1 = CreateOperatorNode(static_cast<FunctionCode>(oper), p1, p2);
    }

    if( using_not )
        p1 = CreateOperatorNode(FunctionCode::NOT_CODE, p1, 0);

    return p1;
}


int LogicCompiler::expr()
{
    int p1;

    if( Tkn == TOKMINUS ) // unary minus (-) operator
    {
        NextToken();
        p1 = term();

        p1 = CreateOperatorNode(FunctionCode::MINUS_CODE, p1, -1);
    }

    else
    {
        p1 = term();
    }

    while( Tkn == TOKADDOP || Tkn == TOKMINOP ) // arithmetic operator ( +, - )
    {
        TokenCode oper = Tkn;

        NextToken();
        int p2 = term();

        FunctionCode function_code = ( oper == TOKADDOP ) ? FunctionCode::ADD_CODE :
                                                            FunctionCode::SUB_CODE;
        p1 = CreateOperatorNode(function_code, p1, p2);
    }

    return p1;
}


int LogicCompiler::term()
{
    int p1 = factor();

    while( Tkn == TOKMULOP || Tkn == TOKDIVOP || Tkn == TOKMODOP ) // arithmetic operator ( * , /, % )
    {
        TokenCode oper = Tkn;

        NextToken();
        int p2 = factor();

        FunctionCode function_code = ( oper == TOKMULOP ) ? FunctionCode::MULT_CODE :
                                     ( oper == TOKDIVOP ) ? FunctionCode::DIV_CODE :
                                                            FunctionCode::MOD_CODE;

        p1 = CreateOperatorNode(function_code, p1, p2);
    }

    return p1;
}


int LogicCompiler::factor()
{
    int p1 = prim();

    if( Tkn == TOKEXPOP ) // arithmetic operator ( *, / )
    {
        NextToken();
        int p2 = factor();

        p1 = CreateOperatorNode(FunctionCode::EXP_CODE, p1, p2);
    }

    return p1;
}


int LogicCompiler::prim()
{
    int p1;

    auto& [call_tester, is_lone_function_call] = get_COMPILER_DLL_TODO_m_loneAlphaFunctionCallTester();
    ++call_tester;

    if( Tkn == TOKLPAREN ) // an expression in parentheses
    {
        NextToken();

        p1 = exprlog();

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_19);

        NextToken();
    }

    else if( Tkn == TOKMINOP ) // 20120405 allow negative constants
    {
        NextToken();
        IssueErrorOnTokenMismatch(TOKCTE, MGF::arithmetic_expression_invalid_21);

        p1 = CreateNumericConstantNode(-1 * Tokvalue);
        NextToken();
    }

    else if( Tkn == TOKCTE ) // constant
    {
        p1 = CreateNumericConstantNode(Tokvalue);
        NextToken();
    }

    else if( IsCurrentTokenString() )
    {
        p1 = crelalpha_COMPILER_DLL_TODO();
    }

    else if( Tkn == TOKVAR )
    {
        p1 = varsanal_COMPILER_DLL_TODO('N');
    }

    else if( Tkn == TOKCROSSTAB )
    {
        p1 = tvarsanal_COMPILER_DLL_TODO();
    }

    else if( Tkn == TOKARRAY )
    {
        p1 = CompileLogicArrayReference();
    }

    else if( Tkn == TOKFUNCTION || Tkn == TOKUSERFUNCTION )
    {
        p1 = rutfunc_COMPILER_DLL_TODO();
    }

    else if( Tkn == TOKLIST )
    {
        p1 = CompileLogicListReference();
    }

    else if( Tkn == TOKHASHMAP )
    {
        p1 = CompileLogicHashMapReference();
    }

    else if( Tkn == TOKFREQ )
    {
        p1 = CompileNamedFrequencyReference();
    }

    else if( IsArithmeticOperator(Tkn) ) // two consecutive operators
    {
        IssueError(MGF::two_consecutive_operators_20);
    }

    else
    {
        IssueError(MGF::arithmetic_expression_invalid_21);
    }

    return p1;
}
