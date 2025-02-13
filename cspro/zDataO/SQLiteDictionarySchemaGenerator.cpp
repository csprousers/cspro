#include "stdafx.h"
#include "SQLiteDictionarySchemaGenerator.h"
#include "SQLiteSchemaHelpers.h"
#include <zUtilO/SQLiteSchema.h>


using namespace SQLiteSchema;

namespace {

    ColumnType GenerateColumnType(const CDictItem& item)
    {
        switch( item.GetDataType() )
        {
            case DataType::Numeric: return ColumnType::INTEGER;
            case DataType::String:  return ColumnType::TEXT;
            case DataType::Binary:  return ColumnType::TEXT;
            default:                return ReturnProgrammingError(ColumnType::BLOB);
        }
    }
}


Schema SQLiteDictionarySchemaGenerator::GenerateDictionary(const CDataDict& dictionary)
{
    Schema schema;
    const DictLevel* parent_level = nullptr;
    for( const DictLevel& dict_level : dictionary.GetLevels() ) {
        schema += GenerateLevel(dict_level, parent_level);
        parent_level = &dict_level;
    }

    schema += GenerateBinaryTable();
    schema += GenerateCaseBinaryTable();
    schema += GenerateBinarySyncHistoryTable();
    schema += GenerateBinarySyncHistoryArchiveTable();

    return schema;
}


SQLiteSchema::Schema SQLiteDictionarySchemaGenerator::GenerateLevel(const DictLevel& level, const DictLevel* parent_level)
{
    SQLiteSchema::Schema schema = GenerateLevelIdsTable(level, parent_level);

    for (int record_number = 0; record_number < level.GetNumRecords(); ++record_number) {
        schema += GenerateRecordTable(*level.GetRecord(record_number));
    }

    return schema;
}


SQLiteSchema::Schema SQLiteDictionarySchemaGenerator::GenerateBinaryTable()
{
    Table table{ "binary-data" };
    table.columns.emplace_back(Column{ "signature", ColumnType::TEXT, ColumnFlags(ColumnFlags::NOT_NULL | ColumnFlags::PRIMARY) });
    table.columns.emplace_back(Column{ "data", ColumnType::BLOB });
    //binary-data table to store binary data items with md5 signature as the primary key
    table.columns.emplace_back(Column{ "last_modified_revision", ColumnType::INTEGER });
    Index last_modified_index = Index(table.name, { "last_modified_revision" }, false);

    //  Set up Foreign Key from binary-data to file_revisions.
    std::string  column = "last_modified_revision";
    std::string  parentTable = "file_revisions";
    std::string  parentColumn = "id";

    table.foreign_keys.emplace_back(ForeignKey{ column, parentTable, parentColumn });
    return Schema{ {table}, {last_modified_index} };
}


SQLiteSchema::Schema SQLiteDictionarySchemaGenerator::GenerateCaseBinaryTable()
{
    //case-binary-data table to associate binary data items belonging to a case for easier access 
    Table table{ "case-binary-data" };
    table.columns.emplace_back(Column{ "id", ColumnType::INTEGER, ColumnFlags(ColumnFlags::NOT_NULL | ColumnFlags::PRIMARY) });
    table.columns.emplace_back(Column{ "case-id", ColumnType::TEXT });
    table.columns.emplace_back(Column{ "binary-data-signature", ColumnType::TEXT });
    Index case_id_index = Index(table.name, { "case-id" }, false);
    Index binary_data_signature_index = Index(table.name, { "binary-data-signature" }, false);
    Index case_id_binary_data_signature_unique = Index(table.name, { "case-id", "binary-data-signature" }, true);

    //Disabling binary-data reference as in case of Filebased sync for dropbox or ftp the order of file download
    //changes if the binary item content comes in a different paackage leading to non-exisitng binary content at the time of
    // the processing
    //  Set up Foreign Key from case-binary-data to binary-data.
    //std::string column = "binary-data-signature";
    //std::string parentTable = "binary-data";
    //std::string parentColumn = "signature";
    //table.foreign_keys.emplace_back(ForeignKey{ column, parentTable, parentColumn });

    //  Set up Foreign Key from case-binary-data to cases.
    std::string column = "case-id";
    std::string parentTable = "cases";
    std::string parentColumn = "id";
    table.foreign_keys.emplace_back(ForeignKey{ column, parentTable, parentColumn, ForeignKeyFlags::DELETE_CASCADE });

    return Schema{ {table}, {case_id_index, binary_data_signature_index, case_id_binary_data_signature_unique} };
}


SQLiteSchema::Schema SQLiteDictionarySchemaGenerator::GenerateBinarySyncHistoryTable()
{
    Table table{ "binary-sync-history" };
    table.columns.emplace_back(Column{ "id", ColumnType::INTEGER, ColumnFlags(ColumnFlags::NOT_NULL | ColumnFlags::PRIMARY) });
    table.columns.emplace_back(Column{ "binary-data-signature", ColumnType::TEXT });
    table.columns.emplace_back(Column{ "sync-history-id", ColumnType::INTEGER });
    Index binary_data_signature_index = Index(table.name, { "binary-data-signature" }, false);
    Index sync_history_id_index = Index(table.name, { "sync-history-id" }, false);

    //  Set up Foreign Key from binary-sync-history to binary-data.
    std::string  column = "binary-data-signature";
    std::string  parentTable = "binary-data";
    std::string  parentColumn = "signature";
    table.foreign_keys.emplace_back(ForeignKey{ column, parentTable, parentColumn });

    //  Set up Foreign Key from binary-sync-history to sync_history.
    column = "sync-history-id";
    parentTable = "sync_history";
    parentColumn = "id";
    table.foreign_keys.emplace_back(ForeignKey{ column, parentTable, parentColumn });

    return Schema{ {table}, {binary_data_signature_index, sync_history_id_index} };
}


