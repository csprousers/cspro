#include "StandardSystemIncludes.h"
#include "INTERPRE.H"


void CIntDriver::RethrowProgramControlExceptions()
{
    if( m_caughtProgramControlException )
    {
        auto exception = m_caughtProgramControlException;
        m_caughtProgramControlException = nullptr;
        std::rethrow_exception(exception);
    }
}
