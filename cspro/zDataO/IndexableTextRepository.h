#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepository.h>

struct sqlite3;
class SQLiteStatement;


// --------------------------------------------------------------------------
// IndexableTextRepositoryIndexDetails
// --------------------------------------------------------------------------

struct IndexableTextRepositoryIndexDetails
{
    std::string key;
    int64_t position = 0;
    size_t bytes = 0;
    std::optional<int64_t> line_number;
    bool case_prevents_index_creation = false;
};


// --------------------------------------------------------------------------
// IndexableTextRepositoryIndexerCallback
// used in CSIndex (by the Indexer class)
// --------------------------------------------------------------------------

struct IndexableTextRepositoryIndexerCallback
{
    virtual ~IndexableTextRepositoryIndexerCallback() { }

    virtual void IndexCallback(const IndexableTextRepositoryIndexDetails& index_details) = 0;
};


// --------------------------------------------------------------------------
// IndexableTextRepository
// --------------------------------------------------------------------------

class ZDATAO_API IndexableTextRepository : public DataRepository
{
    friend class Indexer;

protected:
    constexpr static size_t MaxNumberSqlInsertsInOneTransaction = 32 * 1024;

    IndexableTextRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);

public:
    ~IndexableTextRepository();

    // Returns the SQLite index
    sqlite3* GetIndexSqlite() const { return m_db; }

    // DataRepository overrides
    bool ContainsCase(const std::string& key) override;
    void ReadCase(Case& data_case, const std::string& key) override;
    void ReadCase(Case& data_case, double position_in_repository) override;
    void DeleteCase(double position_in_repository, bool deleted = true) override;
    void DeleteCase(const std::string& key) override;
    size_t GetNumberCases() override;


    // the interface for creating an index from scratch
    struct IndexCreator
    {
        virtual ~IndexCreator() { }

        virtual void Initialize(bool throw_exceptions_on_duplicate_keys) = 0;
        virtual const char* GetCreateKeyTableSql() const = 0;
        virtual const std::vector<const char*>& GetCreateIndexSqlStatements() const = 0;
        virtual int64_t GetFileSize() const = 0;
        virtual int GetPercentRead() const = 0;
        virtual bool ReadCaseAndUpdateIndex(IndexableTextRepositoryIndexDetails& index_details) = 0;
        virtual void OnSuccessfulCreation() = 0;
    };


protected:
    // --------------------------------------------------------------------------
    // methods that must be overridden by subclasses
    // --------------------------------------------------------------------------
    
    // Returns the ID structure hash needed for this repository.
    virtual uint32_t GetIdStructureHashForKeyIndex() const = 0;

    // Returns an index creator that will be used to create an index from scratch.
    virtual std::shared_ptr<IndexCreator> GetIndexCreator() = 0;

    // Returns any SQL statements (command and prepared statement) to prepare.
    virtual std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>> GetSqlStatementsToPrepare() = 0;

    enum class SqlQueryType { ContainsNotDeletedKey,
                              GetPositionBytesFromNotDeletedKey,
                              GetBytesFromPosition,
                              CountNotDeletedKeys };

    // Returns the SQL command or prepared statement for a query.
    virtual std::variant<const char*, std::shared_ptr<SQLiteStatement>> GetSqlStatementForQuery(SqlQueryType type) = 0;

    // Reads the case at the given position in the file (called by the public ReadCase methods);
    virtual void ReadCase(Case& data_case, int64_t file_position, size_t bytes_for_case) = 0;

    // Deletes the case at the given position in the file (called by the public DeleteCase methods);
    virtual void DeleteCase(int64_t file_position, size_t bytes_for_case, bool deleted, const std::string* key_if_known) = 0;


    // --------------------------------------------------------------------------
    // methods that can be overridden by subclasses
    // --------------------------------------------------------------------------

    // Reopens the data file, previously opened for batch input mode, for use with an index.
    virtual void OpenBatchInputDataFileAsIndexed() { }


