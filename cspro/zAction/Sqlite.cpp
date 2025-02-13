#include "stdafx.h"
#include <SQLite/Encryption.h>
#include <SQLite/SQLite.h>
#include <SQLite/SQLiteStatement.h>
#include <zToolsO/VectorHelpers.h>
#include <zUtilO/SqlLogicFunctions.h>
#include <zParadataO/Logger.h>
#include <zDataO/EncryptedSQLiteRepository.h>


CREATE_JSON_KEY(bindings)
CREATE_JSON_KEY(openFlags)
CREATE_JSON_KEY(encryptionKey)
CREATE_JSON_KEY(encryptionKeyFormat)
CREATE_JSON_KEY(rowFormat)


// --------------------------------------------------------------------------
// SqliteDbWrapper
// --------------------------------------------------------------------------

class ActionInvoker::Runtime::SqliteDbWrapper
{
public:
    struct SqliteDb { sqlite3* db; bool callback_functions_registered; };
    struct DictionaryDb { const std::wstring name; };
    struct ParadataDb { };

    using WrapperType = std::variant<SqliteDb, DictionaryDb, ParadataDb>;

    SqliteDbWrapper(ActionInvoker::Runtime& runtime, WrapperType db_wrapper);
    ~SqliteDbWrapper();

    void Close();

    sqlite3* GetDb(bool for_querying);

    bool WrapsDictionaryOrParadata() const { return !std::holds_alternative<SqliteDb>(m_dbWrapper); }

    static auto GetSqliteDbWrapper(const std::map<int, std::shared_ptr<SqliteDbWrapper>>& sqlite_db_wrappers, int db_id);

    struct EncryptionKey
    {
        std::unique_ptr<std::byte[]> key;
        int key_size;
    };

    static EncryptionKey GetEncryptionKey(const JsonNode<wchar_t>& json_node, bool encryption_key_must_be_specified);

    // for Sqlite.exec
    static void exec_PrepareSql(sqlite3* db, const std::string& sql_statements_text,
                                std::vector<SQLiteStatement>& sql_statements, std::optional<size_t>& sql_statement_with_bindings_index);

    static void exec_BindValues(SQLiteStatement& sql_statement, const JsonNode<wchar_t>& bindings_node,
                                std::unique_ptr<std::vector<std::unique_ptr<std::vector<std::byte>>>>& bound_blobs);

    struct NullValue { };
    using DbValue = std::variant<NullValue, int64_t, double, std::wstring, std::shared_ptr<const std::vector<std::byte>>>;

    static DbValue exec_GetValue(SQLiteStatement& sql_statement, int column_num);

    static void exec_WriteValue(JsonWriter& json_writer, BytesToStringConverter& bytes_to_string_converter, const DbValue& value);
    static void exec_WriteValues(JsonWriter& json_writer, BytesToStringConverter& bytes_to_string_converter,
                                 const std::variant<std::vector<std::wstring>, bool>& column_names_or_write_into_array,
                                 const DbValue* values, size_t column_count);

private:
    sqlite3* GetDb(SqliteDb& db, bool for_querying);
    sqlite3* GetDb(const DictionaryDb& dictionary_db, bool for_querying);
    sqlite3* GetDb(const ParadataDb& paradata_db, bool for_querying);

    void RegisterSqlCallbackFunctions(sqlite3* db);

private:
    ActionInvoker::Runtime& m_runtime;
    WrapperType m_dbWrapper;
};


ActionInvoker::Runtime::SqliteDbWrapper::SqliteDbWrapper(ActionInvoker::Runtime& runtime, WrapperType db_wrapper)
    :   m_runtime(runtime),
        m_dbWrapper(std::move(db_wrapper))
{
    ASSERT(!std::holds_alternative<SqliteDb>(m_dbWrapper) || std::get<SqliteDb>(m_dbWrapper).db != nullptr);
}


