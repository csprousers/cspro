#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/SyncListener.h>


class ZNETWORK_API SyncLogSyncListener : public SyncListener
{
public:
    using SyncListener::SyncListener;

protected:
    void OnStart(const std::string& message_text) override;
    void OnFinish() override;
    void OnProgress(int reportable_progress, cs::cref_optional<std::string> message_text) override;
    bool IsCanceled() const override;
    void OnError(int message_number, const std::string& message_text) override;

private:
    int m_operationIndex = 0;
    int m_lastReportedProgress = -1;
};
