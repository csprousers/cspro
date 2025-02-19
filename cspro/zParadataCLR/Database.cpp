#include "Stdafx.h"
#include "Database.h"
#include "DatabaseQuery.h"
#include <zUtilO/SqlLogicFunctions.h>
#include <zParadataO/Log.h>
#include <zParadataO/ParadataException.h>


namespace CSPro::ParadataViewer::Errors
{
    constexpr const char* CreatePreparedStatement = "Could not create a prepared statement";
    constexpr const char* NonQuery                = "Could not execute a non-query";
    constexpr const char* Query                   = "Could not execute a query";
    constexpr const char* SqlSyntaxFormatter      = "SQL syntax: %s";
}


CSPro::ParadataViewer::Database::Database(System::String^ file_path)
    :   m_db(nullptr),
        m_stmts(nullptr)
{
    try
    {
        m_db = Paradata::Log::GetDatabaseForTool(clr_helpers::to_string(file_path), false);

        SqlLogicFunctions::RegisterCallbackFunctions(m_db);
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(clr_helpers::to_SystemString(exception.what()));
    }

    m_stmts = new std::vector<sqlite3_stmt*>;
}


CSPro::ParadataViewer::Database::!Database()
{
    for( sqlite3_stmt* const stmt : *m_stmts )
        sqlite3_finalize(stmt);

    delete m_stmts;

    if( m_db != nullptr )
        sqlite3_close(m_db);
}


void CSPro::ParadataViewer::Database::ExecuteNonQuery(System::String^ sql)
{
    const std::string utf8_sql = clr_helpers::to_string(sql);

    if( sqlite3_exec(m_db, utf8_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw gcnew System::Exception(gcnew System::String(Errors::NonQuery));
}


int64_t CSPro::ParadataViewer::Database::ExecuteSingleQuery(System::String^ sql)
{
    const std::string utf8_sql = clr_helpers::to_string(sql);
    sqlite3_stmt* stmt;

    if( sqlite3_prepare_v2(m_db, utf8_sql.c_str(), utf8_sql.length(), &stmt, nullptr) != SQLITE_OK )
        throw gcnew System::Exception(gcnew System::String(Errors::CreatePreparedStatement));

    if( sqlite3_step(stmt) != SQLITE_ROW )
        throw gcnew System::Exception(gcnew System::String(Errors::Query));

    int64_t value = sqlite3_column_int64(stmt, 0);

    sqlite3_finalize(stmt);

    return value;
}


System::Collections::Generic::List<array<System::Object^>^>^ CSPro::ParadataViewer::Database::ExecuteQuery(System::String^ sql)
{
    const std::string utf8_sql = clr_helpers::to_string(sql);
    sqlite3_stmt* stmt;

    if( sqlite3_prepare_v2(m_db, utf8_sql.c_str(), utf8_sql.length(), &stmt, nullptr) != SQLITE_OK )
        throw gcnew System::Exception(gcnew System::String(Errors::CreatePreparedStatement));

    const int number_columns = sqlite3_column_count(stmt);

    auto rows = gcnew System::Collections::Generic::List<array<System::Object^>^>();

    while( sqlite3_step(stmt) == SQLITE_ROW )
    {
        auto row = gcnew array<System::Object^>(number_columns);
        rows->Add(row);

        for( int column = 0; column < number_columns; ++column )
        {
            if( sqlite3_column_type(stmt, column) == SQLITE_NULL )
            {
                // nothing to do
            }

            else if( sqlite3_column_type(stmt,column) == SQLITE_TEXT )
            {
                row[column] = clr_helpers::to_SystemString(std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(stmt, column))));
            }

            else
            {
                row[column] = gcnew System::Double(sqlite3_column_double(stmt, column));
            }
        }
    }

    sqlite3_finalize(stmt);

    return rows;
}


CSPro::ParadataViewer::DatabaseQuery^ CSPro::ParadataViewer::Database::CreateQuery(System::String^ sql)
{
    const std::string utf8_sql = clr_helpers::to_string(sql);
    sqlite3_stmt* stmt;

    if( sqlite3_prepare_v2(m_db, utf8_sql.c_str(), utf8_sql.length(), &stmt, nullptr) != SQLITE_OK )
        throw gcnew System::Exception(clr_helpers::to_FormattedSystemString(Errors::SqlSyntaxFormatter, sqlite3_errmsg(m_db)));

    m_stmts->emplace_back(stmt);

    return gcnew DatabaseQuery(stmt);
}
