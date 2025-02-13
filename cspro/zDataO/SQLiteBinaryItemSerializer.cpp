#include "stdafx.h"
#include "SQLiteBinaryItemSerializer.h"
#include "SQLiteBinaryDataReader.h"
#include "SQLiteErrorWithMessage.h"
#include "SyncJsonBinaryDataReader.h"
#include <zCaseO/BinaryCaseItem.h>
#include <mutex>


SQLiteBinaryItemSerializer::SQLiteBinaryItemSerializer(sqlite3* database)
    :   m_db(database),
        m_get_binary_item_size_statement(nullptr),
        m_get_binary_item_data_statement(nullptr),
        m_insert_binary_item_statement(nullptr),
        m_insert_case_binary_item_statement(nullptr),
        m_get_binary_items_for_case(nullptr),
        m_delete_binary_item_by_signature(nullptr)
{
}


SQLiteBinaryItemSerializer::~SQLiteBinaryItemSerializer()
{
    sqlite3_finalize(m_get_binary_item_size_statement);
    sqlite3_finalize(m_get_binary_item_data_statement);
    sqlite3_finalize(m_insert_binary_item_statement);
    sqlite3_finalize(m_insert_case_binary_item_statement);
    sqlite3_finalize(m_get_binary_items_for_case);
    sqlite3_finalize(m_delete_binary_item_by_signature);
}


BinaryData SQLiteBinaryItemSerializer::GetBinaryItemData(const BinaryDataMetadata& binary_data_metadata) const
{
    static std::mutex binary_reader_mutex;
    std::lock_guard<std::mutex> lock(binary_reader_mutex);

    SQLiteStatement statement(m_db, m_get_binary_item_data_statement,
        "SELECT data from `binary-data` WHERE signature=?");
    statement.Bind(1, binary_data_metadata.GetBinaryDataKey());

    if (statement.Step() != SQLITE_ROW) {
        throw SQLiteErrorWithMessage(m_db, _T("The binary data could not be located in the CSPro DB file: "));
    }

    return BinaryData(statement.GetColumn<std::vector<std::byte>>(0), binary_data_metadata);
}


uint64_t SQLiteBinaryItemSerializer::GetBinaryItemSize(const BinaryDataMetadata& binary_data_metadata) const
{
    SQLiteStatement statement(m_db, m_get_binary_item_size_statement,
        "SELECT length(data) FROM `binary-data` WHERE signature=?");
    statement.Bind(1, binary_data_metadata.GetBinaryDataKey());

    if (statement.Step()!= SQLITE_ROW) {
        throw SQLiteErrorWithMessage(m_db);
    }

    return statement.GetColumn<int64_t>(0);
}


std::wstring SQLiteBinaryItemSerializer::SetBinaryItem(int64_t revision, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
{
    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);

    // if the item is not defined, return a blank string
    if( !binary_data_accessor.IsDefined() )
        return std::wstring();

    const BinaryDataReader* binary_data_reader = binary_data_accessor.GetBinaryDataReader();
    const SQLiteBinaryDataReader* sqlite_binary_data_reader = dynamic_cast<const SQLiteBinaryDataReader*>(binary_data_reader);

    // if the data came from this repository and wasn't changed, we don't need to write it out again and can directly return the signature
    if( sqlite_binary_data_reader != nullptr && sqlite_binary_data_reader->IsUpToDate(this) )
        return sqlite_binary_data_reader->GetMetadata().GetBinaryDataKey();

    const SyncJsonBinaryDataReader* sync_json_binary_data_reader = ( sqlite_binary_data_reader == nullptr ) ? dynamic_cast<const SyncJsonBinaryDataReader*>(binary_data_reader) :
                                                                                                              nullptr;

    std::optional<std::wstring> signature;
    bool binary_data_already_written = false;

    // if this is coming from a sync, the signature has already been calculated
    if( sync_json_binary_data_reader != nullptr )
    {
        signature = sync_json_binary_data_reader->GetMetadata().GetBinaryDataKey();
        ASSERT(!signature->empty());

        binary_data_already_written = !sync_json_binary_data_reader->HasSyncedContent();
    }

    try
    {
        return InsertBinaryItem(CS2WS(index.GetCase().GetUuid()), revision, binary_data_accessor, std::move(signature), binary_data_already_written);
    }
    catch(...) { ASSERT(false); }

    return std::wstring();
}


std::wstring SQLiteBinaryItemSerializer::InsertBinaryItem(const std::wstring& case_id, int64_t revision, const BinaryDataAccessor& binary_data_accessor,
                                                          std::optional<std::wstring> signature, bool binary_data_already_written)
{
    ASSERT(!binary_data_already_written || signature.has_value());

    // we only need to get the data if we need to calculate the signature, or if the binary data must be written
    if( !signature.has_value() || !binary_data_already_written )
    {
        const std::vector<std::byte>& content = binary_data_accessor.GetBinaryData().GetContent();

        if( !signature.has_value() )
            signature = PortableFunctions::BinaryMd5(content);

        // if the binary data has not been written, insert the the data into the binary-data table
        if( !binary_data_already_written )
        {
            SQLiteStatement content_stmt(m_db, m_insert_binary_item_statement,
                "INSERT OR IGNORE INTO `binary-data`(`signature`,data,last_modified_revision) VALUES(?,?,?)");
            content_stmt.Bind(1, *signature)
                        .Bind(2, content)
                        .Bind(3, revision);

            if( content_stmt.Step() != SQLITE_DONE )
                throw SQLiteErrorWithMessage(m_db);
        }
    }

    ASSERT(!signature->empty());

    // associate the signature with the case
    SQLiteStatement signature_stmt(m_db, m_insert_case_binary_item_statement,
        "INSERT OR IGNORE INTO `case-binary-data`(`case-id`,`binary-data-signature`) VALUES(?,?)");
    signature_stmt.Bind(1, case_id)
                  .Bind(2, *signature);

    if( signature_stmt.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);

    return *signature;
}


void SQLiteBinaryItemSerializer::DeleteBinaryItem(const std::wstring& signature)
{
    SQLiteStatement statement(m_db, m_delete_binary_item_by_signature,
        "DELETE FROM `binary-data` WHERE signature=?");
    statement.Bind(1, signature.c_str());

    if (statement.Step() != SQLITE_DONE) {
        throw SQLiteErrorWithMessage(m_db);
    }
}
