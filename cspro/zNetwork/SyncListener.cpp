#include "stdafx.h"
#include "SyncListener.h"
#include <zMessageO/SystemMessageIssuer.h>
#include <zEngineO/Messages/EngineMessages.h>


SyncListener::SyncListener(std::shared_ptr<SystemMessageFormatter> system_message_formatter)
    :   m_systemMessageFormatter(std::move(system_message_formatter))
{
    if( m_systemMessageFormatter == nullptr )
        m_systemMessageFormatter = std::make_unique<SystemMessageFormatter>();

    ResetProgress();
}


void SyncListener::ResetProgress()
{
    m_progressTotal = -1;
    m_progressPreviousStepsTotal = 0;
    m_showProgressUpdates = true;
}


int SyncListener::GetReportableProgress(const int64_t progress) const
{
    if( !m_showProgressUpdates )
        return NoProgressUpdate;

    if( m_progressTotal <= 0 )
        return IndeterminateProgress;

    if( progress < 0 )
        return NoProgressUpdate;

    return std::min(100, CreatePercent(progress + m_progressPreviousStepsTotal, m_progressTotal));
}


void SyncListener::ReportError(const CSProException& exception)
{
    const SyncError* const sync_error = dynamic_cast<const SyncError*>(&exception);

    if( sync_error != nullptr )
    {
        ReportError(*sync_error);
    }

    else
    {
        ReportError(MGF::sync_generic_error_100114, exception.what());
    }
}


void SyncListener::ReportError(const SyncError& exception)
{
    ReportError(exception.GetErrorMessageNumber(), exception.what());
}


std::string SyncListener::GetFormattedMessageWorker(const int message_number, ...) const
{
    va_list parg;
    va_start(parg, message_number);
    std::string message_text = m_systemMessageFormatter->GetFormattedMessageVA(message_number, parg);
    va_end(parg);
    return message_text;
}
