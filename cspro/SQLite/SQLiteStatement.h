#pragma once

#include <SQLite/SQLite.h>
#include <zToolsO/Utf8Convert.h>


// SQLiteStatement is a helper class for simplifying code to handle SQLite prepared statements.
// Use it to implement the common pattern of preparing the statement if it has not already been
// prepared, bind parameters, step the statement, and reset the statement.

CREATE_CSPRO_EXCEPTION(SQLiteStatementException)


class SQLiteStatement
{
    friend class SQLiteResetOnDestruction;

public:
    // ------------------------------------------------------------------------------------
    // CONSTRUCTION
    // ------------------------------------------------------------------------------------

    // Create a statement from a UTF-8 string. The statement is finalized on destruction.
    SQLiteStatement(sqlite3* db, const char* sql, const bool throw_prepare_exception = false)
        :   m_finalizeStatementWhenComplete(true)
    {
        if( sqlite3_prepare_v2(db, sql, -1, &m_stmt, nullptr) != SQLITE_OK && throw_prepare_exception )
            throw SQLiteStatementException(_T("SQLite: Could not prepare: %s"), UTF8Convert::UTF8ToWide(sql).c_str());
    }

    // Create a statement from a UTF-8 string. The statement is finalized on destruction.
    SQLiteStatement(sqlite3* db, const std::string& sql, const bool throw_prepare_exception = false)
        :   m_finalizeStatementWhenComplete(true)
    {
        if( sqlite3_prepare_v2(db, sql.c_str(), sql.length() + 1, &m_stmt, nullptr) != SQLITE_OK && throw_prepare_exception )
            throw SQLiteStatementException(_T("SQLite: Could not prepare: %s"), UTF8Convert::UTF8ToWide(sql).c_str());
    }

    // Create a statement from a wide character string. The statement is finalized on destruction.
    SQLiteStatement(sqlite3* db, wstring_view sql_sv, const bool throw_prepare_exception = false)
        :   SQLiteStatement(db, UTF8Convert::WideToUTF8(sql_sv), throw_prepare_exception)
    {
    }

    // Wrap an existing prepared statement. The statement is reset but not finalized on destruction (unless specified).
    SQLiteStatement(sqlite3_stmt* stmt, const bool finalize_statement_when_complete = false)
        :   m_stmt(stmt),
            m_finalizeStatementWhenComplete(finalize_statement_when_complete)
    {
    }

    // Create a statement from a SQL string and store the SQLite prepared statement pointer
    // in stmt so that it can be reused later. If stmt is not null it will be reused instead
    // of being prepared. This allows you to store the statement in a member for reuse.
    // In this case the underlying statement is not finalized upon destruction but it is
    // reset upon destruction.
    SQLiteStatement(sqlite3* db, sqlite3_stmt*& stmt, const char* const sql, const bool throw_prepare_exception = false)
        :   m_finalizeStatementWhenComplete(false)
    {
        // initialize the prepared statement if it has not already been initialized
        if( stmt == nullptr && sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK && throw_prepare_exception )
            throw SQLiteStatementException(_T("SQLite: Could not prepare: %s"), UTF8Convert::UTF8ToWide(sql).c_str());

        m_stmt = stmt;
    }


    // forbid copying of the SQLiteStatement object to prevent issues with both the original
    // and copied statement being finalized...
    SQLiteStatement(const SQLiteStatement&) = delete;

    // ...but allow moving
    SQLiteStatement(SQLiteStatement&& rhs) noexcept
        :   m_finalizeStatementWhenComplete(rhs.m_finalizeStatementWhenComplete)
    {
        m_stmt = rhs.Release();
    }

    // ------------------------------------------------------------------------------------
    // DECONSTRUCTION AND RELEASE
    // ------------------------------------------------------------------------------------

    ~SQLiteStatement()
    {
        if( m_stmt != nullptr )
        {
            if( m_finalizeStatementWhenComplete )
            {
                sqlite3_finalize(m_stmt);
            }

            else
            {
                // ensure that the statement gets reset when done; per the SQLite documentation
                // implicit transactions are only commited when all statements are finalized or reset
                // so we don't want to leave unreset statements around
                sqlite3_reset(m_stmt);
            }
        }
    }

