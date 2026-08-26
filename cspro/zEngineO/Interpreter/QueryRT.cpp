#include "stdafx.h"
#include "IncludesRT.h"
#include "Array.h"
#include "EngineDictionary.h"
#include "List.h"
#include "UserFunctionArgumentEvaluator.h"
#include "Nodes/Query.h"
#include <engine/DicX.h>
#include <zLogicO/SymbolTableIterator.h>
#include <zToolsO/DirectoryLister.h>
#include <zSql/DB.h>
#include <zSql/SQLite.h>
#include <zSql/SQLiteHelpers.h>
#include <zUtilO/SqlLogicFunctions.h>
#include <zParadataO/Concatenator.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zDataO/EncryptedSQLiteRepository.h>


namespace QueryRT
{
    class EngineParadataConcatenator;
    class SqlQueryProcessor;
    class SqlQueryUserFunctionArgumentEvaluator;

    using InterpreterAndUserFunction = std::tuple<LogicInterpreter&, UserFunction&>;

    void SqlCallbackFunction(sqlite3_context* context, int iArgC, sqlite3_value** ppArgV);
}


// --------------------------------------------------------------------------
// Paradata: paradata concatenator
// --------------------------------------------------------------------------

class QueryRT::EngineParadataConcatenator : public Paradata::Concatenator
{
public:
    EngineParadataConcatenator(LogicInterpreter& interpreter);

    Engine::Value GetReturnValue() const noexcept;

protected:
    void OnInputProcessedSuccess(const std::variant<std::string, sqlite3*>& output_file_path_or_database, int64_t events_processed) override final;
    void OnInputProcessedError(const std::string& input_file_path, const char* error_message) override final;
    bool UserRequestsCancellation() override;

private:
    LogicInterpreter& m_interpreter;
    size_t m_logsConcatenated;
    bool m_processingErrors;
};


QueryRT::EngineParadataConcatenator::EngineParadataConcatenator(LogicInterpreter& interpreter)
    :   m_interpreter(interpreter),
        m_logsConcatenated(0),
        m_processingErrors(false)
{
}


Engine::Value QueryRT::EngineParadataConcatenator::GetReturnValue() const noexcept
{
    return m_processingErrors ? Engine::Value::Invalid<double>() :
                                Engine::Value::Integer(m_logsConcatenated);
}


void QueryRT::EngineParadataConcatenator::OnInputProcessedSuccess(const std::variant<std::string, sqlite3*>& /*output_file_path_or_database*/, int64_t /*events_processed*/)
{
    ++m_logsConcatenated;
}


void QueryRT::EngineParadataConcatenator::OnInputProcessedError(const std::string& input_file_path, const char* const error_message)
{
    m_interpreter.IssueMessage(MessageType::Error, MGF::Query_paradata_concat_error_8291,
                               FormatText(" in file %s", input_file_path.c_str()).c_str(),
                               error_message);
}


bool QueryRT::EngineParadataConcatenator::UserRequestsCancellation()
{
    return m_interpreter.m_bStopProc;
}



// --------------------------------------------------------------------------
// Paradata: paradata function
// --------------------------------------------------------------------------

