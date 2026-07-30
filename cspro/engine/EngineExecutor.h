#pragma once

#include <engine/StandardSystemIncludes.h>
#include <engine/Interpreter.h>
#include <engine/InterpreterAccessor.h>
#include <engine/ProgramControl.h>


template<typename CF>
bool CIntDriver::Execute(const CF callback_function)
{
    ASSERT(!m_caughtProgramControlException);

    // these statements clear any preexisting stuff that might have been going on
    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        callback_function();
    }

    catch( const ProgramControlException& )
    {
        m_caughtProgramControlException = std::current_exception();
    }

    m_bStopExec = ( m_bSkipStmt || m_bStopProc );

    return ( m_caughtProgramControlException ||
             m_bStopExec ||
             GetRequestIssued() );
}


template<typename CF>
InterpreterExecuteResult CIntDriver::Execute(const DataType callback_result_data_type, const CF callback_function)
{
    std::variant<double, SharableString> result;
    bool program_control_executed = Execute([&]() { result = callback_function(); });

    ASSERT(std::holds_alternative<double>(result));

    if( callback_result_data_type == DataType::String )
        result = GetWorkingSharableString(std::get<double>(result));

    return InterpreterExecuteResult { std::move(result), program_control_executed };
}
