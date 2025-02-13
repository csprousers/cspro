#pragma once

class BinaryCaseItem;
class BinaryData;
class BinaryDataAccessor;
class BinaryDataMetadata;
struct sqlite3;
struct sqlite3_stmt;


class SQLiteBinaryItemSerializer
{
public:
    SQLiteBinaryItemSerializer(sqlite3* db);
    ~SQLiteBinaryItemSerializer();

    BinaryData GetBinaryItemData(const BinaryDataMetadata& binary_data_metadata) const;
    uint64_t GetBinaryItemSize(const BinaryDataMetadata& binary_data_metadata) const;
    std::wstring SetBinaryItem(int64_t revision, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index);

private:
    std::wstring InsertBinaryItem(const std::wstring& case_id, int64_t revision, const BinaryDataAccessor& binary_data_accessor,
                                  std::optional<std::wstring> signature, bool binary_data_already_written);

    void DeleteBinaryItem(const std::wstring& signature);

private:
    sqlite3* m_db;
    mutable sqlite3_stmt* m_get_binary_item_size_statement;
    mutable sqlite3_stmt* m_get_binary_item_data_statement;
    sqlite3_stmt* m_insert_binary_item_statement;
    sqlite3_stmt* m_insert_case_binary_item_statement;  
    sqlite3_stmt* m_get_binary_items_for_case;
    sqlite3_stmt* m_delete_binary_item_by_signature;
};
