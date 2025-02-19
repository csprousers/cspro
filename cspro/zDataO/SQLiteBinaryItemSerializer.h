#pragma once

#include <zToolsO/UniqueId.h>
#include <zSql/SQLiteStatement.h>
#include <mutex>

class BinaryCaseItem;


class SQLiteBinaryItemSerializer
{
public:
    SQLiteBinaryItemSerializer(UniqueId repository_id, sqlite3* db);

    // returns the size of the content associated with the signature
    uint64_t GetContentSize(const std::string& signature);

    // returns the content associated with the signature
    std::vector<std::byte> GetContent(const std::string& signature);

    // inserts the binary content (if necessary), adding a reference that the case uses this content;
    // the content's signature, calculated as necessary, is returned
    std::string InsertContent(const BinaryDataAccessor& binary_data_accessor, const std::string& case_uuid, const int64_t revision);

private:
    // retrieves the binary content and adds it to the database
    void GetContentAndInsert(const BinaryDataAccessor& binary_data_accessor, const std::string& signature, const int64_t revision);

private:
    UniqueId m_repositoryId;
    sqlite3* m_db;
    SQLiteStatement m_stmtGetContent;
    SQLiteStatement m_stmtGetContentSize;
    SQLiteStatement m_stmtHasContentAssociatedWithCaseUuid;
    SQLiteStatement m_stmtHasContentAssociatedWithAnyCase;
    SQLiteStatement m_stmtAssociateContentWithCase;
    SQLiteStatement m_stmtInsertContent;
    std::mutex m_binaryReaderMutex;
};
