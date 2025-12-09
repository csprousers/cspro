#include "Stdafx.h"
#include "Database.h"
#include "DatabaseQuery.h"


CSPro::ParadataViewer::DatabaseQuery::DatabaseQuery(Sqlite::Statement stmt)
    :   m_stmt(new Sqlite::Statement(std::move(stmt))),
        m_numberColumns(m_stmt->GetColumnCount()),
        m_getResultsExecutedAtLeastOnce(false),
        m_nextRowAlreadyStepped(false)
{
}


CSPro::ParadataViewer::DatabaseQuery::!DatabaseQuery()
{
    delete m_stmt;
}


int CSPro::ParadataViewer::DatabaseQuery::ColumnCount::get()
{
    return m_numberColumns;
}


array<System::String^>^ CSPro::ParadataViewer::DatabaseQuery::ColumnNames::get()
{
    auto names = gcnew array<System::String^>(m_numberColumns);

    for( int column = 0; column < m_numberColumns; ++column )
        names[column] = clr_helpers::to_SystemString(m_stmt->GetColumnName(column));

    return names;
}


System::Collections::Generic::List<array<System::Object^>^>^ CSPro::ParadataViewer::DatabaseQuery::GetResults(const int max_number_results)
{
    m_getResultsExecutedAtLeastOnce = true;

    auto rows = gcnew System::Collections::Generic::List<array<System::Object^>^>();
    int rows_count = 0;
    std::optional<int> sql_result;

    while( ( rows_count < max_number_results ) &&
           ( m_nextRowAlreadyStepped || ( *( sql_result = m_stmt->Step() ) == Sqlite::Result::Row ) ) )
    {
        m_nextRowAlreadyStepped = false;

        auto row = gcnew array<System::Object^>(m_numberColumns);
        rows->Add(row);
        ++rows_count;

        for( int column = 0; column < m_numberColumns; ++column )
            row[column] = CSPro::ParadataViewer::Database::GetSqlResult(*m_stmt, column);
    }

    // if Sqlite::Result::Done was not the last return value, then max_number_results was hit,
    // but read the next row to see if all rows have been read
    if( sql_result != Sqlite::Result::Done )
        m_nextRowAlreadyStepped = ( m_stmt->Step() == Sqlite::Result::Row );

    // reset the statement if all results have been returned
    if( !m_nextRowAlreadyStepped )
        m_stmt->Reset();

    return rows;
}


System::Nullable<bool> CSPro::ParadataViewer::DatabaseQuery::AdditionalResultsAvailable::get()
{
    return m_getResultsExecutedAtLeastOnce ? System::Nullable<bool>(m_nextRowAlreadyStepped) :
                                             System::Nullable<bool>();
}