    // Returns a pointer to the SQLite prepared statement, transfering ownership to caller.
    // The destructor will no longer reset or finalize the statement.
    sqlite3_stmt* Release()
    {
        sqlite3_stmt* stmt = m_stmt;
        m_stmt = nullptr;
        return stmt;
    }


    // ------------------------------------------------------------------------------------
    // BINDING
    // Parameter numbers start with 1.
    // ------------------------------------------------------------------------------------

public:
    int GetBindingsCount()
    {
        return sqlite3_bind_parameter_count(m_stmt);
    }

	template<typename PT>
	int GetParameterNumber(PT parameter_number_or_name)
    {
        int pn;

        if constexpr(std::is_same_v<PT, const char*>)
        {
            ASSERT(*parameter_number_or_name == '@');
            pn = sqlite3_bind_parameter_index(m_stmt, parameter_number_or_name);
        }

        else
        {
#ifndef __clang__
            static_assert(constexpr(std::is_same_v<PT, int>));
#endif
            pn = parameter_number_or_name;
        }

        ASSERT(pn >= 1);

        return pn;
    }

    // Binds a value to a parameter in a prepared statement using either the parameter number
    // or the parameter name. Numbering is from left to right starting at 1. Returns *this so
    // that statements may be chained. e.g. stmt.Bind(1, "foo").Bind(2, "bar);
	template<typename VT, typename PT>
	SQLiteStatement& Bind(PT parameter_number_or_name, VT value)
    {
        const int pn = GetParameterNumber(parameter_number_or_name);

        if constexpr(std::is_same_v<VT, int>)
        {
            sqlite3_bind_int(m_stmt, pn, value);
        }

        else if constexpr(std::is_same_v<VT, int64_t> || std::is_same_v<VT, size_t> || std::is_same_v<VT, long>)
        {
            sqlite3_bind_int64(m_stmt, pn, static_cast<int64_t>(value));
        }

        else if constexpr(std::is_same_v<VT, double> || std::is_same_v<VT, float>)
        {
            sqlite3_bind_double(m_stmt, pn, value);
        }

        else if constexpr(std::is_same_v<VT, bool>)
        {
            sqlite3_bind_int(m_stmt, pn, value ? 1 : 0);
        }

        else if constexpr(std::is_same_v<VT, const char*>)
        {
            sqlite3_bind_text(m_stmt, pn, value, -1, SQLITE_TRANSIENT);
        }

        else if constexpr(std::is_same_v<VT, const TCHAR*> || std::is_same_v<VT, wstring_view>)
        {
            sqlite3_bind_text(m_stmt, pn, UTF8Convert::WideToUTF8(value).c_str(), -1, SQLITE_TRANSIENT);
        }

        else if constexpr(std::is_same_v<VT, std::tuple<const std::byte*, size_t>>)
        {
            sqlite3_bind_blob64(m_stmt, pn, std::get<0>(value), static_cast<sqlite3_uint64>(std::get<1>(value)), SQLITE_STATIC);
        }

        else
        {
            static_assert_false();
        }

        return *this;
    }

	template<typename PT>
	SQLiteStatement& Bind(PT parameter_number_or_name, const std::string& value)
    {
        return Bind(parameter_number_or_name, value.c_str());
    }

	template<typename PT>
	SQLiteStatement& Bind(PT parameter_number_or_name, const std::wstring& value)
    {
        return Bind(parameter_number_or_name, wstring_view(value));
    }

	template<typename PT>
	SQLiteStatement& Bind(PT parameter_number_or_name, const CString& value)
    {
        return Bind(parameter_number_or_name, wstring_view(value));
    }

	template<typename PT>
	SQLiteStatement& Bind(PT parameter_number_or_name, const std::byte* value, size_t size)
    {
        return Bind(parameter_number_or_name, std::make_tuple(value, size));
    }

    template<typename PT>
	SQLiteStatement& Bind(PT parameter_number_or_name, const std::vector<std::byte>& value)
    {
        return Bind(parameter_number_or_name, std::make_tuple(value.data(), value.size()));
    }

    // Binds null to a parameter in a prepared statement using either the parameter number
    // or the parameter name. Numbering is from left to right starting at 1. Returns *this so
    // that statements may be chained. e.g. stmt.Bind(1, "foo").Bind(2, "bar);
	template<typename PT>
	SQLiteStatement& BindNull(PT parameter_number_or_name)
    {
        sqlite3_bind_null(m_stmt, GetParameterNumber(parameter_number_or_name));
        return *this;
    }


