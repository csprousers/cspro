#include "stdafx.h"
#include "Table.h"
#include <SQLite/SQLiteHelpers.h>

namespace Paradata
{
    Table::Table(sqlite3* db, ParadataTable type)
        :   m_db(db),
            m_tableDefinition(GetTableDefinition(type)),
            m_insertStmt(nullptr),
            m_findDuplicateRowStmt(nullptr)
    {
        m_autoIncrementId =
            ( m_tableDefinition.insert_type == InsertType::AutoIncrement ) ||
            ( m_tableDefinition.insert_type == InsertType::AutoIncrementIfUnique );
    }

    Table::~Table()
    {
        safe_sqlite3_finalize(m_insertStmt);
        safe_sqlite3_finalize(m_findDuplicateRowStmt);
    }

    Table& Table::AddColumn(const CString& name, ColumnType type, bool nullable/* = false*/)
    {
        m_columns.emplace_back(ColumnEntry { name, type, nullable });
        return *this;
    }

    Table& Table::AddCode(int code, const CString& value)
    {
        m_codes.emplace_back(CodeEntry { m_columns.size() - 1, code, value });
        return *this;
    }

    Table& Table::AddIndex(CString name, const std::vector<size_t>& indices)
    {
        CString sql;
        sql.Format(_T("CREATE INDEX IF NOT EXISTS `%s` ON `%s`("), (LPCTSTR)name, m_tableDefinition.name);

        for( auto index_itr = indices.cbegin(); index_itr != indices.cend(); ++index_itr )
            sql.AppendFormat(_T("%s`%s`"), ( index_itr == indices.cbegin() ) ? _T("") : _T(", "), (LPCTSTR)m_columns[*index_itr].name);

        sql.Append(_T(");"));

        m_indicesSql.emplace_back(sql);

        return *this;
    }

    void Table::CreateTable(bool check_for_column_completeness)
    {
        CString create_sql;
        create_sql.Format(_T("CREATE TABLE IF NOT EXISTS `%s` (`id` INTEGER PRIMARY KEY"), m_tableDefinition.name);

        CString insert_sql;
        insert_sql.Format(_T("INSERT %sINTO `%s` ( %s"),
            ( m_tableDefinition.insert_type == InsertType::WithIdIfNotExist ) ? _T("OR IGNORE ") : _T(""),
            m_tableDefinition.name,
            m_autoIncrementId ? _T("") : _T("`id`"));
        CString insert_values_sql = m_autoIncrementId ? _T("") : _T("?");

        m_findDuplicateRowSql.Format(_T("SELECT `id` FROM `%s` WHERE"), m_tableDefinition.name);

        for( auto column_itr = m_columns.cbegin(); column_itr != m_columns.cend(); ++column_itr )
        {
            bool first_column = ( column_itr == m_columns.cbegin() );

            create_sql.AppendFormat(_T(", `%s` %s"), (LPCTSTR)column_itr->name, ColumnTypeToSqlType(column_itr->type));

            const TCHAR* comma_text = ( m_autoIncrementId && first_column ) ? _T("") : _T(", ");
            insert_sql.AppendFormat(_T("%s`%s`"), comma_text, (LPCTSTR)column_itr->name);
            insert_values_sql.AppendFormat(_T("%s?"), comma_text);

            const TCHAR* and_text = first_column ? _T("") : _T(" AND");
            m_findDuplicateRowSql.AppendFormat(_T("%s `%s` = ?"), and_text, (LPCTSTR)column_itr->name);
        }

        create_sql.Append(_T(");"));

        insert_sql.AppendFormat(_T(" ) VALUES ( %s );"), (LPCTSTR)insert_values_sql);

        m_findDuplicateRowSql.Append(_T(" LIMIT 1;"));

        // create the table (if necessary)
        if( sqlite3_exec(m_db, ToUtf8(create_sql), nullptr, nullptr, nullptr) != SQLITE_OK )
            throw Exception(Exception::Type::CreateTable);

        // the table may have already been created, and if so, make sure that all the columns are defined
        if( check_for_column_completeness )
        {
            CString column_query_sql;
            column_query_sql.Format(_T("PRAGMA table_info(`%s`)"), m_tableDefinition.name);

            sqlite3_stmt* stmt;

            if( sqlite3_prepare_v2(m_db,ToUtf8(column_query_sql), -1, &stmt, nullptr) != SQLITE_OK )
                throw Exception(Exception::Type::UpdateTable);

            std::vector<ColumnEntry> columns_to_add = m_columns;

            while( sqlite3_step(stmt) == SQLITE_ROW )
            {
                CString column_name = FromUtf8(sqlite3_column_text(stmt, 1));

                for( auto itr = columns_to_add.begin(); itr != columns_to_add.end(); ++itr )
                {
                    if( column_name.CompareNoCase(itr->name) == 0 )
                    {
                        columns_to_add.erase(itr);
                        break;
                    }
                }
            }

            safe_sqlite3_finalize(stmt);

            for( const auto& column_to_add : columns_to_add )
            {
                // add the missing columns
                CString alter_table_sql;
                alter_table_sql.Format(_T("ALTER TABLE `%s` ADD COLUMN `%s` %s;"),
                    m_tableDefinition.name, (LPCTSTR)column_to_add.name, ColumnTypeToSqlType(column_to_add.type));

                if( sqlite3_exec(m_db, ToUtf8(alter_table_sql), nullptr, nullptr, nullptr) != SQLITE_OK )
                    throw Exception(Exception::Type::UpdateTable);
            }
        }

        // create any additional indices
        for( const auto& index_sql : m_indicesSql )
        {
            if( sqlite3_exec(m_db, ToUtf8(index_sql), nullptr, nullptr, nullptr) != SQLITE_OK )
                throw Exception(Exception::Type::CreateIndex);
        }

        // create the prepared statement
        if( sqlite3_prepare_v2(m_db, ToUtf8(insert_sql), -1, &m_insertStmt, nullptr) != SQLITE_OK )
            throw Exception(Exception::Type::CreatePreparedStatement);

        if( m_tableDefinition.insert_type == InsertType::AutoIncrementIfUnique )
        {
            if( sqlite3_prepare_v2(m_db, ToUtf8(m_findDuplicateRowSql), -1, &m_findDuplicateRowStmt, nullptr) != SQLITE_OK )
                throw Exception(Exception::Type::CreatePreparedStatement);
        }
    }

