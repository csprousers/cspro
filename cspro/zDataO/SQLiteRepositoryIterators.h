#pragma once

#include <zDataO/CaseIterator.h>
#include <zDataO/SQLiteRepository.h>
#include <SQLite/SQLiteStatement.h>


class SQLiteRepositoryCaseIterator : public CaseIterator
{
public:
    SQLiteRepositoryCaseIterator(SQLiteRepository& repository, CaseIterationContent iteration_content,
                                 std::unique_ptr<SQLiteStatement> statement);

    SQLiteRepositoryCaseIterator(SQLiteRepository& repository, CaseIterationContent iteration_content,
                                 std::unique_ptr<SQLiteStatement> statement,
                                 CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters);

    bool NextCaseKey(CaseKey& case_key) override;
    bool NextCaseSummary(CaseSummary& case_summary) override;
    bool NextCase(Case& data_case) override;
    int GetPercentRead() const override;

private:
    bool Step();

    template<typename T>
    bool NextCaseForNonCaseReading(T& case_object);

private:
    SQLiteRepository& m_repository;
    CaseIterationContent m_iterationContent;
    std::unique_ptr<SQLiteStatement> m_statement;
    mutable std::optional<std::tuple<CaseIterationCaseStatus, std::unique_ptr<CaseIteratorParameters>>> m_progressBarParameters;
    mutable double m_percentMultiplier;
    size_t m_casesRead;
    bool m_processCaseNote;
    std::unique_ptr<Case> m_case;
};