    // ------------------------------------------------------------------------------------
    // STEPPING AND RESETTING
    // ------------------------------------------------------------------------------------

    // Steps the prepared statement.
    int Step()
    {
        return sqlite3_step(m_stmt);
    }

    // Steps the prepared statement and throws an exception if the result does not match the argument.
    void StepCheckResult(const int result)
    {
        if( sqlite3_step(m_stmt) != result )
            throw SQLiteStatementException(_T("SQLite: Stepping did not result in code: %d"), result);
    }

    // Resets the underlying prepared statement so that it may be executed again.
    // Resetting does not clear the bindings.
    SQLiteStatement& Reset()
    {
        sqlite3_reset(m_stmt);
        return *this;
    }


    // ------------------------------------------------------------------------------------
    // RETRIEVAL
    // Column numbers start with 0.
    // ------------------------------------------------------------------------------------

    int GetColumnCount()
    {
        return sqlite3_column_count(m_stmt);
    }

    std::wstring GetColumnName(const int column_num)
    {
        return UTF8Convert::UTF8ToWide(sqlite3_column_name(m_stmt, column_num));
    }

    // Returns the type of the data in this column: SQLITE_NULL, SQLITE_INTEGER,
    // SQLITE_FLOAT, SQLITE_TEXT, or SQLITE_BLOB. Column numbers start with 0.
    int GetColumnType(const int column_num)
    {
        return sqlite3_column_type(m_stmt, column_num);
    }

    // Returns true if the column has a null value. Column numbers start with 0.
    bool IsColumnNull(const int column_num)
    {
        return ( GetColumnType(column_num) == SQLITE_NULL );
    }

    // Returns the value of the column. Column numbers start with 0.
    template <typename VT>
    VT GetColumn(const int column_num)
    {
        if constexpr(std::is_same_v<VT, int>)
        {
            return sqlite3_column_int(m_stmt, column_num);
        }

        else if constexpr(std::is_same_v<VT, int64_t> || std::is_same_v<VT, size_t> || std::is_same_v<VT, long>)
        {
            return static_cast<VT>(sqlite3_column_int64(m_stmt, column_num));
        }

        else if constexpr(std::is_same_v<VT, double>)
        {
            return sqlite3_column_double(m_stmt, column_num);
        }

        else if constexpr(std::is_same_v<VT, float>)
        {
            return static_cast<float>(sqlite3_column_double(m_stmt, column_num));
        }

        else if constexpr(std::is_same_v<VT, bool>)
        {
            return ( sqlite3_column_int(m_stmt, column_num) != 0 );
        }

        else if constexpr(std::is_same_v<VT, const char*>)
        {
            return reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, column_num));
        }

        else if constexpr(std::is_same_v<VT, std::string>)
        {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, column_num));
            return ( text != nullptr ) ? std::string(text) : std::string();
        }

        else if constexpr(std::is_same_v<VT, std::wstring> || std::is_same_v<VT, CString>)
        {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, column_num));
            return UTF8Convert::UTF8ToWide<VT>(text);
        }

        else if constexpr(std::is_same_v<VT, std::vector<std::byte>>)
        {
            const int size = sqlite3_column_bytes(m_stmt, column_num);
            const std::byte* bytes = reinterpret_cast<const std::byte*>(sqlite3_column_blob(m_stmt, column_num));
            return std::vector<std::byte>(bytes, bytes + size);
        }

        else
        {
            static_assert_false();
        }
    }


private:
    sqlite3_stmt* m_stmt;
    bool m_finalizeStatementWhenComplete;
};



class SQLiteResetOnDestruction
{
public:
    SQLiteResetOnDestruction(SQLiteStatement& sqlite_statement)
        :   m_sqliteStatement(sqlite_statement)
    {
    }

    ~SQLiteResetOnDestruction()
    {
        m_sqliteStatement.Reset();
    }

private:
    SQLiteStatement& m_sqliteStatement;
};



class SQLiteFinalizeOnDestruction
{
public:
    SQLiteFinalizeOnDestruction(sqlite3_stmt* stmt)
        :   m_stmt(stmt)
    {
        ASSERT(m_stmt != nullptr);
    }

    ~SQLiteFinalizeOnDestruction()
    {
        sqlite3_finalize(m_stmt);
    }

private:
    sqlite3_stmt* m_stmt;
};
