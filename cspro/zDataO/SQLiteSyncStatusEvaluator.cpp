#include "stdafx.h"
#include "SQLiteSyncStatusEvaluator.h"
#include "SQLiteRepositoryIterators.h"


CREATE_JSON_KEY(deviceNames)
CREATE_JSON_KEY(firstSyncTime)
CREATE_JSON_KEY(lastGet)
CREATE_JSON_KEY(lastPut)
CREATE_JSON_KEY(lastSyncedUuid)
CREATE_JSON_KEY(lastSyncTime)
CREATE_JSON_KEY(partial)
CREATE_JSON_KEY(syncHistory)


SQLiteRepository::SyncStatusEvaluator::SyncStatusEvaluator(SQLiteRepository& repository)
    :   m_repository(repository)
{
}


void SQLiteRepository::SyncStatusEvaluator::ClearPreparedStatements()
{
    m_stmtGetDeviceIdFromName.Finalize();
    m_stmtGetUniqueDeviceId.Finalize();
    m_stmtGetSyncTimeData.Finalize();
    m_stmtGetCaseRevision.Finalize();
    m_stmtGetSyncServices.Finalize();
    m_stmtGetSyncHistory.Finalize();
}


std::optional<double> SQLiteRepository::SyncStatusEvaluator::GetSyncTime(const SharableString& device_identifier, const SharableString& case_uuid)
{
    // first get the sync details for the device identifier
    const std::map<int, std::vector<SyncTimeData>>& file_revision_to_sync_time_data_map = GetSyncTimesForDeviceIdentifier(device_identifier);

    // we are done if there has never been a sync with this device...
    if( file_revision_to_sync_time_data_map.empty() )
        return std::nullopt;

    // ...or if not querying for the sync time of a specific case
    if( case_uuid->empty() )
        return file_revision_to_sync_time_data_map.crbegin()->second.back().timestamp;

    // otherwise see what revision the case is currently at
    const auto [case_key, case_revision] = GetCaseRevisionFromUuid(*case_uuid);

    if( !case_key.empty() )
    {
        // see if there was a sync with this device at or after the case's revision number
        for( auto sync_time_data_itr = file_revision_to_sync_time_data_map.lower_bound(case_revision);
             sync_time_data_itr != file_revision_to_sync_time_data_map.cend();
             ++sync_time_data_itr )
        {
            for( const SyncTimeData& sync_time_data : sync_time_data_itr->second )
            {
                // skip syncs where the case would not have been synced due to the use of a universe
                if( !SO::StartsWith(case_key, sync_time_data.universe) )
                    continue;

                // skip partial syncs in which this case was not synced
                if( sync_time_data.last_uuid_of_partial_sync.has_value() && *sync_time_data.last_uuid_of_partial_sync < case_key )
                    continue;

                // finally here is the case's sync time
                return sync_time_data.timestamp;
            }
        }
    }

    return std::nullopt;
}


std::string SQLiteRepository::SyncStatusEvaluator::GetDeviceIdFromName(const std::string& device_name, const bool ensure_that_only_one_device_matches)
{
    const Sqlite::Statement::Runner stmt_runner(m_repository.m_db, m_stmtGetDeviceIdFromName,
        "SELECT `device_id` "
        "FROM `sync_history` "
        "WHERE INSTR(LOWER(`device_name`), LOWER(?)) = 1 "
        "LIMIT ?;"
    );

    m_stmtGetDeviceIdFromName.Bind(1, device_name);
    m_stmtGetDeviceIdFromName.Bind(2, ensure_that_only_one_device_matches ? -1 : 1);

    if( m_stmtGetDeviceIdFromName.Step() == Sqlite::Result::Row )
    {
        std::string device_id = m_stmtGetDeviceIdFromName.GetColumn<std::string>(0);

        if( ensure_that_only_one_device_matches &&
            m_stmtGetDeviceIdFromName.Step() != Sqlite::Result::Done )
        {
            throw CSProException("There are multiple devices associated with the name '%s', including '%s' and '%s'.",
                                 device_name.c_str(),
                                 device_id.c_str(),
                                 m_stmtGetDeviceIdFromName.GetColumn<std::string>(0).c_str());
        }

        return device_id;
    }

    else if( ensure_that_only_one_device_matches )
    {
        throw CSProException("There is no device associated with the name '%s'.",
                             device_name.c_str());
    }

    return std::string();
}


