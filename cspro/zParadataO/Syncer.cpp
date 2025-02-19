#include "stdafx.h"
#include "Syncer.h"
#include "Concatenator.h"
#include <zToolsO/Encoders.h>
#include <zSql/Transaction.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/TemporaryFile.h>

using namespace Paradata;


namespace SyncerErrors
{
    constexpr const char* SyncParadataStateTable = "Could not create or query the `syncparadata_state` table.";
    constexpr const char* AttachDetachDatabase   = "Could not attach or detach the extracted paradata log.";
    constexpr const char* Pragma                 = "Could not set a pragma on the paradata log.";
    constexpr const char* Insert                 = "Could not insert events to the extracted paradata log.";
    constexpr const char* ModifySyncState        = "Could not modify the paradata log's sync state.";
}


Syncer::Syncer(cs::non_null_shared_or_raw_ptr<Log> log)
    :   m_log(std::move(log)),
        m_inputDb(m_log->m_db)
{
    SetupSyncTableAndGetLogUuid();
}


Syncer::Syncer(const std::string& file_path)
    :   Syncer(std::make_unique<Log>(file_path))
{
}


Syncer::~Syncer()
{
}


void Syncer::FlushAndResetPreparedStatements()
{
    // flush any events
    m_log->WriteEvents(false);

    // reset all of the prepared statements; if this is not done, the detach command will fail
    for( Table& table : VI_V(m_log->m_tables) )
    {
        if( table.m_insertStmt != nullptr )
            table.m_insertStmt->Reset();

        if( table.m_findDuplicateRowStmt != nullptr )
            table.m_findDuplicateRowStmt->Reset();
    }
}


