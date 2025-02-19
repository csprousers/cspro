#pragma once

#include <DataManager/TaskRunner.h>
#include <zNetwork/SyncListener.h>


class TaskRunnerSyncListener : public SyncListener
{
public:
    TaskRunnerSyncListener(TaskRunner* const task_runner, const bool* const cancel_flag)
        :   m_taskRunner(task_runner),
            m_cancelFlag(cancel_flag)
    {
        ASSERT(m_taskRunner != nullptr && cancel_flag != nullptr);
        m_taskRunner->SetTitle("CSPro Synchronization");
    }

protected:
    void OnStart(const std::string& message_text) override
    {
        m_taskRunner->UpdateProgress(IndeterminateProgress);
        m_taskRunner->LogText(message_text);
    }

    void OnFinish() override
    {
        m_taskRunner->UpdateProgress(100);
    }

    void OnProgress(const int reportable_progress, const cs::cref_optional<std::string> message_text) override
    {
        m_taskRunner->UpdateProgress(reportable_progress);

        if( message_text.has_value() )
            m_taskRunner->LogText(*message_text);
    }

    void SetLastCaseSynced(const Case& data_case, const bool received) override
    {
        m_taskRunner->LogText("Last case %s: %s", received ? "received" : "sent", data_case.GetKey().c_str());
    }

    bool IsCanceled() const override
    {
        return *m_cancelFlag;
    }

    void OnError(int /*message_number*/, const std::string& message_text) override
    {
        m_taskRunner->LogText(message_text);
    }

private:
    TaskRunner* m_taskRunner;
    const bool* m_cancelFlag;
};
