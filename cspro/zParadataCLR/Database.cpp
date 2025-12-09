#include "Stdafx.h"
#include "Database.h"
#include "DatabaseQuery.h"
#include <zUtilO/SqlLogicFunctions.h>
#include <zParadataO/Log.h>


CSPro::ParadataViewer::Database::Database(System::String^ file_path)
    :   m_db(nullptr)
{
    try
    {
        sqlite3* const db = Paradata::Log::GetDatabaseForTool(clr_helpers::to_string(file_path), false);
        SqlLogicFunctions::RegisterCallbackFunctions(db);

        m_db = new Sqlite::DB(Sqlite::DB::CreateWrapper(db, true));
    }

    catch( const CSProException& exception )
    {
        throw clr_helpers::to_SystemException(exception);
    }
}


CSPro::ParadataViewer::Database::!Database()
{
    delete m_db;
}


System::Object^ CSPro::ParadataViewer::Database::GetSqlResult(Sqlite::Statement& stmt, const int column_number)
{
    switch( stmt.GetColumnType(column_number) )
    {
        case Sqlite::ColumnType::Null:
            return nullptr;

        case Sqlite::ColumnType::Text:
            return clr_helpers::to_SystemString(stmt.GetColumn<std::string>(column_number));

        default:
            return stmt.GetColumn<double>(column_number);
    }
}


void CSPro::ParadataViewer::Database::ExecuteNonQuery(System::String^ sql)
{
    try
    {
        const std::string utf8_sql = clr_helpers::to_string(sql);
        m_db->Execute(utf8_sql);
    }

    catch( const CSProException& exception )
    {
        throw clr_helpers::to_SystemException(exception);
    }
}


int64_t CSPro::ParadataViewer::Database::ExecuteSingleQuery(System::String^ sql)
{
    try
    {
        const std::string utf8_sql = clr_helpers::to_string(sql);
        Sqlite::Statement stmt = m_db->PrepareStatement(utf8_sql);

        stmt.StepCheckResult(Sqlite::Result::Row);

        return stmt.GetColumn<int64_t>(0);
    }

    catch( const CSProException& exception )
    {
        throw clr_helpers::to_SystemException(exception);
    }
}


System::Collections::Generic::List<array<System::Object^>^>^ CSPro::ParadataViewer::Database::ExecuteQuery(System::String^ sql)
{
    try
    {
        const std::string utf8_sql = clr_helpers::to_string(sql);
        Sqlite::Statement stmt = m_db->PrepareStatement(utf8_sql);

        const int number_columns = stmt.GetColumnCount();

        auto rows = gcnew System::Collections::Generic::List<array<System::Object^>^>();

        while( stmt.Step() == Sqlite::Result::Row )
        {
            auto row = gcnew array<System::Object^>(number_columns);
            rows->Add(row);

            for( int column = 0; column < number_columns; ++column )
                row[column] = GetSqlResult(stmt, column);
        }

        return rows;
    }

    catch( const CSProException& exception )
    {
        throw clr_helpers::to_SystemException(exception);
    }
}


CSPro::ParadataViewer::DatabaseQuery^ CSPro::ParadataViewer::Database::CreateQuery(System::String^ sql)
{
    try
    {
        const std::string utf8_sql = clr_helpers::to_string(sql);
        return gcnew DatabaseQuery(m_db->PrepareStatement(utf8_sql));
    }

    catch( const CSProException& exception )
    {
        throw clr_helpers::to_SystemException(exception);
    }
}