void Syncer::SetupSyncTableAndGetLogUuid()
{
    // create the sync information table if it does not exist
    constexpr const char* CreateParadataStateTableSql =
        "CREATE TABLE IF NOT EXISTS `syncparadata_state` "
        "(`log_uuid` TEXT NOT NULL, `start_event_id` INT NOT NULL, `end_event_id` INT NOT NULL, "
        "PRIMARY KEY( `log_uuid`, `start_event_id` ) );";

    if( sqlite3_exec(m_inputDb, CreateParadataStateTableSql, nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(SyncerErrors::SyncParadataStateTable);

    // get the current log's unique identifier, or if one doesn't exist, create one
    SQLiteStatement query_log_uuid_stmt(m_inputDb,
        "SELECT `log_uuid` "
        "FROM `syncparadata_state` "
        "WHERE `start_event_id` = 0 AND `end_event_id` = 0 "
        "LIMIT 1;");

    if( query_log_uuid_stmt.Step() == SQLITE_ROW )
    {
        m_logUuid = query_log_uuid_stmt.GetColumn<std::string>(0);
    }

    // if no identifier exists, then this is the first time syncing, so create a new identifier for this log
    else
    {
        m_logUuid = CreateUuid();

        SQLiteStatement insert_log_uuid_stmt(m_inputDb,
            "INSERT INTO `syncparadata_state` "
            "( `log_uuid`, `start_event_id`, `end_event_id` ) "
            "VALUES (?, 0, 0 );");

        insert_log_uuid_stmt.Bind(1, m_logUuid);

        if( insert_log_uuid_stmt.Step() != SQLITE_DONE )
            throw CSProException(SyncerErrors::SyncParadataStateTable);
    }
}


std::optional<int64_t> Syncer::GetMaxEventId()
{
    SQLiteStatement query_event_id_stmt(m_inputDb,
        "SELECT `id` "
        "FROM `event` "
        "ORDER BY 1 DESC "
        "LIMIT 1;");

    if( query_event_id_stmt.Step() == SQLITE_ROW )
        return query_event_id_stmt.GetColumn<int64_t>(0);

    return std::nullopt;
}


std::optional<std::string> Syncer::GetExtractedSyncableDatabaseFilePath()
{
    FlushAndResetPreparedStatements();

    CalculateSyncableEventBoundaries();

    if( m_syncableEventBoundaries.empty() )
        return std::nullopt;

    ExtractSyncableDatabase();

    return m_extractedDatabaseTemporaryFile->GetPath();
}


void Syncer::ConsolidateEventBoundaries(std::vector<EventBoundary>& event_boundaries)
{
    if( event_boundaries.size() < 2 )
        return;

    // sort the event boundaries
    std::sort(event_boundaries.begin(), event_boundaries.end(),
        [](const EventBoundary& eb1, const EventBoundary& eb2)
        {
            return ( eb1.start_event_id != eb2.start_event_id ) ? ( eb1.start_event_id < eb2.start_event_id ) :
                                                                  ( eb1.end_event_id < eb2.end_event_id );
        });

    // consolidate the event boundaries
    for( auto current_event_boundary = event_boundaries.end() - 1;
         current_event_boundary != event_boundaries.begin();
         --current_event_boundary )
    {
        auto previous_event_boundary = current_event_boundary - 1;

        // + 1 used because 1-19 and 20-25 should be joined as 1-25
        if( current_event_boundary->start_event_id <= ( previous_event_boundary->end_event_id + 1 ) )
        {
            previous_event_boundary->end_event_id = std::max(previous_event_boundary->end_event_id, current_event_boundary->end_event_id);
            current_event_boundary = event_boundaries.erase(current_event_boundary);
        }
    }
}


void Syncer::RemovePreviouslySyncedFromSyncableEventBoundaries()
{
    ConsolidateEventBoundaries(m_previouslySyncedEventBoundaries);

    if( m_syncableEventBoundaries.empty() )
        return;

    ConsolidateEventBoundaries(m_syncableEventBoundaries);

    // remove any previously synced boundaries from the boundaries to be synced in this session
    auto syncable_event_boundary = m_syncableEventBoundaries.end() - 1;

    while( true )
    {
        int64_t& sync_start = syncable_event_boundary->start_event_id;
        int64_t& sync_end = syncable_event_boundary->end_event_id;

        for( const EventBoundary& previously_synced_event_boundary : m_previouslySyncedEventBoundaries )
        {
            const int64_t& previous_start = previously_synced_event_boundary.start_event_id;
            const int64_t& previous_end = previously_synced_event_boundary.end_event_id;

            // skip processing previous event boundaries that are completely out of this boundary
            if( previous_start > sync_end || previous_end < sync_start )
                continue;

            // adjust the sync start position
            //     [previous]
            //           [current]
            //               *****
            if( previous_start <= sync_start && previous_end >= sync_start )
                sync_start = previous_end + 1;

            // adjust the sync end position
            //          [previous]
            //     [current]
            //     *****
            if( previous_end >= sync_end && previous_start <= sync_end )
                sync_end = previous_start - 1;

            // the previous adjustments may have rendered this event boundary unnecessary; for example:
            //     [****previous****]
            //         [current]
            if( sync_start > sync_end )
            {
                syncable_event_boundary = m_syncableEventBoundaries.erase(syncable_event_boundary);
                break;
            }

            // split this event boundary if it has been partially synced
            //        [previous]
            //     [****current****]
            //     ***          ****
            if( previous_start >= sync_start && previous_end <= sync_end )
            {
                const int64_t new_sync1_start = sync_start;
                const int64_t new_sync1_end = previous_start - 1;
                ASSERT(new_sync1_start <= new_sync1_end);

                const int64_t new_sync2_start = previous_end + 1;
                const int64_t new_sync2_end = sync_end;
                ASSERT(new_sync2_start <= new_sync2_end);

                syncable_event_boundary->start_event_id = new_sync1_start;
                syncable_event_boundary->end_event_id = new_sync1_end;

                m_syncableEventBoundaries.insert(syncable_event_boundary + 1, EventBoundary { new_sync2_start, new_sync2_end });

                // reprocess all the boundaries
                RemovePreviouslySyncedFromSyncableEventBoundaries();
                return;
            }
        }

        if( syncable_event_boundary == m_syncableEventBoundaries.begin() )
            break;

        --syncable_event_boundary;
    }
}


void Syncer::CalculateSyncableEventBoundaries()
{
    ASSERT(!m_peerLogUuid.empty() && m_syncableEventBoundaries.empty());

    // the last syncable event ID is either the largest event ID, or the largest one before
    // the first event written as part of this paradata log's session; this means
    // that events that are part of the currently running application aren't included in a sync
    std::optional<int64_t> syncable_event_end_id;

    if( m_log->m_firstWrittenEventId.has_value() )
    {
        syncable_event_end_id = *m_log->m_firstWrittenEventId - 1;
    }

    else
    {
        syncable_event_end_id = GetMaxEventId();
    }

    if( !syncable_event_end_id.has_value() )
        return;

    if( *syncable_event_end_id >= 1 )
        m_syncableEventBoundaries.emplace_back(EventBoundary { 1, *syncable_event_end_id });


    // when multiple syncs (with different devices) occur during one session, there may be
    // additional blocks of events that can be transferred
    SQLiteStatement query_events_synced_during_this_session_stmt(m_inputDb,
        "SELECT `start_event_id`, `end_event_id` "
        "FROM `syncparadata_state` "
        "WHERE `log_uuid` != ? AND `start_event_id` > ?;");

    query_events_synced_during_this_session_stmt.Bind(1, m_peerLogUuid);
    query_events_synced_during_this_session_stmt.Bind(2, *syncable_event_end_id);

    while( query_events_synced_during_this_session_stmt.Step() == SQLITE_ROW )
    {
        m_syncableEventBoundaries.emplace_back(EventBoundary
            {
                query_events_synced_during_this_session_stmt.GetColumn<int64_t>(0),
                query_events_synced_during_this_session_stmt.GetColumn<int64_t>(1)
            });
    }


    // query the events that have already been synced
    SQLiteStatement query_sync_state_stmt(m_inputDb,
        "SELECT `start_event_id`, `end_event_id` "
        "FROM `syncparadata_state` "
        "WHERE `log_uuid` = ?;");

    query_sync_state_stmt.Bind(1, m_peerLogUuid);

    while( query_sync_state_stmt.Step() == SQLITE_ROW )
    {
        m_previouslySyncedEventBoundaries.emplace_back(EventBoundary
            {
                query_sync_state_stmt.GetColumn<int64_t>(0),
                query_sync_state_stmt.GetColumn<int64_t>(1)
            });
    }


    // get the final set of events to be synced
    RemovePreviouslySyncedFromSyncableEventBoundaries();
}


void Syncer::ExtractSyncableDatabase()
{
    ASSERT(!m_syncableEventBoundaries.empty() && m_extractedDatabaseTemporaryFile == nullptr && m_linkingTableStartIds.empty());

    m_extractedDatabaseTemporaryFile = std::make_unique<TemporaryFile>();

    StartExtraction(m_extractedDatabaseTemporaryFile->GetPath());

    try
    {
        // construct a list of all the tables, many of which will be linking tables
        std::vector<Table*> event_tables;

        for( const std::shared_ptr<Table>& table : m_log->m_tables )
        {
            m_linkingTableStartIds.try_emplace(table, std::nullopt);

            if( table->m_tableDefinition.table_code > 0 || table->m_tableDefinition.type == ParadataTable::BaseEvent )
                event_tables.emplace_back(table.get());
        }


        // convert the event boundaries to SQL
        std::string where_sql;

        for( const EventBoundary& event_boundary : m_syncableEventBoundaries )
        {
            where_sql.append(FormatText("%s( `id` >= " Formatter_int64_t " AND `id` <= " Formatter_int64_t " )",
                                        where_sql.empty() ? "" : " OR ",
                                        event_boundary.start_event_id,
                                        event_boundary.end_event_id));
        }

        // extract all of the events from each event table
        for( Table* const event_table : event_tables )
            ExtractEvents(*event_table, where_sql);

        // cycle through to get all of the linking table start IDs (until no changes are made)
        while( true )
        {
            bool changes_made = false;

            for( const auto& [table, start_id] : m_linkingTableStartIds )
            {
                if( start_id.has_value() )
                    changes_made |= UpdateLinkingTableStartIds(*table, start_id);
            }

            if( !changes_made )
                break;
        }

        // write any data in linking tables
        for( const auto& [table, start_id] : m_linkingTableStartIds )
        {
            if( start_id.has_value() )
                ExtractLinkingTableData(*table, *start_id);
        }
    }

    catch(...)
    {
        try
        {
            StopExtraction();
        }
        catch(...) { }

        throw;
    }

    StopExtraction();
}


void Syncer::SetJournalModeAndSynchronous(const bool speedup)
{
    std::string journal_mode_setting;
    std::string synchronous_setting;

    if( speedup )
    {
        // save the default pragma settings
        auto get_current_pragma = [&](const char* const sql)
        {
            SQLiteStatement query_pragma_stmt(m_inputDb, sql);

            if( query_pragma_stmt.Step() != SQLITE_ROW )
                throw CSProException(SyncerErrors::Pragma);

            return query_pragma_stmt.GetColumn<std::string>(0);
        };

        m_journalModeSetting = get_current_pragma("PRAGMA journal_mode;");
        m_synchronousSetting = get_current_pragma("PRAGMA synchronous;");

        journal_mode_setting = "OFF";
        synchronous_setting = "OFF";
    }

    else
    {
        ASSERT(!m_journalModeSetting.empty() && !m_synchronousSetting.empty());
        journal_mode_setting = m_journalModeSetting;
        synchronous_setting = m_synchronousSetting;
    }

    if( sqlite3_exec(m_inputDb, FormatText("PRAGMA journal_mode = %s;", journal_mode_setting.c_str()).c_str(), nullptr, nullptr, nullptr) != SQLITE_OK ||
        sqlite3_exec(m_inputDb, FormatText("PRAGMA synchronous = %s;", synchronous_setting.c_str()).c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
    {
        throw CSProException(SyncerErrors::Pragma);
    }
}


void Syncer::StartExtraction(const std::string& extracted_events_file_path)
{
    // create a log with all tables and then immediately close it
    {
        Log log(extracted_events_file_path);
    }

    // attach this file to the current log
    const std::string attach_database_sql = FormatText("ATTACH DATABASE \"%s\" AS `extract_db`;", Encoders::ToFileUrl(extracted_events_file_path).c_str());

    if( sqlite3_exec(m_inputDb, attach_database_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(SyncerErrors::AttachDetachDatabase);

    SetJournalModeAndSynchronous(true);
}


void Syncer::StopExtraction()
{
    SetJournalModeAndSynchronous(false);

    if( sqlite3_exec(m_inputDb, "DETACH `extract_db`;", nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(SyncerErrors::AttachDetachDatabase);
}


void Syncer::ExtractEvents(Table& event_table, const std::string& where_sql)
{
    const std::string insert_events_sql = FormatText("INSERT INTO `extract_db`.`%s` SELECT * FROM `%s` WHERE %s;",
                                                     event_table.m_tableDefinition.name,
                                                     event_table.m_tableDefinition.name,
                                                     where_sql.c_str());

    if( sqlite3_exec(m_inputDb, insert_events_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(SyncerErrors::Insert);

    // if any events were copied, process the events to get details on any info or instance linking tables
    if( sqlite3_changes(m_inputDb) > 0 )
        UpdateLinkingTableStartIds(event_table, std::nullopt);
}


void Syncer::ExtractLinkingTableData(const Table& table, const int64_t start_id)
{
    const std::string insert_data_sql = FormatText("INSERT INTO `extract_db`.`%s` SELECT * FROM `%s` WHERE `id` >= " Formatter_int64_t ";",
                                                   table.m_tableDefinition.name,
                                                   table.m_tableDefinition.name,
                                                   start_id);

    if( sqlite3_exec(m_inputDb, insert_data_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(SyncerErrors::Insert);
}


bool Syncer::UpdateLinkingTableStartIds(Table& table, const std::optional<int64_t> table_start_id)
{
    bool changes_made = false;

    for( const Table::ColumnEntry& column : table.m_columns )
    {
        // check if this column's value points to another table
        if( column.type == Table::ColumnType::Long )
        {
            for( auto& [column_table, start_id] : m_linkingTableStartIds )
            {
                // no need to search if the start ID cannot be any lower
                if( start_id == 1 )
                    continue;

                const size_t table_name_pos = column.name.find(column_table->m_tableDefinition.name);

                // if the table name is at the end, it's a match
                if( ( table_name_pos != std::string::npos ) &&
                    ( ( table_name_pos + strlen(column_table->m_tableDefinition.name) ) == column.name.length() ) )
                {
                    std::string query_min_id_used_sql;

                    // if no start ID is specified, then this is a search of an events table (in the extract database)
                    if( !table_start_id.has_value() )
                    {
                        query_min_id_used_sql = FormatText("SELECT `%s` "
                                                           "FROM `extract_db`.`%s` "
                                                           "WHERE `%s` IS NOT NULL "
                                                           "ORDER BY 1 "
                                                           "LIMIT 1;",
                                                           column.name.c_str(),
                                                           table.m_tableDefinition.name,
                                                           column.name.c_str());
                    }

                    // otherwise this is a search of a secondary link in a linking table (in the main database)
                    else
                    {
                        query_min_id_used_sql = FormatText("SELECT `%s` "
                                                           "FROM `%s` "
                                                           "WHERE ( `id` >= " Formatter_int64_t " ) AND ( `%s` IS NOT NULL ) "
                                                           "ORDER BY 1 "
                                                           "LIMIT 1;",
                                                           column.name.c_str(),
                                                           table.m_tableDefinition.name,
                                                           *table_start_id,
                                                           column.name.c_str());
                    }

                    SQLiteStatement query_min_id_used_stmt(m_inputDb, query_min_id_used_sql);

                    if( query_min_id_used_stmt.Step() == SQLITE_ROW )
                    {
                        const int64_t linking_min_id = query_min_id_used_stmt.GetColumn<int64_t>(0);

                        if( !start_id.has_value() || linking_min_id < *start_id )
                        {
                            start_id = linking_min_id;
                            changes_made = true;
                        }
                    }
                }
            }
        }
    }

    return changes_made;
}


const std::string& Syncer::GetFilePathForReceivedSyncableDatabase()
{
    ASSERT(m_receivedDatabaseTemporaryFiles.empty());

    return m_receivedDatabaseTemporaryFiles.emplace_back().GetPath();
}


void Syncer::SetReceivedSyncableDatabases(std::vector<TemporaryFile> received_database_temporary_files)
{
    ASSERT(m_receivedDatabaseTemporaryFiles.empty());

    m_receivedDatabaseTemporaryFiles = std::move(received_database_temporary_files);
}


void Syncer::UpdateEventBoundaries(const std::vector<EventBoundary>& event_boundaries,
                                   const std::optional<int64_t> delete_events_boundaries_up_to_including_id)
{
    ASSERT(!m_peerLogUuid.empty());

    Sqlite::Transaction transaction(m_inputDb);
    transaction.Begin();

    // delete all of the current entries detailing previous syncs
    if( delete_events_boundaries_up_to_including_id.has_value() )
    {
        SQLiteStatement delete_sync_states_stmt(m_inputDb,
            "DELETE FROM `syncparadata_state` "
            "WHERE `log_uuid` = ? AND `start_event_id` <= ?;");

        delete_sync_states_stmt.Bind(1, m_peerLogUuid);
        delete_sync_states_stmt.Bind(2, *delete_events_boundaries_up_to_including_id);

        if( delete_sync_states_stmt.Step() != SQLITE_DONE )
            throw CSProException(SyncerErrors::ModifySyncState);
    }

    // insert the new entries
    SQLiteStatement insert_sync_state_stmt(m_inputDb,
        "INSERT INTO `syncparadata_state` "
        "( `log_uuid`, `start_event_id`, `end_event_id` ) "
        "VALUES (?, ?, ? );");

    for( const EventBoundary& event_boundary : event_boundaries )
    {
        insert_sync_state_stmt.Bind(1, m_peerLogUuid);
        insert_sync_state_stmt.Bind(2, event_boundary.start_event_id);
        insert_sync_state_stmt.Bind(3, event_boundary.end_event_id);

        if( insert_sync_state_stmt.Step() != SQLITE_DONE )
            throw CSProException(SyncerErrors::ModifySyncState);

        insert_sync_state_stmt.Reset();
    }

    transaction.Commit();
}


void Syncer::MergeReceivedSyncableDatabases()
{
    if( m_receivedDatabaseTemporaryFiles.empty() )
        return;

    // if paradata was received, add it to this log
    FlushAndResetPreparedStatements();

    const int64_t next_event_id = GetMaxEventId().value_or(0) + 1;
    ASSERT(m_syncableEventBoundaries.empty() || m_syncableEventBoundaries.back().end_event_id < next_event_id);
    ASSERT(m_previouslySyncedEventBoundaries.empty() || m_previouslySyncedEventBoundaries.back().end_event_id < next_event_id);

    Concatenator concatenator;

    std::set<std::string> paradata_log_file_paths;

    for( const TemporaryFile& received_database_temporary_file : VI_V(m_receivedDatabaseTemporaryFiles) )
        paradata_log_file_paths.emplace(received_database_temporary_file.GetPath());

    concatenator.Run(m_inputDb, paradata_log_file_paths);

    const std::optional<int64_t> max_event_id_after_concatenation = GetMaxEventId();
    ASSERT(max_event_id_after_concatenation.has_value());

    // add the events to the sync state so that they are not sent back to the peer during a future sync
    if( max_event_id_after_concatenation.has_value() && *max_event_id_after_concatenation >= next_event_id )
        UpdateEventBoundaries({ EventBoundary { next_event_id, *max_event_id_after_concatenation } }, std::nullopt);
}


void Syncer::RunPostSuccessfulSyncTasks()
{
    FlushAndResetPreparedStatements();

    // no need to further update the event boundaries if no paradata was sent
    if( m_syncableEventBoundaries.empty() )
        return;

    // insert the full set of synced events, which includes...

    // ...newly synced (sent) events.
    std::vector<EventBoundary> all_synced_event_boundaries = m_syncableEventBoundaries;

    // ...and previously synced events
    all_synced_event_boundaries.insert(all_synced_event_boundaries.end(), m_previouslySyncedEventBoundaries.begin(),
                                                                          m_previouslySyncedEventBoundaries.end());

    ConsolidateEventBoundaries(all_synced_event_boundaries);

    UpdateEventBoundaries(all_synced_event_boundaries, all_synced_event_boundaries.back().end_event_id);
}
