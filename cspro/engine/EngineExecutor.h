#pragma once

#include <engine/StandardSystemIncludes.h>
#include <engine/Interpreter.h>
#include <engine/InterpreterAccessor.h>
#include <engine/ProgramControl.h>


template<typename CF>
InterpreterExecuteResult CIntDriver::Execute(const CF callback_function)
{
    ASSERT(!m_caughtProgramControlException);

    InterpreterExecuteResult execute_result { Engine::Value::Undefined<double>(), false };

    // these statements clear any preexisting stuff that might have been going on
    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        execute_result.result = callback_function();
    }

    catch( const ProgramControlException& )
    {
        m_caughtProgramControlException = std::current_exception();
    }

    m_bStopExec = ( m_bSkipStmt || m_bStopProc );

    if( m_caughtProgramControlException || m_bStopExec || GetRequestIssued() )
        execute_result.program_control_executed = true;

    return execute_result;
}
