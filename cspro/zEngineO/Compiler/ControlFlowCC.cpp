#include "stdafx.h"
#include "IncludesCC.h"
#include "LoopStack.h"
#include "WorkVariable.h"
#include "Nodes/ControlFlow.h"
#include <zLogicO/LocalSymbolStack.h>


int LogicCompiler::CompileIfStatement()
{
    ASSERT(Tkn == TOKIF);

    const Nodes::If* first_if_node = nullptr;
    Nodes::If* previous_if_node = nullptr;

    do
    {
        ASSERT(Tkn == TOKIF || Tkn == TOKELSEIF);

        NextToken();

        auto& if_node = CreateNode<Nodes::If>(FunctionCode::IF_CODE);

        if( first_if_node == nullptr )
        {
            first_if_node = &if_node;
        }

        else if( previous_if_node != nullptr )
        {
            previous_if_node->else_program_index = GetProgramIndex(if_node);
        }

        if_node.next_st = -1;

        if_node.conditional_expression = exprlog();

        IssueErrorOnTokenMismatch(TOKTHEN, MGF::expecting_then_keyword_6);

        NextToken();
        if_node.then_program_index = instruc_COMPILER_DLL_TODO();

        previous_if_node = &if_node;

    } while( Tkn == TOKELSEIF );

    if( Tkn == TOKELSE )
    {
        NextToken();
        previous_if_node->else_program_index = instruc_COMPILER_DLL_TODO();
    }

    else if( Tkn == TOKENDIF )
    {
        previous_if_node->else_program_index = -1;
    }

    else
    {
        IssueError(MGF::expecting_else_elseif_endif_keyword_7);
    }

    IssueErrorOnTokenMismatch(TOKENDIF, MGF::expecting_endif_keyword_8);

    NextToken();

    return GetProgramIndex(*first_if_node);
}



int LogicCompiler::CompileWhileLoop()
{
    ASSERT(Tkn == TOKWHILE);

    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::While);

    auto& while_node = CreateNode<Nodes::While>(FunctionCode::WHILE_CODE);

    while_node.next_st = -1;

    // read the condition
    NextToken();
    while_node.conditional_expression = exprlog();

    IssueErrorOnTokenMismatch(TOKDO, MGF::expecting_do_keyword_9);

    // read the loop contents
    NextToken();
    while_node.block_program_index = instruc_COMPILER_DLL_TODO();

    IssueErrorOnTokenMismatch(TOKENDDO, MGF::expecting_enddo_keyword_10);

    NextToken();

    // ideally we would check for a semicolon but this was allowed
    // without a semicolon for a long time so it will not be checked

    return GetProgramIndex(while_node);
}


int LogicCompiler::CompileDoLoop()
{
    ASSERT(Tkn == TOKDO);

    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::Do);

    // create the local symbol stack so that a possible counter varible is created in a new scope
    Logic::LocalSymbolStack local_symbol_stack = m_symbolTable.CreateLocalSymbolStack();

    auto& do_node = CreateNode<Nodes::Do>(FunctionCode::DO_CODE);

    do_node.next_st = -1;
    do_node.counter_symbol_value_node_index = -1;
    do_node.counter_initial_value_expression = -1;
    do_node.counter_increment_by_expression = -1;

    NextToken();

    // skip the optional varying keyword
    if( Tkn == TOKVARYING )
        NextToken();

    // allow for the declaration of a new counter variable in the do loop
    if( Tkn == TOKNUMERIC )
    {
        WorkVariable* work_variable = CompileWorkVariableDeclaration();
        do_node.counter_symbol_value_node_index = CompileDestinationVariable(work_variable);
    }

    // or use an existing variable
    else if( Tkn != TOKUNTIL && Tkn != TOKWHILE )
    {
        do_node.counter_symbol_value_node_index = CompileDestinationVariable(DataType::Numeric);
    }

    // read the initial value for the counter
    if( do_node.counter_symbol_value_node_index != -1 )
    {
        IssueErrorOnTokenMismatch(TOKEQOP, MGF::Do_invalid_initial_expression_8001);

        NextToken();
        do_node.counter_initial_value_expression = exprlog();
    }

    // read the loop type
    if( Tkn != TOKUNTIL && Tkn != TOKWHILE )
        IssueError(MGF::Do_expecting_while_until_keyword_8002);

    do_node.loop_type = Tkn;

    // read the condition
    NextToken();
    do_node.conditional_expression = exprlog();

    // read the optional by value
    if( Tkn == TOKBY )
    {
        if( do_node.counter_symbol_value_node_index == -1 )
            IssueError(MGF::Do_by_must_have_a_varying_variable_8003);

        NextToken();

        do_node.counter_increment_by_expression = exprlog();
    }

    // read the loop contents (using false because the local symbol stack was created above)
    do_node.block_program_index = instruc_COMPILER_DLL_TODO(false);

    IssueErrorOnTokenMismatch(TOKENDDO, MGF::expecting_enddo_keyword_10);

    NextToken();

    // ideally we would check for a semicolon but this was allowed
    // without a semicolon for a long time so it will not be checked

    return WrapNodeAroundScopeChange(local_symbol_stack, GetProgramIndex(do_node));
}


int LogicCompiler::CompileNextOrBreakInLoop()
{
    FunctionCode function_code = ( Tkn == TOKNEXT )  ? FunctionCode::FORNEXT_CODE :
                                 ( Tkn == TOKBREAK ) ? FunctionCode::FORBREAK_CODE :
                                                       throw ProgrammingErrorException();

    if( GetLoopStack().GetLoopStackCount() == 0 )
        IssueError(MGF::NextBreak_must_be_used_in_loop_8004, Tokstr.c_str());

    NextToken();

    auto& statement_node = CreateNode<Nodes::Statement>(function_code);

    statement_node.next_st = -1;

    // ideally we would check for a semicolon but this was allowed
    // without a semicolon for a long time so it will not be checked

    return GetProgramIndex(statement_node);
}
