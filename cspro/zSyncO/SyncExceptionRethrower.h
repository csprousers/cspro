#pragma once

#include <zJson/JsonParseException.h>
#include <zNetwork/SyncException.h>
#include <zNetwork/SyncLog.h>


// logs and rethrows as a SyncError any exception caught by a sync routine

inline void RethrowAsSyncError(std::exception_ptr caught_exception)
{
    ASSERT(caught_exception);

    try
    {
        std::rethrow_exception(caught_exception);
    }

    catch( const SyncError& exception )
    {
        SYNCLOG_ERROR << "SyncError reading server cases: " << exception.what();
        throw;
    }

    catch( const JsonParseException& exception )
    {
        SYNCLOG_ERROR << "Invalid JSON parsing server cases: " << exception.what();
        throw SyncError(100121);
    }

    catch( const std::exception& exception )
    {
        SYNCLOG_ERROR << "Error reading server cases: " << exception.what();
        throw SyncError(100121);
    }
}
