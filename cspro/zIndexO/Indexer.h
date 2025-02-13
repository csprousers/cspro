#pragma once

#include <zIndexO/zIndexO.h>
#include <zDataO/IndexableTextRepository.h>

class CStdioFileUnicode;
class PFF;
struct sqlite3_stmt;


#define IndexerFilenameWildcard _T("<filename>")


struct IndexResult
{
    const size_t file_index;
    const ConnectionString input_connection_string;
    ConnectionString output_connection_string;
    std::wstring repository_name;
    size_t number_cases;
    size_t number_internal_duplicates;
    size_t number_global_duplicates;
    size_t number_cases_written;
    size_t number_duplicates_kept;
    size_t number_duplicates_skipped;
    bool indexable_text_repository_index_created;
    std::wstring exception_message;

    IndexResult(size_t file_index_, ConnectionString connection_string)
        :   file_index(file_index_),
            input_connection_string(std::move(connection_string)),
            repository_name(WS2CS(input_connection_string.ToString())),
            number_cases(0),
            number_internal_duplicates(0),
            number_global_duplicates(0),
            number_cases_written(0),
            number_duplicates_kept(0),
            number_duplicates_skipped(0),
            indexable_text_repository_index_created(0)
    {
    }
};


struct DuplicateInfo
{
    const IndexResult* index_result;
    size_t case_index;
    int64_t line_number;
    std::shared_ptr<Case> data_case;
    bool keep;
};


class ZINDEXO_API Indexer : public IndexableTextRepositoryIndexerCallback
{
public:
    Indexer();
    virtual ~Indexer();

    void Run(const PFF& pff, bool silent, std::shared_ptr<const CDataDict> embedded_dictionary = nullptr);

    // some functionality will be different when run by CSIndex as opposed to the PFFExecutor or in the portable environments
protected:
    virtual bool SupportsInteractiveMode() const = 0;
    virtual void DisplayInteractiveModeMessage(NullTerminatedString message) const = 0;
    virtual void ChooseDuplicate(std::vector<DuplicateInfo>& case_duplicates, size_t duplicate_index, size_t number_duplicates) const = 0;
    virtual bool RethrowTerminatingException() const = 0;

protected:
    void IndexCallback(const IndexableTextRepositoryIndexDetails& index_details) override;

private:
    void IndexCallback(const std::wstring& key, double position_in_repository, size_t bytes_for_case, int64_t line_number);

    void RunIndexer(bool silent);

    void StartLog();
    void StopLog(const CSProException* exception = nullptr);

    void IndexFile();

    void CalculateDuplicateCounts();
    void WriteDuplicatesToLog();

    void CalculateOutputConnectionStrings();

    void ReadDuplicates();

    void ChooseDuplicatesToKeep();
    static bool CaseEquals(const Case& case1, const Case& case2);

    void WriteCases();
    void WriteCases(const std::unique_ptr<DataRepository>& output_repository, IndexResult& index_result);

private:
    const PFF* m_pff;

    std::unique_ptr<CStdioFileUnicode> m_log;
    std::optional<ULONGLONG> m_endTimePosition;

    std::shared_ptr<const CDataDict> m_dictionary;
    std::shared_ptr<CaseAccess> m_keyReaderCaseAccess;
    std::shared_ptr<CaseAccess> m_fullCaseAccess;

    sqlite3* m_db;
    sqlite3_stmt* m_stmtPutCase;
    sqlite3_stmt* m_stmtDeleteCasesByFileIndex;
    sqlite3_stmt* m_stmtMinimalDuplicateIteratorByKey;
    sqlite3_stmt* m_stmtFullDuplicateIteratorByKey;
    sqlite3_stmt* m_stmtReadDuplicatesIterator;
    sqlite3_stmt* m_stmtUpdateCaseDoNotKeepByIndex;
    sqlite3_stmt* m_stmtUpdateCaseDoNotKeepByKey;
    sqlite3_stmt* m_stmtWriteCaseIterator;

    std::vector<IndexResult> m_indexResults;
    IndexResult* m_currentlyProcessingIndexResult;

    size_t m_maxRepositoryNameLength;
    size_t m_numberDuplicates;
    std::map<std::wstring, std::vector<DuplicateInfo>> m_duplicates;
    bool m_writeToCombinedFile;
};
