#pragma once

#include <cstring>
#include <ostream>
#include <vector>
#include <sstream>
#include <variant>
#include <SQLite/SQLite.h>
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/zUtilO.h>

/// <summary>
/// Classes to represent the structure of a SQLite database
/// </summary>
namespace SQLiteSchema {

    /// <summary>
    /// Identifier for name of column/table/index - wrapper on std::string
    /// </summary>
    struct Name {
        std::string name;
        Name() = default;
        Name(const std::string& name) : name(name) {}
        Name(const char* name) : name(name) {}
        bool operator==(const Name& rhs) const
        {
            return name == rhs.name;
        }
    };

    typedef std::variant<double, std::string> DefaultValue;

    enum class ColumnType { INTEGER, TEXT, BLOB };
    enum ColumnFlags { NOT_NULL = 1, PRIMARY = 2 };

    /// <summary>
    /// Column in SQLite table
    /// </summary>
    struct Column {
        Name name;
        ColumnType type;
        ColumnFlags flags = ColumnFlags(0);
        std::optional<DefaultValue> default_value = {};
    };

    enum ForeignKeyFlags { DELETE_CASCADE = 1 };

    /// <summary>
    /// Foreign key in SQLite table
    /// </summary>
    struct ForeignKey {
        Name column;
        Name parent_table;
        Name parent_column;
        ForeignKeyFlags flags = ForeignKeyFlags(0);
    };

    /// <summary>
    /// Table in SQLite database
    /// </summary>
    struct Table {
        Name name;
        std::vector<Column> columns;
        std::vector<ForeignKey> foreign_keys;

        Table& operator+=(const Column&& rhs)
        {
            columns.emplace_back(rhs);
            return *this;
        }

        Table& operator+=(std::vector<Column>&& rhs)
        {
            columns.insert(columns.end(), std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
            return *this;
        }

        Table& operator+=(ForeignKey&& rhs)
        {
            foreign_keys.emplace_back(std::move(rhs));
            return *this;
        }

    };

    /// <summary>
    /// Index (constraint) in SQLite database
    /// </summary>
    struct Index {
        Name name;
        Name table;
        std::vector<Name> columns;
        bool unique;

        Index(const Name& name, const Name& table_name, const std::vector<Name>& column_names, bool unique) :
            name(name),
            table(table_name),
            columns(column_names),
            unique(unique)
        {}

        Index(const Name& table_name, const std::vector<Name>& column_names, bool unique) :
            name(DefaultName(table_name, column_names)),
            table(table_name),
            columns(column_names),
            unique(unique)
        {}

        static Name DefaultName(const Name& table_name, const std::vector<Name>& column_names)
        {
            // Default index name is table_column1_column2...
            Name name(table_name);
            for (const auto& column : column_names)
                name.name += '-' + column.name;
            return name;
        }
    };

    /// <summary>
    /// Schema of a SQLite database
    /// </summary>
    struct CLASS_DECL_ZUTILO Schema {
        std::vector<Table> tables;
        std::vector<Index> indexes;

        Schema() = default;

        Schema(const std::vector<Table>& tables, const std::vector<Index>& indexes) :
            tables(tables),
            indexes(indexes)
        {}

        Schema(std::vector<Table>&& tables,std::vector<Index>&& indexes) :
            tables(std::move(tables)),
            indexes(std::move(indexes))
        {}

        /// <summary>
        /// Create from existing database
        /// </summary>
        Schema(sqlite3* database);

        /// <summary>
        /// Create a database from a schema
        /// </summary>
        void CreateDatabase(sqlite3* database);

        /// <summary>
        /// Combine two schemas
        /// </summary>
        Schema& operator+=(const Schema&& rhs)
        {
            tables.insert(tables.end(), std::make_move_iterator(rhs.tables.begin()), std::make_move_iterator(rhs.tables.end()));
            indexes.insert(indexes.end(), std::make_move_iterator(rhs.indexes.begin()), std::make_move_iterator(rhs.indexes.end()));
            return *this;
        }
    };

    struct NewTable {
        Table table;
    };