SQLiteSchema::Schema SQLiteDictionarySchemaGenerator::GenerateBinarySyncHistoryArchiveTable()
{
    Table table{ "binary-sync-history-archive" };
    table.columns.emplace_back(Column{ "id", ColumnType::INTEGER, ColumnFlags(ColumnFlags::NOT_NULL | ColumnFlags::PRIMARY) });
    table.columns.emplace_back(Column{ "binary-sync-history-id", ColumnType::INTEGER});
    table.columns.emplace_back(Column{ "binary-data-signature", ColumnType::TEXT });
    table.columns.emplace_back(Column{ "sync-history-id", ColumnType::INTEGER });
    Index binary_data_signature_index = Index(table.name, { "binary-data-signature" }, false);
    Index sync_history_id_index = Index(table.name, { "sync-history-id" }, false);

    return Schema{ {table}, {binary_data_signature_index, sync_history_id_index} };
}


SQLiteSchema::Schema SQLiteDictionarySchemaGenerator::GenerateLevelIdsTable(const DictLevel& level, const DictLevel* parent_level)
{
    const std::string level_name = "level-" + std::to_string(level.GetLevelNumber() + 1);
    const std::string parent_level_name = parent_level ? "level-" + std::to_string(parent_level->GetLevelNumber() + 1) : "case";
    const std::string parent_id = parent_level_name + "-id";
    Table table{level_name};
    table.columns.emplace_back(Column{ level_name + "-id", ColumnType::INTEGER, ColumnFlags(ColumnFlags::NOT_NULL | ColumnFlags::PRIMARY) });

    // First level is linked to uuid in cases table, others are linked by integer to parent level
    const ColumnType parent_key_type = parent_level ? ColumnType::INTEGER : ColumnType::TEXT;
    table.columns.emplace_back(Column{ parent_id, parent_key_type, ColumnFlags::NOT_NULL });

    // For second and third level nodes need an order column
    if (level.GetLevelNumber() > 0)
        table.columns.emplace_back(Column{ "occ", ColumnType::INTEGER, ColumnFlags::NOT_NULL, DefaultValue(1.0) });

    table += GenerateRecordItems(*level.GetIdItemsRec());

    if (parent_level)
        table.foreign_keys.emplace_back(ForeignKey{ parent_id,parent_level_name, parent_id, ForeignKeyFlags::DELETE_CASCADE });

    // Only first level parent id is unique since that is case.id, at higher levels can have multiple child nodes
    const bool unique = parent_level == nullptr; 
    Index parent_id_index = Index(table.name, {parent_id}, unique);

    return Schema{ {table}, {parent_id_index} };
}


Schema SQLiteDictionarySchemaGenerator::GenerateRecordTable(const CDictRecord& record)
{
    const std::string parent_level_name = "level-" + std::to_string(record.GetLevel()->GetLevelNumber() + 1);
    const std::string parent_id = parent_level_name + "-id";

    Table table{ ToLowerUtf8(record.GetName()) };
    table.columns.emplace_back(Column{ ToLowerUtf8(record.GetName()) + "-id", ColumnType::INTEGER, ColumnFlags(ColumnFlags::NOT_NULL | ColumnFlags::PRIMARY) });
    table.columns.emplace_back(Column{ parent_id, ColumnType::INTEGER, ColumnFlags::NOT_NULL });

    if (record.GetMaxRecs() > 1) {
        table.columns.emplace_back(Column{ "occ", ColumnType::INTEGER, ColumnFlags::NOT_NULL, DefaultValue(1.0) });
    }

    table += GenerateRecordItems(record);

    table.foreign_keys.emplace_back(ForeignKey{ parent_id,parent_level_name, parent_id, ForeignKeyFlags::DELETE_CASCADE });

    Index parent_id_index = Index(table.name, {parent_id}, false);

    return Schema{ {table}, {parent_id_index} };
}


std::vector<Column> SQLiteDictionarySchemaGenerator::GenerateRecordItems(const CDictRecord& record)
{
    std::vector<Column> columns;

    for (int item_number = 0; item_number < record.GetNumItems(); ++item_number)
        GenerateItem(*record.GetItem(item_number), columns);

    return columns;
}


void SQLiteDictionarySchemaGenerator::GenerateItem(const CDictItem& item, std::vector<Column>& columns)
{
    const std::string item_name = ToLowerUtf8(item.GetName());
    const ColumnType item_type = GenerateColumnType(item);
    const int item_occurrences = item.GetItemSubitemOccurs();

    auto create_column_name = [](std::string item_name_prefix, const int* occurrence)
    {
        if( occurrence != nullptr )
            item_name_prefix += "(" + std::to_string(*occurrence) + ")";

        return item_name_prefix;
    };

    auto create_columns = [&](const int* occurrence)
    {
        if( IsBinary(item) )
        {
            // for binary data, create two text columns: <item_name>-signature   <item-name>-metadata.
            columns.emplace_back(Column { create_column_name(item_name + BinaryColumnPostfixSignature, occurrence), item_type });
            columns.emplace_back(Column { create_column_name(item_name + BinaryColumnPostfixMetadata, occurrence), item_type });
        }

        else
        {
            columns.emplace_back(Column { create_column_name(item_name, occurrence), item_type });
        }
    };

    if( item_occurrences == 1 )
    {
        create_columns(nullptr);
    }

    else
    {
        for( int occurrence = 1; occurrence <= item_occurrences; ++occurrence )
            create_columns(&occurrence);
    }
}