std::string SQLiteRepository::SyncStatusEvaluator::GetDeviceIdIfUnique()
{
    const Sqlite::Statement::Runner stmt_runner(m_repository.m_db, m_stmtGetUniqueDeviceId,
        "SELECT DISTINCT `device_id` "
        "FROM `sync_history` "
        "LIMIT 2;"
    );

    if( m_stmtGetUniqueDeviceId.Step() != Sqlite::Result::Row )
        throw CSProException("The data source has never been synchronized.");

    std::string device_id = m_stmtGetUniqueDeviceId.GetColumn<std::string>(0);

    if( m_stmtGetUniqueDeviceId.Step() != Sqlite::Result::Done )
        throw CSProException("You must specify a synchronization service as the data source has been synchronized with multiple services.");

    return device_id;
}


const std::map<int, std::vector<SQLiteRepository::SyncStatusEvaluator::SyncTimeData>>&
    SQLiteRepository::SyncStatusEvaluator::GetSyncTimesForDeviceIdentifier(const SharableString& device_identifier)
{
    const auto& lookup = m_deviceIdentifierToFileRevisionToSyncTimeDataMap.find(*device_identifier);

    if( lookup != m_deviceIdentifierToFileRevisionToSyncTimeDataMap.cend() )
        return lookup->second;

    // the device identifier can be either the device name or the device ID;
    // we will lookup the device assuming the identifier is the name so that
    // we can make sure that we include all relevant sync history;
    // for example, if the device name is sometimes localhost and other times
    // the IP address, then this would ensure that all sync history is included
    SharableString device_id = device_identifier;
    SharableString device_name;

    ASSERT(device_identifier.IsSet() == !device_identifier->empty());

    if( device_identifier.IsSet() )
    {
        std::string matched_device_id = GetDeviceIdFromName(*device_identifier, false);

        if( !matched_device_id.empty() )
        {
            device_id = std::move(matched_device_id);
            device_name = device_identifier;
        }
    }

    // get all of the sync times for this device
    const Sqlite::Statement::Runner stmt_runner(m_repository.m_db, m_stmtGetSyncTimeData,
        "SELECT `file_revision`, `timestamp`, `universe`, `partial`, `last_id` "
        "FROM `sync_history` "
        "WHERE ( @di IS NULL AND @dn IS NULL ) OR "
              "( @di IS NOT NULL AND `device_id` = @di ) OR "
              "( @dn IS NOT NULL AND INSTR(LOWER(`device_name`), LOWER(@dn)) = 1 ) "
        "ORDER BY `id`;"
    );

    m_stmtGetSyncTimeData.ClearBindings();

    if( device_id.IsSet() )
        m_stmtGetSyncTimeData.Bind("@di", *device_id);

    if( device_name.IsSet() )
        m_stmtGetSyncTimeData.Bind("@dn", *device_name);

    std::map<int, std::vector<SyncTimeData>> new_file_revision_to_sync_time_data_map;

    while( m_stmtGetSyncTimeData.Step() == Sqlite::Result::Row )
    {
        const int file_revision = m_stmtGetSyncTimeData.GetColumn<int>(0);
        SyncTimeData& sync_time_data = new_file_revision_to_sync_time_data_map[file_revision].emplace_back();

        sync_time_data.timestamp = m_stmtGetSyncTimeData.GetColumn<double>(1);
        sync_time_data.universe = m_stmtGetSyncTimeData.GetColumn<std::string>(2);

        const SyncHistoryEntry::SyncState sync_state = static_cast<SyncHistoryEntry::SyncState>(m_stmtGetSyncTimeData.GetColumn<int>(3));

        if( sync_state != SyncHistoryEntry::SyncState::Complete )
        {
            if( m_stmtGetSyncTimeData.IsColumnNull(4) )
            {
                sync_time_data.last_uuid_of_partial_sync.emplace();
            }

            else
            {
                sync_time_data.last_uuid_of_partial_sync = m_stmtGetSyncTimeData.GetColumn<std::string>(4);
            }
        }
    }

    return m_deviceIdentifierToFileRevisionToSyncTimeDataMap.try_emplace(
        *device_identifier,
        std::move(new_file_revision_to_sync_time_data_map)
    ).first->second;
}


