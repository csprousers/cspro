#include "stdafx.h"
#include "SQLiteRepositoryIterators.h"


SQLiteRepositoryCaseIterator::SQLiteRepositoryCaseIterator(SQLiteRepository& repository, CaseIterationContent iteration_content,
                                                           std::unique_ptr<SQLiteStatement> statement)
    :   m_repository(repository),
        m_iterationContent(iteration_content),
        m_statement(std::move(statement)),
        m_percentMultiplier(0),
        m_casesRead(0)
{
    m_processCaseNote = ( iteration_content == CaseIterationContent::CaseSummary && m_repository.GetCaseAccess()->GetUsesNotes() && RequiresCaseNote() );
}


SQLiteRepositoryCaseIterator::SQLiteRepositoryCaseIterator(SQLiteRepository& repository, CaseIterationContent iteration_content,
                                                           std::unique_ptr<SQLiteStatement> statement,
                                                           CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters)
    :   SQLiteRepositoryCaseIterator(repository, iteration_content, std::move(statement))        
{
    m_progressBarParameters.emplace(case_status, ( start_parameters != nullptr ) ? std::make_unique<CaseIteratorParameters>(*start_parameters) : nullptr);
}


bool SQLiteRepositoryCaseIterator::Step()
{
    if( m_statement->Step() == SQLITE_ROW )
    {
        ++m_casesRead;
        return true;
    }

    return false;
}


template<typename T>
bool SQLiteRepositoryCaseIterator::NextCaseForNonCaseReading(T& case_object)
{
    // in the rare event that the CaseKey or CaseSummary is being queried from
    // a case iterator, create a case that can be used to access the case contents
    if( m_case == nullptr )
        m_case = m_repository.GetCaseAccess()->CreateCase();

    if( NextCase(*m_case) )
    {
        case_object = *m_case;
        return true;
    }

    return false;
}


bool SQLiteRepositoryCaseIterator::NextCaseKey(CaseKey& case_key)
{
    if( m_iterationContent == CaseIterationContent::Case )
    {
        return NextCaseForNonCaseReading(case_key);
    }

    else if( !Step() )
    {
        return false;
    }

    case_key.SetKey(m_statement->GetColumn<CString>(0));
    case_key.SetPositionInRepository(m_statement->GetColumn<double>(1));

    return true;
}


bool SQLiteRepositoryCaseIterator::NextCaseSummary(CaseSummary& case_summary)
{
    if( m_iterationContent != CaseIterationContent::CaseSummary )
        return NextCaseForNonCaseReading(case_summary);

    if( !NextCaseKey(case_summary) )
        return false;

    int column = 2;
    case_summary.SetDeleted(m_statement->GetColumn<bool>(column++));

    if( m_repository.GetCaseAccess()->GetUsesCaseLabels() )
        case_summary.SetCaseLabel(m_statement->GetColumn<CString>(column++));

    if( m_repository.GetCaseAccess()->GetUsesStatuses() )
    {
        case_summary.SetVerified(m_statement->GetColumn<bool>(column++));

        PartialSaveMode partial_save_mode;

        if( m_statement->IsColumnNull(column) )
        {
            partial_save_mode = PartialSaveMode::None;
        }

        else
        {
            partial_save_mode = (PartialSaveMode)m_statement->GetColumn<int>(column);
        }

        case_summary.SetPartialSaveMode(partial_save_mode);
        ++column;
    }

    if( m_processCaseNote )
    {
        CString case_note;

        if( !m_statement->IsColumnNull(column) )
            case_note = m_statement->GetColumn<CString>(column);

        case_summary.SetCaseNote(case_note);
        // ++column;
    }

    return true;
}


bool SQLiteRepositoryCaseIterator::NextCase(Case& data_case)
{
    if( m_iterationContent != CaseIterationContent::Case )
    {
        if( !Step() )
            return false;

        double position_in_repository = m_statement->GetColumn<double>(1);
        m_repository.ReadCase(data_case, position_in_repository);
    }

    else
    {
        if( !m_repository.ReadCaseFromDatabase(data_case, *m_statement) )
            return false;

        ++m_casesRead;
    }

    return true;
}


int SQLiteRepositoryCaseIterator::GetPercentRead() const
{
    // get the number of cases if necessary
    if( m_progressBarParameters.has_value() )
    {
        // max used to avoid a divide by zero error
        size_t number_cases = m_repository.GetNumberCases(std::get<0>(*m_progressBarParameters), std::get<1>(*m_progressBarParameters).get());
        m_percentMultiplier = 100.0 / std::max<size_t>(number_cases, 1);
        m_progressBarParameters.reset();
    }

    return (int)( m_casesRead * m_percentMultiplier );
}
