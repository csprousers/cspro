#pragma once

#include <zDataO/ISQLiteQuestionnaireSerializer.h>
#include <zDataO/SQLiteBinaryItemSerializer.h>
struct sqlite3_stmt;
class SQLiteStatement;

/// <summary>
/// Serialize questionnaires in separate tables/columns used from version 7.4+.
/// </summary>
class SQLiteQuestionnaireSerializer : public ISQLiteQuestionnaireSerializer
{
public:

    SQLiteQuestionnaireSerializer(sqlite3* database);

    void ReadQuestionnaire(Case& data_case) const override;

    void WriteQuestionnaire(const Case& data_case, int64_t revision) const override;

    void SetCaseAccess(std::shared_ptr<const CaseAccess> case_access, bool read_only) override;

    ~SQLiteQuestionnaireSerializer();

private:
    struct PreparedStatements {
        sqlite3_stmt* m_id_record;
        std::vector<sqlite3_stmt*> m_records;
        std::unique_ptr<PreparedStatements> m_child_level;

        ~PreparedStatements();
    };
    int64_t WriteLevel(const CaseLevel& level, const std::wstring& parent_level_id, std::optional<int> level_occ, const PreparedStatements& prepared_statements, int64_t revision) const;
    int64_t WriteRecord(const CaseRecord& record, sqlite3_stmt* prepared_statement, const std::wstring& level_id, int64_t revision, std::optional<int> level_occ = {}) const;
    void BindCaseItem(SQLiteStatement& statement, int param_number, const CaseItem& item, const CaseItemIndex& index, int64_t revision) const;
    void ReadLevelRecords(CaseLevel& level, int64_t level_pk, const std::vector<sqlite3_stmt*>& read_statements) const;
    void ReadChildLevel(CaseLevel& parent_level, int64_t level_pk, const PreparedStatements& prepared_statements) const;
    void ReadRecord(CaseRecord& record, int64_t level_pk, sqlite3_stmt* prepared_statement) const;
    void ReadRecordItems(CaseRecord& record, CaseItemIndex& index, SQLiteStatement& select_statement, int start_column) const;
    void SetItemValueFromStatement(const CaseItem& item, CaseItemIndex& index, SQLiteStatement& statement, int column_number) const;
    void CreateWriteStatements();
    void CreateReadStatements();
    void CreateDeleteStatement();
    void ClearPreparedStatements();
    std::unique_ptr<PreparedStatements> CreateWriteStatementsForLevel(const CaseLevelMetadata& level_metadata, const std::string& parent_level_table_name);
    sqlite3_stmt* CreateWriteStatementsForRecord(const CaseRecordMetadata& record_metadata, const std::string& record_table_name, const std::string& parent_level_table_name);
    std::unique_ptr<PreparedStatements> CreateReadStatementsForLevel(const CaseLevelMetadata& level_metadata, const std::string& parent_level_table_name);
    sqlite3_stmt* CreateReadStatementsForRecord(const CaseRecordMetadata& record_metadata, const std::string& record_table_name, const std::string& parent_level_table_name, bool include_id);
    void DeleteCase(const std::wstring& case_id) const;
    std::shared_ptr<const CaseAccess> m_caseAccess;
    std::unique_ptr<PreparedStatements> m_write_statements;
    std::unique_ptr<PreparedStatements> m_read_statements;
    std::unique_ptr<SQLiteBinaryItemSerializer> m_binary_serializer;
    sqlite3_stmt* m_delete_statement;
    sqlite3* m_db;
};