ActionInvoker::Runtime::SqliteDbWrapper::~SqliteDbWrapper()
{
    try
    {
        Close();
    }
    catch(...) { }
}


void ActionInvoker::Runtime::SqliteDbWrapper::Close()
{
    if( std::holds_alternative<SqliteDb>(m_dbWrapper) )
    {
        sqlite3*& db = std::get<SqliteDb>(m_dbWrapper).db;

        if( db != nullptr && sqlite3_close(db) != SQLITE_OK )
            throw CSProException("Error closing SQLite database: " + std::string(sqlite3_errmsg(db)));

        db = nullptr;
    }
}


sqlite3* ActionInvoker::Runtime::SqliteDbWrapper::GetDb(const bool for_querying)
{
    return std::visit([&](auto& db_wrapper) { return SqliteDbWrapper::GetDb(db_wrapper, for_querying); },
                          m_dbWrapper);
}


sqlite3* ActionInvoker::Runtime::SqliteDbWrapper::GetDb(SqliteDb& db, const bool for_querying)
{
    ASSERT(db.db != nullptr);

    if( for_querying && !db.callback_functions_registered )
    {
        RegisterSqlCallbackFunctions(db.db);
        db.callback_functions_registered = true;
    }

    return db.db;
}


sqlite3* ActionInvoker::Runtime::SqliteDbWrapper::GetDb(const DictionaryDb& dictionary_db, const bool for_querying)
{
    sqlite3& db = m_runtime.GetInterpreterAccessor().GetSqliteDbForDictionary(dictionary_db.name);

    if( for_querying )
        RegisterSqlCallbackFunctions(&db);

    return &db;
}


sqlite3* ActionInvoker::Runtime::SqliteDbWrapper::GetDb(const ParadataDb& /*paradata_db*/, const bool for_querying)
{
    sqlite3* db = Paradata::Logger::GetSqlite();

    if( db == nullptr )
        throw CSProException("No paradata log is open.");

    if( for_querying )
    {
        if( !Paradata::Logger::Flush() )
            throw CSProException("The paradata log could not be flushed.");

        RegisterSqlCallbackFunctions(db);
    }

    return db;
}


void ActionInvoker::Runtime::SqliteDbWrapper::RegisterSqlCallbackFunctions(sqlite3* const db)
{
    // register the SQL callback functions, using the interpreter to get the user-specified logic functions, when possible
    InterpreterAccessor* interpreter_accessor;

    try
    {
        interpreter_accessor = &m_runtime.GetInterpreterAccessor();
    }

    catch(...)
    {
        interpreter_accessor = nullptr;
    }

    if( interpreter_accessor != nullptr )
    {
        interpreter_accessor->RegisterSqlCallbackFunctions(db);
    }

    else
    {
        SqlLogicFunctions::RegisterCallbackFunctions(db);
    }
}


auto ActionInvoker::Runtime::SqliteDbWrapper::GetSqliteDbWrapper(const std::map<int, std::shared_ptr<SqliteDbWrapper>>& sqlite_db_wrappers, const int db_id)
{
    const auto& lookup = sqlite_db_wrappers.find(db_id);

    if( lookup == sqlite_db_wrappers.cend() )
        throw CSProException(_T("No SQLite database is associated with the ID '%d'."), db_id);

    return lookup;
}


ActionInvoker::Runtime::SqliteDbWrapper::EncryptionKey ActionInvoker::Runtime::SqliteDbWrapper::GetEncryptionKey(const JsonNode<wchar_t>& json_node, const bool encryption_key_must_be_specified)
{
    EncryptionKey encryption_key { nullptr, 0 };

    if( encryption_key_must_be_specified || json_node.Contains(JK::encryptionKey) )
    {
        const wstring_view encryption_key_sv = json_node.Get<wstring_view>(JK::encryptionKey);

        if( !encryption_key_sv.empty() )
        {
            const std::vector<std::byte> key_bytes = StringToBytesConverter::Convert(encryption_key_sv, json_node, JK::encryptionKeyFormat);

            // prefix the key with the encryption type
            encryption_key.key_size = EncryptedSQLiteRepository::EncryptionType_sv.size() + key_bytes.size();
            encryption_key.key = std::make_unique<std::byte[]>(encryption_key.key_size);

            memcpy(encryption_key.key.get(), EncryptedSQLiteRepository::EncryptionType_sv.data(), EncryptedSQLiteRepository::EncryptionType_sv.size());
            memcpy(encryption_key.key.get() + EncryptedSQLiteRepository::EncryptionType_sv.size(), key_bytes.data(), key_bytes.size());
        }
    }

    return encryption_key;
}


