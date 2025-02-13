#include "stdafx.h"
#include "SQLiteDictionarySchemaReconciler.h"
#include "SQLiteDictionarySchemaGenerator.h"
#include "SQLiteErrorWithMessage.h"
#include "SQLiteSchemaHelpers.h"
#include <zToolsO/NumberConverter.h>
#include <zUtilO/SQLiteSchema.h>
#include <zUtilO/SQLiteTransaction.h>
#include <zDictO/DictionaryComparer.h>
#include <zDictO/Rules.h>
#include <sstream>


SQLiteDictionarySchemaReconciler::SQLiteDictionarySchemaReconciler(sqlite3* database, const CDataDict& original_dictionary, const CaseAccess* case_access)
    :   m_case_access(case_access)
{
    const CDataDict& new_dictionary = case_access->GetDataDict();
    m_keys_changed = KeysChanged(original_dictionary, new_dictionary);

    SQLiteDictionarySchemaGenerator schema_generator;
    const SQLiteSchema::Schema new_schema = schema_generator.GenerateDictionary(new_dictionary);
    const SQLiteSchema::Schema current_schema = GetCurrentSchema(database);

    m_schema_diffs = SQLiteSchema::SchemaDifferences(current_schema, new_schema);
}


void SQLiteDictionarySchemaReconciler::Reconcile(sqlite3* database)
{
    SQLiteTransaction transaction(database);
    if (!m_schema_diffs.Empty()) {
        m_schema_diffs.Migrate(database);
    }
    if (m_keys_changed)
        UpdateKeys(database, *m_case_access);
}


bool SQLiteDictionarySchemaReconciler::NeedsReconcile() const
{
    return m_keys_changed || !m_schema_diffs.Empty();
}


bool SQLiteDictionarySchemaReconciler::KeysChanged(const CDataDict& original_dictionary, const CDataDict& new_dictionary)
{
    const DictionaryComparer comparer(original_dictionary, new_dictionary);
    const std::vector<DictionaryDifference>& dictionary_diffs = comparer.GetDifferences();
    return std::find_if(dictionary_diffs.cbegin(),
                        dictionary_diffs.cend(),
                        [](const DictionaryDifference& d) { return d.key_related; }) != dictionary_diffs.cend();
}


void SQLiteDictionarySchemaReconciler::UpdateKeys(sqlite3* database, const CaseAccess& case_access)
{
    auto first_level = case_access.GetCaseMetadata().GetCaseLevelsMetadata().front();
    auto id_items_record = first_level->GetIdCaseRecordMetadata();
    auto id_items = id_items_record->GetCaseItems();
    const auto key_length = first_level->GetLevelKeyLength();

    std::ostringstream ss;
    ss << "SELECT cases.id";
    for (auto id_item : id_items)
        ss << ",`level-1`.`" << ToLowerUtf8(id_item->GetDictionaryItem().GetName()) << '`';
    ss << " FROM cases JOIN `level-1` ON cases.id = `level-1`.`case-id`";

    SQLiteStatement list_ids(database, ss.str().c_str());

    SQLiteStatement update_key(database, "UPDATE cases SET `key`=? WHERE id=?");

    int list_ids_result;
    while ((list_ids_result = list_ids.Step()) == SQLITE_ROW) {

        CString key;
        TCHAR* key_buffer = key.GetBufferSetLength(key_length);
        _tmemset(key_buffer, ' ', key_length);

        for (size_t i = 0; i < id_items.size(); ++i)
        {
            const int column_index = i + 1;
            const auto& item = id_items[i]->GetDictionaryItem();
            ASSERT(DictionaryRules::CanBeIdItem(item));

            if (item.GetContentType() == ContentType::Alpha) {
                CString val = list_ids.GetColumn<CString>(column_index);
                _tcsncpy(key_buffer, (LPCTSTR) val, std::min((size_t)val.GetLength(), (size_t)item.GetLen()));
            }

            else {
                ASSERT(item.GetContentType() == ContentType::Numeric);
                double val;
                auto column_type = list_ids.GetColumnType(column_index);
                if (column_type == SQLITE_NULL) {
                    val = NOTAPPL;
                } else if (column_type == SQLITE_FLOAT || column_type == SQLITE_INTEGER) {
                    val = list_ids.GetColumn<double>(column_index);
                } else {
                    val = DEFAULT;
                }
                NumberConverter::DoubleToText(val, key_buffer, item.GetLen(), item.GetDecimal(), item.GetZeroFill(), item.GetDecChar());
            }
            key_buffer += item.GetLen();
        }

        key.ReleaseBuffer(key_length);

        update_key.Reset();
        update_key.Bind(1, key).Bind(2, list_ids.GetColumn<const char*>(0));
        int update_result = update_key.Step();
        if (update_result != SQLITE_DONE)
            throw SQLiteErrorWithMessage(database);
    }

    if (list_ids_result != SQLITE_DONE)
        throw SQLiteErrorWithMessage(database);
}


SQLiteSchema::Schema SQLiteDictionarySchemaReconciler::GetCurrentSchema(sqlite3* database)
{
    SQLiteSchema::Schema current_schema(database);

    std::set<std::string> built_in_tables{ "cases", "file_revisions", "meta", "notes", "sync_history", "vector_clock" };
    current_schema.tables.erase(std::remove_if(current_schema.tables.begin(),
        current_schema.tables.end(),
        [&built_in_tables](const auto& t) { return built_in_tables.find(t.name.name) != built_in_tables.end(); }),
        current_schema.tables.end());
    current_schema.indexes.erase(std::remove_if(current_schema.indexes.begin(),
        current_schema.indexes.end(),
        [&built_in_tables](const auto& i) { return built_in_tables.find(i.table.name) != built_in_tables.end(); }),
        current_schema.indexes.end());
    return current_schema;
}
