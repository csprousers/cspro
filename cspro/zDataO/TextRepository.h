#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/IndexableTextRepository.h>
#include <zUtilO/TransactionManager.h>
#include <zCaseO/FullLineProcessor.h>
#include <zCaseO/TextToCaseConverter.h>

enum class Encoding : int;
class TextRepositoryIndexCreator;
class TextRepositoryNotesFile;
class TextRepositoryStatusFile;


class ZDATAO_API TextRepository : public IndexableTextRepository, public TransactionGenerator
{
    friend class TextRepositoryBatchCaseIterator;
    friend class TextRepositoryCaseIterator;
    friend class TextRepositoryIndexCreator;
    friend class TextRepositoryNotesFile;
    friend class TextRepositoryStatusFile;

public:
    TextRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);
    ~TextRepository();

    // DataRepository overrides
    void ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access) override;
    void ToggleReadWriteMode() override;
    void Close() override;
    void DeleteRepository() override;
    void PopulateCaseIdentifiers(std::string& key, std::string& uuid, double& position_in_repository) override;
    DataRepositoryUniqueCaseIdentifer GetUniqueCaseIdentifer(const CaseKey& case_key) override;
    std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
                                       const CaseIteratorParameters* start_parameters = nullptr) override;
    void ReadCaseByUuid(Case& data_case, const std::string& uuid) override;
    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter = nullptr) override;
    size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content,
                                                 const CaseIteratorSettings& iterator_settings,
                                                 size_t offset = 0, size_t limit = SIZE_MAX) override;

    // TransactionGenerator overrides
    bool CommitTransactions() override;

    // other methods
    static void RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string);
    static std::vector<std::string> GetAssociatedFileList(const ConnectionString& connection_string);


protected:
    // DataRepository overrides
    void Open(DataRepositoryOpenFlag open_flag) override;

    // IndexableTextRepository overrides
    uint32_t GetIdStructureHashForKeyIndex() const override;
    std::shared_ptr<IndexCreator> GetIndexCreator() override;
    std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>> GetSqlStatementsToPrepare() override;
    std::variant<const char*, std::shared_ptr<SQLiteStatement>> GetSqlStatementForQuery(SqlQueryType type) override;
    void ReadCase(Case& data_case, int64_t file_position, size_t bytes_for_case) override;
    void DeleteCase(int64_t file_position, size_t bytes_for_case, bool deleted, const std::string* key_if_known) override;


private:
    // Makes sure that a transaction is in process, periodically ending previous transitions.
    void WrapInTransaction();

    // Opens the data file.
    void OpenDataFile();

    // Closes the data file but does not touch the index, which must be closed with CloseIndex.
    void CloseDataFile();

    // Returns the percent of the file that has been read (if in batch input mode).
    int GetPercentRead() const;

    // Deletes the data, index, notes, and status files.
    static void DeleteRepositoryFiles(const ConnectionString& connection_string);

    // Fills the UTF-8/ANSI buffer with bytes from the file.
    void FillUtf8AnsiTextBufferForCaseReading(int64_t file_position, size_t bytes_for_case);

    // Sets up the case read during batch (sequential file) processing.
    void SetUpBatchCase(Case& data_case);

    // Sets up the non-CaseLevel attributes of a case.
    void SetUpOtherCaseAttributes(Case& data_case, double file_position) const;

    // Moves to a given location in the file and resets the read buffers.
    void ResetPosition(int64_t file_position);

    // Moves to the beginning of the file and resets the read buffers.
    void ResetPositionToBeginning();

    // Reads from the current position of the file, reading lines until the key changes.
    // Returns false if at the end of the file.
    bool ReadUntilKeyChange();

    // Fills the UTF-8/ANSI buffer with bytes from the file. Returns false if there are no more bytes to read.
    bool FillUtf8AnsiTextBufferForKeyChangeReading();

    // Returns the key. If no case is in the repository at the given position,
    // DataRepositoryException::CaseNotFound will be thrown.
    std::string GetKeyFromPosition(int64_t file_position);

    // Deletes the case from the text file by putting a tilde at the beginning of each record line.
    void DeleteCaseInPlace(int64_t file_position, size_t bytes_for_case);

    // Grows or shrinks the file, starting at the given position, shifting all content by a certain amount.
    // The index is also modified to reflect the shift.
    void GrowOrShrinkFileAndIndex(int64_t file_position, int bytes_differential);

    // Creates the SQLite prepared statement for key searches and iterators.
    SQLiteStatement CreateKeySearchIteratorStatement(const char* columns_to_query,
                                                     const CaseIteratorSettings& iterator_settings,
                                                     size_t offset, size_t limit);

private:
    std::unique_ptr<TextToCaseConverter> m_textToCaseConverter;
    const TextToCaseConverter::TextBasedKeyMetadata* m_keyMetadata;
    size_t m_keyEnd;

    // the index
    std::shared_ptr<SQLiteStatement> m_stmtInsertKey;
    std::shared_ptr<SQLiteStatement> m_stmtKeyExists;
    std::shared_ptr<SQLiteStatement> m_stmtQueryKeyByPosition;
    std::shared_ptr<SQLiteStatement> m_stmtQueryIsLastPosition;
    std::shared_ptr<SQLiteStatement> m_stmtQueryPreviousKeyByPosition;
    std::shared_ptr<SQLiteStatement> m_stmtQueryNextKeyByPosition;
    std::shared_ptr<SQLiteStatement> m_stmtDeleteKeyByPosition;
    std::shared_ptr<SQLiteStatement> m_stmtModifyKey;
    std::shared_ptr<SQLiteStatement> m_stmtShiftKeys;

    // the data file
    Encoding m_encoding;
    FILE* m_file;
    int64_t m_fileSize;

    // buffers for reading and text conversions
    size_t m_utf8AnsiBufferSize;
    char* m_utf8AnsiBuffer;
    size_t m_wideBufferSize;
    wchar_t* m_wideBuffer;

    // variables used while reading the data in file sequential order
    FullLineProcessor m_firstLineKeyFullLineProcessor;
    FullLineProcessor m_currentLineKeyFullLineProcessor;
    std::vector<TextToCaseConverter::TextBufferLine> m_wideBufferLines;
    size_t m_wideBufferLineCaseStartLineIndex;

    int64_t m_fileBytesRemaining;
    const char* m_utf8AnsiBufferPosition;
    const char* m_utf8AnsiBufferEnd;
    const char* m_utf8AnsiBufferActualEnd;
    const wchar_t* m_wideBufferPosition;
    const wchar_t* m_wideBufferEnd;
    bool m_ignoreNextCharacterIfNewline;

    std::shared_ptr<TextRepositoryIndexCreator> m_indexCreator;

    // CSEntry-specific objects
    std::unique_ptr<TextRepositoryNotesFile> m_notesFile;
    std::unique_ptr<TextRepositoryStatusFile> m_statusFile;

    // transaction objects
    bool m_useTransactionManager;
    size_t m_numberTransactions;
};