Engine::Value LogicInterpreter::ex_paradata(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const int action = va_node.arguments[0];

    // --------------------------------------------------------------------------
    // open
    // --------------------------------------------------------------------------
    if( action == 1 )
    {
        std::string file_path = EvaluatePath(va_node.arguments[1]);

        Paradata::Logger::Stop();

        return Engine::Value::Bool(
            Paradata::Logger::Start(std::move(file_path), m_engineData->application)
        );
    }


    // --------------------------------------------------------------------------
    // close
    // --------------------------------------------------------------------------
    else if( action == 2 )
    {
        if( Paradata::Logger::IsOpen() && Paradata::Logger::Flush() )
        {
            Paradata::Logger::Stop();

            ClearParadataCachedObjects_INTERPRETER_DLL_TODO();

            if( !Paradata::Logger::IsOpen() )
                return Engine::Value::Bool(true);
        }

        return Engine::Value::Bool(false);
    }


    // --------------------------------------------------------------------------
    // flush
    // --------------------------------------------------------------------------
    else if( action == 3 )
    {
        return Engine::Value::Bool(
            ( Paradata::Logger::IsOpen() &&
              Paradata::Logger::Flush() )
        );
    }


    // --------------------------------------------------------------------------
    // concat
    // --------------------------------------------------------------------------
    else if( action == 4 )
    {
        if( Paradata::Logger::IsOpen() )
            Paradata::Logger::Flush();

        std::string output_file_path;
        bool output_file_path_is_currently_open_paradata_log = false;
        std::set<std::string> paradata_log_file_paths;
        const int number_arguments = va_node.arguments[1];
        bool output_is_also_an_input = false;

        for( int i = 0; i < number_arguments; i += 2 )
        {
            const bool argument_is_file_path = ( va_node.arguments[i + 2] == 0 );
            const int argument = va_node.arguments[i + 3];
            std::vector<std::string> file_paths;

            auto add_logs_with_wildcard_support = [&](const std::string& file_path)
            {
                // evaluate the path in case it uses wildcards
                DirectoryLister::AddFilePathsWithPossibleWildcard(file_paths, file_path, true);
            };

            if( argument_is_file_path )
            {
                std::string file_path = EvaluatePath(argument);

                if( i == 0 )
                {
                    output_file_path = std::move(file_path);
                    output_file_path_is_currently_open_paradata_log = SO::EqualsNoCase(Paradata::Logger::GetFilePath(), output_file_path);
                }

                else
                {
                    add_logs_with_wildcard_support(file_path);
                }
            }

            else // the argument is a list
            {
                ASSERT(i != 0);
                const LogicList& logic_list = GetSymbolLogicList(argument);
                const size_t list_count = logic_list.GetCount();

                for( size_t j = 1; j <= list_count; ++j )
                {
                    const std::string file_path = GetAbsolutePath(logic_list.GetValue<SharableString>(j).GetString());
                    add_logs_with_wildcard_support(file_path);
                }
            }

            if( i > 0 )
            {
                for( std::string& file_path : file_paths )
                {
                    if( SO::EqualsNoCase(file_path, output_file_path) )
                    {
                        output_is_also_an_input = true;

                        // change the file path so that the insert below is in the right case in the case
                        // that the file path is later removed (if concatenating to the current paradata log)
                        file_path = output_file_path;
                    }

                    paradata_log_file_paths.insert(std::move(file_path));
                }
            }
        }

        try
        {
            sqlite3* output_db_override = nullptr;

            // some checks if concatenating into the currently open paradata file
            if( output_file_path_is_currently_open_paradata_log )
            {
                if( !output_is_also_an_input )
                {
                    throw CSProException("You cannot concatenate into the currently open paradata log without "
                                         "also specifying that log as an input log.");
                }

                output_db_override = Paradata::Logger::GetSqlite();
            }

            QueryRT::EngineParadataConcatenator engine_paradata_concatenator(*this);

            if( output_db_override != nullptr )
            {
                paradata_log_file_paths.erase(output_file_path);
                engine_paradata_concatenator.Run(output_db_override, paradata_log_file_paths);
            }

            else
            {
                engine_paradata_concatenator.Run(output_file_path, paradata_log_file_paths);
            }

            return engine_paradata_concatenator.GetReturnValue();
        }

        catch( const CSProException& exception )
        {
            IssueMessage(MessageType::Error, MGF::Query_paradata_concat_error_8291, "", exception.what());
            return Engine::Value::Invalid<double>();
        }
    }


    return ReturnProgrammingError(Engine::Value::Invalid<double>());
}



// --------------------------------------------------------------------------
// SQLite: sqlquery function
// --------------------------------------------------------------------------

class QueryRT::SqlQueryProcessor
{
public:
    SqlQueryProcessor(LogicInterpreter& interpreter) noexcept;

    sqlite3* GetDb() noexcept { return m_db; }

    void UseParadataDb();
    void UseDataRepository(DataRepository& data_repository);
    void UseFile(const ConnectionString& connection_string);

    void ExecuteStatements(const std::string& sql_query);

    Engine::Value ProcessResult(Symbol* symbol);

private:
    template<typename T>
    T GetValue(int column_number);

