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

    void Close() override;
    void DeleteRepository() override;
    void PopulateCaseIdentifiers(CString& key, CString& uuid, double& position_in_repository) const override;
    std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
	    const CaseIteratorParameters* start_parameters = nullptr) const override;
    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter = nullptr) override;
    size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) const override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
        std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
        const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) override;

    // TransactionGenerator overrides
    bool CommitTransactions() override;

    // other methods
    static void RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string);
    static std::vector<std::wstring> GetAssociatedFileList(const ConnectionString& connection_string);


protected:
    // DataRepository overrides
    void Open(DataRepositoryOpenFlag open_flag) override;

    // IndexableTextRepository overrides
    size_t GetIdStructureHashForKeyIndex() const override;
    std::shared_ptr<IndexCreator> GetIndexCreator() override;
    std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>> GetSqlStatementsToPrepare() override;
    std::variant<const char*, std::shared_ptr<SQLiteStatement>> GetSqlStatementForQuery(SqlQueryType type) override;
    void ReadCase(Case& data_case, int64_t file_position, size_t bytes_for_case) override;
    void DeleteCase(int64_t file_position, size_t bytes_for_case, bool deleted, const CString* key_if_known) override;


private:
    /// <summary>
    /// Makes sure a transaction is in process, periodically ending previous transitions.
    /// </summary>
    void WrapInTransaction();

    /// <summary>
    /// Opens the data file.
    /// </summary>
    void OpenDataFile();

    /// <summary>
    /// Closes the data file but does not touch the index, which must be closed with CloseIndex.
    /// </summary>
    void CloseDataFile();

    /// <summary>
    /// Returns the percent of the file that has been read (if in batch input mode).
    /// </summary>
    int GetPercentRead() const;

    /// <summary>
    /// Deletes the data, index, notes, and status files.
    /// </summary>
    static void DeleteRepositoryFiles(const ConnectionString& connection_string);

    /// <summary>
    /// Fills the UTF-8/ANSI buffer with bytes from the file.
    /// </summary>
    void FillUtf8AnsiTextBufferForCaseReading(int64_t file_position, size_t bytes_for_case);

    /// <summary>
    /// Sets up the case read during batch (sequential file) processing.
    /// </summary>
    void SetupBatchCase(Case& data_case);

    /// <summary>
    /// Sets up the non-CaseLevel attributes of a case.
    /// </summary>
    void SetupOtherCaseAttributes(Case& data_case, double file_position) const;

    /// <summary>
    /// Moves to a given location in the file and resets the read buffers.
    /// </summary>
    void ResetPosition(int64_t file_position);

    /// <summary>
    /// Moves to the beginning of the file and resets the read buffers.
    /// </summary>
    void ResetPositionToBeginning();

    /// <summary>
    /// Reads from the current position of the file, reading lines until the key changes.
    /// Returns false if at the end of the file.
    /// </summary>
    bool ReadUntilKeyChange();

    /// <summary>
    /// Fills the UTF-8/ANSI buffer with bytes from the file. Returns false if there are no more bytes to read.
    /// </summary>
    bool FillUtf8AnsiTextBufferForKeyChangeReading();

    /// <summary>
    /// Returns the key. If no case is in the repository at the given position,
    /// DataRepositoryException::CaseNotFound will be thrown.
    /// </summary>
    CString GetKeyFromPosition(int64_t file_position) const;

    /// <summary>
    /// Deletes the case from the text file by putting a tilde at the beginning of each record line.
    /// </summary>
    void DeleteCaseInPlace(int64_t file_position, size_t bytes_for_case);

    /// <summary>
    /// Grows or shrinks the file, starting at the given position, shifting all content by a certain amount.
    /// The index is also modified to reflect the shift.
    /// </summary>
    void GrowOrShrinkFileAndIndex(int64_t file_position, int bytes_differential);

    /// <summary>
    /// Creates the SQLite prepared statement for key searches and iterators.
    /// </summary>
    SQLiteStatement CreateKeySearchIteratorStatement(const TCHAR* columns_to_query,
                                                     size_t offset, size_t limit,
                                                     const std::optional<CaseIterationMethod>& iteration_method,
                                                     const std::optional<CaseIterationOrder>& iteration_order,
                                                     const CaseIteratorParameters* start_parameters) const;


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
    TCHAR* m_wideBuffer;

    // variables used while reading the data in file sequential order
    FullLineProcessor m_firstLineKeyFullLineProcessor;
    FullLineProcessor m_currentLineKeyFullLineProcessor;
    std::vector<TextToCaseConverter::TextBufferLine> m_wideBufferLines;
    size_t m_wideBufferLineCaseStartLineIndex;

    int64_t m_fileBytesRemaining;
    const char* m_utf8AnsiBufferPosition;
    const char* m_utf8AnsiBufferEnd;
    const char* m_utf8AnsiBufferActualEnd;
    const TCHAR* m_wideBufferPosition;
    const TCHAR* m_wideBufferEnd;
    bool m_ignoreNextCharacterIfNewline;

    std::shared_ptr<TextRepositoryIndexCreator> m_indexCreator;

    // CSEntry-specific objects
    std::unique_ptr<TextRepositoryNotesFile> m_notesFile;
    std::unique_ptr<TextRepositoryStatusFile> m_statusFile;

    // transaction objects
    bool m_useTransactionManager;
    size_t m_numberTransactions;
};
