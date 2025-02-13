#pragma once

#include <zDataO/IndexableTextRepository.h>
#include <zDataO/JsonRepository.h>


class JsonRepositoryIndexCreator : public IndexableTextRepository::IndexCreator
{
public:
    JsonRepositoryIndexCreator(JsonRepository& json_repository);

    void Initialize(bool throw_exceptions_on_duplicate_keys) override;
    const char* GetCreateKeyTableSql() const override;
    const std::vector<const char*>& GetCreateIndexSqlStatements() const override;
    int64_t GetFileSize() const override;
    int GetPercentRead() const override;;
    bool ReadCaseAndUpdateIndex(IndexableTextRepositoryIndexDetails& index_details) override;
    void OnSuccessfulCreation() override;

private:
    static constexpr unsigned char EndOfFile = static_cast<unsigned char>(EOF);

    // returns the current unprocessed data position (relative to the entire file)
    int64_t GetUnprocessedDataPosition() const { return m_bufferStartFilePosition + m_nextReadUnprocessedDataPosition; }

    // moves any unprocessed data to the beginning of the buffer and then fills the buffer
    // with more content, returning true if more content was added
    bool FillBuffer();

    // reads the file, skipping whitespace characters, until one of the characters is found;
    // the character found is returned; if not found, either 0 or EndOfFile is returned
    unsigned char ReadUntilCharacter(const char* allowable_characters);

    // reads until the JSON end object character is found, properly handling nested objects and strings;
    // the end object character is consumed and the JSON object text and UTF-8 length is returned
    std::tuple<std::wstring, size_t> ReadObject();

    // parses the remaining file, throwing an exception if any non-whitespace character is found
    void EnsureAtEndOfFile();

    // checks for duplicate UUID values and updates the index
    void UpdateIndex(IndexableTextRepositoryIndexDetails& index_details);

private:
    JsonRepository& m_jsonRepository;
    int64_t m_fileBytesRemaining;

    bool throwExceptionsOnDuplicateKeys;

    std::vector<char> m_buffer;
    int64_t m_bufferStartFilePosition;
    size_t m_nextReadUnprocessedDataPosition;
    bool m_endOfFileReached;

    std::unique_ptr<Case> m_case;
};
