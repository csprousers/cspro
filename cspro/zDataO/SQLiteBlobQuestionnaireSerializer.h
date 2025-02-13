#pragma once

#include <zDataO/ISQLiteQuestionnaireSerializer.h>

struct sqlite3_stmt;


// Serialize questionnaires as blobs - single column in cases table.
// Used for compatability with version 7.0-7.3 csdb files.

class SQLiteBlobQuestionnaireSerializer : public ISQLiteQuestionnaireSerializer
{
 public:
    SQLiteBlobQuestionnaireSerializer(sqlite3* database);
    ~SQLiteBlobQuestionnaireSerializer();

    void ReadQuestionnaire(Case& data_case) const override;

    void WriteQuestionnaire(const Case& data_case, int64_t revision) const override;

    void SetCaseAccess(std::shared_ptr<const CaseAccess> case_access, bool read_only) override;

private:
    sqlite3* m_db;
    sqlite3_stmt* m_statement_get_questionnaire;
    sqlite3_stmt* m_statement_update_questionnaire;
    std::unique_ptr<TextToCaseConverter> m_text_to_case_converter;
};
