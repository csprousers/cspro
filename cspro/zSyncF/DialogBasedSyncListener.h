#pragma once

#include <zSyncF/zSyncF.h>
#include <zNetwork/SyncListener.h>

class SystemMessageIssuer;
class ThreadedProgressDlg;


class ZSYNCF_API DialogBasedSyncListener : public SyncListener
{
public:
    // if system_message_issuer is null, error messages will be displayed using ErrorMessage::Display
    DialogBasedSyncListener(std::shared_ptr<SystemMessageIssuer> system_message_issuer);
    ~DialogBasedSyncListener();

protected:
    void OnStart(const std::string& message_text) override;
    void OnFinish() override;
    void OnProgress(int reportable_progress, cs::cref_optional<std::string> message_text) override;
    bool IsCanceled() const override;
    void OnError(int message_number, const std::string& message_text) override;

private:
    std::shared_ptr<SystemMessageIssuer> m_systemMessageIssuer;

#ifdef WIN_DESKTOP
    std::unique_ptr<ThreadedProgressDlg> m_threadedProgressDlg;
#else
    volatile bool m_canceled = false;
#endif
};
