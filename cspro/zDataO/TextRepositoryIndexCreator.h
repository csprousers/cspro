#pragma once

#include <zDataO/IndexableTextRepository.h>


class TextRepositoryIndexerPositions
{
    static constexpr size_t MinPositionsResizeCount = 8 * 1024;

public:
    TextRepositoryIndexerPositions(int64_t file_bytes_remaining, int64_t file_size)
        :   line_number(0),
            file_position(file_size - file_bytes_remaining),
            m_fileSize(file_size),
            m_nextLookupPosition(0)
    {
        ASSERT(file_position == Utf8BOM_sv.length() || file_position == 0);
        AddEntry();
    }

    void AddEntry()
    {
        m_positions.emplace_back(line_number, file_position);
    }

    int64_t LookupEntry(int64_t lookup_line_number)
    {
        const auto& line_search = std::find_if(m_positions.begin() + m_nextLookupPosition, m_positions.end(),
                                               [&](const std::tuple<int64_t, int64_t>& position) { return ( std::get<0>(position) == lookup_line_number ); });
        int64_t lookup_file_position;

        // if there is no newline at the end of the file, the lookup will fail and the
        // position should be the file size
        if( line_search == m_positions.end() )
        {
            ASSERT(lookup_line_number == ( std::get<0>(m_positions.back()) + 1 ));
            lookup_file_position = m_fileSize;
        }

        else
        {
            lookup_file_position = std::get<1>(*line_search);

            m_nextLookupPosition = line_search - m_positions.begin();

            // don't allow the lookup to get too large
            if( m_nextLookupPosition > MinPositionsResizeCount )
            {
                m_positions.erase(m_positions.begin(), m_positions.begin() + m_nextLookupPosition);
                m_nextLookupPosition = 0;
            }
        }

        return lookup_file_position;
    }

    int64_t line_number;
    int64_t file_position;

private:
    std::vector<std::tuple<int64_t, int64_t>> m_positions;
    size_t m_nextLookupPosition;
    int64_t m_fileSize;
};



class TextRepositoryIndexCreator : public IndexableTextRepository::IndexCreator
{
public:
    TextRepositoryIndexCreator(TextRepository& text_repository, const char* create_key_table_sql, const std::vector<const char*>& create_index_sql_statements)
        :   m_textRepository(text_repository),
            m_createKeyTableSql(create_key_table_sql),
            m_createIndexSqlStatements(create_index_sql_statements),
            throwExceptionsOnDuplicateKeys(true),
            m_indexerPositions(m_textRepository.m_fileBytesRemaining, m_textRepository.m_fileSize),
            m_keyLength(m_textRepository.m_keyMetadata->key_length),
            m_processedLineNumbers(0)
    {
    }

    TextRepositoryIndexerPositions& GetIndexerPositions()
    {
        return m_indexerPositions;
    }

    void Initialize(bool throw_exceptions_on_duplicate_keys) override
    {
        throwExceptionsOnDuplicateKeys = throw_exceptions_on_duplicate_keys;
    }

    const char* GetCreateKeyTableSql() const override
    {
        return m_createKeyTableSql;
    }

    const std::vector<const char*>& GetCreateIndexSqlStatements() const override
    {
        return m_createIndexSqlStatements;
    }

    int64_t GetFileSize() const override
    {
        return m_textRepository.m_fileSize;
    }

    int GetPercentRead() const override
    {
        return m_textRepository.GetPercentRead();
    }

    bool ReadCaseAndUpdateIndex(IndexableTextRepositoryIndexDetails& index_details) override
    {
        if( !m_textRepository.ReadUntilKeyChange() )
            return false;

        // generate the key finding a non-skipped record up to the second-to-last line (because the last line is for the next case)
        const auto& line_iterator_begin = m_textRepository.m_wideBufferLines.cbegin() + m_textRepository.m_wideBufferLineCaseStartLineIndex;
        const auto& line_iterator_end = m_textRepository.m_wideBufferLines.cend() - 1;

        for( auto line_iterator = line_iterator_begin; line_iterator != line_iterator_end; ++line_iterator )
        {
            // skip blank records
            if( line_iterator->length == 0 )
                continue;

            const TCHAR* first_line_key = m_textRepository.m_firstLineKeyFullLineProcessor.GetLine(m_textRepository.m_wideBuffer + line_iterator->offset,
                                                                                                   line_iterator->length, m_textRepository.m_keyEnd);

            index_details.key.resize(m_keyLength);
            TCHAR* key_iterator = index_details.key.data();

            for( const auto& key_span : m_textRepository.m_keyMetadata->key_spans )
            {
                _tmemcpy(key_iterator, first_line_key + key_span.start, key_span.length);
                key_iterator += key_span.length;
            }

            break;
        }

        // turn ␤ -> \n
        NewlineSubstitutor::MakeUnicodeNLToNewline(index_details.key);

#ifdef _DEBUG
        // check that the key generated is the same as what is generated by TextToCaseConverter
        Case index_case(m_textRepository.m_caseAccess->GetCaseMetadata());
        m_textRepository.SetupBatchCase(index_case);
        ASSERT(SO::Equals(index_case.GetKey(), index_details.key));
#endif

        // get the information on the line number and file position
        index_details.line_number = m_processedLineNumbers + 1;

        index_details.position = m_indexerPositions.LookupEntry(m_processedLineNumbers);

        m_processedLineNumbers += ( line_iterator_end - line_iterator_begin );
        int64_t file_position_next_case = m_indexerPositions.LookupEntry(m_processedLineNumbers);

        index_details.bytes = static_cast<size_t>(file_position_next_case - index_details.position);

        // update the index
        UpdateIndex(index_details);

        return true;
    }

    void OnSuccessfulCreation() override
    {
        m_textRepository.m_indexCreator.reset();

        m_textRepository.ResetPositionToBeginning();
    }

private:
    void UpdateIndex(IndexableTextRepositoryIndexDetails& index_details)
    {
        // check if the key already exists in the index
        const std::string utf8_key = UTF8Convert::WideToUTF8(index_details.key);

        ASSERT(m_textRepository.m_stmtKeyExists != nullptr);
        SQLiteResetOnDestruction key_exists_rod(*m_textRepository.m_stmtKeyExists);

        m_textRepository.m_stmtKeyExists->Bind(1, utf8_key);

        index_details.case_prevents_index_creation = ( m_textRepository.m_stmtKeyExists->Step() == SQLITE_ROW );

        if( index_details.case_prevents_index_creation )
        {
            if( throwExceptionsOnDuplicateKeys )
            {
                throw DataRepositoryException::DuplicateCaseWhileCreatingIndex(
                    FormatText(_T("An index could not be created for a data file with duplicate case IDs, including: '%s'"), index_details.key.c_str()));
            }
        }

        else
        {
            // insert the key information into the index
            ASSERT(m_textRepository.m_stmtInsertKey != nullptr);
            SQLiteResetOnDestruction insert_key_rod(*m_textRepository.m_stmtInsertKey);

            m_textRepository.m_stmtInsertKey->Bind(1, utf8_key)
                                             .Bind(2, index_details.position)
                                             .Bind(3, index_details.bytes);

            if( m_textRepository.m_stmtInsertKey->Step() != SQLITE_DONE )
                throw DataRepositoryException::SQLiteError();
        }
    }

private:
    TextRepository& m_textRepository;

    const char* m_createKeyTableSql;
    const std::vector<const char*>& m_createIndexSqlStatements;
    bool throwExceptionsOnDuplicateKeys;

    TextRepositoryIndexerPositions m_indexerPositions;
    size_t m_keyLength;
    int64_t m_processedLineNumbers;
};
