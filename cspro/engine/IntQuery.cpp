#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "Exappl.h"
#include <zLogicO/SymbolTableIterator.h>
#include <zEngineO/Array.h>
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/List.h>
#include <zEngineO/UserFunction.h>
#include <zEngineO/Nodes/Query.h>
#include <SQLite/SQLite.h>
#include <SQLite/SQLiteHelpers.h>
#include <zToolsO/DirectoryLister.h>
#include <zUtilO/SqlLogicFunctions.h>
#include <zDictO/DDClass.h>
#include <ZBRIDGEO/npff.h>
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
        LogicConcatenator(CIntDriver* pIntDriver)
            :   m_pEngineDriver(pIntDriver->m_pEngineDriver),
                m_pIntDriver(pIntDriver),
                m_logsConcatenated(0)
        {
        }

        double GetReturnValue() const
        {
            return m_logsConcatenated;
        }

    protected:
        void OnInputProcessedSuccess(const std::variant<std::wstring, sqlite3*>& /*filename_or_database*/, int64_t /*iEventsProcessed*/) override
        {
            if( m_logsConcatenated != DEFAULT )
                ++m_logsConcatenated;
        }

        void OnInputProcessedError(NullTerminatedString input_filename, const std::wstring& error_message) override
        {
            issaerror(MessageType::Error, 8291, FormatText(_T(" in file %s"), input_filename.c_str()).GetString(), error_message.c_str());
            m_logsConcatenated = DEFAULT;
        }

        bool UserRequestsCancellation() override
        {
            return m_pIntDriver->m_bStopProc;
        }

    private:
        CEngineDriver* m_pEngineDriver;
        CIntDriver* m_pIntDriver;
        double m_logsConcatenated;
    };
}


