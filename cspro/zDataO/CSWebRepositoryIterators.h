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

private:
    template<bool requires_metadata>
    typename std::conditional<requires_metadata, std::optional<std::tuple<JsonNode, JsonNode>>, std::optional<JsonNode>>::type Step();

    void QueryNextSet(const std::string& arguments_json_text);

    template<typename T>
    T& GetTemporaryCaseObject();

private:
    CSWebRepository& m_cswebRepository;

    const char* m_iterationContent;
    CaseIterationCaseStatus m_caseStatus;
    std::optional<CaseIterationMethod> m_iterationMethod;
    std::optional<CaseIterationOrder> m_iterationOrder;
    std::unique_ptr<CaseIteratorParameters> m_startParameters;
    size_t m_offset;
    size_t m_limit;

    struct Query
    {
        JsonNode json_node;
        JsonNodeArray content_json_array_node;
        std::optional<JsonNodeArray> metadata_json_array_node;
        size_t case_count;
        bool limit_satisfied;
        size_t iterator_case_pos;
    };

    std::optional<Query> m_query;

    mutable std::optional<double> m_percentMultiplier;
    size_t m_casesRead;

    std::optional<std::tuple<std::unique_ptr<CaseKey>, std::unique_ptr<Case>>> m_temporaryCaseObjects;
};
