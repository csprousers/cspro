#pragma once

#include <zDataO/CaseIterator.h>
#include <zDataO/JsonRepository.h>


class JsonRepositoryCaseIterator : public CaseIterator
{
public:
    JsonRepositoryCaseIterator(JsonRepository& json_repository, CaseIterationContent iteration_content, SQLiteStatement stmt_query_keys,
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
    JsonRepository& m_jsonRepository;
    CaseIterationContent m_iterationContent;
    SQLiteStatement m_stmtQueryKeys;
    mutable std::optional<std::tuple<CaseIterationCaseStatus, std::unique_ptr<CaseIteratorParameters>>> m_progressBarParameters;
    mutable double m_percentMultiplier;
    size_t m_casesRead;
};



class JsonRepositoryBatchCaseIterator : public CaseIterator
{
public:
    JsonRepositoryBatchCaseIterator(JsonRepository& json_repository, CaseIterationCaseStatus case_status);

    bool NextCaseKey(CaseKey& case_key) override;
    bool NextCaseSummary(CaseSummary& case_summary) override;
    bool NextCase(Case& data_case) override;
    int GetPercentRead() const override;

private:
    template<typename T>
    bool NextCaseForNonCaseReading(T& case_object);

private:
    JsonRepository& m_jsonRepository;
    CaseIterationCaseStatus m_caseStatus;
};
