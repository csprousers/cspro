#include "StandardSystemIncludes.h"
#include "SyncListener.h"
#include "Engine.h"
#include <zPlatformO/PlatformInterface.h>
#include <zUtilO/Interapp.h>
#include <zMessageO/SystemMessageIssuer.h>

namespace {
    // Can't easily send a float as wParam so we convert to fixed point.
    int PROGRESS_FIXED_POINT_MULTIPLIER = 10000;

    int progressToFixed(float progress)
    {
        return progress < 0 ?
            (int) progress :
            int(PROGRESS_FIXED_POINT_MULTIPLIER * progress);
    }
};

SyncListener::SyncListener(std::shared_ptr<ISyncListenerErrorReporter> error_reporter)
    :   m_errorReporter(std::move(error_reporter)),
        m_bCancelled(false)
{
}

void SyncListener::onStart(int messageNumber, ...)
{
    m_bCancelled = false;
    m_progressHelper.reset();

    va_list args;
    va_start(args, messageNumber);
    CString msg = WS2CS(m_errorReporter->Format(messageNumber, args));
    va_end(args);
    showDialog(msg);
}

void SyncListener::onFinish()
{
    hideDialog();
}

void SyncListener::onProgress()
{
    updateDialog(progressToFixed(m_progressHelper.progressPercent(-1)), nullptr);
}

void SyncListener::onProgress(int64_t progress)
{
    updateDialog(progressToFixed(m_progressHelper.progressPercent(progress)), nullptr);
}

void SyncListener::onProgress(int64_t progress, int messageNumber, ...)
{
    va_list args;
    va_start(args, messageNumber);
    CString msg = WS2CS(m_errorReporter->Format(messageNumber, args));
    va_end(args);
    updateDialog(progressToFixed(m_progressHelper.progressPercent(progress)), &msg);
}

void SyncListener::setProgressTotal(int64_t total)
{
    m_progressHelper.m_progressTotal = total;
}

int64_t SyncListener::getProgressTotal() const
{
    return m_progressHelper.m_progressTotal;
}

void SyncListener::addToProgressPreviousStepsTotal(int64_t update)
{
    m_progressHelper.m_progressPreviousStepsTotal += update;
}

void SyncListener::showProgressUpdates(bool show)
{
    m_progressHelper.m_showProgressUpdates = show;
}

bool SyncListener::isCancelled() const
{
    return m_bCancelled;
}

void SyncListener::onError(int messageNumber, ...)
{
    // Close the progress dialog before showing the error.
    onFinish();

    // Pass the error on to the usual error dialog box (issaerror)
    va_list parg;
    va_start(parg, messageNumber);
    m_errorReporter->OnError(messageNumber, parg);
    va_end(parg);
}

void SyncListener::showDialog(const CString& msg)
{
#ifndef WIN_DESKTOP
    PlatformInterface::GetInstance()->GetApplicationInterface()->ShowProgressDialog(msg);
#else
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_PROGRESS_DIALOG_SHOW, (WPARAM)0, (LPARAM) &msg);
#endif
}

void SyncListener::hideDialog()
{
#ifndef WIN_DESKTOP
    PlatformInterface::GetInstance()->GetApplicationInterface()->HideProgressDialog();
#else
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_PROGRESS_DIALOG_HIDE, (WPARAM) 0, (LPARAM) 0);
#endif
}

void SyncListener::updateDialog(int progressPercent, CString const* msg)
{
#ifndef WIN_DESKTOP
    bool bResult = PlatformInterface::GetInstance()->GetApplicationInterface()->UpdateProgressDialog(progressPercent, msg);
#else
    bool bResult = AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_PROGRESS_DIALOG_UPDATE, (WPARAM) progressPercent, (LPARAM) msg) != 0;
#endif

    // If the message returns true that means the cancel button was hit.
    // If it returns false it doesn't neccessarily mean that it wasn't cancelled,
    // could be that the dialog is already gone.
    if (bResult) {
        m_bCancelled = true;
    }
}


