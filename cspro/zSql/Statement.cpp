#include "stdafx.h"
#include "Statement.h"


void Sqlite::Statement::CheckDefinitionsAtCompileTime()
{
    static_assert(sizeof(int64_t) == sizeof(sqlite3_int64));

    static_assert(Result::OK == SQLITE_OK);
    static_assert(Result::Row == SQLITE_ROW);
    static_assert(Result::Done == SQLITE_DONE);

    static_assert(ColumnType::Integer == SQLITE_INTEGER);
    static_assert(ColumnType::Float == SQLITE_FLOAT);
    static_assert(ColumnType::Text == SQLITE_TEXT);
    static_assert(ColumnType::Blob == SQLITE_BLOB);
    static_assert(ColumnType::Null == SQLITE_NULL);

#ifdef WIN32
    static_assert(Code_SQLITE_TRANSIENT == reinterpret_cast<INT_PTR>(SQLITE_TRANSIENT));
#endif
}


Sqlite::Statement::~Statement() noexcept
{
    Finalize();
}


Sqlite::Statement Sqlite::Statement::Prepare(sqlite3* const db, const std::string_view sql_sv,
                                             const char** end_of_parsed_statement/* = nullptr*/)
{
    ASSERT(db != nullptr);

    auto statement_ptr = std::make_unique<sqlite3_stmt*>();

    if( sqlite3_prepare_v2(db, sql_sv.data(), int32_cast(sql_sv.length()), statement_ptr.get(), end_of_parsed_statement) != SQLITE_OK )
        throw Exception(db, SO::Empty_string, "Error creating SQLite statement: %s", std::string(sql_sv).c_str());

    return Statement(std::move(statement_ptr));
}


void Sqlite::Statement::Finalize() noexcept
{
    if( IsPrepared() )
    {
        sqlite3_finalize(*m_statementPtr);
        *m_statementPtr = nullptr;
    }
}


void Sqlite::Statement::ThrowExceptionForCheckStatementIsPrepared()
{
    throw Exception("The SQLite statement is not prepared.");
}


void Sqlite::Statement::ThrowExceptionForCheckValidBinding(const int result)
{
    ASSERT(result != SQLITE_OK);

    throw Exception("The SQLite statement binding failed: %s", sqlite3_errstr(result));
}


int Sqlite::Statement::GetBindingsCount() const
{
    CheckStatementIsPrepared();

    return sqlite3_bind_parameter_count(*m_statementPtr);
}


int Sqlite::Statement::GetParameterNumber(const cs::string_sz parameter_name) const
{
    ASSERT(!parameter_name.empty() && parameter_name.front() == '@');

    CheckStatementIsPrepared();

    const int parameter_number = sqlite3_bind_parameter_index(*m_statementPtr, parameter_name.c_str());

    if( parameter_number < 1 )
        throw Exception("The SQLite binding '%s' is not associated with any parameter.", parameter_name.c_str());

    return parameter_number;
}


std::string Sqlite::Statement::EscapeForLike(std::string value, const char escape/* = '!'*/)
{
    char escape_sv_mocked[] = { escape, escape };

    auto escape_chars = [&]()
    {
        SO::Replace(value, std::string_view(escape_sv_mocked + 1, 1),
                           std::string_view(escape_sv_mocked, 2) );
    };

    escape_chars(); // ! -> !!

    escape_sv_mocked[1] = '%';
    escape_chars(); // % -> !%

    escape_sv_mocked[1] = '_';
    escape_chars(); // _ -> !_

    return value;
}


void Sqlite::Statement::BindBlobWorker(const int parameter_number, const void* const value, const size_t value_size, const bool value_should_be_copied)
{
    ASSERT(IsPrepared());

    const int result = sqlite3_bind_blob64(*m_statementPtr, parameter_number, value, static_cast<sqlite3_uint64>(value_size),
                                           value_should_be_copied ? SQLITE_TRANSIENT : SQLITE_STATIC);
    CheckValidBinding(result);
}
