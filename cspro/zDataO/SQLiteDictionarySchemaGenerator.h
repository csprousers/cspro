#pragma once

#include <zDictO/DDClass.h>
#include <zUtilO/SQLiteSchema.h>


/// <summary>
/// Create SQLite SQL statements to create tables for a data dictionary.
/// </summary>
class SQLiteDictionarySchemaGenerator
{
public:
    /// <summary>
    /// Write SQL statements to create dictionary tables to output stream
    /// </summary>
    /// <param name="os">stream to write SQL statements to</param>
    /// <param name="dictionary">data dictionary to generate tables for</param>
    SQLiteSchema::Schema GenerateDictionary(const CDataDict& dictionary);

    SQLiteSchema::Schema GenerateBinaryTable();
    SQLiteSchema::Schema GenerateBinarySyncHistoryTable();
    SQLiteSchema::Schema GenerateBinarySyncHistoryArchiveTable();

    static constexpr const char* BinaryColumnPostfixSignature = "-signature";
    static constexpr const char* BinaryColumnPostfixMetadata = "-metadata";

private:
    SQLiteSchema::Schema GenerateCaseBinaryTable();

    SQLiteSchema::Schema GenerateLevel(const DictLevel& level, const DictLevel* parent_level);

    SQLiteSchema::Schema GenerateLevelIdsTable(const DictLevel& level, const DictLevel* parent_level);

    SQLiteSchema::Schema GenerateRecordTable(const CDictRecord& record);

    std::vector<SQLiteSchema::Column> GenerateRecordItems(const CDictRecord& record);

    void GenerateItem(const CDictItem& item, std::vector<SQLiteSchema::Column>& columns);
};
