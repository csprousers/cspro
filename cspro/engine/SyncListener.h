#pragma once

#include <zSyncO/ISyncListener.h>


// Interface for reporting error messages.
struct ISyncListenerErrorReporter
{
    virtual ~ISyncListenerErrorReporter() {}
    virtual void OnError(int message_number, va_list parg) = 0;
    virtual std::wstring Format(int message_number, va_list parg) = 0;
};


class SyncListener : public ISyncListener
{
public:
    /// <summary>Construct listener for sync client</summary>
    SyncListener(std::shared_ptr<ISyncListenerErrorReporter> error_reporter);

    // Called when first starting a sync operation (connect, sync_data, sync_file...)
    // Message number is from the system runtime message file
    // and remaining arguments are fills in the message where there are %s or %d.
    virtual void onStart(int messageNumber, ...) override;

    // Called when sync operation is complete
    virtual void onFinish() override;

    // Update progress.
    // Tick any progress UI and check for cancellation.
    // This version is for indeterminate progress.
    virtual void onProgress() override;

    // Update progress with amount complete.
    virtual void onProgress(int64_t progress) override;

    // Update progress with message.
    // Message number is from the system runtime message file
    // and remaining arguments are fills in the message where there are %s or %d.
    virtual void onProgress(int64_t progress, int messageNumber, ...) override;

    void setProgressTotal(int64_t total) override;

    int64_t getProgressTotal() const override;

    void addToProgressPreviousStepsTotal(int64_t update) override;

    // Pause updating the progress indicator and leave it at its current level
    void showProgressUpdates(bool show) override;

    // Check if the user has requested that the operation be cancelled
    // Depending on the threading implementation, this may be only up
    // to date as of the last call to onProgress.
    bool isCancelled() const override;

    // Called to show an error message
    // Message number is from the system runtime message file
    // and remaining arguments are fills in the message where there are %s or %d.
    void onError(int messageNumber, ...) override;

private:
    void showDialog(const CString& msg);
    void hideDialog();
    void updateDialog(int progressPercent, CString const* msg);

private:
    SyncListenerProgressHelper m_progressHelper;
    std::shared_ptr<ISyncListenerErrorReporter> m_errorReporter;
    volatile bool m_bCancelled;
};