protected:
    // Sets a callback to be called when indexing the file.
    void SetIndexerCallback(IndexableTextRepositoryIndexerCallback& indexer_callback) { m_indexerCallback = &indexer_callback; }

    // Opens the index if it is valid, creating a new one if not.
    void CreateOrOpenIndex(bool create_new_data_file);

    // Prepares the statement and adds it to the list of prepared statements to be finalized when the index is closed.
    void PrepareSqlStatementForQuery(const char* sql, std::shared_ptr<SQLiteStatement>& stmt);

    // Prepares the statement.
    SQLiteStatement PrepareSqlStatementForQuery(const std::string& sql);

    // Ensures that a statement has been prepared.
    template<typename T>
    inline void EnsureSqlStatementIsPrepared(T&& sql_or_sql_query_type, std::shared_ptr<SQLiteStatement>& stmt)
    {
        if( stmt == nullptr )
            PrepareSqlStatementForQuery(std::forward<T>(sql_or_sql_query_type), stmt);
    }

    // Returns the position and bytes for a non-deleted case. If no case with the given key is in the repository,
    // DataRepositoryException::CaseNotFound will be thrown (or a position of -1 will be returned).
    std::tuple<int64_t, size_t> GetPositionBytesFromKey(const std::string& key, bool throw_exception = true);

    // Returns the bytes for a case. If no case is in the repository at the given position,
    // DataRepositoryException::CaseNotFound will be thrown.
    size_t GetBytesFromPosition(int64_t file_position);

    // Closes the index. Any prepared statements will be finalized.
    void CloseIndex();


private:
    // Sets the index file path.
    void SetIndexFilePath();

    // Determines whether an index exists for the data file, and if so, checks if it is up to
    // date based on the timestamp of the data file and the dictionary's key. If the index is
    // valid, the index is kept open.
    bool IsIndexValid();

    // Creates an index for the data file. If there are duplicates, the information about the duplicate
    // will be available in the DataRepositoryException::DuplicateCaseWhileCreatingIndex exception thrown.
    // The index is kept open if one was created successfully.
    void CreateIndex();

    // Ensures that an index is open. This will generally do nothing, but if a file is opened as a
    // BatchInput, then it will attempt to open an index for the file.
    inline void EnsureIndexIsOpen()
    {
        ASSERT(m_db != nullptr || !m_requiresIndex);

        if( !m_requiresIndex && m_db == nullptr )
            OpenInitiallyNonRequiredIndex();
    }

    // Opens an index for a file that was initially opened without an index.
    void OpenInitiallyNonRequiredIndex();

    // Creates the SQL prepared statements for use with the index.
    void CreatePreparedStatements();

    // Queries a subclass for the prepared statement. If GetSqlStatementForQuery returns the 
    // SQL command (as text), the statement is prepared and added to the group of prepared statements.
    void PrepareSqlStatementForQuery(SqlQueryType type, std::shared_ptr<SQLiteStatement>& stmt);


protected:
    const bool m_requiresIndex;
    sqlite3* m_db;

private:
    std::string m_indexFilePath;
    bool m_triedCreatingNonRequiredIndex;

    // pointers to prepared statements (that will be reset on closing the index)
    std::vector<std::shared_ptr<SQLiteStatement>*> m_preparedStatements;

    // prepared statements used for index querying methods done by this class
    std::shared_ptr<SQLiteStatement> m_stmtNotDeletedKeyExists;
    std::shared_ptr<SQLiteStatement> m_stmtQueryPositionBytesByNotDeletedKey;
    std::shared_ptr<SQLiteStatement> m_stmtQueryBytesByPosition;
    std::shared_ptr<SQLiteStatement> m_stmtCountNotDeletedKeys;

    // to callback into CSIndex
    IndexableTextRepositoryIndexerCallback* m_indexerCallback;
};
