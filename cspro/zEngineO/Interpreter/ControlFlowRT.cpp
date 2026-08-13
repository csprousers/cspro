#include "stdafx.h"
#include "IncludesRT.h"
#include "Nodes/ControlFlow.h"


double CIntDriver::exif(int iExpr)
{
    const auto& if_node = GetNode<Nodes::If>(iExpr);

    const bool condition_is_true = EvaluateConditional(if_node.conditional_expression);

    // if a request was issued in the conditional check (e.g., from a reenter in a
    // user-defined function), then exit immediately
    if( GetRequestIssued() )
        return 0;

    ExecuteProgramStatements(condition_is_true ? if_node.then_program_index :
                                                 if_node.else_program_index);

    return 0;
}


double CIntDriver::exwhile(int iExpr)
{
    const auto& while_node = GetNode<Nodes::While>(iExpr);

    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::While);
    ASSERT(loop_stack_entry.IsValid());

    while( EvaluateConditional(while_node.conditional_expression) )
    {
        try
        {
            ExecuteProgramStatements(while_node.block_program_index);
        }

        catch( const NextProgramControlException& )  { }
        catch( const BreakProgramControlException& ) { break; }

        catch( LogicStackSaver& logic_stack_saver )
        {
            // add the while loop statement to the logic stack so that it is evaluated after any
            // additional statements in the loop
            logic_stack_saver.PushStatement(iExpr);

            // the next statement should not be added because it will be evaluated after this
            // while loop is executed
            logic_stack_saver.SuppressNextPush(while_node.next_st);

            throw;
        }

        if( m_bStopExec )
            break;
    }

    return 0;
}


double CIntDriver::ex_do(const int program_index)
{
    const auto& do_node = GetNode<Nodes::Do>(program_index);
    const Nodes::SymbolValue* counter_symbol_value_node = nullptr;
    double* counter_work_variable_address = nullptr;

    const LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::Do);
    ASSERT(loop_stack_entry.IsValid());

    if( do_node.counter_symbol_value_node_index != -1 )
    {
        counter_symbol_value_node = &GetNode<Nodes::SymbolValue>(do_node.counter_symbol_value_node_index);

        // work variables, since they will make up the vast majority of loop counters,
        // will be handled in a special way to make the iterations more efficient
        if( NPT_Ref(counter_symbol_value_node->symbol_index).IsA(SymbolType::WorkVariable) )
            counter_work_variable_address = GetSymbolWorkVariable(counter_symbol_value_node->symbol_index).GetValueAddress();

        const double initial_value = evalexpr(do_node.counter_initial_value_expression);
        AssignValueToSymbol(*counter_symbol_value_node, initial_value);
    }

    const bool check_conditional_value_is_true = ( do_node.loop_type == TOKWHILE );
    const bool increment_variable_by_one = ( counter_symbol_value_node != nullptr && do_node.counter_increment_by_expression == -1 );
    const bool increment_work_variable_by_one = ( increment_variable_by_one && counter_work_variable_address != nullptr );

    while( EvaluateConditional(do_node.conditional_expression) == check_conditional_value_is_true )
    {
        try
        {
            ExecuteProgramStatements(do_node.block_program_index);
        }

        catch( const NextProgramControlException& )  { }
        catch( const BreakProgramControlException& ) { break; }

        if( m_bStopExec )
            break;

        if( increment_work_variable_by_one )
        {
            ++(*counter_work_variable_address);
        }

        else if( counter_symbol_value_node != nullptr )
        {
            double increment_value = increment_variable_by_one ? 1 : evalexpr(do_node.counter_increment_by_expression);

            if( counter_work_variable_address != nullptr )
            {
                *counter_work_variable_address += increment_value;
            }

            else
            {
                ModifySymbolValue<double>(*counter_symbol_value_node, [increment_value](double& value) { value += increment_value; });
            }
        }
    }

    return 0;
}


double CIntDriver::exfornext(int /*iExpr*/)
{
    ASSERT(GetLoopStack().GetLoopStackCount() > 0);

    throw NextProgramControlException();
}


double CIntDriver::exforbreak(int /*iExpr*/)
{
    ASSERT(GetLoopStack().GetLoopStackCount() > 0);

    throw BreakProgramControlException();
}


double CIntDriver::ex_exit(const int program_index)
{
    const auto& statement_node = GetNode<STN_NODE>(program_index);

    if( statement_node.arguments[0] != -1 )
    {
        // set the user function's return value
        UserFunction& user_function = GetSymbolUserFunction(statement_node.arguments[0]);
        user_function.SetReturnValue(Evaluate<Engine::Value>(statement_node.arguments[1]));
    }

    throw ExitProgramControlException();
}
