#include "stdafx.h"
#include "DataWrapper.h"
#include <zParadataO/Syncer.h>
#include <zSyncO/SyncRunnerActionInvoker.h>


CREATE_JSON_KEY(conflicts)
namespace JK { constexpr const char* new_ = "new"; }
CREATE_JSON_KEY(received)
CREATE_JSON_KEY(sent)
CREATE_JSON_KEY(stale)
CREATE_JSON_KEY(updates)


// --------------------------------------------------------------------------
// SyncServiceWrapper
// --------------------------------------------------------------------------

class ActionInvoker::Runtime::SyncServiceWrapper
{
public:
    SyncServiceWrapper(Runtime& runtime, std::unique_ptr<ActionInvokerSyncRunner> sync_runner);

    static int GetSyncId(Runtime& runtime, const JsonNode& json_node, Caller& caller);
    static auto GetSyncServiceWrapper(Runtime& runtime, int sync_id);
    static auto GetSyncServiceWrapper(Runtime& runtime, const JsonNode& json_node, Caller& caller);
    static ActionInvokerSyncRunner& GetSyncRunner(Runtime& runtime, const JsonNode& json_node, Caller& caller);

    ActionInvokerSyncRunner& GetSyncRunner() { return *m_syncRunner; }

private:
    Runtime& m_runtime;
    std::unique_ptr<ActionInvokerSyncRunner> m_syncRunner;
};


ActionInvoker::Runtime::SyncServiceWrapper::SyncServiceWrapper(Runtime& runtime, std::unique_ptr<ActionInvokerSyncRunner> sync_runner)
    :   m_runtime(runtime),
        m_syncRunner(std::move(sync_runner))
{
    ASSERT(m_syncRunner != nullptr);
}


int ActionInvoker::Runtime::SyncServiceWrapper::GetSyncId(Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    return runtime.GetResourceId(Resource::SyncService, json_node, caller, JK::syncId,
                                 "You must specify which synchronization service to access using '%s'.",
                                 "Multiple synchronization services are open so you must specify which one to access using '%s'.");
}


auto ActionInvoker::Runtime::SyncServiceWrapper::GetSyncServiceWrapper(Runtime& runtime, const int sync_id)
{
    const auto& lookup = runtime.m_syncServiceWrappers.find(sync_id);

    if( lookup == runtime.m_syncServiceWrappers.cend() )
        throw CSProException("No synchronization service is associated with the ID '%d'.", sync_id);

    return lookup;
}


auto ActionInvoker::Runtime::SyncServiceWrapper::GetSyncServiceWrapper(Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    const int sync_id = GetSyncId(runtime, json_node, caller);
    return GetSyncServiceWrapper(runtime, sync_id);
}


ActionInvokerSyncRunner& ActionInvoker::Runtime::SyncServiceWrapper::GetSyncRunner(Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    return GetSyncServiceWrapper(runtime, json_node, caller)->second->GetSyncRunner();
}



// --------------------------------------------------------------------------
// Sync actions
// --------------------------------------------------------------------------

ActionInvoker::Result ActionInvoker::Runtime::Sync_connect(const JsonNode& json_node, Caller& caller)
{
    std::unique_ptr<ActionInvokerSyncRunner> sync_runner = ObjectTransporter::CreateActionInvokerSyncRunner();
    ASSERT(sync_runner != nullptr);

    const SyncConnectionString sync_connection_string = json_node.Get<SyncConnectionString>(JK::connection);
    sync_runner->Connect(caller, sync_connection_string);

    const int sync_id = CreateResourceId(Resource::SyncService, caller);

    m_syncServiceWrappers.try_emplace(sync_id, std::make_unique<SyncServiceWrapper>(*this, std::move(sync_runner)));

    return Result::Number(sync_id);
}