    const TCHAR* ColumnTypeStrings[][2] =
    {
        { _T("boolean"), _T("INTEGER") },
        { _T("integer"), _T("INTEGER") },
        { _T("long"),    _T("INTEGER") },
        { _T("double"),  _T("REAL") },
        { _T("text"),    _T("TEXT") }
    };

    Table::ColumnType Table::StringToColumnType(const CString& column_type_text)
    {
        for( int i = 0; i < _countof(ColumnTypeStrings); i++ )
        {
            if( column_type_text.CompareNoCase(ColumnTypeStrings[i][0]) == 0 )
                return (Table::ColumnType)i;
        }

        ASSERT(false);
        return ColumnType::Text;
    }

    const TCHAR* Table::ColumnTypeToString(Table::ColumnType column_type)
    {
        return ColumnTypeStrings[(int)column_type][0];
    }

    const TCHAR* Table::ColumnTypeToSqlType(Table::ColumnType column_type)
    {
        return ColumnTypeStrings[(int)column_type][1];
    }

    void Table::AddMetadata(Table& metadata_table_info_table, Table& metadata_column_info_table, Table& metadata_code_info_table)
    {
        // add information about the table
        const TCHAR* insert_type_text =
            m_tableDefinition.insert_type == InsertType::AutoIncrement            ?   _T("AutoIncrement") :
            m_tableDefinition.insert_type == InsertType::AutoIncrementIfUnique    ?   _T("AutoIncrementIfUnique") :
            m_tableDefinition.insert_type == InsertType::WithId                   ?   _T("WithId") :
            /*m_tableDefinition.insert_type == InsertType::WithIdIfNotExist       ? */_T("WithIdIfNotExist");

        long metadata_table_info_id = 0;
        metadata_table_info_table.Insert(&metadata_table_info_id,
                m_tableDefinition.name,
                m_tableDefinition.table_code,
                insert_type_text
            );


        // add information about the table's columns
        std::vector<long> metadata_column_info_ids(m_columns.size(), 0);

        for( size_t i = 0; i < m_columns.size(); ++i )
        {
            metadata_column_info_table.Insert(&metadata_column_info_ids[i],
                metadata_table_info_id,
                (LPCTSTR)m_columns[i].name,
                ColumnTypeToString(m_columns[i].type),
                m_columns[i].nullable
            );
        }


        // add information about the table's codes
        for( const auto& code : m_codes )
        {
            long code_id = 0;
            metadata_code_info_table.Insert(&code_id,
                metadata_column_info_ids[code.column_index],
                code.code,
                (LPCTSTR)code.value
            );
        }
    }

