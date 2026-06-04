#include "stdafx.h"
#include "SQLiteSyncStatusEvaluator.h"


SQLiteRepository::SyncStatusEvaluator::SyncStatusEvaluator(SQLiteRepository& repository)
    :   m_repository(repository)
{
}


void SQLiteRepository::SyncStatusEvaluator::ClearPreparedStatements()
{
    m_stmtGetDeviceIdFromName.Finalize();
    m_stmtGetSyncTimeData.Finalize();
    m_stmtGetCaseRevision.Finalize();
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


std::string SQLiteRepository::SyncStatusEvaluator::GetDeviceIdFromName(const std::string& device_name)
{
    const Sqlite::Statement::Runner stmt_runner_gdifn(m_repository.m_db, m_stmtGetDeviceIdFromName,
        "SELECT `device_id` FROM `sync_history` "
        "WHERE INSTR(LOWER(`device_name`), LOWER(?)) = 1 "
        "LIMIT 1;"
    );

    m_stmtGetDeviceIdFromName.Bind(1, device_name);

    if( m_stmtGetDeviceIdFromName.Step() == Sqlite::Result::Row )
        return m_stmtGetDeviceIdFromName.GetColumn<std::string>(0);

    return std::string();
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
        std::string matched_device_id = GetDeviceIdFromName(*device_identifier);

        if( !matched_device_id.empty() )
        {
            device_id = std::move(matched_device_id);
            device_name = device_identifier;
        }
    }

    // get all of the sync times for this device
    const Sqlite::Statement::Runner stmt_runner_gcr(m_repository.m_db, m_stmtGetSyncTimeData,
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
    const Sqlite::Statement::Runner stmt_runner_gcr(m_repository.m_db, m_stmtGetCaseRevision,
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