std::tuple<std::string, int> SQLiteRepository::SyncStatusEvaluator::GetCaseRevisionFromUuid(const std::string& case_uuid)
{
    const Sqlite::Statement::Runner stmt_runner(m_repository.m_db, m_stmtGetCaseRevision,
        "SELECT `key`, `last_modified_revision` "
        "FROM `cases` "
        "WHERE `id` = ? "
        "LIMIT 1;"
    );

    m_stmtGetCaseRevision.Bind(1, case_uuid);

    if( m_stmtGetCaseRevision.Step() == Sqlite::Result::Row )
    {
        return
        {
            m_stmtGetCaseRevision.GetColumn<std::string>(0),
            m_stmtGetCaseRevision.GetColumn<int>(1)
        };
    }

    return { std::string(), -1 };
}


struct SQLiteRepository::SyncStatusEvaluator::SyncHistoryData
{
    int64_t time;
    std::string device_id;
    std::string device_name;
    std::string username;
    std::string universe;
    SyncDirection direction;
    bool partial;
    std::optional<std::string> last_case_uuid;
};


std::unique_ptr<SQLiteRepository::SyncStatusEvaluator::SyncHistoryData>
    SQLiteRepository::SyncStatusEvaluator::CreateSyncHistoryData(Sqlite::Statement& stmt)
{
    static_assert(static_cast<int>(SyncHistoryEntry::SyncState::Complete) == 0);

    return std::unique_ptr<SQLiteRepository::SyncStatusEvaluator::SyncHistoryData>(new SyncHistoryData
    {
        stmt.GetColumn<int64_t>(0),
        stmt.GetColumn<std::string>(1),
        stmt.GetColumn<std::string>(2),
        stmt.GetColumn<std::string>(3),
        stmt.GetColumn<std::string>(4),
        static_cast<SyncDirection>(stmt.GetColumn<int>(5)),
        stmt.GetColumn<bool>(6),
        stmt.GetOptionalColumn<std::string>(7)
    });
}


void SQLiteRepository::SyncStatusEvaluator::WriteSyncHistoryData(JsonWriter& json_writer, const SyncHistoryData& sync_history_data)
{
    ASSERT(sync_history_data.direction != SyncDirection::Both);

    json_writer.BeginObject()
               .WriteDate(JK::time, sync_history_data.time)
               .Write(JK::deviceId, sync_history_data.device_id)
               .Write(JK::deviceName, sync_history_data.device_name)
               .Write(JK::username, sync_history_data.username)
               .Write(JK::direction, ( sync_history_data.direction == SyncDirection::Get ) ? "get" : "put")
               .Write(JK::partial, sync_history_data.partial)
               .WriteIfHasValue(JK::lastSyncedUuid, sync_history_data.last_case_uuid)
               .EndObject();
}


void SQLiteRepository::SyncStatusEvaluator::WriteSyncHistoryData(JsonWriter& json_writer, const char* const key,
                                                                 const SyncHistoryData& sync_history_data)
{
    json_writer.Key(key);
    WriteSyncHistoryData(json_writer, sync_history_data);
}


