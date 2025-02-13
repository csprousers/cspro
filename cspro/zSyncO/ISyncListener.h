#pragma once

/**
** Interface for sync observer
** Pass an implementation of this class to the SyncClient
** and receive callbacks on error or progress.
*/
struct ISyncListener
{
    virtual ~ISyncListener() {}

    // Called when first starting a sync operation (connect, sync_data, sync_file...)
    // Message number is from the system runtime message file
    // and remaining arguments are fills in the message where there are %s or %d.
    virtual void onStart(int messageNumber, ...) = 0;

    // Called when sync operation is complete
    virtual void onFinish() = 0;

    // Update progress.
    // Tick any progress UI and check for cancellation.
    // This version runs event loop but does not update progress bar.
    virtual void onProgress() = 0;

    // Update progress.
    // Tick any progress UI and check for cancellation.
    virtual void onProgress(int64_t progress) = 0;

    // Update progress with message.
    // Message number is from the system runtime message file
    // and remaining arguments are fills in the message where there are %s or %d.
    virtual void onProgress(int64_t progress, int messageNumber, ...) = 0;

    // Set the denominator to use to calculate % complete for progress bar.
    // When onProgress is called with a value it will be divided by this number.
    // Use -1 to indicate total is unknown to show interderminate progress indicator.
    virtual void setProgressTotal(int64_t total) = 0;
    virtual int64_t getProgressTotal() const = 0;

    // For multi-step operations set total so far from previous steps.
    // This will be added to the value from onProgress before dividing by
    // total to compute % complete.
    virtual void addToProgressPreviousStepsTotal(int64_t update) = 0;

    // Pause updating the progress indicator and leave it at its current level
    virtual void showProgressUpdates(bool show) = 0;

    // Check if the user has requested that the operation be cancelled
    // Depending on the threading implementation, this may be only up
    // to date as of the last call to onProgress.
    virtual bool isCancelled() const = 0;

    // Report an error
    // Message number is from the system runtime message file
    // and remaining arguments are fills in the message where there are %s or %d.
    virtual void onError(int messageNumber, ...) = 0;
};


// Helper class to close listener when method exits
// even when exceptions are thrown.
class SyncListenerCloser
{
public:
    SyncListenerCloser(ISyncListener* pListener)
        : m_pListener(pListener)
    {
    }

    ~SyncListenerCloser()
    {
        if (m_pListener)
            m_pListener->onFinish();
    }

private:
    ISyncListener* m_pListener;
};


class SyncListenerProgressHelper
{
public:
    SyncListenerProgressHelper()
    {
        reset();
    }

    void reset()
    {
        m_progressTotal = -1;
        m_progressPreviousStepsTotal = 0;
        m_showProgressUpdates = true;
    }

    enum { INDETERMINATE_PROGRESS = -1, NO_PROGRESS_UPDATE = -2 };

    float progressPercent(int64_t progress)
    {
        if (!m_showProgressUpdates)
            return NO_PROGRESS_UPDATE;

        if (m_progressTotal <= 0)
            return INDETERMINATE_PROGRESS;

        if (progress < 0)
            return NO_PROGRESS_UPDATE;

        return float(progress + m_progressPreviousStepsTotal) / m_progressTotal;
    }

    int64_t m_progressTotal;
    int64_t m_progressPreviousStepsTotal;
    bool m_showProgressUpdates;
};
