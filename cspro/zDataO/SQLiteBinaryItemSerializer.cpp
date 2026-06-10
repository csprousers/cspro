#include "stdafx.h"
#include "SQLiteBinaryItemSerializer.h"
#include "SQLiteBinaryContentReader.h"
#include "SQLiteErrorWithMessage.h"
#include "SyncWithDataBinaryContentReader.h"


SQLiteBinaryItemSerializer::SQLiteBinaryItemSerializer(UniqueId repository_id, sqlite3* const db)
    :   m_repositoryId(std::move(repository_id)),
        m_db(db),
        m_stmtGetContent(db, "SELECT `data` FROM `binary-data` WHERE `signature` = ?;"),
        m_stmtGetContentSize(db, "SELECT length(`data`) FROM `binary-data` WHERE `signature` = ?;"),
        m_stmtHasContentAssociatedWithCaseUuid(db, "SELECT 1 FROM `case-binary-data` WHERE `binary-data-signature` = ? AND `case-id` = ? LIMIT 1;"),
        m_stmtHasContentAssociatedWithAnyCase(db, "SELECT 1 FROM `binary-data` WHERE `signature` = ? LIMIT 1;"),
        m_stmtAssociateContentWithCase(db, "INSERT INTO `case-binary-data` ( `case-id`, `binary-data-signature` ) VALUES( ?, ? );"),
        m_stmtInsertContent(db, "INSERT INTO `binary-data` ( `signature`, `data`, `last_modified_revision`) VALUES( ?, ?, ? )")
{
    ASSERT(m_db != nullptr);
}


uint64_t SQLiteBinaryItemSerializer::GetContentSize(const std::string& signature)
{
    const SQLiteResetOnDestruction rod(m_stmtGetContentSize);
    m_stmtGetContentSize.Bind(1, signature);

    if( m_stmtGetContentSize.Step() != SQLITE_ROW )
        throw SQLiteErrorWithMessage(m_db);

    return m_stmtGetContentSize.GetColumn<uint64_t>(0);
}


std::vector<std::byte> SQLiteBinaryItemSerializer::GetContent(const std::string& signature)
{
    std::lock_guard<std::mutex> lock(m_binaryReaderMutex);

    const SQLiteResetOnDestruction rod(m_stmtGetContent);
    m_stmtGetContent.Bind(1, signature);

    if( m_stmtGetContent.Step() != SQLITE_ROW )
        throw SQLiteErrorWithMessage(m_db, "The binary data could not be located in the CSPro DB file: ");

    return m_stmtGetContent.GetColumn<std::vector<std::byte>>(0);
}


std::string SQLiteBinaryItemSerializer::InsertContent(const BinaryDataAccessor& binary_data_accessor, const std::string& case_uuid, const int64_t revision)
{
    ASSERT(binary_data_accessor.IsDefined());

    const std::string& signature = binary_data_accessor.GetSignature();
    ASSERT(BinaryDataAccessor::IsValidSignature(signature));

    // we only want to add the content when necessary, as it may be an expensive operation to retrieve;
    // this method binds the signature (first), and optionally, another value

    auto bind_and_execute_scalar_query = [&](SQLiteStatement& stmt, const std::string* const optional_bind_value)
    {
        const SQLiteResetOnDestruction rod(stmt);
        stmt.Bind(1, signature);

        if( optional_bind_value != nullptr )
            stmt.Bind(2, *optional_bind_value);

        switch( stmt.Step() )
        {
            case SQLITE_DONE: return false;
            case SQLITE_ROW:  return true;
            default:          throw SQLiteErrorWithMessage(m_db);
        }
    };

    // if the content already exists and is already associated with this case, there is nothing to do
    if( bind_and_execute_scalar_query(m_stmtHasContentAssociatedWithCaseUuid, &case_uuid) )
        return signature;

    // if the content is not associated with any case, add it
    if( !bind_and_execute_scalar_query(m_stmtHasContentAssociatedWithAnyCase, nullptr) )
    {
        GetContentAndInsert(binary_data_accessor, signature, revision);
        ASSERT81(bind_and_execute_scalar_query(m_stmtHasContentAssociatedWithAnyCase, nullptr));
    }

    // associate this content with this case
    const SQLiteResetOnDestruction rod(m_stmtAssociateContentWithCase);
    m_stmtAssociateContentWithCase.Bind(1, case_uuid)
                                  .Bind(2, signature);

    if( m_stmtAssociateContentWithCase.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);

    return signature;
}


void SQLiteBinaryItemSerializer::GetContentAndInsert(const BinaryDataAccessor& binary_data_accessor, const std::string& signature, const int64_t revision)
{
    ASSERT(binary_data_accessor.IsDefined());

    // checks on data that comes from a variety of binary data readers
#ifdef _DEBUG
    // content served using SQLiteBinaryContentReader should already be in the database and
    // we would never get to this method unless it came from a different .csdb file
    const SQLiteBinaryContentReader* const sqlite_binary_content_reader = dynamic_cast<const SQLiteBinaryContentReader*>(binary_data_accessor.GetBinaryContentReader());
    ASSERT(sqlite_binary_content_reader == nullptr || sqlite_binary_content_reader->GetUniqueId() != &m_repositoryId);

    // content served using SyncWithDataBinaryContentReader could have come in a previous chunk, but that
    // chunk should already have been added to the database, so only readers with data should be used here
    const SyncWithDataBinaryContentReader* const sync_with_data_binary_content_reader = dynamic_cast<const SyncWithDataBinaryContentReader*>(binary_data_accessor.GetBinaryContentReader());
    ASSERT(sync_with_data_binary_content_reader == nullptr || sync_with_data_binary_content_reader->ContentReceivedDuringSync());
#endif

    // insert the new binary content
    const std::vector<std::byte>& content = binary_data_accessor.GetBinaryData().GetContent();
    ASSERT(binary_data_accessor.IsDefinedAndContentLoaded() && signature == Hash::Md5::Create(content));

    const SQLiteResetOnDestruction rod(m_stmtInsertContent);
    m_stmtInsertContent.Bind(1, signature)
                       .Bind(2, content)
                       .Bind(3, revision);

    if( m_stmtInsertContent.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);
}
