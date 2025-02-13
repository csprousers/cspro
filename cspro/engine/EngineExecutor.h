#pragma once

#include <engine/StandardSystemIncludes.h>
#include <engine/INTERPRE.H>
#include <engine/InterpreterAccessor.h>
#include <engine/ProgramControl.h>


template<typename CF>
bool CIntDriver::Execute(CF callback_function)
{
    ASSERT(!m_caughtProgramControlException);

    // these statements clear any preexisting stuff that might have been going on
    m_iSkipStmt = FALSE;
    m_iStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        callback_function();
    }

    catch( const ProgramControlException& )
    {
        m_caughtProgramControlException = std::current_exception();
    }

    m_iStopExec = ( m_iSkipStmt || m_bStopProc );

    return ( ( m_caughtProgramControlException ) ||
             ( m_iStopExec != 0 ) ||
             ( GetRequestIssued() ) );
}


template<typename CF>
InterpreterExecuteResult CIntDriver::Execute(DataType callback_result_data_type, CF callback_function)
{
    std::variant<double, std::wstring> result;
    bool program_control_executed = Execute([&]() { result = callback_function(); });

    ASSERT(std::holds_alternative<double>(result));

    if( callback_result_data_type == DataType::String )
        result = CharacterObjectToString(std::get<double>(result));

    return InterpreterExecuteResult { std::move(result), program_control_executed };
}