double CIntDriver::exparadata(int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    int action = va_node.arguments[0];

    // --------------------------------------------------------------------------
    // open
    // --------------------------------------------------------------------------
    if( action == 1 )
    {
        std::wstring filename = EvalFullPathFileName(va_node.arguments[1]);

        Paradata::Logger::Stop();

        return Paradata::Logger::Start(std::move(filename), m_pEngineDriver->m_pPifFile->GetApplication());
    }


    // --------------------------------------------------------------------------
    // close
    // --------------------------------------------------------------------------
    else if( action == 2 )
    {
        if( Paradata::Logger::IsOpen() && Paradata::Logger::Flush() )
        {
            Paradata::Logger::Stop();
            m_pParadataDriver->ClearCachedObjects();
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

        std::wstring output_filename;
        bool bOutputFilenameIsCurrentlyOpenParadataLog = false;
        std::set<std::wstring> paradata_log_filenames;
        int iNumberArguments = va_node.arguments[1];
        bool bOutputIsAlsoAnInput = false;

        for( int i = 0; i < iNumberArguments; i += 2 )
        {
            bool bArgumentIsFilename = ( va_node.arguments[i + 2] == 0 );
            int iArgument = va_node.arguments[i + 3];
            std::vector<std::wstring> filenames;

            if( bArgumentIsFilename )
            {
                std::wstring filename = EvalFullPathFileName(iArgument);

                if( i == 0 )
                {
                    output_filename = std::move(filename);
                    bOutputFilenameIsCurrentlyOpenParadataLog = SO::EqualsNoCase(Paradata::Logger::GetFilename(), output_filename);
                }

                else
                {
                    // evaluate the filename in case it uses wildcards
                    DirectoryLister::AddFilenamesWithPossibleWildcard(filenames, filename, true);
                }
            }

            else // the argument is a list
            {
                ASSERT(i != 0);
                const LogicList& logic_list = GetSymbolLogicList(iArgument);
                size_t list_count = logic_list.GetCount();

                for( size_t j = 1; j <= list_count; ++j )
                {
                    std::wstring& filename = filenames.emplace_back(logic_list.GetString(j));
                    MakeFullPathFileName(filename);
                }
            }

            if( i > 0 )
            {
                for( std::wstring& filename : filenames )
                {
                    if( SO::EqualsNoCase(filename, output_filename) )
                    {
                        bOutputIsAlsoAnInput = true;

                        // change the filename so that the insert below is in the right case in the case
                        // that the filename is later removed (if concatenating to the current paradata log)
                        filename = output_filename;
                    }

                    paradata_log_filenames.insert(filename);
                }
            }
        }

        try
        {
            sqlite3* outputDbOverride = nullptr;

            // some checks if concatenating into the currently open paradata file
            if( bOutputFilenameIsCurrentlyOpenParadataLog )
            {
                if( !bOutputIsAlsoAnInput )
                    throw CSProException("You cannot concatenate into the currently open paradata log without also specifying that log as an input log");

                outputDbOverride = Paradata::Logger::GetSqlite();
            }

            LogicConcatenator logicConcatenator(this);

            if( outputDbOverride != nullptr )
            {
                paradata_log_filenames.erase(output_filename);
                logicConcatenator.Run(outputDbOverride, paradata_log_filenames);
            }

            else
            {
                logicConcatenator.Run(output_filename, paradata_log_filenames);
            }

            return logicConcatenator.GetReturnValue();
        }

        catch( const CSProException& exception )
        {
            issaerror(MessageType::Error, 8291, _T(""), exception.GetErrorMessage().c_str());
            return DEFAULT;
        }
    }


    return ReturnProgrammingError(0);
}


double CIntDriver::exsqlquery(int program_index)
{
    return exsqlquery(program_index, nullptr);
}


double CIntDriver::exsqlquery(int program_index, const std::function<double(sqlite3*, const std::string&)>* setreportdata_callback)
{
    const auto& sqlquery_node = GetNode<Nodes::SqlQuery>(program_index);
    std::string sql_query = EvalAlphaExpr<std::string>(sqlquery_node.sql_query_expression);

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
                DICX* pDicX = assert_cast<DICT&>(symbol).GetDicX();
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
            ConnectionString connection_string(EvalAlphaExpr(sqlquery_node.source_symbol_index_or_expression));
            MakeFullPathFileName(connection_string);

            bool success = PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(connection_string.GetFilename()));

            if( success )
            {
                // if specifying an Encrypted CSPro DB file, open it so that a password can be processed
                if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(connection_string.GetFilename()), FileExtensions::Data::EncryptedCSProDB) )
                {
                    success = ( EncryptedSQLiteRepository::OpenSQLiteDatabaseFile(nullptr, connection_string, &db, SQLITE_OPEN_READWRITE) == SQLITE_OK );
                }

                else
                {
                    success = ( sqlite3_open(ToUtf8(connection_string.GetFilename()), &db) == SQLITE_OK );
                }
            }

            if( !success )
                throw CSProException(_T("The SQLite file could not be opened: %s"), connection_string.GetFilename().c_str());

            must_close_db = true;
        }

        ASSERT(db != nullptr);

        // register any user-specified logic functions as SQL functions
        RegisterSqlCallbackFunctions(db);


        // if called from setreportdata, call back into the report system
        if( sqlquery_node.destination_symbol_index == Nodes::SqlQuery::SetReportDataDestinationJson )
        {
            ASSERT(setreportdata_callback != nullptr);
            return_value = (*setreportdata_callback)(db, sql_query);
        }

        else // called from a standard sqlquery call
        {
            Symbol* symbol = ( sqlquery_node.destination_symbol_index >= 0 ) ? &NPT_Ref(sqlquery_node.destination_symbol_index) :
                                                                               nullptr;

            // execute the query
            sqlite3_stmt* stmt = nullptr;

            std::vector<std::string> sql_statements = SQLiteHelpers::SplitSqlStatement(sql_query);

            if( sql_statements.empty() )
                throw CSProException("Empty SQL statement");

            // execute any helper statements
            for( size_t i = 0; i < ( sql_statements.size() - 1 ); i++ )
            {
                if( sqlite3_exec(db, sql_statements[i].c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
                    throw CSProException(_T("SQL syntax: %s"), FromUtf8(sqlite3_errmsg(db)).GetString());
            }

            if( sqlite3_prepare_v2(db, sql_statements.back().c_str(), -1, &stmt, nullptr) != SQLITE_OK )
                throw CSProException(_T("SQL syntax: %s"), FromUtf8(sqlite3_errmsg(db)).GetString());

            int sql_result = sqlite3_step(stmt);

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
                int number_columns = sqlite3_column_count(stmt);
                ASSERT(number_columns > 0);

                if( symbol == nullptr )
                {
                    // the return value will be the first row / first column result
                    bool value_is_null = ( sqlite3_column_type(stmt, 0) == SQLITE_NULL );
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
                        size_t max_rows_to_read = logic_array.GetDimension(0) - 1;
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

                                bool value_is_null = ( sqlite3_column_type(stmt, column) == SQLITE_NULL );

                                if( logic_array.IsNumeric() )
                                {
                                    logic_array.SetValue(indices, value_is_null ? NOTAPPL :
                                                                                  sqlite3_column_double(stmt, column));
                                }

                                else
                                {
                                    logic_array.SetValue(indices, value_is_null ? std::wstring() :
                                                                                  FromUtf8WS(sqlite3_column_text(stmt, column)));
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
                            bool value_is_null = ( sqlite3_column_type(stmt, 0) == SQLITE_NULL );

                            if( logic_list.IsNumeric() )
                            {
                                logic_list.AddValue(value_is_null ? NOTAPPL :
                                                                    sqlite3_column_double(stmt, 0));
                            }

                            else
                            {
                                logic_list.AddString(value_is_null ? std::wstring() :
                                                                     FromUtf8WS(sqlite3_column_text(stmt, 0)));
                            }

                        } while( ++row_number < MaximumRowsToRead && sqlite3_step(stmt) == SQLITE_ROW );

                        return_value = row_number;
                    }


                    // --------------------------------------------------------------------------
                    // fill in a record
                    // --------------------------------------------------------------------------
                    else
                    {
                        SECT* pSecT = assert_cast<SECT*>(symbol);

                        // map the columns
                        std::vector<VART*> aItemMapping(number_columns, nullptr);

                        for( int iColumn = 0; iColumn < number_columns; iColumn++ )
                        {
                            CString csColumnName = FromUtf8(sqlite3_column_name(stmt,iColumn));
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

                                else if( SO::EqualsNoCase(pVarT->GetName(), csColumnName) )
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
                                VART* pVarT = aItemMapping[iColumn];

                                if( pVarT == nullptr ) // the column wasn't mapped
                                    continue;

                                bool value_is_null = ( sqlite3_column_type(stmt, iColumn) == SQLITE_NULL );

                                if( pVarT->IsNumeric() )
                                {
                                    VARX* pVarX = pVarT->GetVarX();
                                    double dValue = value_is_null ? NOTAPPL : sqlite3_column_double(stmt, iColumn);
                                    SetVarFloatValue(dValue,pVarX,theIndex);
                                }

                                else
                                {
                                    CString csValue = value_is_null ? CString() : FromUtf8(sqlite3_column_text(stmt, iColumn));
                                    TCHAR* lpszBuffer = GetVarAsciiAddr(pVarT,theIndex);
                                    _tmemcpy(lpszBuffer, CIMSAString::MakeExactLength(csValue, pVarT->GetLength()), pVarT->GetLength());
                                }
                            }

                        } while( ( iRowNumber < pSecT->GetMaxOccs() ) && ( sqlite3_step(stmt) == SQLITE_ROW ) );

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
        std::wstring filename;

        if( db != nullptr )
            filename = FormatTextCS2WS(_T("(%s)"), PortableFunctions::PathGetFilename(FromUtf8WS(sqlite3_db_filename(db, nullptr))));

        issaerror(MessageType::Error, 8292, filename.c_str(), exception.GetErrorMessage().c_str());
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

    void SqlCallbackFunction(sqlite3_context* context, int iArgC, sqlite3_value** ppArgV)
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

                        if( sqlite3_create_function(db, ToUtf8(user_function.GetName()), user_function.GetNumberParameters(),
                                                    SQLITE_UTF8, interpreter_and_user_function.get(), SqlCallbackFunction, nullptr, nullptr) != SQLITE_OK )
                        {
                            throw CSProException("There was an error adding a user-defined function as a SQL callback function");
                        }

                        m_sqlCallbackFunctions.emplace_back(std::move(interpreter_and_user_function));
                    }
                });
        });
}


