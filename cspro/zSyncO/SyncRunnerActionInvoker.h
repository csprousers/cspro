#pragma once

#include <zSyncO/zSyncO.h>

namespace ActionInvoker { class Caller; }
namespace Paradata { class Syncer; }
class SyncConnectionString;
enum class SyncDirection;


class ActionInvokerSyncRunner
{
public:
    virtual ~ActionInvokerSyncRunner() { }

    // Returns a non-null pointer to ActionInvokerSyncRunnerImpl, a class that the
    // Action Invoker can use to call SyncRunner routines without depending on access
    // to the zSyncO DLL.
    SYNC_API static std::unique_ptr<ActionInvokerSyncRunner> Instantiate();

    // Calls SyncRunner::Connect and holds the connection.
    virtual void Connect(ActionInvoker::Caller& caller, const SyncConnectionString& sync_connection_string) = 0;

    // Calls ISyncService::Disconnect. The connection object will be destroyed
    // regardless of whether an exception is thrown.
    virtual void Disconnect(ActionInvoker::Caller& caller) = 0;

    // Calls SyncRunner::SendSyncMessage (with a constructed SyncMessage).
    virtual std::optional<JsonNode> SendSyncMessage(ActionInvoker::Caller& caller, SharableString message_name, JsonNode message_value) = 0;

    // Calls SyncRunner::SyncParadata.
    virtual void SyncParadata(ActionInvoker::Caller& caller, Paradata::Syncer& paradata_syncer, SyncDirection sync_direction) = 0;
};