    static Engine::Value ProcessNoRows(Symbol* symbol);

    // When not storing results in an object, the return value will
    // be the numeric result of the the first row / first column.
    Engine::Value ProcessRows();

    // When using an object, the results will be placed in the object and
    // the return value will be the number of results placed in the object.
    Engine::Value ProcessRows(LogicArray& logic_array);
    Engine::Value ProcessRows(LogicList& logic_list);
    Engine::Value ProcessRows(SECT* const pSecT);

private:
    LogicInterpreter& m_interpreter;
    sqlite3* m_db;
    std::unique_ptr<Sqlite::DB> m_openedDb;
    Sqlite::Statement m_stmt;
    bool m_queryHasRows;
};


QueryRT::SqlQueryProcessor::SqlQueryProcessor(LogicInterpreter& interpreter) noexcept
    :   m_interpreter(interpreter),
        m_db(nullptr),
        m_queryHasRows(false)
{
}


void QueryRT::SqlQueryProcessor::UseParadataDb()
{
    m_db = Paradata::Logger::GetSqlite();

    if( m_db == nullptr )
        throw CSProException("No paradata log is open.");

    Paradata::Logger::Flush();
}


void QueryRT::SqlQueryProcessor::UseDataRepository(DataRepository& data_repository)
{
    m_db = DataRepositoryHelpers::GetSqliteDatabase(data_repository);

    if( m_db == nullptr )
    {
        throw CSProException(
            ( data_repository.GetRepositoryType() == DataRepositoryType::Text )
            ? "You can only execute queries on Text data sources that use an index."
            : "You can only execute queries on CSPro DB, Text, or JSON data sources."
        );
    }
}


void QueryRT::SqlQueryProcessor::UseFile(const ConnectionString& connection_string)
{
    if( connection_string.HasFilePath() &&
        PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(connection_string.GetFilePath())) )
    {
        // if specifying an Encrypted CSPro DB file, open it so that a password can be processed
        if( connection_string.GetType() == DataRepositoryType::EncryptedSQLite )
        {
            const int result = EncryptedSQLiteRepository::OpenSQLiteDatabaseFile(nullptr, connection_string, &m_db, SQLITE_OPEN_READWRITE);

            if( result == Sqlite::Result::OK )
                m_openedDb = std::make_unique<Sqlite::DB>(Sqlite::DB::CreateWrapper(m_db, true));
        }

        else
        {
            m_openedDb = std::make_unique<Sqlite::DB>();

            m_openedDb->Open(connection_string.GetFilePath(),
                             Sqlite::OpenFlags::ReadWrite | Sqlite::OpenFlags::Create);

            m_db = m_openedDb->GetDb();
        }
    }

    if( m_openedDb == nullptr )
        throw CSProException("The SQLite file could not be opened: " + connection_string.ToDisplayString());
}


void QueryRT::SqlQueryProcessor::ExecuteStatements(const std::string& sql_query)
{
    const std::vector<std::string> sql_statements = SQLiteHelpers::SplitSqlStatement(sql_query);
    auto sql_statements_itr = sql_statements.cbegin();
    const auto sql_statements_end = sql_statements.cend();

    if( sql_statements_itr == sql_statements_end )
        throw CSProException("Empty SQL statement.");

    do
    {
        m_stmt = Sqlite::Statement::Prepare(m_db, *sql_statements_itr);

        const int result = m_stmt.Step();
        m_queryHasRows = ( result == Sqlite::Result::Row );

        if( !m_queryHasRows && result != Sqlite::Result::Done )
            throw CSProException("Error executing SQLite statement: %s", sqlite3_errmsg(m_db));

    } while( ++sql_statements_itr != sql_statements_end );
}


template<>
double QueryRT::SqlQueryProcessor::GetValue(const int column_number)
{
    ASSERT(m_queryHasRows);
    ASSERT(Engine::Value::Undefined<double>().get<double>() == NOTAPPL);

    return m_stmt.IsColumnNull(column_number)
        ? NOTAPPL
        : m_stmt.GetColumn<double>(column_number);
}


