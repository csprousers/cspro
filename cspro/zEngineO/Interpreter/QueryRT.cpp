#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "Engine.h"
#include "Exappl.h"
#include <zLogicO/SymbolTableIterator.h>
#include <zEngineO/Array.h>
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/List.h>
#include <zEngineO/UserFunctionArgumentEvaluator.h>
#include <zEngineO/Nodes/Query.h>
#include <zToolsO/DirectoryLister.h>
#include <zSql/SQLite.h>
#include <zSql/SQLiteHelpers.h>
#include <zUtilO/SqlLogicFunctions.h>
#include <zDictO/DDClass.h>
#include <zBridgeO/NPff.h>
#include <zParadataO/Logger.h>
#include <zParadataO/Concatenator.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zDataO/EncryptedSQLiteRepository.h>
#include <zDataO/SQLiteRepository.h>
#include <zDataO/TextRepository.h>


namespace
{
    class LogicConcatenator : public Paradata::Concatenator
    {
    public:
        LogicConcatenator(CIntDriver* const interpreter)
            :   m_pEngineDriver(interpreter->m_pEngineDriver),
                m_pIntDriver(interpreter),
                m_logsConcatenated(0)
        {
        }

        double GetReturnValue() const
        {
            return m_logsConcatenated.value_or(DEFAULT);
        }

    protected:
        void OnInputProcessedSuccess(const std::variant<std::string, sqlite3*>& /*output_file_path_or_database*/, int64_t /*events_processed*/) override
        {
            if( m_logsConcatenated.has_value() )
                ++(*m_logsConcatenated);
        }

        void OnInputProcessedError(const std::string& input_file_path, const char* const error_message) override
        {
            issaerror(MessageType::Error, 8291, FormatText(" in file %s", input_file_path.c_str()).c_str(), error_message);
            m_logsConcatenated.reset();
        }

        bool UserRequestsCancellation() override
        {
            return m_pIntDriver->m_bStopProc;
        }

    private:
        CEngineDriver* m_pEngineDriver;
        CIntDriver* m_pIntDriver;
        std::optional<double> m_logsConcatenated;
    };
}


double CIntDriver::ex_paradata(const int program_index)
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

        return Paradata::Logger::Start(std::move(file_path), m_pEngineDriver->m_pPifFile->GetApplication());
    }


    // --------------------------------------------------------------------------
    // close
    // --------------------------------------------------------------------------
    else if( action == 2 )
    {
        if( Paradata::Logger::IsOpen() && Paradata::Logger::Flush() )
        {
            Paradata::Logger::Stop();
            m_paradataDriver->ClearCachedObjects();
            return !Paradata::Logger::IsOpen();
        }

        return 0;
    }


    // --------------------------------------------------------------------------
    // flush
    // --------------------------------------------------------------------------
    else if( action == 3 )
    {
        if( Paradata::Logger::IsOpen() )
            return Paradata::Logger::Flush();

        return 0;
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
            const bool arugment_is_file_path = ( va_node.arguments[i + 2] == 0 );
            const int argument = va_node.arguments[i + 3];
            std::vector<std::string> file_paths;

            if( arugment_is_file_path )
            {
                std::string file_path = EvaluatePath(argument);

                if( i == 0 )
                {
                    output_file_path = std::move(file_path);
                    output_file_path_is_currently_open_paradata_log = SO::EqualsNoCase(Paradata::Logger::GetFilePath(), output_file_path);
                }

                else
                {
                    // evaluate the filename in case it uses wildcards
                    DirectoryLister::AddFilePathsWithPossibleWildcard(file_paths, file_path, true);
                }
            }

            else // the argument is a list
            {
                ASSERT(i != 0);
                const LogicList& logic_list = GetSymbolLogicList(argument);
                const size_t list_count = logic_list.GetCount();

                for( size_t j = 1; j <= list_count; ++j )
                    file_paths.emplace_back(GetAbsolutePath(logic_list.GetValue<SharableString>(j).GetString()));
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
                    throw CSProException("You cannot concatenate into the currently open paradata log without also specifying that log as an input log");

                output_db_override = Paradata::Logger::GetSqlite();
            }

            LogicConcatenator logic_concatenator(this);

            if( output_db_override != nullptr )
            {
                paradata_log_file_paths.erase(output_file_path);
                logic_concatenator.Run(output_db_override, paradata_log_file_paths);
            }

            else
            {
                logic_concatenator.Run(output_file_path, paradata_log_file_paths);
            }

            return logic_concatenator.GetReturnValue();
        }

        catch( const CSProException& exception )
        {
            issaerror(MessageType::Error, 8291, "", exception.what());
            return DEFAULT;
        }
    }


    return ReturnProgrammingError(0);
}


