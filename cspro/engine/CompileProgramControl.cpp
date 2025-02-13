#include "StandardSystemIncludes.h"
#include "ExpresC_Include.h"
#include <zEngineO/LoopStack.h>
#include <zEngineO/UserFunction.h>


int CEngineCompFunc::CompileProgramControl()
{
    TokenCode program_control_token = Tkn;
    int program_index;

    auto get_statement_name = [&]
    {
        return ( program_control_token == TOKKWCASE ) ? _T("SKIP CASE") :
                                                        Logic::KeywordTable::GetKeywordName(program_control_token);
    };

    auto issue_error_if_not_batch_application = [&]
    {
        if( Appl.ApplicationType != ModuleType::Batch )
            IssueError(821, get_statement_name());
    };

    auto issue_error_if_level0_proc = [&]
    {
        if( LvlInComp == 0 )
            IssueError(822, get_statement_name());
    };

    
    // endcase
    if( program_control_token == TOKENDCASE )
    {
        issue_error_if_not_batch_application();
        issue_error_if_level0_proc();

        NextToken();

        auto& statement_node = CreateCompilationNode<ST_NODE>(FNENDCASE_CODE);
        statement_node.next_st = -1;

        program_index = GetCompilationNodeProgramIndex(statement_node);
    }


    // universe
    else if( program_control_token == TOKUNIVERSE )
    {
        NextToken();

        int condition_expression = exprlog();

        bool endcase_mode = ( Tkn == TOKKWCASE );

        if( endcase_mode )
        {
            issue_error_if_not_batch_application();
            issue_error_if_level0_proc();

            NextToken();
        }

        auto& statement_node = CreateVariableArgumentCompilationNode<STN_NODE>(FNUNIVERSE_CODE, 2);
        statement_node.next_st = -1;
        statement_node.arguments[0] = condition_expression;
        statement_node.arguments[1] = endcase_mode ? 1 : 0;

        program_index = GetCompilationNodeProgramIndex(statement_node);
    }


    // skip case (coming from CompileSkipStatement)
    else if( program_control_token == TOKKWCASE )
    {
        issue_error_if_not_batch_application();
        issue_error_if_level0_proc();

        NextToken();

        auto& statement_node = CreateCompilationNode<ST_NODE>(SKIPCASE_CODE);
        statement_node.next_st = -1;

        program_index = GetCompilationNodeProgramIndex(statement_node);
    }


    // exit
    else if( program_control_token == TOKEXIT )
    {
        NextToken();

        int user_function_symbol_index = -1;
        int exit_expression = -1;

        // in a user function, a return value can be specified
        if( Tkn != TOKSEMICOLON && NPT(InCompIdx)->IsA(SymbolType::UserFunction) )
        {
            user_function_symbol_index = InCompIdx;
            const UserFunction& user_function = GetSymbolUserFunction(user_function_symbol_index);

            exit_expression = CompileExpression(user_function.GetReturnDataType());
        }

        auto& statement_node = CreateVariableArgumentCompilationNode<STN_NODE>(EXIT_CODE, 2);
        statement_node.next_st = -1;
        statement_node.arguments[0] = user_function_symbol_index;
        statement_node.arguments[1] = exit_expression;

        program_index = GetCompilationNodeProgramIndex(statement_node);
    }


    else
    {
        return ReturnProgrammingError(-1);
    }


    // all program control statements should end in a semicolon
    IssueErrorOnTokenMismatch(TOKSEMICOLON, 30);

    return program_index;
}


int CEngineCompFunc::CompileEnter()
{
    // don't allow enter statements inside a function
    if( ObjInComp == SymbolType::Application )
        IssueError(562);

    // enter will be ignored in non-entry applications
    if( Appl.ApplicationType != ModuleType::Entry )
        IssueWarning(10547);

    // enter only allowed in a single while loop
    size_t loop_stack_count = GetLoopStack().GetLoopStackCount();

    if( loop_stack_count > 1 || loop_stack_count != GetLoopStack().GetLoopStackCount(LoopStackSource::While) )
        IssueError(10546);

    // check that the target is a secondary flow
    NextToken();

    int flow_symbol_index = Tokstindex;

    if( Tkn != TOKFLOW_PRE80 || LPT(flow_symbol_index)->GetSubType() != SymbolSubType::Secondary ) // FLOW_TODO support enter statement
        IssueError(10545);

#ifdef GENCODE
    // mark elements of entered dictionary to "used"
    int flow_dictionary_symbol_index = LPT(flow_symbol_index)->GetSymDicAt(0);
    m_pEngineArea->dicttrip(DPT(flow_dictionary_symbol_index), (pDictTripFunc)&CEngineArea::setup_vmark);
#endif

    auto& statement_node = CreateVariableArgumentCompilationNode<STN_NODE>(ENTER_CODE, 1);
    statement_node.next_st = -1;
    statement_node.arguments[0] = flow_symbol_index;

    NextToken();

    return GetCompilationNodeProgramIndex(statement_node);
}
