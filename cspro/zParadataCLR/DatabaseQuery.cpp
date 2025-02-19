#include "Stdafx.h"
#include "Database.h"
#include "DatabaseQuery.h"


CSPro::ParadataViewer::DatabaseQuery::DatabaseQuery(sqlite3_stmt* const stmt)
    :   m_stmt(stmt),
        m_numberColumns(sqlite3_column_count(m_stmt)),
        m_getResultsExecutedAtLeastOnce(false),
        m_nextRowAlreadyStepped(false)
{
}


int CSPro::ParadataViewer::DatabaseQuery::ColumnCount::get()
{
    return m_numberColumns;
}


array<System::String^>^ CSPro::ParadataViewer::DatabaseQuery::ColumnNames::get()
{
    auto names = gcnew array<System::String^>(m_numberColumns);

    for( int column = 0; column < m_numberColumns; ++column )
        names[column] = clr_helpers::to_SystemString(std::string_view(sqlite3_column_name(m_stmt, column)));

    return names;
}


System::Collections::Generic::List<array<System::Object^>^>^ CSPro::ParadataViewer::DatabaseQuery::GetResults(const int max_number_results)
{
    m_getResultsExecutedAtLeastOnce = true;

    auto rows = gcnew System::Collections::Generic::List<array<System::Object^>^>();
    int rows_count = 0;
    int sql_result = SQLITE_ROW;

    while( ( rows_count < max_number_results ) &&
        ( m_nextRowAlreadyStepped || ( ( sql_result = sqlite3_step(m_stmt) ) == SQLITE_ROW ) ) )
    {
        m_nextRowAlreadyStepped = false;

        auto row = gcnew array<System::Object^>(m_numberColumns);
        rows->Add(row);
        ++rows_count;

        for( int column = 0; column < m_numberColumns; ++column )
        {
            if( sqlite3_column_type(m_stmt, column) == SQLITE_NULL )
            {
                // nothing to do
            }

            else if( sqlite3_column_type(m_stmt, column) == SQLITE_TEXT )
            {
                row[column] = clr_helpers::to_SystemString(std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, column))));
            }

            else
            {
                row[column] = gcnew System::Double(sqlite3_column_double(m_stmt, column));
            }
        }
    }

    // if SQLITE_DONE wasn't the last return value, then max_number_results was hit, but read
    // the next row to see if all rows have been read
    if( sql_result != SQLITE_DONE )
        m_nextRowAlreadyStepped = ( sqlite3_step(m_stmt) == SQLITE_ROW );

    // reset the statement if all results have been returned
    if( !m_nextRowAlreadyStepped )
        sqlite3_reset(m_stmt);

    return rows;
}


System::Nullable<bool> CSPro::ParadataViewer::DatabaseQuery::AdditionalResultsAvailable::get()
{
    return m_getResultsExecutedAtLeastOnce ? System::Nullable<bool>(m_nextRowAlreadyStepped) :
                                             System::Nullable<bool>();
}
