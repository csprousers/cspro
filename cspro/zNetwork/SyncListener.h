#pragma once

#include <zNetwork/zNetwork.h>

class Case;
class SyncError;
class SystemMessageFormatter;


// --------------------------------------------------------------------------
// SyncListener
//
// Base class for a sync observer.
//
// Pass an implementation of this class to the SyncClient and receive
// callbacks on error or progress.
// --------------------------------------------------------------------------

class ZNETWORK_API SyncListener
{
    friend class WrapperSyncListener;

public:
    static constexpr int IndeterminateProgress = -1;
    static constexpr int NoProgressUpdate      = -2;

    SyncListener(std::shared_ptr<SystemMessageFormatter> system_message_formatter = nullptr);
    virtual ~SyncListener() { }

    // Called when first starting a sync operation (connect, sync data, sync file, etc.).
    // Message number is from the system runtime message file and remaining arguments
    // are fills in the message where there are %s or %d.
    template<typename... Args>
    void Start(int message_number, Args const&... args)
    {
        ResetProgress();
        OnStart(GetFormattedMessage(message_number, args...));
    }

    // Called when sync operation is complete.
    void Finish()
    {
        OnFinish();
    }

    // Update progress.
    // Tick any progress UI and check for cancellation.
    // This version runs event loop but does not update progress bar.
    void Progress()
    {
        OnProgress(GetReportableProgress(IndeterminateProgress), cs::cref_optional<std::string>());
    }

    // Update progress.
    // Tick any progress UI and check for cancellation.
    void Progress(int64_t progress)
    {
        OnProgress(GetReportableProgress(progress), cs::cref_optional<std::string>());
    }

    // Update progress with message.
    // Message number is from the system runtime message file
    // and remaining arguments are fills in the message where there are %s or %d.
    template<typename... Args>
    void Progress(int64_t progress, int message_number, Args const&... args)
    {
        OnProgress(GetReportableProgress(progress), GetFormattedMessage(message_number, args...));
    }

    // Set the denominator to use to calculate % complete for progress bar.
    // When Progress is called with a value it will be divided by this number.
    // Use -1 to indicate total is unknown to show interderminate progress indicator.
    void SetProgressTotal(int64_t total) { m_progressTotal = total; }
    int64_t GetProgressTotal() const     { return m_progressTotal; }

    // For multi-step operations set total so far from previous steps.
    // This will be added to the value from Progress before dividing by
    // total to compute % complete.
    void AddToProgressPreviousStepsTotal(int64_t update) { m_progressPreviousStepsTotal += update; }

    // Pause updating the progress indicator and leave it at its current level.
    void ShowProgressUpdates(bool show) { m_showProgressUpdates = show; }

    // Provides information on the last case synced.
    virtual void SetLastCaseSynced(const Case& data_case, bool received) { data_case; received; }

    // Check if the user has requested that the operation be cancelled
    // Depending on the threading implementation, this may be only up
    // to date as of the last call to Progress.
    virtual bool IsCanceled() const = 0;

    // Report an error.
    // Message number is from the system runtime message file
    // and remaining arguments are fills in the message where there are %s or %d.
    template<typename... Args>
    void ReportError(int message_number, Args const&... args)
    {
        OnError(message_number, GetFormattedMessage(message_number, args...));
    }

    // Report an error from a caught exception.
    void ReportError(const CSProException& exception);
    void ReportError(const SyncError& exception);

protected:
    // methods for subclasses to implement (in addition to IsCanceled)
    virtual void OnStart(const std::string& message_text) = 0;
    virtual void OnFinish() = 0;
    virtual void OnProgress(int reportable_progress, cs::cref_optional<std::string> message_text) = 0;
    virtual void OnError(int message_number, const std::string& message_text) = 0;

private:
    void ResetProgress();
    int GetReportableProgress(int64_t progress) const;

    std::string GetFormattedMessageWorker(int message_number, ...) const;

    template<typename... Args>
    std::string GetFormattedMessage(int message_number, Args const&... args) const
    {
#ifdef _DEBUG
        ValidateFormatTextArgumentTypes(args...);
#endif
        return GetFormattedMessageWorker(message_number, args...);
    }

protected:
    int64_t m_progressTotal;
    int64_t m_progressPreviousStepsTotal;
    bool m_showProgressUpdates;

private:
    std::shared_ptr<SystemMessageFormatter> m_systemMessageFormatter; // non-null
};