template<>
SharableString QueryRT::SqlQueryProcessor::GetValue(const int column_number)
{
    ASSERT(m_queryHasRows);
    ASSERT(Engine::Value::Undefined<SharableString>().get<SharableString>() == SharableString());

    return m_stmt.IsColumnNull(column_number)
        ? SharableString()
        : m_stmt.GetColumn<std::string>(column_number);
}


Engine::Value QueryRT::SqlQueryProcessor::ProcessResult(Symbol* const symbol)
{
    ASSERT(m_stmt.IsPrepared());

    return ( !m_queryHasRows )                  ? ProcessNoRows(symbol) :
           ( symbol == nullptr )                ? ProcessRows() :
           ( symbol->IsA(SymbolType::Array) )   ? ProcessRows(assert_cast<LogicArray&>(*symbol)) :
           ( symbol->IsA(SymbolType::List) )    ? ProcessRows(assert_cast<LogicList&>(*symbol)) :
           ( symbol->IsA(SymbolType::Section) ) ? ProcessRows(assert_cast<SECT*>(symbol)) :
                                                  throw ProgrammingErrorException();
}


Engine::Value QueryRT::SqlQueryProcessor::ProcessNoRows(Symbol* const symbol)
{
    // zero out the length of the object (if applicable)
    if( symbol != nullptr )
    {
        if( symbol->IsA(SymbolType::List) )
        {
            assert_cast<LogicList&>(*symbol).Reset();
        }

        else if( symbol->IsA(SymbolType::Section) )
        {
            assert_cast<SECT*>(symbol)->GetGroup(0)->SetTotalOccurrences(0);
        }
    }

    // the return value will signal that no rows were returned or that something
    // (like a CREATE TABLE) succeeded but that the return value is not applicable
    return Engine::Value::Integer(0);
}


Engine::Value QueryRT::SqlQueryProcessor::ProcessRows()
{
    return GetValue<double>(0);
}


Engine::Value QueryRT::SqlQueryProcessor::ProcessRows(LogicArray& logic_array)
{
    ASSERT(logic_array.GetNumberDimensions() <= 2);

    const int number_columns = m_stmt.GetColumnCount();
    ASSERT(number_columns > 0);

    // - 1 in the next two statements because the arrays will be filled in starting at index 1
    const size_t max_rows_to_read = logic_array.GetDimension(0) - 1;

    const size_t columns_to_read = ( logic_array.GetNumberDimensions() == 1 )
        ? 1
        : std::min(static_cast<size_t>(number_columns), logic_array.GetDimension(1) - 1);

    std::vector<size_t> indices(logic_array.GetNumberDimensions(), 0);

    do
    {
        ++indices[0];

        for( int column = 0; column < columns_to_read; ++column )
        {
            if( columns_to_read != 1 )
                indices[1] = column + 1;

            ASSERT(logic_array.IsValidIndex(indices));

            if( logic_array.IsNumeric() )
            {
                logic_array.SetValue(indices, GetValue<double>(column));
            }

            else
            {
                logic_array.SetValue(indices, GetValue<SharableString>(column));
            }
        }

    } while( indices[0] < max_rows_to_read && m_stmt.Step() == Sqlite::Result::Row );

    return Engine::Value::Integer(indices[0]);
}


Engine::Value QueryRT::SqlQueryProcessor::ProcessRows(LogicList& logic_list)
{
    constexpr size_t MaximumRowsToRead = 10000;

    logic_list.Reset();

    size_t row = 0;

    do
    {
        if( logic_list.IsNumeric() )
        {
            logic_list.AddValue(GetValue<double>(0));
        }

        else
        {
            logic_list.AddValue(GetValue<SharableString>(0));
        }

    } while( ++row < MaximumRowsToRead && m_stmt.Step() == Sqlite::Result::Row );

    return Engine::Value::Integer(row);
}