namespace
{
    class SqlQueryUserFunctionArgumentEvaluator : public UserFunctionArgumentEvaluator
    {
    public:
        SqlQueryUserFunctionArgumentEvaluator(sqlite3_value** ppArgV)
            :   m_ppArgV(ppArgV)
        {
        }

        double GetNumeric(int parameter_number) override
        {
            return sqlite3_value_double(m_ppArgV[parameter_number]);
        }

        std::wstring GetString(int parameter_number) override
        {
            return UTF8Convert::UTF8ToWide(sqlite3_value_text(m_ppArgV[parameter_number]));
        }

    private:
        sqlite3_value** m_ppArgV;
    };
}


void CIntDriver::ProcessSqlCallbackFunction(UserFunction& user_function, void* void_context, int iArgC, void* void_ppArgV)
{
    sqlite3_context* context = reinterpret_cast<sqlite3_context*>(void_context);
    sqlite3_value** ppArgV = reinterpret_cast<sqlite3_value**>(void_ppArgV);

    ASSERT(user_function.GetNumberParameters() == static_cast<size_t>(iArgC));

    SqlQueryUserFunctionArgumentEvaluator argument_evaluator(ppArgV);
    double return_value = CallUserFunction(user_function, argument_evaluator);

    if( user_function.GetReturnType() == SymbolType::WorkVariable )
    {
        sqlite3_result_double(context, return_value);
    }

    else
    {
        std::wstring value = CharacterObjectToString(return_value);
        sqlite3_result_text(context, ToUtf8(value), -1, SQLITE_TRANSIENT);
    }
}
