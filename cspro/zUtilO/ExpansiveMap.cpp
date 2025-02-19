#include "StdAfx.h"
#include "ExpansiveMap.h"
#include <zSql/Commands.h>


ExpansiveMap_SqliteDb::ExpansiveMap_SqliteDb(std::string create_index_sql, SQLiteStatement stmt_insert, SQLiteStatement stmt_find)
    :   m_createIndexSql(std::move(create_index_sql)),
        m_stmtInsert(std::move(stmt_insert)),
        m_stmtFind(std::move(stmt_find))
{
}


std::unique_ptr<ExpansiveMap_SqliteDb> ExpansiveMap_SqliteDb::Create(sqlite3*& db, const int number_keys, const std::vector<const char*>& key_data_types,
                                                                     const char* const value_data_type)
{
    const bool must_finalize_db = ( db == nullptr );

    if( must_finalize_db )
    {
        // passing "" creates a temporary database that is automatically cleaned up when the connection is closed;
        // SQLite will create the database in memory although it may flush certain parts to temporary files on disk if needed
        if( sqlite3_open("", &db) != SQLITE_OK )
            throw CSProException(ExceptionMessage);

        // disable synchronous writes and journaling to get the max performance of writes
        ExpansiveMap_SqliteDb::DoExec(db, "PRAGMA synchronous = off;");
        ExpansiveMap_SqliteDb::DoExec(db, "PRAGMA journal_mode = off;");
    }

    std::string create_table_sql = "CREATE TABLE `ex_map` (";
    std::string create_index_sql = "CREATE INDEX `ex_map_index` ON `ex_map` (";
    std::string insert_sql = "INSERT INTO `ex_map` (";
    std::string find_sql = "SELECT `value` FROM `ex_map` WHERE ";
    int key_counter = 0;

    for( const char* const key_data_type : key_data_types )
    {
        if( key_counter != 0 )
        {
            create_table_sql.append(", ");
            create_index_sql.append(", ");
            insert_sql.append(", ");
            find_sql.append(" AND ");
        }

        ++key_counter;
        const std::string key_name = FormatText("`key%d`", key_counter);

        create_table_sql.append(key_name).append(" ").append(key_data_type).append(" NOT NULL");
        create_index_sql.append(key_name);
        insert_sql.append(key_name);
        find_sql.append(key_name).append("= ?");
    }

    ASSERT(key_counter > 0 && key_counter == number_keys);

    create_table_sql.append(", `value` ")
                    .append(value_data_type)
                    .append(" NOT NULL);");
    DoExec(db, create_table_sql);

    create_index_sql.append(");");
    DoExec(db, create_index_sql);

    insert_sql.append(", `value`) VALUES (?");

    for( int i = 0; i < number_keys; ++i )
        insert_sql.append(",?");

    insert_sql.append(");");

    find_sql.append(" LIMIT 1;");

    try
    {
        return std::unique_ptr<ExpansiveMap_SqliteDb>(new ExpansiveMap_SqliteDb(std::move(create_index_sql),
                                                                                SQLiteStatement(db, insert_sql.c_str(), true),
                                                                                SQLiteStatement(db, find_sql.c_str(), true)));
    }

    catch(...)
    {
        throw CSProException(ExceptionMessage);
    }
}


void ExpansiveMap_SqliteDb::DoExec(sqlite3* const db, const cs::string_sz sql)
{
    if( sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(ExceptionMessage);
}


void ExpansiveMap_SqliteDb::DoBulkInsertions(sqlite3* const db, const double rebuild_index_max_proportion,
                                             const size_t number_insertions, const std::function<void()>& callback_function)
{
    bool need_to_drop_and_rebuild_index;

    if( rebuild_index_max_proportion == 0 )
    {
        need_to_drop_and_rebuild_index = false;
    }

    else if( rebuild_index_max_proportion == 1 )
    {
        need_to_drop_and_rebuild_index = true;
    }

    else
    {
        // if specified, count how many values have been inserted and determine if it makes sense to drop and rebuild the index
        if( m_stmtCount == nullptr )
            m_stmtCount = std::make_unique<SQLiteStatement>(db, "SELECT COUNT(*) FROM `ex_map`;", true);

        const SQLiteResetOnDestruction rod(*m_stmtCount);
        m_stmtCount->StepCheckResult(SQLITE_ROW);

        const int64_t existing_values = m_stmtCount->GetColumn<int64_t>(0);

        if( existing_values == 0 )
        {
            need_to_drop_and_rebuild_index = false;
        }

        else
        {
            const double insertion_proportion = 1 - CreateProportion(number_insertions, number_insertions + existing_values);
            need_to_drop_and_rebuild_index = ( insertion_proportion <= rebuild_index_max_proportion );
        }
    }

    // drop the index before doing the insertions
    if( need_to_drop_and_rebuild_index )
        DoExec(db, "DROP INDEX `ex_map_index`;");

    // do the insertions
    DoExec(db, Sqlite::Commands::BeginTransaction);
    callback_function();
    DoExec(db, Sqlite::Commands::EndTransaction);

    // rebuild the index
    if( need_to_drop_and_rebuild_index )
        DoExec(db, m_createIndexSql);
}