void SQLiteRepository::SyncStatusEvaluator::WriteSyncStatus(JsonWriter& json_writer, const JsonNode& json_node,
                                                            const SharableString& device_id, const SharableString& device_name)
{
    const std::string_view content_sv = json_node.GetOrDefault<std::string_view>(JK::content, "summary");

    if( content_sv == "syncServices" )
    {
        WriteSyncStatus_syncServices(json_writer);
    }

    else if( content_sv == "syncHistory" )
    {
        WriteSyncStatus_syncHistory(json_writer, device_id, device_name);
    }

    else if( content_sv == "casesPendingSync" )
    {
        WriteSyncStatus_casesPendingSync(json_writer, device_id, device_name, json_node.GetOrConstruct<std::string>(JK::universe));
    }

    else
    {
        throw CSProException("'%s' is not a valid content type.", std::string(content_sv).c_str());
    }
}


void SQLiteRepository::SyncStatusEvaluator::WriteSyncStatus_syncServices(JsonWriter& json_writer)
{
    const Sqlite::Statement::Runner stmt_runner(m_repository.m_db, m_stmtGetSyncServices,
        "SELECT `device_id`, `device_name`, MIN(`timestamp`), MAX(`timestamp`) "
        "FROM `sync_history` "
        "GROUP BY `device_id`, `device_name` "
        "ORDER BY `device_id`;"
    );

    // because a device may have multiple names, we may have to process multiple rows before writing the data
    struct Data
    {
        std::string device_id;
        std::vector<std::string> device_names;
        int64_t min_timestamp;
        int64_t max_timestamp;
    };

    Data data;

    auto write_data = [&]()
    {
        ASSERT(!data.device_id.empty());

        json_writer.BeginObject()
                   .Write(JK::deviceId, data.device_id)
                   .Write(JK::deviceNames, data.device_names)
                   .WriteDate(JK::firstSyncTime, data.min_timestamp)
                   .WriteDate(JK::lastSyncTime, data.max_timestamp)
                   .EndObject();
    };

    json_writer.BeginArray();

    while( m_stmtGetSyncServices.Step() == Sqlite::Result::Row )
    {
        std::string device_id = m_stmtGetSyncServices.GetColumn<std::string>(0);
        const int64_t min_timestamp = m_stmtGetSyncServices.GetColumn<int64_t>(2);
        const int64_t max_timestamp = m_stmtGetSyncServices.GetColumn<int64_t>(3);

        if( device_id != data.device_id )
        {
            if( !data.device_id.empty() )
                write_data();

            data.device_id = std::move(device_id);
            data.device_names.clear();
            data.min_timestamp = min_timestamp;
            data.max_timestamp = max_timestamp;
        }

        else
        {
            data.min_timestamp = std::min(data.min_timestamp, min_timestamp);
            data.max_timestamp = std::max(data.min_timestamp, max_timestamp);
        }

        data.device_names.emplace_back(m_stmtGetSyncServices.GetColumn<std::string>(1));
    }

    if( !data.device_id.empty() )
        write_data();

    json_writer.EndArray();
}


void SQLiteRepository::SyncStatusEvaluator::WriteSyncStatus_syncHistory(JsonWriter& json_writer, const SharableString& device_id,
                                                                        const SharableString& device_name)
{
    const Sqlite::Statement::Runner stmt_runner(m_repository.m_db, m_stmtGetSyncHistory,
        "SELECT `timestamp`, `device_id`, `device_name`, `user_name`, `universe`, `direction`, `partial`, `last_id` "
        "FROM `sync_history` "
        "WHERE ( @di IS NULL AND @dn IS NULL ) OR "
              "( @di IS NOT NULL AND `device_id` = @di ) OR "
              "( @dn IS NOT NULL AND INSTR(LOWER(`device_name`), LOWER(@dn)) = 1 ) "
        "ORDER BY `id` DESC;"
    );

    m_stmtGetSyncHistory.ClearBindings();

    if( device_id.IsSet() )
        m_stmtGetSyncHistory.Bind("@di", *device_id);

    if( device_name.IsSet() )
        m_stmtGetSyncHistory.Bind("@dn", *device_name);

    std::shared_ptr<const SyncHistoryData> last_get;
    std::shared_ptr<const SyncHistoryData> last_put;

    json_writer.BeginObject()
               .BeginArray(JK::syncHistory);

    while( m_stmtGetSyncHistory.Step() == Sqlite::Result::Row )
    {
        std::unique_ptr<const SyncHistoryData> sync_history_data = CreateSyncHistoryData(m_stmtGetSyncHistory);
        WriteSyncHistoryData(json_writer, *sync_history_data);

        if( sync_history_data->direction == SyncDirection::Get )
        {
            if( last_get == nullptr )
                last_get = std::move(sync_history_data);
        }

        else if( last_put == nullptr )
        {
            ASSERT(sync_history_data->direction == SyncDirection::Put);
            last_put = std::move(sync_history_data);
        }
    }

    json_writer.EndArray();

    if( last_get != nullptr )
        WriteSyncHistoryData(json_writer, JK::lastGet, *last_get);

    if( last_put != nullptr )
        WriteSyncHistoryData(json_writer, JK::lastPut, *last_put);

    json_writer.EndObject();
}


