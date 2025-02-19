#pragma once

#include <zSql/zSql.h>
#include <zSql/Definitions.h>


// --------------------------------------------------------------------------
// Statement is a wrapper around a SQLite prepared statement.
//
// After preparing a statement, you can bind parameters, step the statement,
// and reset the statement.
//
// The statement will be finalized either when the class is destructed,
// or when the database is closed.
// --------------------------------------------------------------------------

class ZSQL_API Sqlite::Statement
{
    friend class DB;

private:
    Statement(std::shared_ptr<sqlite3_stmt*> statement_ptr);

public:
    Statement();
    Statement(const Statement& rhs) = delete;
    Statement(Statement&& rhs) = default;
    ~Statement();

    Statement& operator=(const Statement& rhs) = delete;
    Statement& operator=(Statement&& rhs) = default;

    // Returns true if the statement is prepared and still valid.
    bool IsPrepared() const;


    // --------------------------------------------------------------------------
    // BINDING
    // Parameter numbers start with 1.
    // --------------------------------------------------------------------------

    // Returns the number of bindings associated with the prepared statement.
    int GetBindingsCount() const;

    // Returns the parameter number associated with the parameter name, throwing
    // an exception is the parameter name is not associated with any parameter.
    int GetParameterNumber(cs::string_sz parameter_name) const;

    // Binds a value to a parameter in a prepared statement using either the parameter number or name.
    // Exceptions are thrown on binding errors (e.g., if the parameter number or name are not valid).
    // Numbering is from left to right starting at 1.
    // Returns *this so that statements may be chained: e.g. stmt.Bind(1, "foo").Bind(2, "bar);
    template<typename PT, typename VT>
    Statement& Bind(PT&& parameter_number_or_name, VT value);

    // Binds a string to a parameter.
    // Numbering is from left to right starting at 1.
    template<typename PT>
    Statement& Bind(PT&& parameter_number_or_name, const std::string& value);

    // Escapes the characters % and _ in the string using the escape char.
    static std::string EscapeForLike(std::string value, char escape = '!');

    // Escapes the appropriate characters and binds the escaped string to a parameter for the expression:
    // LIKE "%[escaped value]%" ESCAPE '[escape]'.
    // Numbering is from left to right starting at 1.
    template<typename PT>
    Statement& BindForLike(PT&& parameter_number_or_name, std::string value, char escape = '!');

    // Binds a blob to a parameter. If the data pointed to by value needs to be copied because its lifetime
    // is shorter than that of the statement (or before the statement is reset or bound to something else),
    // set value_should_be_copied to true.
    // Numbering is from left to right starting at 1.
    template<typename PT>
    Statement& BindBlob(PT&& parameter_number_or_name, const void* value, size_t value_size, bool value_should_be_copied = false);

    template<typename PT>
    Statement& BindBlob(PT&& parameter_number_or_name, const std::vector<std::byte>& value, bool value_should_be_copied = false);

    // Binds null to a parameter.
    // Numbering is from left to right starting at 1.
    template<typename PT>
    Statement& BindNull(PT&& parameter_number_or_name);


    // --------------------------------------------------------------------------
    // STEPPING AND RESETTING
    // --------------------------------------------------------------------------

    // Steps the prepared statement.
    int Step();

    // Resets the underlying prepared statement so that it may be executed again.
    // Resetting does not clear the bindings.
    void Reset();


    // --------------------------------------------------------------------------
    // RETRIEVAL
    // --------------------------------------------------------------------------

    // Returns the number of columns in the result set.
    int GetColumnCount() const;

    // Returns the name of a column in the result set.
    std::string GetColumnName(const int column_number) const;

    // Returns the type of the data in this column:
    // SQLITE_NULL, SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, or SQLITE_BLOB.
    // Column numbers start with 0.
    int GetColumnType(int column_number);

    // Returns true if the column has a null value.
    // Column numbers start with 0.
    bool IsColumnNull(int column_number);

    // Returns the value of the column.
    // Column numbers start with 0.
    template <typename VT>
    VT GetColumn(int column_number);


private:
    // A method, never called, that ensures (using static_assert) that all definitions are valid.
    static constexpr int Code_SQLITE_TRANSIENT = -1;
    static void CheckDefinitionsAtCompileTime();

