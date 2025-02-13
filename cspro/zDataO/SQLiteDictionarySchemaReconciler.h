#pragma once

#include <zUtilO/SQLiteSchema.h>

class CaseAccess;
class CDataDict;
struct sqlite3;


/// <summary>
/// Update SQLite data tables when dictionary changes.
/// </summary>
class SQLiteDictionarySchemaReconciler
{
public:

    /// <summary>
    /// Create reconciler for database
    /// </summary>
    /// <param name="database">SQL lite database to update</param>
    /// <param name="original_dictionary">data dictionary that matches current table schema in database</param>
    /// <param name="case_access">CaseAccess that contains new dictionary that database needs to be modified to match</param>
    SQLiteDictionarySchemaReconciler(sqlite3* database, const CDataDict& original_dictionary, const CaseAccess* case_access);

    /// <summary>
    /// Return true if database needs to be modified to match dictionary
    /// </summary>
    bool NeedsReconcile() const;

    /// <summary>
    /// Update database based on dictionary differences
    /// </summary>
    void Reconcile(sqlite3* database);

private:
    void GetSchemaDiffs(sqlite3* database, const CDataDict& new_dictionary);
    void UpdateSchema();
    static bool KeysChanged(const CDataDict& original_dictionary, const CDataDict& new_dictionary);
    void UpdateKeys(sqlite3* database, const CaseAccess& case_access);
    SQLiteSchema::Schema GetCurrentSchema(sqlite3* database);

    const CaseAccess* m_case_access;
    bool m_keys_changed;
    SQLiteSchema::SchemaDifferences m_schema_diffs;
};