ActionInvoker::Result ActionInvoker::Runtime::Sync_disconnect(const JsonNode& json_node, Caller& caller)
{
    const int sync_id = SyncServiceWrapper::GetSyncId(*this, json_node, caller);
    const auto& sync_service_wrapper_lookup = SyncServiceWrapper::GetSyncServiceWrapper(*this, sync_id);

    const std::shared_ptr<SyncServiceWrapper> sync_service_wrapper = sync_service_wrapper_lookup->second;
    ActionInvokerSyncRunner& sync_runner = sync_service_wrapper->GetSyncRunner();

    // destroy the wrapper before disconnecting in case an exception is thrown while disconnecting
    m_syncServiceWrappers.erase(sync_service_wrapper_lookup);
    DestroyResourceId(sync_id);

    sync_runner.Disconnect(caller);

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::Sync_syncData(const JsonNode& json_node, Caller& caller)
{
    ActionInvokerSyncRunner& sync_runner = SyncServiceWrapper::GetSyncRunner(*this, json_node, caller);

    const std::shared_ptr<DataWrapper> data_wrapper = DataWrapper::GetDataWrapper(*this, json_node, caller);
    DataRepository& data_repository = data_wrapper->GetDataRepository();
    ISyncableDataRepository* const syncable_data_repository = data_repository.GetSyncableDataRepository();

    if( syncable_data_repository == nullptr )
    {
        throw CSProException("Synchronization routines are not supported using data sources of type: %s",
                             ToString(data_repository.GetRepositoryType()));
    }

    const SyncDirection sync_direction = json_node.GetOrDefault(JK::direction, SyncDirection::Both);

    // make sure that data sources connected to dictionaries owned by the interpreter are properly updated
    const std::unique_ptr<EngineDictionaryModifier> engine_dictionary_modifier =
        ( sync_direction == SyncDirection::Put ) ? nullptr :
                                                   data_wrapper->CreateEngineDictionaryModifier(*this);

    DataSyncStatistics sync_stats;

    auto run_sync = [&](const std::string& universe)
    {
        sync_stats += sync_runner.SyncData(caller, *syncable_data_repository, sync_direction, universe);
    };

    if( json_node.Contains(JK::universe) )
    {
        const JsonNode universe_json_node = json_node.Get(JK::universe);

        if( universe_json_node.IsArray() )
        {
            for( const JsonNode& array_node : universe_json_node.GetArray() )
                run_sync(array_node.Get<std::string>());
        }

        else
        {
            run_sync(universe_json_node.Get<std::string>());
        }
    }

    else
    {
        run_sync(SO::Empty_string);
    }

    if( engine_dictionary_modifier != nullptr )
        engine_dictionary_modifier->FinishedWithModifications();

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    if( sync_direction != SyncDirection::Get )
    {
        json_writer->BeginObject(JK::sent)
                    .Write(JK::count, sync_stats.cases_sent)
                    .EndObject();
    }

    if( sync_direction != SyncDirection::Put )
    {
        json_writer->BeginObject(JK::received)
                    .Write(JK::count, sync_stats.cases_received)
                    .Write(JK::new_, sync_stats.cases_not_in_repository)
                    .Write(JK::updates, sync_stats.cases_newer_on_remote)
                    .Write(JK::stale, sync_stats.cases_newer_in_repository)
                    .Write(JK::conflicts, sync_stats.cases_with_conflicts)
                    .EndObject();
    }

    json_writer->EndObject();

    return Result::JsonText(*json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::Sync_sendMessage(const JsonNode& json_node, Caller& caller)
{
    ActionInvokerSyncRunner& sync_runner = SyncServiceWrapper::GetSyncRunner(*this, json_node, caller);

    const std::optional<JsonNode> response_json_node = sync_runner.SendSyncMessage(caller, json_node.Get<SharableString>(JK::name),
                                                                                           json_node.GetOrEmpty(JK::value));

    return response_json_node.has_value() ? Result::FromJsonNode(*response_json_node) :
                                            Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::Sync_syncParadata(const JsonNode& json_node, Caller& caller)
{
    ActionInvokerSyncRunner& sync_runner = SyncServiceWrapper::GetSyncRunner(*this, json_node, caller);

    const SyncDirection sync_direction = json_node.GetOrDefault(JK::direction, SyncDirection::Both);
    const bool paradata_log_is_currently_open = Paradata::Logger::IsOpen();

    auto sync_paradata = [&](Paradata::Syncer* const paradata_syncer)
    {
        ASSERT(paradata_syncer != nullptr);
        sync_runner.SyncParadata(caller, *paradata_syncer, sync_direction);
    };

    if( json_node.Contains(JK::path) )
    {
        const auto [file_paths, return_results_as_an_array] = EvaluateFilePaths(json_node.Get(JK::path), caller, false, false);

        for( const std::string& file_path : file_paths )
        {
            if( paradata_log_is_currently_open && SO::EqualsNoCase(file_path, Paradata::Logger::GetFilePath()) )
            {
                sync_paradata(Paradata::Logger::GetSyncer().get());
            }

            else
            {
                if( sync_direction == SyncDirection::Put && !PortableFunctions::FileIsRegular(file_path) )
                  throw CSProException("When syncing a paradata log that does not already exist, you cannot use the direction 'put': " + file_path);

                Paradata::Syncer paradata_syncer(file_path);
                sync_paradata(&paradata_syncer);
            }

            if( caller.GetCancelFlag() )
                throw UserCanceledException();
        }
    }

    else
    {
        if( !paradata_log_is_currently_open )
            throw CSProException("No paradata log is open.");

        sync_paradata(Paradata::Logger::GetSyncer().get());
    }

    return Result::Undefined();
}
