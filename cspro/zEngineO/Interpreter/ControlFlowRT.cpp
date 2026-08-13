#include "stdafx.h"
#include "IncludesRT.h"
#include "LoopStack.h"
#include "ProgramControlException.h"
#include "UserFunction.h"
#include "WorkVariable.h"
#include "Nodes/ControlFlow.h"


Engine::Value LogicInterpreter::ex_if(const int program_index)
{
    const auto& if_node = GetNode<Nodes::If>(program_index);

    const bool condition_is_true = EvaluateConditional(if_node.conditional_expression);

    // exit immediately if a request was issued in the conditional check
    // (e.g., from a reenter in a user-defined function)
    if( GetRequestIssued_INTERPRETER_DLL_TODO() )
        return Engine::Value::Undefined<double>();

    return ExecuteInstructions(condition_is_true ? if_node.then_program_index :
                                                   if_node.else_program_index);
}


Engine::Value LogicInterpreter::ex_while(const int program_index)
{
    const auto& while_node = GetNode<Nodes::While>(program_index);

    const LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::While);
    ASSERT(loop_stack_entry.IsValid());

    std::optional<Engine::Value> last_evaluated_value;

    while( EvaluateConditional(while_node.conditional_expression) )
    {
        try
        {
            last_evaluated_value = ExecuteInstructions(while_node.block_program_index);
        }

        catch( const NextProgramControlException& )  { }
        catch( const BreakProgramControlException& ) { break; }

        catch( LogicStackSaver& logic_stack_saver )
        {
            // add the while loop statement to the logic stack so that it is evaluated after any
            // additional statements in the loop
            logic_stack_saver.PushStatement(program_index);

            // the next statement should not be added because it will be evaluated after this
            // while loop is executed
            logic_stack_saver.SuppressNextPush(while_node.next_st);

            throw;
        }

        if( Get_m_bStopExec_INTERPRETER_DLL_TODO() )
            break;
    }

    // return the last evaluated value
    if( last_evaluated_value.has_value() )
        return std::move(*last_evaluated_value);

    return Engine::Value::Undefined<double>();
}


Engine::Value LogicInterpreter::ex_do(const int program_index)
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
        Symbol& symbol = NPT_Ref(counter_symbol_value_node->symbol_index);

        if( symbol.IsA(SymbolType::WorkVariable) )
            counter_work_variable_address = assert_cast<WorkVariable&>(symbol).GetValueAddress();

        const double initial_value = Evaluate<double>(do_node.counter_initial_value_expression);
        AssignValueToSymbol(*counter_symbol_value_node, initial_value);
    }

    const bool check_conditional_value_is_true = ( do_node.loop_type == TokenCode::TOKWHILE );
    const bool increment_variable_by_one = ( counter_symbol_value_node != nullptr &&
                                             do_node.counter_increment_by_expression == -1 );
    const bool increment_work_variable_by_one = ( increment_variable_by_one &&
                                                  counter_work_variable_address != nullptr );

    std::optional<Engine::Value> last_evaluated_value;

    while( EvaluateConditional(do_node.conditional_expression) == check_conditional_value_is_true )
    {
        try
        {
            last_evaluated_value = ExecuteInstructions(do_node.block_program_index);
        }

        catch( const NextProgramControlException& )  { }
        catch( const BreakProgramControlException& ) { break; }

        if( Get_m_bStopExec_INTERPRETER_DLL_TODO() )
            break;

        if( increment_work_variable_by_one )
        {
            ++(*counter_work_variable_address);
        }

        else if( counter_symbol_value_node != nullptr )
        {
            const double increment_value = increment_variable_by_one
                ? 1
                : Evaluate<double>(do_node.counter_increment_by_expression);

            if( counter_work_variable_address != nullptr )
            {
                *counter_work_variable_address += increment_value;
            }

            else
            {
                ModifySymbolValue<double>(
                    *counter_symbol_value_node,
                    [increment_value](double& value) { value += increment_value; }
                );
            }
        }
    }

    // return the last evaluated value
    if( last_evaluated_value.has_value() )
        return std::move(*last_evaluated_value);

    return Engine::Value::Undefined<double>();
}


Engine::Value LogicInterpreter::ex_for_next(int /*program_index*/)
{
    ASSERT(GetLoopStack().GetLoopStackCount() > 0);

    throw NextProgramControlException();
}


Engine::Value LogicInterpreter::ex_for_break(int /*program_index*/)
{
    ASSERT(GetLoopStack().GetLoopStackCount() > 0);

    throw BreakProgramControlException();
}


Engine::Value LogicInterpreter::ex_exit(const int program_index)
{
    const auto& statement_node = GetNode<Nodes::StatementWithArguments>(program_index);

    if( statement_node.expressions[0] != -1 )
    {
        // set the user function's return value
        UserFunction& user_function = GetSymbolUserFunction(statement_node.expressions[0]);
        user_function.SetReturnValue(Evaluate<Engine::Value>(statement_node.expressions[1]));
    }

    throw ExitProgramControlException();
}
