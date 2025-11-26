#pragma once

#include <zSql/DB.h>


class SortableKeyDatabase
{
public:
    enum class SortType { CaseSort, RecordSort, CaseOnly };

    SortableKeyDatabase(SortType sort_type);

    void AddKeyType(ContentType content_type, bool ascending);

    void Open();

    bool CaseExists(const std::string& key);

    void InitCaseInfo(double position_in_repository, const std::string& key = SO::Empty_string);
    void InitRecordInfo(size_t record_index, const std::vector<std::byte>& id_record_buffer, const std::vector<std::byte>& record_buffer);
    void AddCaseKeyValue(double value);
    void AddCaseKeyValue(const std::string& value);
    void AddCaseKeyValue(const CaseItem& case_item, const CaseItemIndex& index);
    void AddCaseKeyValue(const NumericCaseItem& numeric_case_item, const CaseItemIndex& index);
    void AddCaseKeyValue(const StringCaseItem& string_case_item, const CaseItemIndex& index);
    void AddCaseKeyValue(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index);
    bool AddCase();

    bool NextPosition(double& position_in_repository);
    bool NextRecord(size_t& record_index, std::vector<std::byte>& id_binary_buffer, std::vector<std::byte>& record_binary_buffer);

private:
    SortType m_sortType;
    std::vector<std::tuple<bool, bool>> m_keyTypes;

    Sqlite::DB m_db;
    Sqlite::Statement m_stmtPut;
    Sqlite::Statement m_stmtExists;
    Sqlite::Statement m_stmtIterator;
    int m_putArgumentCounter;
};
