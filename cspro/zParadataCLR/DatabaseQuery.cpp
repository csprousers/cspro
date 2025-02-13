#include "Stdafx.h"
#include "Database.h"
#include "DatabaseQuery.h"
#include <SQLite/SQLiteHelpers.h>

namespace CSPro
{
    namespace ParadataViewer
    {
        DatabaseQuery::DatabaseQuery(sqlite3_stmt* stmt)
            :   m_stmt(stmt)
        {
            m_iNumberColumns = sqlite3_column_count(m_stmt);
            m_bGetResultsExecutedAtLeastOnce = false;
            m_bNextRowAlreadyStepped = false;
        }

        int DatabaseQuery::ColumnCount::get()
        {
            return m_iNumberColumns;
        }

        array<System::String^>^ DatabaseQuery::ColumnNames::get()
        {
            auto names = gcnew array<System::String^>(m_iNumberColumns);

            for( int iColumn = 0; iColumn < m_iNumberColumns; iColumn++ )
            {
                CString csColumnName = FromUtf8(sqlite3_column_name(m_stmt,iColumn));
                names[iColumn] = gcnew System::String(csColumnName);
            }

            return names;
        }

        System::Collections::Generic::List<array<System::Object^>^>^ DatabaseQuery::GetResults(int iMaxNumberResults)
        {
            m_bGetResultsExecutedAtLeastOnce = true;

            auto rows = gcnew System::Collections::Generic::List<array<System::Object^>^>();
            int iRows = 0;
            int iSqlResult = SQLITE_ROW;

            while( ( iRows < iMaxNumberResults ) &&
                ( m_bNextRowAlreadyStepped || ( ( iSqlResult = sqlite3_step(m_stmt) ) == SQLITE_ROW ) ) )
            {
                m_bNextRowAlreadyStepped = false;

                auto row = gcnew array<System::Object^>(m_iNumberColumns);
                rows->Add(row);
                iRows++;

                for( int iColumn = 0; iColumn < m_iNumberColumns; iColumn++ )
                {
                    if( sqlite3_column_type(m_stmt,iColumn) == SQLITE_NULL )
                        continue;

                    else if( sqlite3_column_type(m_stmt,iColumn) == SQLITE_TEXT )
                    {
                        CString csValue = FromUtf8(sqlite3_column_text(m_stmt,iColumn));
                        row[iColumn] = gcnew System::String(csValue);
                    }

                    else
                    {
                        double dValue = sqlite3_column_double(m_stmt,iColumn);
                        row[iColumn] = gcnew System::Double(dValue);
                    }
                }
            }

            // if SQLITE_DONE wasn't the last return value, then iMaxNumberResults was hit, but read
            // the next row to see if all rows have been read
            if( iSqlResult != SQLITE_DONE )
                m_bNextRowAlreadyStepped = ( sqlite3_step(m_stmt) == SQLITE_ROW );

            // reset the statement if all results have been returned
            if( !m_bNextRowAlreadyStepped )
                sqlite3_reset(m_stmt);

            return rows;
        }

        System::Nullable<bool> DatabaseQuery::AdditionalResultsAvailable::get()
        {
            return m_bGetResultsExecutedAtLeastOnce ?
                System::Nullable<bool>(m_bNextRowAlreadyStepped) :
                System::Nullable<bool>();
        }
    }
}
