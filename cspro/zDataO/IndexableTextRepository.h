#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepository.h>

struct sqlite3;
class SQLiteStatement;


struct IndexableTextRepositoryIndexDetails
{
    std::wstring key;
    int64_t position = 0;
    size_t bytes = 0;
    std::optional<int64_t> line_number;
    bool case_prevents_index_creation = false;
};


// used in CSIndex (by the Indexer class)
struct IndexableTextRepositoryIndexerCallback
{
    virtual ~IndexableTextRepositoryIndexerCallback() { }

    virtual void IndexCallback(const IndexableTextRepositoryIndexDetails& index_details) = 0;
};



class ZDATAO_API IndexableTextRepository : public DataRepository
{
    friend class Indexer;

protected:
    constexpr static size_t MaxNumberSqlInsertsInOneTransaction = 32 * 1024;

    IndexableTextRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);

public:
    ~IndexableTextRepository();

    /// <summary>
    /// Returns the SQLite index (for the sqlquery function).
    /// </summary>
    sqlite3* GetIndexSqlite() const { return m_db; }

    // DataRepository overrides
    bool ContainsCase(const CString& key) const override;
    void ReadCase(Case& data_case, const CString& key) override;
    void ReadCase(Case& data_case, double position_in_repository) override;
    void DeleteCase(double position_in_repository, bool deleted = true) override;
    void DeleteCase(const CString& key) override;
    size_t GetNumberCases() const override;


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
    // methods that must be overriden by subclasses
    // ----------------------------------------------

    /// <summary>
    /// Returns the ID structure hash needed for this repository.
    /// </summary>
    virtual size_t GetIdStructureHashForKeyIndex() const = 0;

    /// <summary>
    /// Returns an index creator that will be used to create an index from scratch.
    /// </summary>
    virtual std::shared_ptr<IndexCreator> GetIndexCreator() = 0;

    /// <summary>
    /// Returns any SQL statements (command and prepared statement) to prepare.
    /// </summary>
    virtual std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>> GetSqlStatementsToPrepare() = 0;

    enum class SqlQueryType { ContainsNotDeletedKey,
                              GetPositionBytesFromNotDeletedKey,
                              GetBytesFromPosition,
                              CountNotDeletedKeys };

    /// <summary>
    /// Returns the SQL command or prepared statement for a query.
    /// </summary>
    virtual std::variant<const char*, std::shared_ptr<SQLiteStatement>> GetSqlStatementForQuery(SqlQueryType type) = 0;

    /// <summary>
    /// Reads the case at the given position in the file (called by the public ReadCase methods);
    /// </summary>
    virtual void ReadCase(Case& data_case, int64_t file_position, size_t bytes_for_case) = 0;

    /// <summary>
    /// Deletes the case at the given position in the file (called by the public DeleteCase methods);
    /// </summary>
    virtual void DeleteCase(int64_t file_position, size_t bytes_for_case, bool deleted, const CString* key_if_known) = 0;


    // methods that can be overriden by subclasses
    // ----------------------------------------------

    /// <summary>
    /// Reopens the data file, previously opened for batch input mode, for use with an index.
    /// </summary>
    virtual void OpenBatchInputDataFileAsIndexed() { }


protected:
    /// <summary>
    /// Sets a callback to be called when indexing the file.
    /// </summary>
    void SetIndexerCallback(IndexableTextRepositoryIndexerCallback& indexer_callback) { m_indexerCallback = &indexer_callback; }

    /// <summary>
    /// Opens the index if it is valid, creating a new one if not.
    /// </summary>
    void CreateOrOpenIndex(bool create_new_data_file);

    /// <summary>
    /// Prepares the statement and adds it to the list of prepared statements to be finalized when the index is closed.
    /// </summary>
    void PrepareSqlStatementForQuery(const char* sql, std::shared_ptr<SQLiteStatement>& stmt);

    /// <summary>
    /// Prepares the statement.
    /// </summary>
    SQLiteStatement PrepareSqlStatementForQuery(wstring_view sql) const;

    /// <summary>
    /// Ensures that a statement has been prepared.
    /// </summary>
    template<typename T>
    inline void EnsureSqlStatementIsPrepared(const T& sql_or_sql_query_type, const std::shared_ptr<SQLiteStatement>& stmt) const
    {
        if( stmt == nullptr )
            const_cast<IndexableTextRepository*>(this)->PrepareSqlStatementForQuery(sql_or_sql_query_type, const_cast<std::shared_ptr<SQLiteStatement>&>(stmt));
    }

    /// <summary>
    /// Returns the position and bytes for a non-deleted case. If no case with the given key is in the repository,
    /// DataRepositoryException::CaseNotFound will be thrown (or a position of -1 will be returned).
    /// </summary>
    std::tuple<int64_t, size_t> GetPositionBytesFromKey(const CString& key, bool throw_exception = true) const;

    /// <summary>
    /// Returns the bytes for a case. If no case is in the repository at the given position,
    /// DataRepositoryException::CaseNotFound will be thrown.
    /// </summary>
    size_t GetBytesFromPosition(int64_t file_position) const;

    /// <summary>
    /// Closes the index. Any prepared statements will be finalized.
    /// </summary>
    void CloseIndex();


private:
    /// <summary>
    /// Sets the index filename.
    /// </summary>
    void SetIndexFilename();

    /// <summary>
    /// Determines whether an index exists for the data file, and if so, checks if it is up to
    /// date based on the timestamp of the data file and the dictionary's key. If the index is
    /// valid, the index is kept open.
    /// </summary>
    bool IsIndexValid();

    /// <summary>
    /// Creates an index for the data file. If there are duplicates, the information about the duplicate
    /// will be available in the DataRepositoryException::DuplicateCaseWhileCreatingIndex exception thrown.
    /// The index is kept open if one was created successfully.
    /// </summary>
    void CreateIndex();

    /// <summary>
    /// Ensures that an index is open. This will generally do nothing, but if a file is opened as a
    /// BatchInput, then it will attempt to open an index for the file.
    /// </summary>
    inline void EnsureIndexIsOpen() const
    {
        ASSERT(m_db != nullptr || !m_requiresIndex);

        if( !m_requiresIndex && m_db == nullptr )
            OpenInitiallyNonRequiredIndex();
    }

    /// <summary>
    /// Opens an index for a file that was initially opened without an index.
    /// </summary>
    void OpenInitiallyNonRequiredIndex() const;

    /// <summary>
    /// Creates the SQL prepared statements for use with the index.
    /// </summary>
    void CreatePreparedStatements();

    /// <summary>
    /// Queries a subclass for the prepared statement. If GetSqlStatementForQuery returns the 
    /// SQL command (as text), the statement is prepared and added to the group of prepared statements.
    /// </summary>
    void PrepareSqlStatementForQuery(SqlQueryType type, std::shared_ptr<SQLiteStatement>& stmt);


protected:
    const bool m_requiresIndex;
    sqlite3* m_db;

private:
    CString m_indexFilename;
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