    struct NewColumn {
        Name table_name;
        Column column;
    };

    struct NewIndex {
        Index index;
    };

    typedef std::variant<NewTable, NewColumn, NewIndex> SchemaDifference;

    /// <summary>
    /// Differences between two schemas
    /// </summary>
    class CLASS_DECL_ZUTILO SchemaDifferences {
    public:

        SchemaDifferences() = default;

        /// <summary>
        /// Construct differences from original and updated schema
        /// </summary>
        /// Finds new columns, tables and indexes that are in new_schema that are not in original_schema.
        /// Does not find removed columns, tables or indexes or columns/indexes that were modified as
        /// those are not needed for csdb file reconcialation.
        SchemaDifferences(const Schema& original_schema, const Schema& new_schema);

        bool Empty() const;

        /// <summary>
        /// Apply the differences to an existing database.
        /// </summary>
        /// If the differences are applied to the database that original_schema
        /// was generated from then after the call to Migrate the database
        /// will now match new_schema.
        void Migrate(sqlite3* database) const;

    private:
        std::vector<SchemaDifference> differences;

        void DiffTables(const Schema& original_schema, const Schema& new_schema);
        void DiffColumns(const Table& original_table, const Table& new_table);
        void DiffIndexes(const Schema& original_schema, const Schema& new_schema);

        friend std::ostream& operator<<(std::ostream& os, const SchemaDifferences& diffs);
    };


    inline std::ostream& operator<<(std::ostream& os, const Name& name)
    {
        os << '`' << name.name << '`';
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, ColumnType type)
    {
        switch (type) {
        case ColumnType::INTEGER:
            os << "INTEGER";
            break;
        case ColumnType::TEXT:
            os << "TEXT";
            break;
        case ColumnType::BLOB:
            os << "BLOB";
            break;
        }
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const DefaultValue& default_value)
    {
        os << " DEFAULT ";
        if (std::holds_alternative<double>(default_value))
            os << std::get<double>(default_value);
        else
            os << "'" << std::get<std::string>(default_value) << "'";
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const Column& column)
    {
        os << column.name << ' ' << column.type;
        if (column.flags & ColumnFlags::PRIMARY)
            os << " PRIMARY KEY";
        if (column.flags & ColumnFlags::NOT_NULL)
            os << " NOT NULL";
        if (column.default_value)
            os << *column.default_value;
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const ForeignKey& foreign_key)
    {
        os << "FOREIGN KEY(" << foreign_key.column << ") REFERENCES " << foreign_key.parent_table << '(' << foreign_key.parent_column << ')';
        if (foreign_key.flags & ForeignKeyFlags::DELETE_CASCADE)
            os << " ON DELETE CASCADE";
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const Table& table)
    {
        os << "CREATE TABLE " << table.name << '(';
        for (size_t i = 0; i < table.columns.size(); ++i) {
            if (i > 0)
                os << ',';
            os << table.columns[i];
        }
        for (size_t i = 0; i < table.foreign_keys.size(); ++i) {
            os << ',' << table.foreign_keys[i];
        }
        os << ");";
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const Index& index)
    {
        os << "CREATE ";
        if (index.unique)
            os << "UNIQUE ";
        os << "INDEX " << index.name << " ON " << index.table << '(';
        for (size_t i = 0; i < index.columns.size(); ++i) {
            if (i > 0)
                os << ',';
            os << index.columns[i];
        }
        os << ");";
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const Schema& schema)
    {
        for (const auto& table : schema.tables) {
            os << table;
        }
        for (const auto& index : schema.indexes) {
            os << index;
        }
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const NewTable& new_table)
    {
        os << new_table.table;
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const NewColumn& new_column)
    {
        os << "ALTER TABLE " << new_column.table_name << " ADD " << new_column.column;
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const NewIndex& new_index)
    {
        os << new_index.index;
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const SchemaDifference& diff)
    {
        std::visit([&os](const auto& elem) { os << elem; }, diff);
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const SchemaDifferences& diffs)
    {
        for (const auto& d : diffs.differences) {
            os << d << ';';
        }
        return os;
    }

}