void ActionInvoker::Runtime::SqliteDbWrapper::exec_PrepareSql(sqlite3* db, const std::string& sql_statements_text,
                                                              std::vector<SQLiteStatement>& sql_statements, std::optional<size_t>& sql_statement_with_bindings_index)
{
    const char* sql = sql_statements_text.c_str();
    const char* sql_end = sql + sql_statements_text.length();

    // prevent preparing blank statements, which result in SQLITE_MISUSE results
    while( std::find_if(sql, sql_end, [](char ch) { return !std::isspace(ch); }) != sql_end )
    {
        sqlite3_stmt* stmt;

        // because multiple statements can be included in a single string, we will keep preparing them until they are all prepared
        const char* next_sql_statement;

        if( sqlite3_prepare_v2(db, sql, -1, &stmt, &next_sql_statement) != SQLITE_OK )
            throw CSProException("Error preparing SQL statement (" + std::string(sqlite3_errmsg(db)) + "): " + sql);

        SQLiteStatement sql_statement(stmt, true);

        if( sql_statement.GetBindingsCount() > 0 )
        {
            if( sql_statement_with_bindings_index.has_value() )
                throw CSProException("You cannot execute multiple statements that have parameters.");

            sql_statement_with_bindings_index = sql_statements.size();
        }

        sql_statements.emplace_back(std::move(sql_statement));

        sql = next_sql_statement;
    }
}


void ActionInvoker::Runtime::SqliteDbWrapper::exec_BindValues(SQLiteStatement& sql_statement, const JsonNode<wchar_t>& bindings_node,
                                                              std::unique_ptr<std::vector<std::unique_ptr<std::vector<std::byte>>>>& bound_blobs)
{
    auto bind = [&](const int parameter_number, const JsonNode<wchar_t>& binding_node)
    {
        // string
        if( binding_node.IsString() )
        {
            sql_statement.Bind(parameter_number, binding_node.Get<std::string>());
        }

        // double
        else if( binding_node.IsDouble() )
        {
            sql_statement.Bind(parameter_number, binding_node.GetDouble());
        }

        // non-double number, boolean
        else if( binding_node.IsNumber() || binding_node.IsBoolean() )
        {
            sql_statement.Bind(parameter_number, binding_node.Get<int64_t>());
        }

        // blob
        else if( binding_node.IsObject() && binding_node.Contains(JK::bytes) )
        {
            const wstring_view bytes_sv = binding_node.Get<wstring_view>(JK::bytes);
            std::unique_ptr<std::vector<std::byte>> bytes = std::make_unique<std::vector<std::byte>>(StringToBytesConverter::Convert(bytes_sv, binding_node, JK::bytesFormat));
            sql_statement.Bind(parameter_number, *bytes);

            // SQLiteStatement::Bind binds using SQLITE_STATIC, so we need to maintain the bytes in memory until the statement is executed
            if( bound_blobs == nullptr )
                bound_blobs = std::make_unique<std::vector<std::unique_ptr<std::vector<std::byte>>>>();

            bound_blobs->emplace_back(std::move(bytes));
        }

        // unknown binding
        else
        {
            throw CSProException(_T("Unknown SQLite binding: ") + binding_node.GetNodeAsString());
        }
    };

    // if an array, bind by position
    if( bindings_node.IsArray() )
    {
        int binding_position = 0;

        for( const JsonNode<wchar_t>& binding_array_node : bindings_node.GetArray() )
            bind(++binding_position, binding_array_node);
    }

    // if an object, bind by name
    else
    {
        bindings_node.ForeachNode(
            [&](const std::wstring_view key_sv, const JsonNode<wchar_t>& binding_array_node)
            {
                const int parameter_number = sql_statement.GetParameterNumber(UTF8Convert::WideToUTF8(key_sv).c_str());

                if( parameter_number < 1 )
                    throw CSProException(_T("The SQLite binding '%s' is not associated with any parameter."), std::wstring(key_sv).c_str());

                bind(parameter_number, binding_array_node);
            });
    }
}


