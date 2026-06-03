#pragma once

#include <zNetwork/ConnectResponse.h>
#include <zSyncO/SyncRunnerActionInvoker.h>


// --------------------------------------------------------------------------
// SyncServiceWrapper
// --------------------------------------------------------------------------

class ActionInvoker::Runtime::SyncServiceWrapper
{
public:
    SyncServiceWrapper(Runtime& runtime, std::unique_ptr<ActionInvokerSyncRunner> sync_runner, SyncConnectionString sync_connection_string,
                       std::shared_ptr<const ConnectResponse> connect_response, int64_t connection_start_time);

    static int GetSyncId(Runtime& runtime, const JsonNode& json_node, Caller& caller);
    static auto GetSyncServiceWrapper(Runtime& runtime, int sync_id);
    static auto GetSyncServiceWrapper(Runtime& runtime, const JsonNode& json_node, Caller& caller);
    static ActionInvokerSyncRunner& GetSyncRunner(Runtime& runtime, const JsonNode& json_node, Caller& caller);

    ActionInvokerSyncRunner& GetSyncRunner()                        { return *m_syncRunner; }
    const SyncConnectionString& GetSafeSyncConnectionString() const { return m_safeSyncConnectionString; }
    const ConnectResponse& GetConnectResponse() const               { return *m_connectResponse; }
    int64_t GetStartConnectionTime() const                          { return m_connectionStartTime; }

private:
    Runtime& m_runtime;
    std::unique_ptr<ActionInvokerSyncRunner> m_syncRunner;
    SyncConnectionString m_safeSyncConnectionString;
    std::shared_ptr<const ConnectResponse> m_connectResponse;
    int64_t m_connectionStartTime;
};
