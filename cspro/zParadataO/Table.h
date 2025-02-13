#pragma once
#include "TableDefinitions.h"

namespace Paradata
{
    class Table
    {
        friend class Log;
        friend class Concatenator;
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
            CString name;
            ColumnType type;
            bool nullable;
        };

    private:
        sqlite3* m_db;
        const TableDefinition& m_tableDefinition;
        sqlite3_stmt* m_insertStmt;
        sqlite3_stmt* m_findDuplicateRowStmt;
        CString m_findDuplicateRowSql;
        bool m_autoIncrementId;

        std::vector<ColumnEntry> m_columns;

        struct CodeEntry
        {
            size_t column_index;
            int code;
            CString value;
        };

        std::vector<CodeEntry> m_codes;

        std::vector<CString> m_indicesSql;

        Table(sqlite3* db, ParadataTable type);

        ///<summary>Creates the table (if necessary) and creates the prepared statement for inserts.</summary>
        void CreateTable(bool check_for_column_completeness);

        static ColumnType StringToColumnType(const CString& column_type_text);
        static const TCHAR* ColumnTypeToString(ColumnType column_type);
        static const TCHAR* ColumnTypeToSqlType(ColumnType column_type);

        void AddMetadata(Table& metadata_table_info_table, Table& metadata_column_info_table, Table& metadata_code_info_table);

        void BindArguments(sqlite3_stmt* stmt, int bind_index, va_list args, std::vector<int>* nullable_arguments = nullptr);

    public:
        ~Table();

        Table& AddColumn(const CString& name, ColumnType type, bool nullable = false);
        Table& AddCode(int code, const CString& value);

        ///<summary>Adds any index on the column indices (zero-indexed, not counting the id column) specified.</summary>
        Table& AddIndex(CString name, const std::vector<size_t>& indices);

        ///<summary>If using an auto increment ID table, id will be filled with the newly inserted ID.
        /// Otherwise, id will be used as the ID for the insert.</summary>
        void Insert(long* id, ...);
    };
}
