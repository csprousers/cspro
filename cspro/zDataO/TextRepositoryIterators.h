#pragma once

#include <zDataO/CaseIterator.h>


// --------------------------------------------------------------------------
// TextRepositoryCaseIterator
// --------------------------------------------------------------------------

class TextRepositoryCaseIterator : public CaseIterator
{
public:
    TextRepositoryCaseIterator(TextRepository& text_repository, SQLiteStatement stmt_query_keys,
                               CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters,
                               std::optional<std::tuple<size_t, size_t>> partials_offset_and_limit);

    bool NextCaseKey(CaseKey& case_key) override;
    bool NextCaseSummary(CaseSummary& case_summary) override;
    bool NextCase(Case& data_case) override;
    int GetPercentRead() const override;

private:
    bool Step();

private:
    TextRepository& m_textRepository;
    SQLiteStatement m_stmtQueryKeys;
    const TextRepositoryNotesFile* m_notesFile;
    const TextRepositoryStatusFile* m_statusFile;
    std::tuple<CaseIterationCaseStatus, std::unique_ptr<CaseIteratorParameters>> m_progressBarParameters;
    mutable std::optional<double> m_percentMultiplier;
    size_t m_casesRead;
    std::optional<std::tuple<size_t, size_t>> m_partialsOffsetLimit;
};



// --------------------------------------------------------------------------
// TextRepositoryBatchCaseIterator
// --------------------------------------------------------------------------

class TextRepositoryBatchCaseIterator : public CaseIterator
{
public:
    TextRepositoryBatchCaseIterator(TextRepository& text_repository, bool partials_only);

public:
    bool NextCaseKey(CaseKey& case_key) override;
    bool NextCaseSummary(CaseSummary& case_summary) override;
    bool NextCase(Case& data_case) override;
    int GetPercentRead() const override;

private:
    template<typename T>
    bool NextCaseForNonCaseReading(T& case_object);

private:
    TextRepository& m_textRepository;
    const bool m_partialsOnly;
    std::unique_ptr<Case> m_case;

#ifdef _DEBUG
    int64_t m_lastFileBytesRemaining; // used to ensure that the file position hasn't been moved by any other calls
#endif
};
