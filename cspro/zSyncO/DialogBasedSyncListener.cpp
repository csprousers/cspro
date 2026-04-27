#include "stdafx.h"
#include "DialogBasedSyncListener.h"
#include <zMessageO/SystemMessageIssuer.h>
#include <zUtilF/ThreadedProgressDlg.h>


DialogBasedSyncListener::DialogBasedSyncListener(std::shared_ptr<SystemMessageIssuer> system_message_issuer)
    :   SyncListener(system_message_issuer),
        m_systemMessageIssuer(std::move(system_message_issuer))
{
}


DialogBasedSyncListener::~DialogBasedSyncListener()
{
}


void DialogBasedSyncListener::OnError(const int message_number, const std::string& message_text)
{
    // Close the progress dialog before showing the error.
    OnFinish();

    if( m_systemMessageIssuer == nullptr )
    {
        ErrorMessage::Display(message_text);
    }

    else
    {
        m_systemMessageIssuer->IssueFormattedMessage(MessageType::Error, message_number, message_text);
    }
}


#ifdef WIN_DESKTOP

void DialogBasedSyncListener::OnStart(const std::string& message_text)
{
    if( m_threadedProgressDlg == nullptr )
    {
        m_threadedProgressDlg = std::make_unique<ThreadedProgressDlg>();
        m_threadedProgressDlg->SetTitle("CSPro Synchronization");
        m_threadedProgressDlg->UseMarqueeStyle();
    }

    m_threadedProgressDlg->SetStatus(message_text);
    m_threadedProgressDlg->Show();
}


void DialogBasedSyncListener::OnFinish()
{
    m_threadedProgressDlg.reset();
}


void DialogBasedSyncListener::OnProgress(const int reportable_progress, const cs::cref_optional<std::string> message_text)
{
    if( m_threadedProgressDlg == nullptr )
        return;

    if( message_text.has_value() )
        m_threadedProgressDlg->SetStatus(*message_text);

    ASSERT(( reportable_progress >= 0 && reportable_progress <= 100 ) ||
           ( reportable_progress == SyncListener::IndeterminateProgress || reportable_progress == SyncListener::NoProgressUpdate ));

    m_threadedProgressDlg->SetPos(reportable_progress);
}


bool DialogBasedSyncListener::IsCanceled() const
{
    return ( m_threadedProgressDlg != nullptr ) ? m_threadedProgressDlg->IsCanceled() :
                                                  false;
}


#else

void DialogBasedSyncListener::OnStart(const std::string& message_text)
{
    m_canceled = false;
    PlatformInterface::GetInstance()->GetApplicationInterface()->ShowProgressDialog(message_text);
}


void DialogBasedSyncListener::OnFinish()
{
    PlatformInterface::GetInstance()->GetApplicationInterface()->HideProgressDialog();
}


void DialogBasedSyncListener::OnProgress(const int reportable_progress, const cs::cref_optional<std::string> message_text)
{
    // If the message returns true that means the cancel button was hit.
    // If it returns false it doesn't neccessarily mean that it wasn't cancelled,
    // could be that the dialog is already gone.
    if( PlatformInterface::GetInstance()->GetApplicationInterface()->UpdateProgressDialog(reportable_progress, message_text.get()) )
        m_canceled = true;
}


bool DialogBasedSyncListener::IsCanceled() const
{
    return m_canceled;
}

#endif
