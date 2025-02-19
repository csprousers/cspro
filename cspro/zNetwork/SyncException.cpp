#include "stdafx.h"
#include "SyncException.h"
#include <zMessageO/SystemMessageFormatter.h>


// --------------------------------------------------------------------------
// SyncErrorFormatter
// --------------------------------------------------------------------------

SyncErrorFormatter::SyncErrorFormatter()
{
}


SyncErrorFormatter::~SyncErrorFormatter()
{
}


std::string SyncErrorFormatter::GetFormattedError(const std::exception& exception)
{
    const SyncError* const sync_error = dynamic_cast<const SyncError*>(&exception);

    if( sync_error == nullptr )
        return exception.what();

    // format the message
    if( m_systemMessageFormatter == nullptr )
        m_systemMessageFormatter = std::make_unique<SystemMessageFormatter>();

    return m_systemMessageFormatter->GetFormattedMessage(sync_error->GetErrorMessageNumber(), sync_error->what());
}
