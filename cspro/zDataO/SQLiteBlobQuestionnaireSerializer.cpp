#include "stdafx.h"
#include "SQLiteBlobQuestionnaireSerializer.h"
#include "SQLiteErrorWithMessage.h"
#include <SQLite/SQLiteHelpers.h>
#include <zCaseO/TextToCaseConverter.h>


SQLiteBlobQuestionnaireSerializer::SQLiteBlobQuestionnaireSerializer(sqlite3* database)
    :   m_db(database)
{
    sqlite3_prepare_v2(database, "SELECT questionnaire FROM cases WHERE file_order=? LIMIT 1", -1, &m_statement_get_questionnaire, nullptr);
    sqlite3_prepare_v2(database, "UPDATE cases SET questionnaire=? WHERE id=?", -1, &m_statement_update_questionnaire, nullptr);
}


SQLiteBlobQuestionnaireSerializer::~SQLiteBlobQuestionnaireSerializer()
{
    safe_sqlite3_finalize(m_statement_get_questionnaire);
    safe_sqlite3_finalize(m_statement_update_questionnaire);
}


void SQLiteBlobQuestionnaireSerializer::ReadQuestionnaire(Case& data_case) const
{
    SQLiteStatement get_questionnaire(m_statement_get_questionnaire);

    const int result = get_questionnaire.Bind(1, data_case.GetPositionInRepository()).Step();
    if (result != SQLITE_ROW)
        throw SQLiteErrorWithMessage(m_db);

    // convert the questionnaire text
    const char* questionnaire_text = get_questionnaire.GetColumn<const char*>(0);
    m_text_to_case_converter->TextUtf8ToCase(data_case, questionnaire_text, strlen(questionnaire_text));
}


void SQLiteBlobQuestionnaireSerializer::WriteQuestionnaire(const Case& data_case, int64_t /*revision*/) const
{
    const char* questionnaire_text = m_text_to_case_converter->CaseToTextUtf8(data_case);

    SQLiteStatement update_questionnaire(m_statement_update_questionnaire);

    const int result = update_questionnaire
        .Bind(1, questionnaire_text)
        .Bind(2, data_case.GetUuid())
        .Step();

    if (result != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);
}


void SQLiteBlobQuestionnaireSerializer::SetCaseAccess(std::shared_ptr<const CaseAccess> case_access, bool /*read_only*/)
{
    m_text_to_case_converter = std::make_unique<TextToCaseConverter>(case_access->GetCaseMetadata());
}