    void Table::BindArguments(sqlite3_stmt* stmt, int bind_index, va_list args, std::vector<int>* nullable_arguments/* = nullptr*/)
    {
        bool fill_nullable_arguments = ( ( nullable_arguments != nullptr ) && nullable_arguments->empty() );
        bool skip_binding_nullable_arguments = ( ( nullable_arguments != nullptr ) && !nullable_arguments->empty() );

        for( const auto& column : m_columns )
        {
            void* nullable_value = nullptr;

            if( column.nullable )
            {
                nullable_value = va_arg(args, void*);

                if( nullable_value == nullptr )
                {
                    if( fill_nullable_arguments )
                        nullable_arguments->emplace_back(bind_index++);

                    else if( !skip_binding_nullable_arguments )
                        sqlite3_bind_null(stmt, bind_index++);

                    continue;
                }
            }

            if( column.type == ColumnType::Boolean )
            {
                // va_arg specifies an int instead of a bool because types smaller than an int are promoted to int
                bool value = column.nullable ? *((bool*)nullable_value) : (bool)va_arg(args, int);
                sqlite3_bind_int(stmt, bind_index++, value ? 1 : 0);
            }

            else if( column.type == ColumnType::Integer )
            {
                int value = column.nullable ? *((int*)nullable_value) : va_arg(args, int);
                sqlite3_bind_int(stmt, bind_index++, value);
            }

            else if( column.type == ColumnType::Long )
            {
                int value = column.nullable ? *((long*)nullable_value) : va_arg(args, long);
                sqlite3_bind_int64(stmt, bind_index++, value);
            }

            else if( column.type == ColumnType::Double )
            {
                double value = column.nullable ? *((double*)nullable_value) : va_arg(args, double);
                sqlite3_bind_double(stmt, bind_index++, value);
            }

            else // if( column.type == ColumnType::Text )
            {
                const TCHAR* value = column.nullable ? (LPCTSTR)nullable_value : va_arg(args, LPCTSTR);
                sqlite3_bind_text(stmt, bind_index++, ToUtf8(value), -1, SQLITE_TRANSIENT);
            }
        }
    }

    void Table::Insert(long* id, ...)
    {
        va_list args;

        // check if the row is a duplicate (if necessary)
        if( m_tableDefinition.insert_type == InsertType::AutoIncrementIfUnique )
        {
            sqlite3_stmt* this_find_duplicate_row_stmt = m_findDuplicateRowStmt;
            sqlite3_stmt* nullable_find_duplicate_row_stmt = nullptr;
            std::vector<int> nullable_arguments;

            sqlite3_reset(m_findDuplicateRowStmt);

            va_start(args, id);
            BindArguments(m_findDuplicateRowStmt, 1, args, &nullable_arguments);
            va_end(args);

            // the prepared statement won't work with nullable arguments; the arguments
            // to an AutoIncrementIfUnique table will almost never be null so a proper
            // query will only be constructed on demand
            if( !nullable_arguments.empty() )
            {
                // replace the '= ?' text in the SQL statement with 'is null'
                CString nullable_find_duplicate_row_sql = m_findDuplicateRowSql;
                int search_pos = -1;
                int arguments_processed = 0;

                for( const auto& nullable_argument : nullable_arguments )
                {
                    for( ; arguments_processed < nullable_argument; ++arguments_processed )
                        search_pos = nullable_find_duplicate_row_sql.Find(_T("= ?"), search_pos + 1);

                    nullable_find_duplicate_row_sql = nullable_find_duplicate_row_sql.Left(search_pos) + _T("is null") +
                        nullable_find_duplicate_row_sql.Mid(search_pos + 3);
                }

                // create the temporary prepared statement
                if( sqlite3_prepare_v2(m_db, ToUtf8(nullable_find_duplicate_row_sql), -1, &nullable_find_duplicate_row_stmt, nullptr) != SQLITE_OK )
                    throw Exception(Exception::Type::CreatePreparedStatement);

                // bind to it (skipping the null arguments)
                va_start(args, id);
                BindArguments(nullable_find_duplicate_row_stmt, 1, args, &nullable_arguments);
                va_end(args);

                this_find_duplicate_row_stmt = nullable_find_duplicate_row_stmt;
            }

            // use the prepared statement
            bool duplicate_found = false;

            if( sqlite3_step(this_find_duplicate_row_stmt) == SQLITE_ROW ) // a duplicate
            {
                duplicate_found = true;
                *id = (long)sqlite3_column_int64(this_find_duplicate_row_stmt, 0);
            }

            safe_sqlite3_finalize(nullable_find_duplicate_row_stmt);

            if( duplicate_found )
                return;
        }


        // insert the row
        sqlite3_reset(m_insertStmt);

        int bind_index = 1;

        if( !m_autoIncrementId )
            sqlite3_bind_int64(m_insertStmt, bind_index++, *id);

        va_start(args, id);
        BindArguments(m_insertStmt, bind_index,args);
        va_end(args);

        if( sqlite3_step(m_insertStmt) != SQLITE_DONE )
            throw Exception(Exception::Type::Insert);

        if( m_autoIncrementId )
            *id = (long)sqlite3_last_insert_rowid(m_db);
    }
}
