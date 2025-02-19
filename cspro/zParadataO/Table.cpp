#include "stdafx.h"
#include "Table.h"
#include <zToolsO/PointerClasses.h>
#include <iterator>

using namespace Paradata;


namespace
{
    constexpr const char* ColumnTypeStrings[][2] =
    {
        { "boolean", "INTEGER" },
        { "integer", "INTEGER" },
        { "long",    "INTEGER" },
        { "double",  "REAL" },
        { "text",    "TEXT" },
    };
}


Table::Table(sqlite3* const db, const ParadataTable type)
    :   m_db(db),
        m_tableDefinition(GetTableDefinition(type)),
        m_autoIncrementId(( m_tableDefinition.insert_type == InsertType::AutoIncrement ||
                            m_tableDefinition.insert_type == InsertType::AutoIncrementIfUnique ))
{
}


Table::~Table()
{
}


Table& Table::AddIndex(const cs::string_sz name, const std::initializer_list<size_t> indices)
{
    if( m_indicesSql == nullptr )
        m_indicesSql = std::make_unique<std::vector<std::string>>();

    std::string& sql = m_indicesSql->emplace_back(FormatText("CREATE INDEX IF NOT EXISTS `%s` ON `%s`(", name.c_str(), m_tableDefinition.name));

    for( auto index_itr = indices.begin(); index_itr != indices.end(); ++index_itr )
    {
        sql.append(FormatText("%s`%s`",
                              ( index_itr == indices.begin() ) ? "" : ", ",
                              m_columns[*index_itr].name.c_str()));
    }

    sql.append(");");

    return *this;
}


