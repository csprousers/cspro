#include "stdafx.h"
#include "IncludesRT.h"
#include "ProgramControlException.h"
#include <engine/InterpreterAccessor.h>


bool LogicInterpreter::IsExecutionInterrupted() const noexcept
{
    return ( m_caughtProgramControlException ||
             m_bStopProc );
}


void LogicInterpreter::RethrowProgramControlExceptions()
{
    if( m_caughtProgramControlException )
        std::rethrow_exception(std::exchange(m_caughtProgramControlException, nullptr));
}


InterpreterExecuteResult LogicInterpreter::Execute(const std::function<Engine::Value()>& callback_function)
{
    ASSERT(!m_caughtProgramControlException);

    Execute_INTERPRETER_DLL_TODO(true);

    std::optional<Engine::Value> result;

    try
    {
        result = callback_function();
    }

    catch( const ProgramControlException& )
    {
        m_caughtProgramControlException = std::current_exception();
    }

    Execute_INTERPRETER_DLL_TODO(false);

    return InterpreterExecuteResult
    {
        result.has_value() ? std::move(*result) : Engine::Value::Undefined<double>(),
        IsExecutionInterrupted()
    };
}