ActionInvoker::Runtime::SqliteDbWrapper::DbValue ActionInvoker::Runtime::SqliteDbWrapper::exec_GetValue(SQLiteStatement& sql_statement, int column_num)
{
    switch( sql_statement.GetColumnType(column_num) )
    {
        case SQLITE_NULL:    return NullValue { };
        case SQLITE_INTEGER: return sql_statement.GetColumn<int64_t>(column_num);
        case SQLITE_FLOAT:   return sql_statement.GetColumn<double>(column_num);
        case SQLITE_TEXT:    return sql_statement.GetColumn<std::wstring>(column_num);
        case SQLITE_BLOB:    return std::make_unique<std::vector<std::byte>>(sql_statement.GetColumn<std::vector<std::byte>>(column_num));
        default:             throw ProgrammingErrorException();
    }
}


void ActionInvoker::Runtime::SqliteDbWrapper::exec_WriteValue(JsonWriter& json_writer, BytesToStringConverter& bytes_to_string_converter, const DbValue& value)
{
    if( std::holds_alternative<NullValue>(value) )
    {
        json_writer.WriteNull();
    }

    else if( std::holds_alternative<int64_t>(value) )
    {
        json_writer.Write(std::get<int64_t>(value));
    }

    else if( std::holds_alternative<double>(value) )
    {
        json_writer.Write(std::get<double>(value));
    }

    else if( std::holds_alternative<std::wstring>(value) )
    {
        json_writer.Write(std::get<std::wstring>(value));
    }

    else
    {
        ASSERT(std::holds_alternative<std::shared_ptr<const std::vector<std::byte>>>(value));

        json_writer.Write(bytes_to_string_converter.Convert(std::get<std::shared_ptr<const std::vector<std::byte>>>(value),
                                                            std::nullopt));
    }
}


void ActionInvoker::Runtime::SqliteDbWrapper::exec_WriteValues(JsonWriter& json_writer, BytesToStringConverter& bytes_to_string_converter,
                                                               const std::variant<std::vector<std::wstring>, bool>& column_names_or_write_into_array,
                                                               const DbValue* values, const size_t column_count)
{
    ASSERT(std::holds_alternative<bool>(column_names_or_write_into_array) ||
           std::get<std::vector<std::wstring>>(column_names_or_write_into_array).size() == column_count);
    ASSERT(values != nullptr);

    const std::wstring* column_name_itr;

    // column_names will be specified when writing as an object
    if( std::holds_alternative<std::vector<std::wstring>>(column_names_or_write_into_array) )
    {
        column_name_itr = std::get<std::vector<std::wstring>>(column_names_or_write_into_array).data();
        json_writer.BeginObject();
    }

    else
    {
        column_name_itr = nullptr;

        if( std::get<bool>(column_names_or_write_into_array) )
            json_writer.BeginArray();
    }

    for( const DbValue* const values_end = values + column_count; values != values_end; ++values )
    {
        if( column_name_itr != nullptr )
        {
            json_writer.Key(*column_name_itr);
            ++column_name_itr;
        }

        exec_WriteValue(json_writer, bytes_to_string_converter, *values);
    }

    if( std::holds_alternative<std::vector<std::wstring>>(column_names_or_write_into_array) )
    {
        json_writer.EndObject();
    }

    else
    {
        if( std::get<bool>(column_names_or_write_into_array) )
            json_writer.EndArray();
    }
}



