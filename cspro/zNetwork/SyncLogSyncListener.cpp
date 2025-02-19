#include "stdafx.h"
#include "SyncLogSyncListener.h"


void SyncLogSyncListener::OnStart(const std::string& message_text)
{
    SyncLog::EnableLogging();

    SYNCLOG_INFO << "Sync Operation #" << ++m_operationIndex << " Starting: " << message_text;
}


void SyncLogSyncListener::OnFinish()
{
    SYNCLOG_INFO << "Sync Operation #" << m_operationIndex << " Finished";
}



void SyncLogSyncListener::OnProgress(const int reportable_progress, const cs::cref_optional<std::string> message_text)
{
    if( reportable_progress == m_lastReportedProgress && !message_text.has_value() )
        return;

    std::string progress_text = message_text.has_value() ? *message_text :
                                                           std::string();

    // display a text-based progress bar
    if( reportable_progress >= 0 )
    {
        constexpr int ProgressBarWidth = 20;
        const int text_percent = reportable_progress * ProgressBarWidth / 100;

        progress_text.append(FormatText("%s%s%s] %3d%%", progress_text.empty() ? "[" : " [",
                                                         SO::GetRepeatingCharacterString('*', text_percent),
                                                         SO::GetRepeatingCharacterString(' ', ProgressBarWidth - text_percent),
                                                         reportable_progress));
    }

    SYNCLOG_INFO << "Sync Operation #" << m_operationIndex << " Progress: " << progress_text;

    m_lastReportedProgress = reportable_progress;
}


bool SyncLogSyncListener::IsCanceled() const
{
    return false;
}


void SyncLogSyncListener::OnError(int /*message_number*/, const std::string& message_text)
{
    SYNCLOG_INFO << "Sync Error: " << message_text;
}
