#include "stdafx.h"
#include "SQLiteSyncStatusEvaluator.h"
#include "SQLiteErrorWithMessage.h"
#include <zSql/SQLiteHelpers.h>


SQLiteRepository::SyncStatusEvaluator::SyncStatusEvaluator(SQLiteRepository& repository)
    :   m_repository(repository),
        m_stmtGetCaseRev(nullptr)
{
}


SQLiteRepository::SyncStatusEvaluator::~SyncStatusEvaluator()
{
    ClearPreparedStatements();
}


void SQLiteRepository::SyncStatusEvaluator::ClearPreparedStatements()
{
    safe_sqlite3_finalize(m_stmtGetCaseRev);
}


std::optional<double> SQLiteRepository::SyncStatusEvaluator::GetSyncTime(const std::string& device_identifier, const std::string& case_uuid)
{
    auto step_statement = [&](SQLiteStatement& statement) -> int
    {
        const int result = statement.Step();

        if( result != SQLITE_ROW && result != SQLITE_DONE )
            throw SQLiteErrorWithMessage(m_repository.m_db);

        return result;
    };


    // first get the sync details for the device identifier
    const std::map<int, std::vector<SyncDetails>>* file_revision_to_sync_details_map = nullptr;
    const auto& file_revision_to_sync_details_map_lookup = m_deviceIdentifierToFileRevisionToSyncDetailsMap.find(device_identifier);

    if( file_revision_to_sync_details_map_lookup != m_deviceIdentifierToFileRevisionToSyncDetailsMap.cend() )
    {
        file_revision_to_sync_details_map = &file_revision_to_sync_details_map_lookup->second;
    }

    else
    {
        // the device identifier can be either the device name or the device ID; we will
        // lookup the device assuming the identifier is the name so that we can make sure that
        // we include all relevant sync history (for example, if a server is sometimes localhost and
        // other times the IP address, then this would ensure that all sync history is included)
        std::string device_id = device_identifier;
        std::string device_name;

        if( !device_identifier.empty() )
        {
            // when searcing by device name, do a case-insensitive search based
            // on the beginning of the string so a name like .../api will match with
            // an entry like .../api/
            SQLiteStatement device_id_from_name_statement(m_repository.m_db,
                "SELECT `device_id` FROM `sync_history` "
                "WHERE INSTR(LOWER(`device_name`), LOWER(?)) = 1 "
                "LIMIT 1;");
            device_id_from_name_statement.Bind(1, device_identifier);

            if( step_statement(device_id_from_name_statement) == SQLITE_ROW )
            {
                device_id = device_id_from_name_statement.GetColumn<std::string>(0);
                device_name = device_identifier;
            }
        }


        // now get all of the sync times for this device
        std::map<int, std::vector<SyncDetails>> new_file_revision_to_sync_details_map;

        std::string sync_details_sql = "SELECT `file_revision`, `timestamp`, `universe`, `partial`, `last_id` "
                                       "FROM `sync_history` ";
        std::map<const char*, const std::string*> strings_to_bind;

        if( !device_id.empty() )
        {
            sync_details_sql.append("WHERE `device_id` = @di ");
            strings_to_bind["@di"] = &device_id;
        }

        if( !device_name.empty() )
        {
            sync_details_sql.append(device_id.empty() ? "WHERE " : "OR ");
            sync_details_sql.append("INSTR(LOWER(`device_name`), LOWER(@dn)) = 1 ");
            strings_to_bind["@dn"] = &device_name;
        }

        sync_details_sql.append("ORDER BY `id`;");

        SQLiteStatement sync_details_statement(m_repository.m_db, sync_details_sql);

        for( const auto& [parameter_name, value] : strings_to_bind )
            sync_details_statement.Bind(parameter_name, *value);

        while( step_statement(sync_details_statement) == SQLITE_ROW )
        {
            const int file_revision = sync_details_statement.GetColumn<int>(0);
            SyncDetails& sync_details = new_file_revision_to_sync_details_map[file_revision].emplace_back();

            sync_details.timestamp = sync_details_statement.GetColumn<double>(1);
            sync_details.universe = sync_details_statement.GetColumn<std::string>(2);

            const int partial = sync_details_statement.GetColumn<int>(3);

            if( partial != static_cast<int>(SyncHistoryEntry::SyncState::Complete) )
            {
                if( sync_details_statement.IsColumnNull(4) )
                {
                    sync_details.last_uuid_of_partial_sync.emplace();
                }

                else
                {
                    sync_details.last_uuid_of_partial_sync = sync_details_statement.GetColumn<std::string>(4);
                }
            }
        }

        file_revision_to_sync_details_map = &m_deviceIdentifierToFileRevisionToSyncDetailsMap.try_emplace(
            device_identifier, new_file_revision_to_sync_details_map).first->second;
    }


    // we are done if there has never been a sync with this device...
    if( file_revision_to_sync_details_map->empty() )
        return std::nullopt;

    // ...or if not querying for the sync time of a specific case
    if( case_uuid.empty() )
        return file_revision_to_sync_details_map->crbegin()->second.back().timestamp;

    // otherwise see what revision the case is currently at
    SQLiteStatement get_case_revision_statement(m_repository.m_db, m_stmtGetCaseRev,
        "SELECT `key`, `last_modified_revision` FROM `cases` WHERE `id` = ? LIMIT 1;");
    get_case_revision_statement.Bind(1, case_uuid);

    if( step_statement(get_case_revision_statement) == SQLITE_ROW )
    {
        // see if there was a sync with this device at or after the case's revision number
        const std::string case_key = get_case_revision_statement.GetColumn<std::string>(0);
        const int case_revision = get_case_revision_statement.GetColumn<int>(1);

        for( auto sync_details_itr = file_revision_to_sync_details_map->lower_bound(case_revision);
             sync_details_itr != file_revision_to_sync_details_map->cend();
             ++sync_details_itr )
        {
            for( const SyncDetails& sync_details : sync_details_itr->second )
            {
                // skip syncs where the case would not have been synced due to the use of a universe
                if( !SO::StartsWith(case_key, sync_details.universe) )
                    continue;

                // skip partial syncs in which this case was not synced
                if( sync_details.last_uuid_of_partial_sync.has_value() && *sync_details.last_uuid_of_partial_sync < case_key )
                    continue;

                // finally here is the case's sync time
                return sync_details.timestamp;
            }
        }
    }

    return std::nullopt;
}