Engine::Value QueryRT::SqlQueryProcessor::ProcessRows(SECT* const pSecT)
{
    const int number_columns = m_stmt.GetColumnCount();
    ASSERT(number_columns > 0);

    // map the columns
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_interpreter.GetSymbolTable(); };

    std::map<int, VART*> item_mapping;

    for( int column = 0; column < number_columns; ++column )
    {
        const std::string column_name = m_stmt.GetColumnName(column);
        VART* pVarT;

        for( int iSymVar = pSecT->SYMTfvar; iSymVar >= 0; iSymVar = pVarT->SYMTfwd )
        {
            pVarT = VPT(iSymVar);

            // do not look at subitems or items that occur
            const CDictItem& dict_item = *pVarT->GetDictItem();

            if( ( dict_item.GetItemType() == ItemType::Subitem ) ||
                ( dict_item.GetOccurs() > 1 ) )
            {
                continue;
            }

            if( SO::EqualsNoCase(pVarT->GetName(), column_name) )
            {
                item_mapping.try_emplace(column, pVarT);
                break;
            }
        }
    }

    // fill the items
    int occurrence = 0;

    do
    {
        for( const auto& [column, pVarT] : item_mapping )
        {
            if( pVarT->IsNumeric() )
            {
                m_interpreter.AssignValueToVART_INTERPRETER_DLL_TODO(*pVarT, occurrence, GetValue<double>(column));
            }

            else
            {
                m_interpreter.AssignValueToVART_INTERPRETER_DLL_TODO(*pVarT, occurrence, GetValue<SharableString>(column));
            }
        }

    } while( ++occurrence < pSecT->GetMaxOccs() && m_stmt.Step() == Sqlite::Result::Row );

    pSecT->GetGroup(0)->SetTotalOccurrences(occurrence);

    return Engine::Value::Integer(occurrence);
}


Engine::Value LogicInterpreter::ex_sqlquery(const int program_index)
{
    return ex_sqlquery(program_index, nullptr);
}


Engine::Value LogicInterpreter::ex_sqlquery(const int program_index, const std::function<double(sqlite3*, const std::string&)>* const setreportdata_callback)
{
    const auto& sqlquery_node = GetNode<Nodes::SqlQuery>(program_index);
    const SharableString sql_query = Evaluate<SharableString>(sqlquery_node.sql_query_expression);

    QueryRT::SqlQueryProcessor processor(*this);

    try
    {
        switch( sqlquery_node.source_type )
        {
            // access paradata
            case Nodes::SqlQuery::Type::Paradata:
            {
                processor.UseParadataDb();
                break;
            }

            // access a data repository
            case Nodes::SqlQuery::Type::Dictionary:
            {
                Symbol& symbol = NPT_Ref(sqlquery_node.source_symbol_index_or_expression);
                DataRepository* data_repository;

                if( symbol.IsA(SymbolType::Dictionary) )
                {
                    EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
                    data_repository = &engine_dictionary.GetEngineDataRepository().GetDataRepository();
                }

                else
                {
                    DICX* const pDicX = assert_cast<DICT&>(symbol).GetDicX();
                    data_repository = &pDicX->GetDataRepository();
                }

                processor.UseDataRepository(*data_repository);
                break;
            }

            // open a SQLite file
            case Nodes::SqlQuery::Type::File:
            {
                const ConnectionString connection_string = EvaluateConnectionString(sqlquery_node.source_symbol_index_or_expression);
                processor.UseFile(connection_string);
                break;
            }

            default:
               throw ProgrammingErrorException();
        }

        ASSERT(processor.GetDb() != nullptr);

        // register any user-specified logic functions as SQL functions
        RegisterSqlCallbackFunctions(processor.GetDb());

        // if called from setreportdata, call back into the report system
        if( sqlquery_node.destination_symbol_index == Nodes::SqlQuery::SetReportDataDestinationJson )
        {
            ASSERT(setreportdata_callback != nullptr);
            return (*setreportdata_callback)(processor.GetDb(), sql_query.GetString());
        }

        // otherwise execute a standard sqlquery call
        else
        {
            Symbol* const symbol = ( sqlquery_node.destination_symbol_index >= 0 )
                ? &NPT_Ref(sqlquery_node.destination_symbol_index)
                : nullptr;

            processor.ExecuteStatements(*sql_query);

            return processor.ProcessResult(symbol);
        }
    }

    catch( const CSProException& exception )
    {
        const char* const file_path = ( processor.GetDb() != nullptr )
            ? sqlite3_db_filename(processor.GetDb(), nullptr)
            : nullptr;

        IssueMessage(MessageType::Error, MGF::Query_sqlquery_error_8292,
                     ( file_path != nullptr ) ? FormatText("(%s)", Path::GetFilename(file_path).c_str()).c_str() : "",
                     exception.what());

        return Engine::Value::Invalid<double>();
    }
}



