#include "Stdafx.h"
#include "Database.h"
#include "DatabaseQuery.h"
#include <SQLite/SQLiteHelpers.h>
#include <zUtilO/SqlLogicFunctions.h>
#include <zParadataO/Log.h>
#include <zParadataO/ParadataException.h>


namespace CSPro
{
    namespace ParadataViewer
    {
        namespace Errors
        {
            constexpr const TCHAR* CreatePreparedStatement = _T("Could not create a prepared statement");
            constexpr const TCHAR* NonQuery                = _T("Could not execute a non-query");
            constexpr const TCHAR* Query                   = _T("Could not execute a query");
            constexpr const TCHAR* SqlSyntaxFormatter      = _T("SQL syntax: %s");
        }

        Database::Database(System::String^ filename)
            :   m_db(nullptr),
                m_paStmts(nullptr)
        {
            try
            {
                m_db = Paradata::Log::GetDatabaseForTool(CString(filename), false);

                SqlLogicFunctions::RegisterCallbackFunctions(m_db);
            }

            catch( const CSProException& exception )
            {
                throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
            }

            m_paStmts = new std::vector<sqlite3_stmt*>;
        }

        Database::!Database()
        {
            for( auto stmt : *m_paStmts )
                safe_sqlite3_finalize(stmt);

            delete m_paStmts;

            if( m_db != nullptr )
                sqlite3_close(m_db);
        }


        void Database::ExecuteNonQuery(System::String^ sql)
        {
            CString csSql(sql);

            if( sqlite3_exec(m_db,ToUtf8(csSql),nullptr,nullptr,nullptr) != SQLITE_OK )
                throw gcnew System::Exception(gcnew System::String(Errors::NonQuery));
        }


        int64_t Database::ExecuteSingleQuery(System::String^ sql)
        {
            CString csSql(sql);
            int64_t iValue = 0;
            sqlite3_stmt* stmt = nullptr;

            if( sqlite3_prepare_v2(m_db,ToUtf8(csSql),-1,&stmt,nullptr) != SQLITE_OK )
                throw gcnew System::Exception(gcnew System::String(Errors::CreatePreparedStatement));

            if( sqlite3_step(stmt) != SQLITE_ROW )
                throw gcnew System::Exception(gcnew System::String(Errors::Query));

            iValue = sqlite3_column_int64(stmt,0);

            safe_sqlite3_finalize(stmt);

            return iValue;
        }


        System::Collections::Generic::List<array<System::Object^>^>^ Database::ExecuteQuery(System::String^ sql)
        {
            CString csSql(sql);
            auto rows = gcnew System::Collections::Generic::List<array<System::Object^>^>();
            sqlite3_stmt* stmt = nullptr;

            if( sqlite3_prepare_v2(m_db,ToUtf8(csSql),-1,&stmt,nullptr) != SQLITE_OK )
                throw gcnew System::Exception(gcnew System::String(Errors::CreatePreparedStatement));

            int iNumberColumns = sqlite3_column_count(stmt);

            while( sqlite3_step(stmt) == SQLITE_ROW )
            {
                auto row = gcnew array<System::Object^>(iNumberColumns);
                rows->Add(row);

                for( int iColumn = 0; iColumn < iNumberColumns; iColumn++ )
                {
                    if( sqlite3_column_type(stmt,iColumn) == SQLITE_NULL )
                        continue;

                    else if( sqlite3_column_type(stmt,iColumn) == SQLITE_TEXT )
                    {
                        CString csValue = FromUtf8(sqlite3_column_text(stmt,iColumn));
                        row[iColumn] = gcnew System::String(csValue);
                    }

                    else
                    {
                        double dValue = sqlite3_column_double(stmt,iColumn);
                        row[iColumn] = gcnew System::Double(dValue);
                    }
                }
            }

            safe_sqlite3_finalize(stmt);

            return rows;
        }


        DatabaseQuery^ Database::CreateQuery(System::String^ sql)
        {
            CString csSql(sql);
            sqlite3_stmt* stmt = nullptr;

            if( sqlite3_prepare_v2(m_db,ToUtf8(csSql),-1,&stmt,nullptr) != SQLITE_OK )
            {
                CString csErrorMessage;
                csErrorMessage.Format(Errors::SqlSyntaxFormatter,(LPCTSTR)FromUtf8(sqlite3_errmsg(m_db)));
                throw gcnew System::Exception(gcnew System::String(csErrorMessage));
            }

            m_paStmts->push_back(stmt);

            return gcnew DatabaseQuery(stmt);
        }
    }
}
