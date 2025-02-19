#pragma once

#include <zDataO/IndexableTextRepository.h>


class TextRepositoryIndexerPositions
{
    static constexpr size_t MinPositionsResizeCount = 8 * 1024;

public:
    TextRepositoryIndexerPositions(int64_t file_bytes_remaining, int64_t file_size);

    void AddEntry() { m_positions.emplace_back(line_number, file_position); }

    int64_t LookupEntry(int64_t lookup_line_number);

    int64_t line_number;
    int64_t file_position;

private:
    std::vector<std::tuple<int64_t, int64_t>> m_positions;
    const int64_t m_fileSize;
    size_t m_nextLookupPosition;
};



class TextRepositoryIndexCreator : public IndexableTextRepository::IndexCreator
{
public:
    TextRepositoryIndexCreator(TextRepository& text_repository, const std::vector<const char*>& create_index_sql_statements);

    TextRepositoryIndexerPositions& GetIndexerPositions() { return m_indexerPositions; }

    void Initialize(bool throw_exceptions_on_duplicate_keys) override;
    const char* GetCreateKeyTableSql() const override;
    const std::vector<const char*>& GetCreateIndexSqlStatements() const override;
    int64_t GetFileSize() const override;
    int GetPercentRead() const override;
    bool ReadCaseAndUpdateIndex(IndexableTextRepositoryIndexDetails& index_details) override;
    void OnSuccessfulCreation() override;

private:
    void UpdateIndex(IndexableTextRepositoryIndexDetails& index_details);

private:
    TextRepository& m_textRepository;
    const std::vector<const char*>& m_createIndexSqlStatements;
    bool throwExceptionsOnDuplicateKeys;

    TextRepositoryIndexerPositions m_indexerPositions;
    size_t m_keyLength;
    int64_t m_processedLineNumbers;
};
