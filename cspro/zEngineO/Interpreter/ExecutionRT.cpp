#include "stdafx.h"
#include "IncludesRT.h"


void LogicInterpreter::RethrowProgramControlExceptions()
{
    if( m_caughtProgramControlException )
        std::rethrow_exception(std::exchange(m_caughtProgramControlException, nullptr));
}