// --------------------------------------------------------------------------
// Sqlite actions
// --------------------------------------------------------------------------

ActionInvoker::Result ActionInvoker::Runtime::Sqlite_open(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const TCHAR* const input_type = GetUniqueKeyFromChoices(json_node, JK::path, JK::name);
    std::unique_ptr<SqliteDbWrapper> db_wrapper;

    // path
    if( input_type == JK::path )
    {
        const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));

        // default to read-only
        const size_t open_flags_index = json_node.Contains(JK::openFlags) ?
            json_node.GetFromStringOptions(JK::openFlags, std::initializer_list<const TCHAR*>({_T("read"), _T("readWrite"), _T("readWriteCreate") })) :
            0;

        const int open_flags = ( open_flags_index ) == 0 ? ( SQLITE_OPEN_READONLY ) :
                               ( open_flags_index ) == 1 ? ( SQLITE_OPEN_READWRITE ) :
                                                           ( SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE );

        // by default, a created SQLite file is 0 bytes, so we will set a pragma to make sure that a proper
        // database is created, which matters mostly when using an encryption key
        const bool set_user_pragmas = ( open_flags_index == 2 &&
                                        !PortableFunctions::FileIsRegular(path) );

        const SqliteDbWrapper::EncryptionKey encryption_key = SqliteDbWrapper::GetEncryptionKey(json_node, false);
        sqlite3* db;

        if( sqlite3_open_v2(UTF8Convert::WideToUTF8(path).c_str(), &db, open_flags, nullptr) != SQLITE_OK )
        {
            throw PortableFunctions::FileIsRegular(path) ? CSProException(_T("The SQLite database could not be opened: ") + path) :
                                                           CSProException(_T("The SQLite database does not exist: ") + path);
        }

        // now wrapped, the database will be closed if any exceptions are thrown below
        db_wrapper = std::make_unique<SqliteDbWrapper>(*this, SqliteDbWrapper::SqliteDb { db, false });

        auto throw_exception = [&]()
        {
            const TCHAR* const message_prefix = ( encryption_key.key == nullptr ) ? _T("The file is not a valid SQLite database: ") :
                                                                                    _T("The file is not a valid SQLite database or the encryption key is invalid: ");
            throw CSProException(message_prefix + path);
        };

        // key the database regardless of whether using an encryption key;
        // it is important for non-encrypted databases in case they are eventually rekeyed: https://www.sqlite.org/see/doc/trunk/www/readme.wiki
        if( SqliteEncryption::sqlite3_key(db, encryption_key.key.get(), encryption_key.key_size) != SQLITE_OK )
            throw_exception();

        // make sure the database is valid
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, nullptr);
        const int key_check_result = sqlite3_step(stmt);
        const int user_version = set_user_pragmas ? sqlite3_column_int(stmt, 0) : 0;
        sqlite3_finalize(stmt);

        if( key_check_result == SQLITE_NOTADB )
            throw_exception();

        // when creating a new database, prevent a 0-byte file
        if( set_user_pragmas )
        {
            const std::wstring sql = FormatTextCS2WS(_T("PRAGMA user_version = %d;"), user_version);

            if( sqlite3_exec(db, UTF8Convert::WideToUTF8(sql).c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
                throw_exception();
        }
    }

    // name
    else
    {
        ASSERT(input_type == JK::name);

        const wstring_view name_sv = json_node.Get<wstring_view>(JK::name);

        // create a wrapper around paradata, or a dictionary name
        db_wrapper = ( name_sv == _T("paradata") ) ? std::make_unique<SqliteDbWrapper>(*this, SqliteDbWrapper::ParadataDb { }) :
                                                     std::make_unique<SqliteDbWrapper>(*this, SqliteDbWrapper::DictionaryDb { name_sv });

        // get the database, which will throw an exception if the input is not valid
        db_wrapper->GetDb(false);
    }

    ASSERT(db_wrapper != nullptr);

    // add the database using a random ID that is not already in use
    int db_id;

    do
    {
        db_id = rand();

    } while( m_sqliteDbWrappers.find(db_id) != m_sqliteDbWrappers.cend() );

    m_sqliteDbWrappers.try_emplace(db_id, std::move(db_wrapper));

    return Result::Number(db_id);
}