// --------------------------------------------------------------------------
// SQLite: routines for calling back into user-defined functions from queries
// --------------------------------------------------------------------------

void QueryRT::SqlCallbackFunction(sqlite3_context* const context, int iArgC, sqlite3_value** const ppArgV)
{
    auto& [interpreter, user_function] = *static_cast<InterpreterAndUserFunction*>(sqlite3_user_data(context));

    interpreter.ProcessSqlCallbackFunction(user_function, static_cast<void*>(context), iArgC, static_cast<void*>(ppArgV));
}


void LogicInterpreter::RegisterSqlCallbackFunctions(sqlite3* const db)
{
    SqlLogicFunctions::RegisterCallbackFunctions(db,
        [&]()
        {
            GetSymbolTable().ForeachSymbol<UserFunction>(
                [&](UserFunction& user_function)
                {
                    if( user_function.IsSqlCallbackFunction() )
                    {
                        auto interpreter_and_user_function = std::make_unique<QueryRT::InterpreterAndUserFunction>(*this, user_function);

                        const int result = sqlite3_create_function(
                            db,
                            user_function.GetName().c_str(),
                            int32_cast(user_function.GetNumberParameters()),
                            SQLITE_UTF8,
                            interpreter_and_user_function.get(),
                            QueryRT::SqlCallbackFunction,
                            nullptr, // xStep
                            nullptr // xFinal
                        );

                        if( result != Sqlite::Result::OK )
                        {
                            throw CSProException("There was an error adding the user-defined function '%s' as a SQL callback function.",
                                                 user_function.GetName().c_str());
                        }

                        m_sqlCallbackFunctions.emplace_back(std::move(interpreter_and_user_function));
                    }
                });
        });
}



// --------------------------------------------------------------------------
// SQLite: routines to pass arguments from SQLite to user-defined functions
// --------------------------------------------------------------------------

class QueryRT::SqlQueryUserFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
{
public:
    SqlQueryUserFunctionArgumentEvaluator(const size_t number_arguments, sqlite3_value** const ppArgV)
        :   m_numberArguments(number_arguments),
            m_ppArgV(ppArgV)
    {
    }

protected:
    std::optional<size_t> GetNumberArguments() override
    {
        return m_numberArguments;
    }

    double GetNumeric(const size_t parameter_number) override
    {
        ASSERT(parameter_number < m_numberArguments);
        return sqlite3_value_double(m_ppArgV[parameter_number]);
    }

    SharableString GetString(const size_t parameter_number) override
    {
        ASSERT(parameter_number < m_numberArguments);
        return reinterpret_cast<const char*>(sqlite3_value_text(m_ppArgV[parameter_number]));
    }

private:
    size_t m_numberArguments;
    sqlite3_value** m_ppArgV;
};


void LogicInterpreter::ProcessSqlCallbackFunction(UserFunction& user_function, void* const void_context,
                                                  const int iArgC, void* const void_ppArgV)
{
    sqlite3_context* const context = reinterpret_cast<sqlite3_context*>(void_context);
    ASSERT(user_function.GetNumberParameters() == static_cast<size_t>(iArgC));

    QueryRT::SqlQueryUserFunctionArgumentEvaluator argument_evaluator(iArgC, reinterpret_cast<sqlite3_value**>(void_ppArgV));
    const Engine::Value return_value = CallUserFunction(user_function, argument_evaluator);

    if( return_value.is<double>() )
    {
        sqlite3_result_double(context, return_value.get<double>());
    }

    else
    {
        ASSERT(return_value.is<SharableString>());
        const SharableString value = return_value.as<SharableString>();
        sqlite3_result_text(context, value->c_str(), int32_cast(value->length()), SQLITE_TRANSIENT);
    }
}
