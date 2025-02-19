#pragma once

#include <zSql/SQLite.h>


class SortableKeyDatabase
{
public:
    enum class SortType { CaseSort, RecordSort, CaseOnly };

    SortableKeyDatabase(SortType sort_type);
    ~SortableKeyDatabase();

    void AddKeyType(ContentType content_type, bool ascending);

    bool Open();

    bool CaseExists(const std::string& key);

    void InitCaseInfo(double position_in_repository, const std::string& key = SO::Empty_string);
    void InitRecordInfo(size_t record_index, const void* id_record_buffer, size_t id_buffer_size, const void* record_buffer, size_t buffer_size);
    void AddCaseKeyValue(double value);
    void AddCaseKeyValue(const std::string& value);
    void AddCaseKeyValue(const CaseItem& case_item, const CaseItemIndex& index);
    void AddCaseKeyValue(const NumericCaseItem& numeric_case_item, const CaseItemIndex& index);
    void AddCaseKeyValue(const StringCaseItem& string_case_item, const CaseItemIndex& index);
    void AddCaseKeyValue(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index);
    bool AddCase();

    bool NextPosition(double* position_in_repository);
    bool NextRecord(size_t* record_index, std::vector<std::byte>* id_binary_buffer, std::vector<std::byte>* record_binary_buffer);

private:
    SortType m_sortType;
    std::vector<std::tuple<bool, bool>> m_keyTypes;

    sqlite3* m_db;
    sqlite3_stmt* m_stmtPut;
    sqlite3_stmt* m_stmtExists;
    sqlite3_stmt* m_stmtIterator;
    int m_putArgumentCounter;
};