double CIntDriver::exsqlquery(const int program_index)
{
    return exsqlquery(program_index, nullptr);
}


double CIntDriver::exsqlquery(const int program_index, const std::function<double(sqlite3*, const std::string&)>* const setreportdata_callback)
{
    const auto& sqlquery_node = GetNode<Nodes::SqlQuery>(program_index);
    const SharableString sql_query = Evaluate<SharableString>(sqlquery_node.sql_query_expression);

    sqlite3* db = nullptr;
    bool must_close_db = false;
    double return_value = DEFAULT;

    try
    {
        // access the database
        if( sqlquery_node.source_type == Nodes::SqlQuery::Type::Paradata )
        {
            db = Paradata::Logger::GetSqlite();

            if( db == nullptr )
                throw CSProException("No paradata log is open");

            Paradata::Logger::Flush();
        }

        else if( sqlquery_node.source_type == Nodes::SqlQuery::Type::Dictionary )
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

            db = DataRepositoryHelpers::GetSqliteDatabase(*data_repository);

            if( db == nullptr )
            {
                if( data_repository->GetRepositoryType() == DataRepositoryType::Text )
                {
                    throw CSProException("You can only execute queries on text files that use an index");
                }

                else
                {
                    throw CSProException("You can only execute queries on CSPro DB or text files");
                }
            }
        }

        else if( sqlquery_node.source_type == Nodes::SqlQuery::Type::File )
        {
            const ConnectionString connection_string = EvaluateConnectionString(sqlquery_node.source_symbol_index_or_expression);

            bool success = ( connection_string.HasFilePath() &&
                             PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(connection_string.GetFilePath())) );

            if( success )
            {
                // if specifying an Encrypted CSPro DB file, open it so that a password can be processed
                if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(connection_string.GetFilePath()), FileExtensions::Data::EncryptedCSProDB) )
                {
                    success = ( EncryptedSQLiteRepository::OpenSQLiteDatabaseFile(nullptr, connection_string, &db, SQLITE_OPEN_READWRITE) == SQLITE_OK );
                }

                else
                {
                    success = ( sqlite3_open(connection_string.GetFilePath().c_str(), &db) == SQLITE_OK );
                }
            }

            if( !success )
                throw CSProException("The SQLite file could not be opened: " + connection_string.ToDisplayString());

            must_close_db = true;
        }

        ASSERT(db != nullptr);

        // register any user-specified logic functions as SQL functions
        RegisterSqlCallbackFunctions(db);


        // if called from setreportdata, call back into the report system
        if( sqlquery_node.destination_symbol_index == Nodes::SqlQuery::SetReportDataDestinationJson )
        {
            ASSERT(setreportdata_callback != nullptr);
            return_value = (*setreportdata_callback)(db, sql_query.GetString());
        }

        else // called from a standard sqlquery call
        {
            Symbol* const symbol = ( sqlquery_node.destination_symbol_index >= 0 ) ? &NPT_Ref(sqlquery_node.destination_symbol_index) :
                                                                                     nullptr;

            // execute the query
            sqlite3_stmt* stmt = nullptr;

            const std::vector<std::string> sql_statements = SQLiteHelpers::SplitSqlStatement(sql_query.GetString());

            if( sql_statements.empty() )
                throw CSProException("Empty SQL statement");

            // execute any helper statements
            for( size_t i = 0; i < ( sql_statements.size() - 1 ); i++ )
            {
                if( sqlite3_exec(db, sql_statements[i].c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
                    throw CSProException("SQL syntax: %s", sqlite3_errmsg(db));
            }

            if( sqlite3_prepare_v2(db, sql_statements.back().c_str(), -1, &stmt, nullptr) != SQLITE_OK )
                throw CSProException("SQL syntax: %s", sqlite3_errmsg(db));

            const int sql_result = sqlite3_step(stmt);

            if( sql_result == SQLITE_DONE )
            {
                // the return value will signal that no rows were returned or that
                // something (like a CREATE TABLE) succeeded but that the return value is not applicable
                return_value = 0;

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
            }

            else if( sql_result == SQLITE_ROW )
            {
                const int number_columns = sqlite3_column_count(stmt);
                ASSERT(number_columns > 0);

                if( symbol == nullptr )
                {
                    // the return value will be the first row / first column result
                    const bool value_is_null = ( sqlite3_column_type(stmt, 0) == SQLITE_NULL );
                    return_value = value_is_null ? NOTAPPL : sqlite3_column_double(stmt, 0);
                }

                else
                {
                    // the results will be placed in the object and the
                    // the return value will be the number of results placed in the object

                    // --------------------------------------------------------------------------
                    // fill in an array
                    // --------------------------------------------------------------------------
                    if( symbol->IsA(SymbolType::Array) )
                    {
                        LogicArray& logic_array = assert_cast<LogicArray&>(*symbol);

                        // - 1 in the next two statements because the arrays will be filled in starting at index 1
                        const size_t max_rows_to_read = logic_array.GetDimension(0) - 1;
                        size_t columns_to_read = 1;

                        std::vector<size_t> indices(logic_array.GetNumberDimensions(), 0);

                        if( logic_array.GetNumberDimensions() == 2 )
                        {
                            columns_to_read = std::min(static_cast<size_t>(number_columns), logic_array.GetDimension(1) - 1);
                        }

                        do
                        {
                            ++indices[0];

                            for( size_t column = 0; column < columns_to_read; ++column )
                            {
                                if( columns_to_read != 1 )
                                    indices[1] = column + 1;

                                ASSERT(logic_array.IsValidIndex(indices));

                                const bool value_is_null = ( sqlite3_column_type(stmt, column) == SQLITE_NULL );

                                if( logic_array.IsNumeric() )
                                {
                                    logic_array.SetValue(indices, value_is_null ? NOTAPPL :
                                                                                  sqlite3_column_double(stmt, column));
                                }

                                else
                                {
                                    logic_array.SetValue(indices, value_is_null ? SharableString() :
                                                                                  SharableString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, column))));
                                }
                            }

                        } while( indices[0] < max_rows_to_read && sqlite3_step(stmt) == SQLITE_ROW );

                        return_value = indices[0];
                    }


                    // --------------------------------------------------------------------------
                    // fill in a list
                    // --------------------------------------------------------------------------
                    else if( symbol->IsA(SymbolType::List) )
                    {
                        LogicList& logic_list = assert_cast<LogicList&>(*symbol);
                        logic_list.Reset();

                        constexpr size_t MaximumRowsToRead = 10000;
                        size_t row_number = 0;

                        do
                        {
                            const bool value_is_null = ( sqlite3_column_type(stmt, 0) == SQLITE_NULL );

                            if( logic_list.IsNumeric() )
                            {
                                logic_list.AddValue(value_is_null ? NOTAPPL :
                                                                    sqlite3_column_double(stmt, 0));
                            }

                            else
                            {
                                logic_list.AddValue(value_is_null ? SharableString() :
                                                                    SharableString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
                            }

                        } while( ++row_number < MaximumRowsToRead && sqlite3_step(stmt) == SQLITE_ROW );

                        return_value = row_number;
                    }


                    // --------------------------------------------------------------------------
                    // fill in a record
                    // --------------------------------------------------------------------------
                    else
                    {
                        SECT* const pSecT = assert_cast<SECT*>(symbol);

                        // map the columns
                        std::vector<VART*> aItemMapping(number_columns, nullptr);

                        for( int iColumn = 0; iColumn < number_columns; iColumn++ )
                        {
                            const std::string column_name = sqlite3_column_name(stmt, iColumn);
                            VART* pVarT = nullptr;

                            for( int iSymVar = pSecT->SYMTfvar; iSymVar >= 0; iSymVar = pVarT->SYMTfwd )
                            {
                                pVarT = VPT(iSymVar);
                                const CDictItem* pDictItem = pVarT->GetDictItem();

                                if( ( pDictItem->GetItemType() == ItemType::Subitem ) || // don't look at subitems
                                    ( pDictItem->GetOccurs() > 1 ) ) // don't look at items that occur
                                {
                                    continue;
                                }

                                else if( SO::EqualsNoCase(pVarT->GetName(), column_name) )
                                {
                                    aItemMapping[iColumn] = pVarT;
                                    break;
                                }
                            }
                        }

                        // fill the items
                        CNDIndexes theIndex(ZERO_BASED);
                        theIndex.setAtOrigin();
                        int iRowNumber = 0;

                        do
                        {
                            theIndex.setIndexValue(CDimension::Record,iRowNumber);
                            iRowNumber++;

                            for( int iColumn = 0; iColumn < number_columns; iColumn++ )
                            {
                                VART* const pVarT = aItemMapping[iColumn];

                                if( pVarT == nullptr ) // the column wasn't mapped
                                    continue;

                                const bool value_is_null = ( sqlite3_column_type(stmt, iColumn) == SQLITE_NULL );

                                if( pVarT->IsNumeric() )
                                {
                                    VARX* const pVarX = pVarT->GetVarX();
                                    const double dValue = value_is_null ? NOTAPPL : sqlite3_column_double(stmt, iColumn);
                                    SetVarFloatValue(dValue,pVarX,theIndex);
                                }

                                else
                                {
                                    CString csValue = value_is_null ? CString() : UTF8_TODO::GetCString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, iColumn)));
                                    TCHAR* lpszBuffer = GetVarAsciiAddr(pVarT,theIndex);
                                    _tmemcpy(lpszBuffer, CIMSAString::MakeExactLength(csValue, pVarT->GetLength()), pVarT->GetLength());
                                }
                            }

                        } while( iRowNumber < pSecT->GetMaxOccs() && sqlite3_step(stmt) == SQLITE_ROW );

                        pSecT->GetGroup(0)->SetTotalOccurrences(iRowNumber);

                        return_value = iRowNumber;
                    }
                }
            }

            safe_sqlite3_finalize(stmt);
        }
    }

    catch( const CSProException& exception )
    {
        const std::string filename = ( db != nullptr ) ? FormatText("(%s)", PortableFunctions::PathGetFilename(sqlite3_db_filename(db, nullptr)).c_str()) :
                                                         std::string();
        issaerror(MessageType::Error, 8292, filename.c_str(), exception.what());
    }

    if( must_close_db )
        sqlite3_close(db);

    return return_value;
}