ActionInvoker::Result ActionInvoker::Runtime::Sqlite_close(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    const auto& db_wrapper_lookup = SqliteDbWrapper::GetSqliteDbWrapper(m_sqliteDbWrappers, json_node.Get<int>(JK::db));
    SqliteDbWrapper& db_wrapper = *db_wrapper_lookup->second;

    db_wrapper.Close();

    m_sqliteDbWrappers.erase(db_wrapper_lookup);

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::Sqlite_rekey(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    SqliteDbWrapper& db_wrapper = *SqliteDbWrapper::GetSqliteDbWrapper(m_sqliteDbWrappers, json_node.Get<int>(JK::db))->second;
    const SqliteDbWrapper::EncryptionKey encryption_key = SqliteDbWrapper::GetEncryptionKey(json_node, true);

    if( db_wrapper.WrapsDictionaryOrParadata() )
        throw CSProException("You cannot rekey a dictionary or paradata log.");

    sqlite3* db = db_wrapper.GetDb(false);

    // if the encryption key is null, the database will be decrypted
    if( SqliteEncryption::sqlite3_rekey(db, encryption_key.key.get(), encryption_key.key_size ) != SQLITE_OK )
        throw CSProException("There was an error rekeying the database: " + std::string(sqlite3_errmsg(db)));

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::Sqlite_exec(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    SqliteDbWrapper& db_wrapper = *SqliteDbWrapper::GetSqliteDbWrapper(m_sqliteDbWrappers, json_node.Get<int>(JK::db))->second;
    sqlite3* db = db_wrapper.GetDb(true);

    // prepare the statements
    std::vector<SQLiteStatement> sql_statements;
    std::optional<size_t> sql_statement_with_bindings_index;

    const JsonNode<wchar_t>& sql_node = json_node.Get(JK::sql);

    // allow the specification of multiple SQL statements using an array
    if( sql_node.IsArray() )
    {
        for( const JsonNode<wchar_t>& sql_array_node : sql_node.GetArray() )
            SqliteDbWrapper::exec_PrepareSql(db, sql_array_node.Get<std::string>(), sql_statements, sql_statement_with_bindings_index);
    }

    else
    {
        SqliteDbWrapper::exec_PrepareSql(db, sql_node.Get<std::string>(), sql_statements, sql_statement_with_bindings_index);
    }

    if( sql_statements.empty() )
        return Result::Undefined();

    // determine how rows, and blobs, should be returned
    enum class RowFormatType { Object, Array, ScalarArray };
    std::optional<RowFormatType> row_format_type = json_node.Contains(JK::rowFormat) ? std::make_optional(static_cast<RowFormatType>(json_node.GetFromStringOptions(JK::rowFormat, std::initializer_list<const TCHAR*>({ _T("object"), _T("array"), _T("scalarArray") })))) :
                                                                                       std::nullopt;
    BytesToStringConverter bytes_to_string_converter(this, json_node, JK::bytesFormat);

    // handle bindings
    std::unique_ptr<std::vector<std::unique_ptr<std::vector<std::byte>>>> bound_blobs;

    if( sql_statement_with_bindings_index.has_value() && json_node.Contains(JK::bindings) )
    {
        SQLiteStatement& sql_statement = sql_statements[*sql_statement_with_bindings_index];
        SqliteDbWrapper::exec_BindValues(sql_statement, json_node.Get(JK::bindings), bound_blobs);
    }

    // execute all the queries
    auto throw_execution_exception = [&]()
    {
        throw CSProException("The SQLite execution led to an error: " + std::string(sqlite3_errmsg(db)));
    };

    int result = SQLITE_OK;

    for( SQLiteStatement& sql_statement : sql_statements )
    {
        result = sql_statement.Step();

        if( result != SQLITE_OK && result != SQLITE_ROW && result != SQLITE_DONE )
            throw_execution_exception();
    }

    // result has the result code of the last query, so either:

    // 1) return no results
    if( result != SQLITE_ROW )
        return Result::Undefined();

    SQLiteStatement& final_sql_statement = sql_statements.back();
    const size_t column_count = final_sql_statement.GetColumnCount();
    ASSERT(column_count > 0);

    // 2) return a scalar
    std::optional<SqliteDbWrapper::DbValue> first_cell;

    if( !row_format_type.has_value() && column_count == 1 )
    {
        first_cell.emplace(SqliteDbWrapper::exec_GetValue(final_sql_statement, 0));

        result = final_sql_statement.Step();

        if( result == SQLITE_DONE )
        {
            if( std::holds_alternative<int64_t>(*first_cell) )
            {
                return Result::Number(std::get<int64_t>(*first_cell));
            }

            else if( std::holds_alternative<double>(*first_cell) )
            {
                return Result::Number(std::get<double>(*first_cell));
            }

            else if( std::holds_alternative<std::wstring>(*first_cell) )
            {
                return Result::String(std::get<std::wstring>(*first_cell));
            }

            else
            {
                auto json_writer = Json::CreateStringWriter();

                SqliteDbWrapper::exec_WriteValue(*json_writer, bytes_to_string_converter, *first_cell);

                return Result::JsonText(json_writer);
            }
        }
    }

    // 3) return rows
    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginArray();

    // if not specified, default to rows as an object
    if( !row_format_type.has_value() )
    {
        row_format_type = RowFormatType::Object;
    }

    // if more than one column is used, a scalar array cannot be returned so return an array
    else if( *row_format_type == RowFormatType::ScalarArray && column_count > 1 )
    {
        row_format_type = RowFormatType::Array;
    }

    // get the column names if returning rows as an object
    std::variant<std::vector<std::wstring>, bool> column_names_or_write_into_array =
        ( *row_format_type == RowFormatType::Object ) ? std::variant<std::vector<std::wstring>, bool>(std::vector<std::wstring>()) :
        ( *row_format_type == RowFormatType::Array )  ? std::variant<std::vector<std::wstring>, bool>(true) :
                                                        std::variant<std::vector<std::wstring>, bool>(false);

    if( *row_format_type == RowFormatType::Object )
    {
        for( size_t i = 0; i < column_count; ++i )
            std::get<std::vector<std::wstring>>(column_names_or_write_into_array).emplace_back(final_sql_statement.GetColumnName(i));
    }

    // if the first cell was set above (to potentially be returned as a scalar), we need to write this row
    if( first_cell.has_value() )
    {
        ASSERT(column_count == 1);
        SqliteDbWrapper::exec_WriteValues(*json_writer, bytes_to_string_converter, column_names_or_write_into_array, &(*first_cell), 1);
    }

    // write subsequent rows
    auto cells = std::make_unique_for_overwrite<SqliteDbWrapper::DbValue[]>(column_count);

    for( ; result == SQLITE_ROW; result = final_sql_statement.Step() )
    {
        for( size_t i = 0; i < column_count; ++i )
            cells[i] = SqliteDbWrapper::exec_GetValue(final_sql_statement, i);

        SqliteDbWrapper::exec_WriteValues(*json_writer, bytes_to_string_converter, column_names_or_write_into_array, cells.get(), column_count);        
    }

    if( result != SQLITE_DONE )
        throw_execution_exception();

    json_writer->EndArray();

    return Result::JsonText(json_writer);
}
