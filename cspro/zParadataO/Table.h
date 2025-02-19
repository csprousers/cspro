#pragma once

#include <zParadataO/TableDefinitions.h>

namespace Paradata { class Table; }

struct sqlite3_stmt;
class SQLiteStatement;


class Paradata::Table
{
    friend class Concatenator;
    friend class Log;
    friend class Syncer;

public:
    enum class ColumnType
    {
        Boolean,
        Integer,
        Long,
        Double,
        Text
    };

    struct ColumnEntry
    {
        std::string name;
        ColumnType type;
        bool nullable;
    };

private:
    struct CodeEntry
    {
        size_t column_index;
        int code;
        std::string value;
    };

private:
    Table(sqlite3* db, ParadataTable type);

public:
    ~Table();

    Table& AddColumn(std::string name, ColumnType type, bool nullable = false);

    template<typename CodeType>
    Table& AddCode(CodeType code, std::string value);

    // Adds an index on the column indices (zero-indexed, not counting the id column) specified.
    Table& AddIndex(cs::string_sz name, std::initializer_list<size_t> indices);

    // If using an auto increment ID table, id will be filled with the newly inserted ID.
    // Otherwise, id will be used as the ID for the insert.
    template<typename... Args>
    void Insert(long* id, Args const&... args);

private:
    // Creates the table (if necessary) and creates the prepared statement for inserts.
    void CreateTable(bool check_for_column_completeness);

    static ColumnType StringToColumnType(std::string_view column_type_text_sv);
    static const char* ColumnTypeToString(ColumnType column_type);
    static const char* ColumnTypeToSqlType(ColumnType column_type);

    void AddMetadata(Table& metadata_table_info_table, Table& metadata_column_info_table, Table& metadata_code_info_table);

    void BindArguments(sqlite3_stmt* stmt, int bind_index, va_list args, std::vector<int>* nullable_arguments = nullptr);

private:
    void InsertWorker(long* id, ...);

private:
    sqlite3* m_db;
    const TableDefinition& m_tableDefinition;
    std::unique_ptr<SQLiteStatement> m_insertStmt;
    std::unique_ptr<SQLiteStatement> m_findDuplicateRowStmt;
    std::string m_findDuplicateRowSql;
    const bool m_autoIncrementId;
    std::vector<ColumnEntry> m_columns;
    std::vector<CodeEntry> m_codes;
    std::unique_ptr<std::vector<std::string>> m_indicesSql;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Paradata::Table& Paradata::Table::AddColumn(std::string name, const ColumnType type, const bool nullable/* = false*/)
{
    m_columns.emplace_back(ColumnEntry { std::move(name), type, nullable });
    return *this;
}


template<typename CodeType>
Paradata::Table& Paradata::Table::AddCode(const CodeType code, std::string value)
{
    static_assert(sizeof(CodeType) == sizeof(int));
    m_codes.emplace_back(CodeEntry { m_columns.size() - 1, static_cast<int>(code), std::move(value) });
    return *this;
}


template<typename... Args>
void Paradata::Table::Insert(long* const id, Args const&... args)
{
#ifdef _DEBUG
    (
        [&]
        {
            using type = std::remove_cvref_t<decltype(args)>;

            if constexpr(!std::is_same_v<type, bool> &&
                         !std::is_same_v<type, const bool*> &&
                         !std::is_same_v<type, const int*> &&
                         !std::is_same_v<type, const long*> &&
                         !std::is_same_v<type, const double*>)
            {
                ValidateFormatTextArgumentTypes(args);
            }
        }
    (), ...);
#endif

    InsertWorker(id, args...);
}