// --------------------------------------------------------------------------
// routines for calling back into user-defined function from SQL queries
// --------------------------------------------------------------------------


namespace
{
    using InterpreterAndUserFunction = std::tuple<CIntDriver&, UserFunction&>;

    void SqlCallbackFunction(sqlite3_context* const context, int iArgC, sqlite3_value** const ppArgV)
    {
        InterpreterAndUserFunction& interpreter_and_user_function = *static_cast<InterpreterAndUserFunction*>(sqlite3_user_data(context));

        std::get<0>(interpreter_and_user_function).ProcessSqlCallbackFunction(std::get<1>(interpreter_and_user_function),
                                                                              static_cast<void*>(context), iArgC, static_cast<void*>(ppArgV));
    }
}


void CIntDriver::RegisterSqlCallbackFunctions(sqlite3* const db)
{
    SqlLogicFunctions::RegisterCallbackFunctions(db,
        [&]()
        {
            GetSymbolTable().ForeachSymbol<UserFunction>(
                [&](UserFunction& user_function)
                {
                    if( user_function.IsSqlCallbackFunction() )
                    {
                        auto interpreter_and_user_function = std::make_unique<InterpreterAndUserFunction>(*this, user_function);

                        if( sqlite3_create_function(db, user_function.GetName().c_str(), user_function.GetNumberParameters(),
                                                    SQLITE_UTF8, interpreter_and_user_function.get(), SqlCallbackFunction, nullptr, nullptr) != SQLITE_OK )
                        {
                            throw CSProException("There was an error adding the user-defined function '%s' as a SQL callback function.",
                                                 user_function.GetName().c_str());
                        }

                        m_sqlCallbackFunctions.emplace_back(std::move(interpreter_and_user_function));
                    }
                });
        });
}


class SqlQueryUserFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
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


void CIntDriver::ProcessSqlCallbackFunction(UserFunction& user_function, void* const void_context, const int iArgC, void* const void_ppArgV)
{
    sqlite3_context* const context = reinterpret_cast<sqlite3_context*>(void_context);
    ASSERT(user_function.GetNumberParameters() == static_cast<size_t>(iArgC));

    SqlQueryUserFunctionArgumentEvaluator argument_evaluator(iArgC, reinterpret_cast<sqlite3_value**>(void_ppArgV));
    const Engine::Value return_value = CallUserFunction(user_function, argument_evaluator);

    if( return_value.is<double>() )
    {
        sqlite3_result_double(context, return_value.get<double>());
    }

    else
    {
        ASSERT(return_value.is<SharableString>());
        const SharableString value = return_value.as<SharableString>();
        sqlite3_result_text(context, value->c_str(), value->length(), SQLITE_TRANSIENT);
    }
}
