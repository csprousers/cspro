#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/IndexableTextRepository.h>
#include <zUtilO/TransactionManager.h>

class CaseJsonWriterSerializerHelper;
class JsonRepositoryCaseJsonParserHelper;
class JsonStream;
class JsonStreamObjectArrayIterator;


class ZDATAO_API JsonRepository : public IndexableTextRepository, public TransactionGenerator
{
    friend class JsonRepositoryBatchCaseIterator;
    friend class JsonRepositoryCaseIterator;
    friend class JsonRepositoryBinaryDataIO;
    friend class JsonRepositoryIndexCreator;

public:
    JsonRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);
    ~JsonRepository();

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
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
                                                 std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
                                                 const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) override;

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
    void OpenBatchInputDataFileAsIndexed() override;


private:
    // Makes sure that a transaction is in process, periodically ending previous transitions.
    void WrapInTransaction();

    // Opens the data file.
    void OpenDataFile();

    // Opens the data file when not using JsonStream for batch input.
    void OpenIndexedDataFile();

    // Checks that the data file is a valid JSON file when in a batch output mode.
    void VerifyBatchOutputDataFile();

    // Closes the data file but does not touch the index, which must be closed with CloseIndex.
    void CloseDataFile();

    // Deletes the data, index, and binary files.
    static void DeleteRepositoryFiles(const ConnectionString& connection_string, bool delete_binary_data_directory = true);

    // Gets the output format for binary data, either std::monostate (for embedded data) or a directory.
    static std::variant<std::monostate, std::string> GetBinaryDataOutput(const ConnectionString& connection_string);

    // Gets the default directory where binary files would be stored if the connection string
    // had no override flags. A blank string is returned if the connection string has no filename.
    static std::string GetDefaultBinaryDataDirectory(const ConnectionString& connection_string);

    // Writes the case in EntryInput/ReadWrite modes.
    // If anchor_file_position is -1, the case will be written at the end of the file.
    // If anchor_file_position is not -1 and bytes_for_case_to_replace is {}, the case will be inserted above the anchor position.
    void WriteCaseForEntryInputReadWrite(Case& data_case, int64_t anchor_file_position, std::optional<size_t> bytes_for_case_to_replace, bool use_new_uuid);

    // Writes the case in batch output mode.
    void WriteCaseForBatchOutput(Case& data_case);

    // Calls fseek to modify the file position and updates m_filePosition.
    void SetFilePosition(int64_t file_position);

    enum class PostWriteDataAction { Nothing, UpdateFileSize, UpdateFileSizeAndTruncate };

    // Writes data to the file, updating m_filePosition (and potentially m_fileSize) on success. An exception is thrown on error.
    void WriteData(const void* buffer, size_t size, PostWriteDataAction post_write_data_action);

    // Grows the file, starting at the specified file position, shifting all content by the number of bytes.
    void GrowFile(int64_t file_position, size_t bytes_to_add);

    // Adds a case to the index.
    void AddCaseToIndex(const Case& data_case, int64_t file_position, size_t bytes_for_case);

    // Modifies a case's entry in the index.
    void ModifyCaseIndexEntry(const Case& data_case, int64_t file_position, size_t bytes_for_case);

    // Shifts the positions of all cases starting at the specified file position (>=), incrementing the position by the number of bytes.
    void ShiftIndexPositions(int64_t first_file_position_to_shift, size_t bytes_to_add);

    // Creates the SQLite prepared statement for key searches and iterators.
    SQLiteStatement CreateKeySearchIteratorStatement(const char* columns_to_query,
                                                     size_t offset, size_t limit,
                                                     CaseIterationCaseStatus case_status,
                                                     const std::optional<CaseIterationMethod>& iteration_method,
                                                     const std::optional<CaseIterationOrder>& iteration_order,
                                                     const CaseIteratorParameters* start_parameters);

    // Creates a batch iterator when using JsonStream for batch input.
    std::unique_ptr<CaseIterator> CreateBatchIterator(CaseIterationCaseStatus case_status);

    // Returns a case that can be used as necessary for reading cases when a case object is not available.
    std::shared_ptr<Case> GetTemporaryCase();

    // Converts the case to JSON format.
    std::string ConvertCaseToJson(const Case& data_case);

    // Converts the JSON node to a case.
    void ConvertJsonToCase(Case& data_case, const JsonNode& json_node);


private:
    // the index
    std::shared_ptr<SQLiteStatement> m_stmtInsertCase;
    std::shared_ptr<SQLiteStatement> m_stmtModifyCase;
    std::shared_ptr<SQLiteStatement> m_stmtModifyCaseBytes;
    std::shared_ptr<SQLiteStatement> m_stmtShiftCasePositions;
    std::shared_ptr<SQLiteStatement> m_stmtUuidExists;
    std::shared_ptr<SQLiteStatement> m_stmtQueryPositionUuidByKey;
    std::shared_ptr<SQLiteStatement> m_stmtQueryPositionBytesByUuid;
    std::shared_ptr<SQLiteStatement> m_stmtQueryKeyPositionByUuid;
    std::shared_ptr<SQLiteStatement> m_stmtQueryKeyUuidByPosition;
    std::shared_ptr<SQLiteStatement> m_stmtQueryUuidByPosition;
    std::shared_ptr<SQLiteStatement> m_stmtQueryIsLastPosition;
    std::shared_ptr<SQLiteStatement> m_stmtQueryLastUuidPositionBytes;

    // the data file, which can be opened via normal file operations, or using JsonStream for batch input;
    // for batch input, the file can also be opened both ways
    FILE* m_file;
    int64_t m_fileSize;
    int64_t m_filePosition;

    std::shared_ptr<JsonStream> m_jsonStream;
    std::unique_ptr<JsonStreamObjectArrayIterator> m_jsonStreamObjectArrayIterator;

    // batch output flags
    bool m_writeCommaBeforeNextCase;
    bool m_endArrayOnFileClose;

    // other variables
    std::variant<std::monostate, std::string> m_binaryDataOutput;

    std::shared_ptr<CaseJsonWriterSerializerHelper> m_caseJsonWriterSerializerHelper;
    bool m_jsonFormatCompact;

    std::unique_ptr<JsonRepositoryCaseJsonParserHelper> m_caseJsonParserHelper;
    std::vector<char> m_buffer;

    std::vector<std::shared_ptr<Case>> m_temporaryCases;

    // transaction objects
    bool m_useTransactionManager;
    size_t m_numberTransactions;
};
