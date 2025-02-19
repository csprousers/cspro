#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/SyncCaseSerializer.h>

class SyncCaseIncrementalJsonObjectArrayParser;


// --------------------------------------------------------------------------
// SyncCaseIncrementalParser
//
// This class uses SyncCaseSerializer to incrementally parse a stream of case
// data. Errors and exceptions are handled as described in
// SyncCaseSerializer::ParseSyncableCaseData.
// --------------------------------------------------------------------------

class ZDATAO_API SyncCaseIncrementalParser
{
    friend SyncCaseIncrementalJsonObjectArrayParser;

public:
    SyncCaseIncrementalParser(SyncCaseSerializer sync_case_serializer);
    ~SyncCaseIncrementalParser();

    // Updates the parser with additional case data.
    void Update(std::string_view case_data_sv);

    // When called, indicates to the parser that all case data has been processed.
    // If not in a finalized state, an exception is thrown.
    // Any cases not already returned using ReleaseParseableCases are returned.
    std::vector<std::shared_ptr<Case>> Finish();

    // Returns the total number of cases parsed.
    size_t GetParsedCaseCount() const { return m_parsedCaseCount; }

    // Returns the total length of the case JSON that has been parsed.
    size_t GetParsedCaseJsonLength() const { return m_parsedCaseJsonLength; }

    // Returns the cases that are currently parseable. For cases with binary data,
    // a case is only parseable once its binary data has been processed.
    // Cases are only returned sequentially, so if case 1 is not parseable but case 2 is,
    // case 2 will only be returned once case 1 is parseable.
    // If no cases are parseable, null is returned.
    std::unique_ptr<std::vector<std::shared_ptr<Case>>> ReleaseParseableCases();

private:
    void Update_AnalyzingHeader(std::string_view case_data_sv);
    void Update_InCaseJson(std::string_view case_data_sv);

private:
    void ParseCaseJsonFromSyncableCaseData(const JsonNode& json_node);

private:
    SyncCaseSerializer m_syncCaseSerializer;
    std::unique_ptr<SyncCaseIncrementalJsonObjectArrayParser> m_incrementalJsonParser;
    size_t m_parsedCaseCount;
    size_t m_parsedCaseJsonLength;
    std::string m_preJsonCaseData;
    std::string m_postJsonCaseData;

    std::unique_ptr<std::vector<std::shared_ptr<Case>>> m_currentCases;
    bool m_currentCasesAreParseable;

    enum class State { AnalyzingHeader, InCaseJson, PostCaseJson, Finalized };
    State m_state;
    std::optional<int32_t> m_caseJsonLengthWhenProcessingBinaryData;
};