void SQLiteRepository::SyncStatusEvaluator::WriteSyncStatus_casesPendingSync(JsonWriter& json_writer, SharableString device_id,
                                                                             const SharableString& device_name, const std::string& universe)
{
    if( device_id.IsSet() )
    {
        if( device_id->empty() )
            throw CSProException("The device ID cannot be blank.");
    }

    else
    {
        device_id = device_name.IsSet() ? GetDeviceIdFromName(*device_name, true) :
                                          GetDeviceIdIfUnique();
    }

    ASSERT(!device_id->empty());

    // this implementation is based on DataSyncer::SyncGet and DataSyncer::GetRevisionFromLastSync
    cs::cref_optional<DeviceId> exclude_gets_from_device_id = *device_id;

    std::optional<SyncHistoryEntry> last_sync_revision = m_repository.GetLastSyncForDevice(
        *device_id,
        SyncDirection::Put
    );

    if( last_sync_revision.has_value() )
    {
        // only use the previous revision number if the universe stayed the same or became more restrictive
        if( !SO::StartsWith(universe, last_sync_revision->GetUniverse()) )
        {
            last_sync_revision.reset();
        }

        // for partial syncs, when the universe doesn't match, everything is synced
        else if( last_sync_revision->IsPartialPut() && universe != last_sync_revision->GetUniverse() )
        {
            exclude_gets_from_device_id.reset();
            last_sync_revision.reset();
        }
    }

    std::string server_revision;
    int client_revision = -1;
    std::string last_case_uuid;

    if( last_sync_revision.has_value() )
    {
        client_revision = last_sync_revision->GetFileRevision();

        // When uploading multiple chunks we store revisions as a comma separated list
        // so we need to grab the last one to get the most recent sync put revision
        if( !last_sync_revision->GetServerFileRevision().empty() )
            server_revision = SO::SplitString(last_sync_revision->GetServerFileRevision(), ',').back();

        if( last_sync_revision->IsPartialPut() )
            last_case_uuid = last_sync_revision->GetLastCaseUuid();
    }

    const std::unique_ptr<SQLiteRepositoryCaseIterator> case_iterator =
        m_repository.GetCasesModifiedSinceRevisionIterator(
            CaseIterationContent::CaseKey,
            true,
            client_revision,
            last_case_uuid,
            universe,
            std::numeric_limits<size_t>::max(),
            nullptr,
            nullptr,
            exclude_gets_from_device_id,
            std::nullopt
        );

    json_writer.BeginArray();

    CaseKey case_key;
    std::string uuid;

    while( case_iterator->NextCaseKeyAndUuid(case_key, uuid) )
    {
        json_writer.BeginObject()
                   .Write(JK::key, case_key.GetKey())
                   .Write(JK::uuid, uuid)
                   .Write(JK::position, case_key.GetPositionInRepository())
                   .EndObject();
    }

    json_writer.EndArray();
}
