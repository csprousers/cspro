#include "StdAfx.h"
#include "ExpansiveMap.h"
#include <zSql/DB.h>


ExpansiveMap_Db::ExpansiveMap_Db(sqlite3* const non_owned_db,
                                 const int number_keys, const std::vector<const char*>& key_data_types,
                                 const char* const value_data_type)
{
    // use a non-owned database if provided
    if( non_owned_db != nullptr )
    {
        m_db = std::make_unique<Sqlite::DB>(Sqlite::DB::CreateWrapper(non_owned_db, false));
    }

    // otherwise passing "" creates a temporary database that is automatically cleaned up when the connection is closed;
    // SQLite will create the database in memory although it may flush certain parts to temporary files on disk if needed
    else
    {
        m_db = std::make_unique<Sqlite::DB>("", Sqlite::OpenFlags::ReadWrite | Sqlite::OpenFlags::Create);

        // disable synchronous writes and journaling to get the max performance of writes
        m_db->Execute("PRAGMA synchronous = off;");
        m_db->Execute("PRAGMA journal_mode = off;");
    }

    std::string create_table_sql = "CREATE TABLE `ex_map` (";
    m_createIndexSql = "CREATE INDEX `ex_map_index` ON `ex_map` (";
    std::string insert_sql = "INSERT INTO `ex_map` (";
    std::string find_sql = "SELECT `value` FROM `ex_map` WHERE ";
    int key_counter = 0;

    for( const char* const key_data_type : key_data_types )
    {
        if( key_counter != 0 )
        {
            create_table_sql.append(", ");
            m_createIndexSql.append(", ");
            insert_sql.append(", ");
            find_sql.append(" AND ");
        }

        ++key_counter;
        const std::string key_name = FormatText("`key%d`", key_counter);

        create_table_sql.append(key_name).append(" ").append(key_data_type).append(" NOT NULL");
        m_createIndexSql.append(key_name);
        insert_sql.append(key_name);
        find_sql.append(key_name).append("= ?");
    }

    ASSERT(key_counter > 0 && key_counter == number_keys);

    create_table_sql.append(", `value` ")
                    .append(value_data_type)
                    .append(" NOT NULL);");
    m_db->Execute(create_table_sql);

    m_createIndexSql.append(");");
    m_db->Execute(m_createIndexSql);

    insert_sql.append(", `value`) VALUES (?");

    for( int i = 0; i < number_keys; ++i )
        insert_sql.append(",?");

    insert_sql.append(");");

    find_sql.append(" LIMIT 1;");

    m_stmtInsert = m_db->PrepareStatement(insert_sql);
    m_stmtFind = m_db->PrepareStatement(find_sql);
}


ExpansiveMap_Db::~ExpansiveMap_Db()
{
}


void ExpansiveMap_Db::Clear()
{
    m_db->Execute("DELETE FROM `ex_map`;");
}


void ExpansiveMap_Db::DoBulkInsertions(const double rebuild_index_max_proportion,
                                       const size_t number_insertions,
                                       const std::function<void()>& callback_function)
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
        // if specified, count how many values have been inserted and determine
        // if it makes sense to drop and rebuild the index
        if( !m_stmtCount.IsPrepared() )
            m_stmtCount = m_db->PrepareStatement("SELECT COUNT(*) FROM `ex_map`;");

        const Sqlite::Statement::Resetter stmt_resetter(m_stmtCount);
        m_stmtCount.StepCheckResult(Sqlite::Result::Row);

        const int64_t existing_values = m_stmtCount.GetColumn<int64_t>(0);

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
        m_db->Execute("DROP INDEX `ex_map_index`;");

    // do the insertions
    Sqlite::Transaction transaction = m_db->CreateTransaction();
    transaction.Begin();

    callback_function();

    transaction.Commit();

    // rebuild the index
    if( need_to_drop_and_rebuild_index )
        m_db->Execute(m_createIndexSql);
}