void Table::CreateTable(const bool check_for_column_completeness)
{
    std::string create_sql = FormatText("CREATE TABLE IF NOT EXISTS `%s` (`id` INTEGER PRIMARY KEY", m_tableDefinition.name);

    std::string insert_sql = FormatText("INSERT %sINTO `%s` ( %s",
                                        ( m_tableDefinition.insert_type == InsertType::WithIdIfNotExist ) ? "OR IGNORE " : "",
                                        m_tableDefinition.name,
                                        m_autoIncrementId ? "" : "`id`");

    std::string insert_values_sql = m_autoIncrementId ? "" : "?";

    m_findDuplicateRowSql = FormatText("SELECT `id` FROM `%s` WHERE", m_tableDefinition.name);

    bool first_column = true;

    for( ColumnEntry& column : m_columns )
    {
        create_sql.append(FormatText(", `%s` %s", column.name.c_str(), ColumnTypeToSqlType(column.type)));

        const char* const comma_text = ( m_autoIncrementId && first_column ) ? "" : ", ";
        insert_sql.append(FormatText("%s`%s`", comma_text, column.name.c_str()));
        insert_values_sql.append(FormatText("%s?", comma_text));

        const char* const and_text = first_column ? "" : " AND";
        m_findDuplicateRowSql.append(FormatText("%s `%s` = ?", and_text, column.name.c_str()));

        first_column = false;
    }

    create_sql.append(");");

    insert_sql.append(FormatText(" ) VALUES ( %s );", insert_values_sql.c_str()));

    m_findDuplicateRowSql.append(" LIMIT 1;");

    // create the table (if necessary)
    if( sqlite3_exec(m_db, create_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw Exception(Exception::Type::CreateTable);

    // the table may have already been created, and if so, make sure that all the columns are defined
    if( check_for_column_completeness )
    {
        std::vector<const ColumnEntry*> columns_to_add;

        try
        {
            const std::string column_query_sql = FormatText("PRAGMA table_info(`%s`);", m_tableDefinition.name);
            SQLiteStatement column_query_stmt(m_db, column_query_sql, true);

            std::transform(m_columns.cbegin(), m_columns.cend(),
                           std::back_inserter(columns_to_add), [](const ColumnEntry& column) { return &column; });

            while( column_query_stmt.Step() == SQLITE_ROW )
            {
                const std::string column_name = column_query_stmt.GetColumn<std::string>(1);
                const auto& lookup = std::find_if(columns_to_add.cbegin(), columns_to_add.cend(),
                                                  [&](const ColumnEntry* const column) { return SO::EqualsNoCase(column_name, column->name); });

                if( lookup != columns_to_add.cend() )
                    columns_to_add.erase(lookup);
            }
        }

        catch(...)
        {
            throw Exception(Exception::Type::UpdateTable);
        }

        for( const ColumnEntry* const column_to_add : columns_to_add )
        {
            // add the missing columns
            const std::string alter_table_sql = FormatText("ALTER TABLE `%s` ADD COLUMN `%s` %s;",
                                                           m_tableDefinition.name,
                                                           column_to_add->name.c_str(),
                                                           ColumnTypeToSqlType(column_to_add->type));

            if( sqlite3_exec(m_db, alter_table_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
                throw Exception(Exception::Type::UpdateTable);
        }
    }

    // create any additional indices
    if( m_indicesSql != nullptr )
    {
        for( const std::string& index_sql : *m_indicesSql )
        {
            if( sqlite3_exec(m_db, index_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
                throw Exception(Exception::Type::CreateIndex);
        }
    }

    // create the prepared statement
    try
    {
        m_insertStmt = std::make_unique<SQLiteStatement>(m_db, insert_sql, true);
    }
    catch(...) { throw Exception(Exception::Type::CreatePreparedStatement); }

    if( m_tableDefinition.insert_type == InsertType::AutoIncrementIfUnique )
    {
        try
        {
            m_findDuplicateRowStmt = std::make_unique<SQLiteStatement>(m_db, m_findDuplicateRowSql, true);
        }
        catch(...) { throw Exception(Exception::Type::CreatePreparedStatement); }
    }
}


Table::ColumnType Table::StringToColumnType(const std::string_view column_type_text_sv)
{
    for( int i = 0; i < _countof(ColumnTypeStrings); i++ )
    {
        if( SO::EqualsNoCase(column_type_text_sv, ColumnTypeStrings[i][0]) )
            return static_cast<Table::ColumnType>(i);
    }

    return ReturnProgrammingError(ColumnType::Text);
}


const char* Table::ColumnTypeToString(const Table::ColumnType column_type)
{
    return ColumnTypeStrings[static_cast<size_t>(column_type)][0];
}


const char* Table::ColumnTypeToSqlType(const Table::ColumnType column_type)
{
    return ColumnTypeStrings[static_cast<size_t>(column_type)][1];
}


void Table::AddMetadata(Table& metadata_table_info_table, Table& metadata_column_info_table, Table& metadata_code_info_table)
{
    // add information about the table
    const char* const insert_type_text =
        m_tableDefinition.insert_type == InsertType::AutoIncrement            ?   "AutoIncrement" :
        m_tableDefinition.insert_type == InsertType::AutoIncrementIfUnique    ?   "AutoIncrementIfUnique" :
        m_tableDefinition.insert_type == InsertType::WithId                   ?   "WithId" :
     /* m_tableDefinition.insert_type == InsertType::WithIdIfNotExist         ? */"WithIdIfNotExist";

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
            m_columns[i].name.c_str(),
            ColumnTypeToString(m_columns[i].type),
            m_columns[i].nullable
        );
    }


    // add information about the table's codes
    for( const CodeEntry& code : m_codes )
    {
        long code_id = 0;
        metadata_code_info_table.Insert(&code_id,
            metadata_column_info_ids[code.column_index],
            code.code,
            code.value.c_str()
        );
    }
}


void Table::BindArguments(sqlite3_stmt* const stmt, int bind_index, va_list args, std::vector<int>* const nullable_arguments/* = nullptr*/)
{
    const bool fill_nullable_arguments = ( nullable_arguments != nullptr && nullable_arguments->empty() );
    const bool skip_binding_nullable_arguments = ( nullable_arguments != nullptr && !nullable_arguments->empty() );

    for( const ColumnEntry& column : m_columns )
    {
        const void* nullable_value = nullptr;

        if( column.nullable )
        {
            nullable_value = va_arg(args, void*);

            if( nullable_value == nullptr )
            {
                if( fill_nullable_arguments )
                {
                    nullable_arguments->emplace_back(bind_index++);
                }

                else if( !skip_binding_nullable_arguments )
                {
                    sqlite3_bind_null(stmt, bind_index++);
                }

                continue;
            }
        }

        if( column.type == ColumnType::Boolean )
        {
            // va_arg specifies an int instead of a bool because types smaller than an int are promoted to int
            const bool value = column.nullable ? *static_cast<const bool*>(nullable_value) :
                                                 static_cast<bool>(va_arg(args, int));
            sqlite3_bind_int(stmt, bind_index++, value ? 1 : 0);
        }

        else if( column.type == ColumnType::Integer )
        {
            const int value = column.nullable ? *static_cast<const int*>(nullable_value) :
                                                va_arg(args, int);
            sqlite3_bind_int(stmt, bind_index++, value);
        }

        else if( column.type == ColumnType::Long )
        {
            const long value = column.nullable ? *static_cast<const long*>(nullable_value) :
                                                 va_arg(args, long);
            sqlite3_bind_int64(stmt, bind_index++, value);
        }

        else if( column.type == ColumnType::Double )
        {
            const double value = column.nullable ? *static_cast<const double*>(nullable_value) :
                                                   va_arg(args, double);
            sqlite3_bind_double(stmt, bind_index++, value);
        }

        else if( column.type == ColumnType::Text )
        {
            const char* const value = column.nullable ? static_cast<const char*>(nullable_value) :
                                                        va_arg(args, const char*);
            sqlite3_bind_text(stmt, bind_index++, value, -1, SQLITE_TRANSIENT);
        }

        else
        {
            ASSERT(false);
        }
    }
}


void Table::InsertWorker(long* const id, ...)
{
    va_list args;

    // check if the row is a duplicate (if necessary)
    if( m_tableDefinition.insert_type == InsertType::AutoIncrementIfUnique )
    {
        cs::non_null_shared_or_raw_ptr<SQLiteStatement> find_duplicate_row_stmt(m_findDuplicateRowStmt.get());
        std::vector<int> nullable_arguments;

        find_duplicate_row_stmt->Reset();

        va_start(args, id);
        BindArguments(m_findDuplicateRowStmt->GetStatement(), 1, args, &nullable_arguments);
        va_end(args);

        // the prepared statement won't work with nullable arguments; the arguments
        // to an AutoIncrementIfUnique table will almost never be null so a proper
        // query will only be constructed on demand
        if( !nullable_arguments.empty() )
        {
            // replace the '= ?' text in the SQL statement with 'is null'
            std::string nullable_find_duplicate_row_sql = m_findDuplicateRowSql;
            size_t search_pos = std::string::npos;
            int arguments_processed = 0;

            for( const int nullable_argument : nullable_arguments )
            {
                for( ; arguments_processed < nullable_argument; ++arguments_processed )
                    search_pos = nullable_find_duplicate_row_sql.find("= ?", search_pos + 1);

                nullable_find_duplicate_row_sql = SO::Concatenate(nullable_find_duplicate_row_sql.substr(0, search_pos),
                                                                  "is null",
                                                                  nullable_find_duplicate_row_sql.substr(search_pos + 3));
            }

            // create the temporary prepared statement
            try
            {
                find_duplicate_row_stmt = std::make_unique<SQLiteStatement>(m_db, nullable_find_duplicate_row_sql, true);
            }
            catch(...) { throw Exception(Exception::Type::CreatePreparedStatement); }

            // bind to it (skipping the null arguments)
            va_start(args, id);
            BindArguments(find_duplicate_row_stmt->GetStatement(), 1, args, &nullable_arguments);
            va_end(args);
        }

        // use the prepared statement to find the duplicate
        if( find_duplicate_row_stmt->Step() == SQLITE_ROW )
        {
            *id = find_duplicate_row_stmt->GetColumn<long>(0);
            return;
        }
    }


    // insert the row
    m_insertStmt->Reset();

    int bind_index = 1;

    if( !m_autoIncrementId )
        m_insertStmt->Bind(bind_index++, *id);

    va_start(args, id);
    BindArguments(m_insertStmt->GetStatement(), bind_index,args);
    va_end(args);

    if( m_insertStmt->Step() != SQLITE_DONE )
        throw Exception(Exception::Type::Insert);

    if( m_autoIncrementId )
        *id = static_cast<long>(sqlite3_last_insert_rowid(m_db));
}
