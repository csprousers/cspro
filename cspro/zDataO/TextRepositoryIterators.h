#pragma once

#include <zDataO/CaseIterator.h>
#include <zDataO/TextRepository.h>
#include <zDataO/TextRepositoryNotesFile.h>
#include <zDataO/TextRepositoryStatusFile.h>


class TextRepositoryCaseIterator : public CaseIterator
{
public:
    TextRepositoryCaseIterator(TextRepository& text_repository, SQLiteStatement stmt_query_keys,
                               CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters,
                               std::optional<std::tuple<size_t, size_t>> partials_offset_and_limit)
        :   m_textRepository(text_repository),
            m_stmtQueryKeys(std::move(stmt_query_keys)),
            m_notesFile(text_repository.m_notesFile.get()),
            m_statusFile(text_repository.m_statusFile.get()),
            m_progressBarParameters({case_status, ( start_parameters != nullptr ) ? std::make_unique<CaseIteratorParameters>(*start_parameters) : nullptr}),
            m_percentMultiplier(0),
            m_casesRead(0),
            m_partialsOffsetLimit(std::move(partials_offset_and_limit))
    {
        ASSERT(m_statusFile != nullptr || !m_partialsOffsetLimit.has_value());
    }

private:
    bool Step()
    {
        // when filtering on partials, quit out if the correct number of cases has been read
        if( m_partialsOffsetLimit.has_value() && m_casesRead == std::get<1>(*m_partialsOffsetLimit) )
            return false;

        // get the next case
        while( m_stmtQueryKeys.Step() == SQLITE_ROW )
        {
            // potentially filter on partials
            if( m_partialsOffsetLimit.has_value() )
            {
                if( !m_statusFile->IsPartial(m_stmtQueryKeys.GetColumn<CString>(0)) )
                    continue;

                // process the offset
                if( std::get<0>(*m_partialsOffsetLimit) > 0 )
                {
                    --std::get<0>(*m_partialsOffsetLimit);
                    continue;
                }
            }

            ++m_casesRead;
            return true;
        }

        return false;
    }

public:
    bool NextCaseKey(CaseKey& case_key) override
    {
        while( Step() )
        {
            case_key.SetKey(m_stmtQueryKeys.GetColumn<CString>(0));
            case_key.SetPositionInRepository(m_stmtQueryKeys.GetColumn<double>(1));
            return true;
        }

        return false;
    }

    bool NextCaseSummary(CaseSummary& case_summary) override
    {
        if( NextCaseKey(case_summary) )
        {
            case_summary.SetDeleted(false);

            // update the case note
            if( RequiresCaseNote() && m_notesFile != nullptr )
                m_notesFile->SetupCaseNote(case_summary);

            // update the status information
            if( m_statusFile != nullptr )
                m_statusFile->SetupCaseSummary(case_summary);

            return true;
        }

        return false;
    }

    bool NextCase(Case& data_case) override
    {
        while( Step() )
        {
            int64_t file_position = m_stmtQueryKeys.GetColumn<int64_t>(1);
            size_t bytes_for_case = m_stmtQueryKeys.GetColumn<size_t>(2);

            m_textRepository.TextRepository::ReadCase(data_case, file_position, bytes_for_case);

            return true;
        }

        return false;
    }

    int GetPercentRead() const override
    {
        // get the number of cases if necessary
        if( m_progressBarParameters.has_value() )
        {
            // max used to avoid a divide by zero error
            size_t number_cases = m_textRepository.GetNumberCases(std::get<0>(*m_progressBarParameters), std::get<1>(*m_progressBarParameters).get());
            m_percentMultiplier = 100.0 / std::max<size_t>(number_cases, 1);
            m_progressBarParameters.reset();
        }

        return (int)( m_casesRead * m_percentMultiplier );
    }

private:
    TextRepository& m_textRepository;
    SQLiteStatement m_stmtQueryKeys;
    const TextRepositoryNotesFile* m_notesFile;
    const TextRepositoryStatusFile* m_statusFile;
    mutable std::optional<std::tuple<CaseIterationCaseStatus, std::unique_ptr<CaseIteratorParameters>>> m_progressBarParameters;
    mutable double m_percentMultiplier;
    size_t m_casesRead;
    std::optional<std::tuple<size_t, size_t>> m_partialsOffsetLimit;
};


class TextRepositoryBatchCaseIterator : public CaseIterator
{
public:
    TextRepositoryBatchCaseIterator(TextRepository& text_repository, bool partials_only)
        :   m_textRepository(text_repository),
            m_partialsOnly(partials_only)
    {
        m_textRepository.ResetPositionToBeginning();

#ifdef _DEBUG
        m_lastFileBytesRemaining = m_textRepository.m_fileBytesRemaining;
#endif
    }

private:
    template<typename T>
    bool NextCaseForNonCaseReading(T& case_object)
    {
        // in the rare event that the CaseKey or CaseSummary is being queried from
        // a batch iterator, create a case that can be used to access the case contents
        if( m_case == nullptr )
            m_case = m_textRepository.GetCaseAccess()->CreateCase();

        if( NextCase(*m_case) )
        {
            case_object = *m_case;
            return true;
        }

        return false;
    }

public:
    bool NextCaseKey(CaseKey& case_key) override
    {
        return NextCaseForNonCaseReading(case_key);
    }

    bool NextCaseSummary(CaseSummary& case_summary) override
    {
        return NextCaseForNonCaseReading(case_summary);
    }

    bool NextCase(Case& data_case) override
    {
        while( true )
        {
            // ensure that the file position hasn't moved
#ifdef _DEBUG
            ASSERT(m_lastFileBytesRemaining == m_textRepository.m_fileBytesRemaining);
#endif

            bool case_read = m_textRepository.ReadUntilKeyChange();

#ifdef _DEBUG
            m_lastFileBytesRemaining = m_textRepository.m_fileBytesRemaining;
#endif

            if( !case_read )
                return false;

            m_textRepository.SetupBatchCase(data_case);

            // potentially filter on partials
            if( !m_partialsOnly || data_case.IsPartial() )
                return true;
        }
    }

    int GetPercentRead() const override
    {
        return m_textRepository.GetPercentRead();
    }

private:
    TextRepository& m_textRepository;
    bool m_partialsOnly;
    std::unique_ptr<Case> m_case;

#ifdef _DEBUG
    int64_t m_lastFileBytesRemaining; // used to ensure that the file position hasn't been moved by any other calls
#endif
};
