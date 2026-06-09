#include "stdafx.h"
#include "SQLiteRepositoryIterators.h"


SQLiteRepositoryCaseIterator::SQLiteRepositoryCaseIterator(SQLiteRepository& repository, const CaseIterationContent iteration_content,
                                                           std::unique_ptr<SQLiteStatement> statement)
    :   m_repository(repository),
        m_iterationContent(iteration_content),
        m_statement(std::move(statement)),
        m_casesRead(0)
{
    m_processCaseNote = ( iteration_content == CaseIterationContent::CaseSummary &&
                          m_repository.GetCaseAccess().GetUsesNotes() &&
                          RequiresCaseNote() );
}


SQLiteRepositoryCaseIterator::SQLiteRepositoryCaseIterator(SQLiteRepository& repository, const CaseIterationContent iteration_content,
                                                           std::unique_ptr<SQLiteStatement> statement,
                                                           const CaseIteratorSettings* const iterator_settings)
    :   SQLiteRepositoryCaseIterator(repository, iteration_content, std::move(statement))
{
    if( iterator_settings != nullptr )
    {
        m_progressBarParameters.emplace(iterator_settings->GetStatus(),
                                        CreateCopyOfPointerValue(iterator_settings->GetParameters()));
    }
}


SQLiteRepositoryCaseIterator::SQLiteRepositoryCaseIterator(SQLiteRepository& repository, const CaseIterationContent iteration_content,
                                                           std::unique_ptr<SQLiteStatement> statement,
                                                           const CaseIterationCaseStatus case_status, const CaseIteratorParameters* const start_parameters)
    :   SQLiteRepositoryCaseIterator(repository, iteration_content, std::move(statement))
{
    m_progressBarParameters.emplace(case_status, CreateCopyOfPointerValue(start_parameters));
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
        m_case = m_repository.GetCaseAccess().CreateCase();

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
        return NextCaseForNonCaseReading(case_key);

    if( !Step() )
        return false;

    case_key.SetKey(m_statement->GetColumn<std::string>(0));
    case_key.SetPositionInRepository(m_statement->GetColumn<double>(1));

    return true;
}


bool SQLiteRepositoryCaseIterator::NextCaseKeyAndUuid(CaseKey& case_key, std::string& uuid)
{
    ASSERT(m_statement->GetColumnCount() == 3);

    if( !NextCaseKey(case_key) )
        return false;

    uuid = m_statement->GetColumn<std::string>(2);

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

    if( m_repository.GetCaseAccess().GetUsesCaseLabels() )
        case_summary.SetCaseLabel(m_statement->GetColumn<std::string>(column++));

    if( m_repository.GetCaseAccess().GetUsesStatuses() )
    {
        case_summary.SetVerified(m_statement->GetColumn<bool>(column++));

        case_summary.SetPartialSaveMode(m_statement->IsColumnNull(column) ? PartialSaveMode::None :
                                                                            static_cast<PartialSaveMode>(m_statement->GetColumn<int>(column)));
        ++column;
    }

    if( m_processCaseNote )
    {
        if( m_statement->IsColumnNull(column) )
        {
            case_summary.ResetCaseNote();
        }

        else
        {
            case_summary.SetCaseNote(m_statement->GetColumn<std::string>(column));
        }

        // ++column;
    }

    return true;
}


bool SQLiteRepositoryCaseIterator::NextCase(Case& data_case)
{
    if( !Step() )
        return false;

    if( m_iterationContent != CaseIterationContent::Case )
    {
        const double position_in_repository = m_statement->GetColumn<double>(1);
        m_repository.ReadCase(data_case, position_in_repository);
    }

    else
    {
        m_repository.ReadCaseFromDatabase(data_case, *m_statement);
    }

    return true;
}


int SQLiteRepositoryCaseIterator::GetPercentRead() const
{
    // get the number of cases if necessary
    if( !m_percentMultiplier.has_value() )
    {
        if( !m_progressBarParameters.has_value() )
            return ReturnProgrammingError(0);

        const size_t number_cases = m_repository.GetNumberCases(std::get<0>(*m_progressBarParameters), std::get<1>(*m_progressBarParameters).get());
        m_percentMultiplier = CreatePercentMultiplier(number_cases);
    }

    return static_cast<int>(m_casesRead * *m_percentMultiplier);
}
