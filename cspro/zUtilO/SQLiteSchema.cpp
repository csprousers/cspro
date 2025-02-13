#include "StdAfx.h"
#include "SQLiteSchema.h"
#include <SQLite/SQLiteStatement.h>
#include <zDataO/SQLiteErrorWithMessage.h>


namespace SQLiteSchema {

    template <typename T>
    void RunSql(const T& t, sqlite3* database)
    {
        std::ostringstream ss;
        ss << t;
        if (sqlite3_exec(database, ss.str().c_str(), NULL, NULL, NULL) != SQLITE_OK) {
            throw SQLiteErrorWithMessage(database);
        }
    }

    void Schema::CreateDatabase(sqlite3* database)
    {
        RunSql(*this, database);
    }

    std::vector<std::string> ReadTableNames(sqlite3* database)
    {
        std::vector<std::string> table_names;
        SQLiteStatement statement(database, "SELECT name FROM sqlite_master WHERE type = 'table'");
        int query_result;
        while ((query_result = statement.Step()) == SQLITE_ROW)
            table_names.emplace_back(statement.GetColumn<std::string>(0));

        if (query_result != SQLITE_DONE)
            throw SQLiteErrorWithMessage(database);

        return table_names;
    }

    Column ReadColumn(SQLiteStatement& statement) {
        auto name = statement.GetColumn<std::string>(0);
        auto typeString = statement.GetColumn<std::string>(1);
        ColumnType type;
        if (typeString == "TEXT")
            type = ColumnType::TEXT;
        else if (typeString == "INTEGER")
            type = ColumnType::INTEGER;
        else
            type = ColumnType::BLOB;

        auto notnull = statement.GetColumn<bool>(2);
        auto pk = statement.GetColumn<bool>(3);


        std::optional<DefaultValue> default_value;
        if (!statement.IsColumnNull(4)) {
            if (type == ColumnType::INTEGER) {
                default_value = statement.GetColumn<double>(4);
            }
            else if (type == ColumnType::TEXT) {
                default_value = statement.GetColumn<std::string>(4);
            }
            // Ignore blobs with default values
        }
        ColumnFlags flags = ColumnFlags((notnull ? ColumnFlags::NOT_NULL : 0) || (pk ? ColumnFlags::PRIMARY : 0));
        return Column{ name, type, flags, default_value };
    }

    Table ReadTable(sqlite3* database, const std::string& name)
    {
        Table table{ name };
        SQLiteStatement statement(database, "SELECT name, type, `notnull`, pk, dflt_value FROM pragma_table_info(?)");
        statement.Bind(1, name);
        int query_result;
        while ((query_result = statement.Step()) == SQLITE_ROW)
            table.columns.emplace_back(ReadColumn(statement));

        if (query_result != SQLITE_DONE)
            throw SQLiteErrorWithMessage(database);

        //TODO: foreign keys
        return table;
    }

    std::vector<Index> ReadIndexes(sqlite3* database, const std::string& table_name)
    {
        std::vector<Index> indexes;
        SQLiteStatement list_indexes(database, "SELECT name, `unique` FROM pragma_index_list(?)");
        list_indexes.Bind(1, table_name);
        SQLiteStatement list_columns(database, "SELECT name FROM pragma_index_info(?)");

        int list_indexes_query_result;
        while ((list_indexes_query_result = list_indexes.Step()) == SQLITE_ROW) {
            auto index_name = list_indexes.GetColumn<std::string>(0);
            auto unique = list_indexes.GetColumn<bool>(1);
            list_columns.Reset();
            list_columns.Bind(1, index_name);
            std::vector<Name> columns;
            int list_columns_query_result;
            while ((list_columns_query_result = list_columns.Step()) == SQLITE_ROW) {
                columns.emplace_back(list_columns.GetColumn<std::string>(0));
            }
            if (list_columns_query_result != SQLITE_DONE)
                throw SQLiteErrorWithMessage(database);
            indexes.emplace_back(Index(index_name, table_name, columns, unique));
        }
        if (list_indexes_query_result != SQLITE_DONE)
            throw SQLiteErrorWithMessage(database);

        return indexes;
    }

    Schema::Schema(sqlite3* database)
    {
        auto table_names = ReadTableNames(database);
        for (const auto& table_name : table_names) {
            tables.emplace_back(ReadTable(database, table_name));
            auto&& table_indexes = ReadIndexes(database, table_name);
            indexes.insert(indexes.end(), std::make_move_iterator(table_indexes.begin()), std::make_move_iterator(table_indexes.end()));
        }
    }

    SchemaDifferences::SchemaDifferences(const Schema& original_schema, const Schema& new_schema)
    {
        DiffTables(original_schema, new_schema);
        DiffIndexes(original_schema, new_schema);
    }

    bool SchemaDifferences::Empty() const
    {
        return differences.empty();
    }

    void SchemaDifferences::Migrate(sqlite3* database) const
    {
        RunSql(*this, database);
    }

    void SchemaDifferences::DiffTables(const Schema& original_schema, const Schema& new_schema)
    {
        for (const auto& table : new_schema.tables) {
            auto matching_table = std::find_if(original_schema.tables.begin(), original_schema.tables.end(), [&table](const auto& t) {return table.name == t.name; });
            if (matching_table == original_schema.tables.end()) {
                // Table is not in original
                differences.emplace_back(NewTable{ table });
            }
            else {
                // Found table - check columns
                DiffColumns(*matching_table, table);
            }
        }
    }

    void SchemaDifferences::DiffColumns(const Table& original_table, const Table& new_table)
    {
        for (const auto& column : new_table.columns) {
            auto matching_column = std::find_if(original_table.columns.begin(), original_table.columns.end(), [&column](const auto& c) {return column.name == c.name; });
            if (matching_column == original_table.columns.end())
                differences.emplace_back(NewColumn{ original_table.name, column });
        }
    }

    void SchemaDifferences::DiffIndexes(const Schema& original_schema, const Schema& new_schema)
    {
        for (const auto& index : new_schema.indexes) {
            auto matching_index = std::find_if(original_schema.indexes.begin(), original_schema.indexes.end(), [&index](const auto& i) {return index.name == i.name; });
            if (matching_index == original_schema.indexes.end()) {
                // Index is not in original
                differences.emplace_back(NewIndex{ index });
            }
        }
    }

}
