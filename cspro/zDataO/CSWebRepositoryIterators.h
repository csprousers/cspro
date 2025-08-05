#pragma once

#include <zDataO/CaseIterator.h>


// --------------------------------------------------------------------------
// CSWebRepositoryIterator
// --------------------------------------------------------------------------

class CSWebRepositoryIterator : public CaseIterator
{
public:
    CSWebRepositoryIterator(CSWebRepository& csweb_repository,
                            CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
                            std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
                            const CaseIteratorParameters* start_parameters, size_t offset, size_t limit);

    bool NextCaseKey(CaseKey& case_key) override;
    bool NextCaseSummary(CaseSummary& case_summary) override;
    bool NextCase(Case& data_case) override;
    int GetPercentRead() const override;

    template<CSWebCaseQuery query>
    std::optional<CSWebCaseResponse> Step();

    void QueryNextSet(const std::string& arguments_json_text);

    template<typename T>
    T& GetTemporaryCaseObject();

private:
    CSWebRepository& m_cswebRepository;

    CSWebCaseQuery m_query;
    const char* m_iterationContent;
    CaseIterationCaseStatus m_caseStatus;
    std::optional<CaseIterationMethod> m_iterationMethod;
    std::optional<CaseIterationOrder> m_iterationOrder;
    std::unique_ptr<CaseIteratorParameters> m_startParameters;
    size_t m_offset;
    size_t m_limit;

    struct QueryResult
    {
        CSWebCaseQueryResponse case_query_response;
        size_t case_count;
        bool limit_satisfied;
        size_t iterator_case_pos;
    };

    std::optional<QueryResult> m_queryResult;

    mutable std::optional<double> m_percentMultiplier;
    size_t m_casesRead;

    std::optional<std::tuple<std::unique_ptr<CaseKey>, std::unique_ptr<Case>>> m_temporaryCaseObjects;
};