    // Throws an exception if the statement is not prepared.
    void CheckStatementIsPrepared() const;
    [[noreturn]] static void ThrowExceptionForCheckStatementIsPrepared();

    // Throws an exception if the binding result is invalid.
    static void CheckValidBinding(int result);
    [[noreturn]] static void ThrowExceptionForCheckValidBinding(int result);

    // Methods defined to facilitate the templated bindings.
    int CheckStatementIsPreparedAndParameterNumber(int parameter_number) const;
    int CheckStatementIsPreparedAndParameterNumber(cs::string_sz parameter_name) const;

    // Binds a blob.
    void BindBlobWorker(int parameter_number, const void* value, size_t value_size, bool value_should_be_copied);

private:
    std::shared_ptr<sqlite3_stmt*> m_statementPtr;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Sqlite::Statement::Statement()
{
    ASSERT(!IsPrepared());
}


inline Sqlite::Statement::Statement(std::shared_ptr<sqlite3_stmt*> statement_ptr)
    :   m_statementPtr(std::move(statement_ptr))
{
    ASSERT(IsPrepared());
}


inline bool Sqlite::Statement::IsPrepared() const
{
    return ( m_statementPtr != nullptr && *m_statementPtr != nullptr );
}


inline void Sqlite::Statement::CheckStatementIsPrepared() const
{
    if( !IsPrepared() )
        ThrowExceptionForCheckStatementIsPrepared();
}


inline void Sqlite::Statement::CheckValidBinding(const int result)
{
    if( result != Result::OK )
        ThrowExceptionForCheckValidBinding(result);
}


inline int Sqlite::Statement::CheckStatementIsPreparedAndParameterNumber(int parameter_number) const
{
    ASSERT(parameter_number >= 1);

    CheckStatementIsPrepared();

    return parameter_number;
}


inline int Sqlite::Statement::CheckStatementIsPreparedAndParameterNumber(const cs::string_sz parameter_name) const
{
    return GetParameterNumber(parameter_name);
}


template<typename PT, typename VT>
Sqlite::Statement& Sqlite::Statement::Bind(PT&& parameter_number_or_name, VT value)
{
    const int parameter_number = CheckStatementIsPreparedAndParameterNumber(std::forward<PT>(parameter_number_or_name));
    int result;

    if constexpr(std::is_same_v<VT, int>)
    {
        result = sqlite3_bind_int(*m_statementPtr, parameter_number, value);
    }

    else if constexpr(std::is_same_v<VT, uint32_t> ||
                      std::is_same_v<VT, int64_t> ||
                      std::is_same_v<VT, uint64_t> ||
                      std::is_same_v<VT, long>)
    {
        result = sqlite3_bind_int64(*m_statementPtr, parameter_number, static_cast<int64_t>(value));
    }

    else if constexpr(std::is_same_v<VT, double> ||
                      std::is_same_v<VT, float>)
    {
        result = sqlite3_bind_double(*m_statementPtr, parameter_number, value);
    }

    else if constexpr(std::is_same_v<VT, bool>)
    {
        result = sqlite3_bind_int(*m_statementPtr, parameter_number, value ? 1 : 0);
    }

    else if constexpr(std::is_same_v<VT, std::string_view>)
    {
        result = sqlite3_bind_text(*m_statementPtr, parameter_number, value.data(), value.length(),
                                   reinterpret_cast<void(*)(void*)>(Code_SQLITE_TRANSIENT));
    }

    else if constexpr(std::is_convertible_v<VT, const char*>)
    {
        result = sqlite3_bind_text(*m_statementPtr, parameter_number, value, -1,
                                   reinterpret_cast<void(*)(void*)>(Code_SQLITE_TRANSIENT));
    }

    else
    {
        static_assert_false();
    }

    CheckValidBinding(result);

    return *this;
}


template<typename PT>
Sqlite::Statement& Sqlite::Statement::Bind(PT&& parameter_number_or_name, const std::string& value)
{
    return Bind(std::forward<PT>(parameter_number_or_name), std::string_view(value));
}


template<typename PT>
Sqlite::Statement& Sqlite::Statement::BindForLike(PT&& parameter_number_or_name, std::string value, const char escape/* = '!'*/)
{
    return Bind(std::forward<PT>(parameter_number_or_name), "%" + EscapeForLike(std::move(value), escape) + "%");
}


template<typename PT>
Sqlite::Statement& Sqlite::Statement::BindBlob(PT&& parameter_number_or_name, const void* const value, const size_t value_size,
                                               const bool value_should_be_copied/* = false*/)
{
    const int parameter_number = CheckStatementIsPreparedAndParameterNumber(std::forward<PT>(parameter_number_or_name));

    BindBlobWorker(parameter_number, value, value_size, value_should_be_copied);

    return *this;
}


template<typename PT>
Sqlite::Statement& Sqlite::Statement::BindBlob(PT&& parameter_number_or_name, const std::vector<std::byte>& value,
                                               const bool value_should_be_copied/* = false*/)
{
    return BindBlob(std::forward<PT>(parameter_number_or_name), value.data(), value.size(), value_should_be_copied);
}


template<typename PT>
Sqlite::Statement& Sqlite::Statement::BindNull(PT&& parameter_number_or_name)
{
    const int parameter_number = CheckStatementIsPreparedAndParameterNumber(std::forward<PT>(parameter_number_or_name));

    const int result = sqlite3_bind_null(*m_statementPtr, parameter_number);

    CheckValidBinding(result);

    return *this;
}


inline int Sqlite::Statement::Step()
{
    CheckStatementIsPrepared();

    return sqlite3_step(*m_statementPtr);
}


inline void Sqlite::Statement::Reset()
{
    CheckStatementIsPrepared();

    sqlite3_reset(*m_statementPtr);
}


inline int Sqlite::Statement::GetColumnCount() const
{
    CheckStatementIsPrepared();

    return sqlite3_column_count(*m_statementPtr);
}


inline std::string Sqlite::Statement::GetColumnName(const int column_number) const
{
    CheckStatementIsPrepared();

    return sqlite3_column_name(*m_statementPtr, column_number);
}


inline int Sqlite::Statement::GetColumnType(const int column_number)
{
    CheckStatementIsPrepared();

    return sqlite3_column_type(*m_statementPtr, column_number);
}


inline bool Sqlite::Statement::IsColumnNull(const int column_number)
{
    return ( GetColumnType(column_number) == ColumnType::Null );
}


template <typename VT>
VT Sqlite::Statement::GetColumn(const int column_number)
{
    CheckStatementIsPrepared();

    if constexpr(std::is_same_v<VT, int>)
    {
        return sqlite3_column_int(*m_statementPtr, column_number);
    }

    else if constexpr(std::is_same_v<VT, uint32_t> ||
                      std::is_same_v<VT, int64_t> ||
                      std::is_same_v<VT, uint64_t> ||
                      std::is_same_v<VT, long>)
    {
        return static_cast<VT>(sqlite3_column_int64(*m_statementPtr, column_number));
    }

    else if constexpr(std::is_same_v<VT, double> ||
                      std::is_same_v<VT, float>)
    {
        return static_cast<VT>(sqlite3_column_double(*m_statementPtr, column_number));
    }

    else if constexpr(std::is_same_v<VT, bool>)
    {
        return ( sqlite3_column_int(*m_statementPtr, column_number) != 0 );
    }

    else if constexpr(std::is_same_v<VT, const char*>)
    {
        return reinterpret_cast<const char*>(sqlite3_column_text(*m_statementPtr, column_number));
    }

    else if constexpr(std::is_same_v<VT, std::string>)
    {
        const char* const text = reinterpret_cast<const char*>(sqlite3_column_text(*m_statementPtr, column_number));
        return ( text != nullptr ) ? std::string(text) :
                                     std::string();
    }

    else if constexpr(std::is_same_v<VT, std::vector<std::byte>>)
    {
        const int value_size = sqlite3_column_bytes(*m_statementPtr, column_number);
        const std::byte* const value = reinterpret_cast<const std::byte*>(sqlite3_column_blob(*m_statementPtr, column_number));
        return std::vector<std::byte>(value, value + value_size);
    }

    else if constexpr(std::is_same_v<VT, BinaryBlock>)
    {
        BinaryBlock binary_block(sqlite3_column_bytes(*m_statementPtr, column_number));
        memcpy(binary_block.data(), sqlite3_column_blob(*m_statementPtr, column_number), binary_block.size());
        return binary_block;
    }

    else
    {
        static_assert_false();
    }
}
