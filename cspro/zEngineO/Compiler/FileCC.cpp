#include "stdafx.h"
#include "IncludesCC.h"
#include "EngineDictionary.h"
#include "File.h"
#include "List.h"
#include "Nodes/Dictionaries.h"
#include "Nodes/File.h"
#include <zToolsO/DirectoryLister.h>
#include <engine/Dict.h>


LogicFile* LogicCompiler::CompileLogicFileDeclaration(bool compiling_function_parameter/* = false*/)
{
    std::wstring file_name = CompileNewSymbolName();

    auto logic_file = std::make_shared<LogicFile>(std::move(file_name));

    if( !compiling_function_parameter && IsGlobalCompilation() )
        logic_file->SetGlobalVisibility();

    m_engineData->AddSymbol(logic_file);

    return logic_file.get();
}


int LogicCompiler::CompileLogicFiles()
{
    ASSERT(Tkn == TOKKWFILE);
    Nodes::SymbolReset* symbol_reset_node = nullptr;

    do
    {
        LogicFile* logic_file = CompileLogicFileDeclaration();

        AddSymbolResetNode(symbol_reset_node, *logic_file);

        NextToken();

    } while( Tkn == TOKCOMMA );

    IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);

    return GetOptionalProgramIndex(symbol_reset_node);
}


int LogicCompiler::CompileLogicFileFunctions()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    LogicFile* dot_notation_logic_file = ( CurrentToken.symbol != nullptr ) ? assert_cast<LogicFile*>(CurrentToken.symbol) : nullptr;
    int number_expected_arguments = CurrentToken.function_details->number_arguments;
    auto& file_node = CreateNode<Nodes::File>(function_code);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    // read the first argument, which will be either a file, a dictionary, or an alpha expression
    // (unless the file is already provided via dot notation)
    LogicFile* logic_file = nullptr;

    if( dot_notation_logic_file != nullptr )
    {
        logic_file = dot_notation_logic_file;
        file_node.symbol_index_or_string_expression = -1 * dot_notation_logic_file->GetSymbolIndex();
    }

    else
    {
        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
        NextToken();

        if( Tkn != TOKFILE && ( function_code == FunctionCode::FNFILE_READ_CODE || function_code == FunctionCode::FNFILE_WRITE_CODE ) )
        {
            IssueError(MGF_TODO::m_33054);
        }

        else if( Tkn == TOKFILE || Tkn == TOKDICT || Tkn == TOKDICT_PRE80 )
        {
            if( function_code == FunctionCode::FNFILE_CONCAT_CODE )
            {
                if( Tkn == TOKDICT )
                {
                    VerifyDictionaryObject();
                }

                else if( Tkn != TOKDICT_PRE80 )
                {
                    IssueError(MGF_TODO::m_33059);
                }
            }

            else
            {
                if( Tkn != TOKFILE )
                    IssueError(MGF_TODO::m_33054);

                logic_file = &GetSymbolLogicFile(Tokstindex);
            }

            file_node.symbol_index_or_string_expression = -1 * Tokstindex;
            NextToken();
        }

        else if( next_token_helper_result == NextTokenHelperResult::List )
        {
            if( function_code == FunctionCode::FNFILE_COPY_CODE || function_code == FunctionCode::FNFILE_DELETE_CODE || function_code == FunctionCode::FNFILE_RENAME_CODE )
            {
                const LogicList& logic_list = GetSymbolLogicList(Tokstindex);

                if( !logic_list.IsString() )
                    IssueError(MGF::List_not_correct_data_type_961, ToString(DataType::String));

                file_node.symbol_index_or_string_expression = -1 * Tokstindex;
                NextToken();
            }

            else
            {
                IssueError(MGF_TODO::m_33055);
            }
        }

        else if( IsCurrentTokenString() )
        {
            file_node.symbol_index_or_string_expression = CompileStringExpression();
        }

        else
        {
            IssueError(MGF_TODO::m_33055);
        }
    }

    if( logic_file != nullptr )
        logic_file->SetUsed();

    // read subsequent arguments
    std::vector<int> subsequent_arguments;
    bool check_for_right_parenthesis = true;


    if( function_code == FunctionCode::FNFILE_WRITE_CODE )
    {
        int expression = 0;

        // write can write out all of the strings in a list
        if( CheckNextTokenHelper() == NextTokenHelperResult::List )
        {
            NextToken();

            if( !GetSymbolLogicList(Tokstindex).IsString() )
                IssueError(MGF::List_not_correct_data_type_961, ToString(DataType::String));

            expression = -1 * Tokstindex;

            NextToken();
        }

        else
        {
            expression = CompileMessageFunction(FunctionCode::FNFILE_WRITE_CODE);
            check_for_right_parenthesis = false;
        }

        subsequent_arguments.emplace_back(expression);

        if( logic_file != nullptr )
            logic_file->SetIsWrittenTo();
    }


    else if( function_code == FunctionCode::FNFILE_READ_CODE )
    {
        if( dot_notation_logic_file == nullptr )
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        // read can read into a string list...
        if( CheckNextTokenHelper() == NextTokenHelperResult::List )
        {
            NextToken();

            const LogicList& logic_list = GetSymbolLogicList(Tokstindex);

            if( !logic_list.IsString() )
                IssueError(MGF::List_not_correct_data_type_961, ToString(DataType::String));

            if( logic_list.IsReadOnly() )
                IssueError(MGF::List_read_only_cannot_be_modified_965, logic_list.GetName().c_str());

            subsequent_arguments.emplace_back(static_cast<int>(SymbolType::List));
            subsequent_arguments.emplace_back(Tokstindex);
            NextToken();
        }

        // or to a string variable
        else
        {
            NextToken();
            subsequent_arguments.emplace_back(-1);
            subsequent_arguments.emplace_back(CompileDestinationVariable(DataType::String));
        }
    }


    else if( number_expected_arguments < 0 || number_expected_arguments > 1 )
    {
        bool check_for_comma = ( dot_notation_logic_file == nullptr );

        while( Tkn != TOKRPAREN )
        {
            int expression = 0;

            if( number_expected_arguments >= 0 && subsequent_arguments.size() >= static_cast<size_t>(number_expected_arguments) )
                IssueError(MGF::function_call_arguments_count_mismatch_50);

            if( check_for_comma )
                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            check_for_comma = true;

            NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
            NextToken();

            if( function_code == FunctionCode::FNFILE_COPY_CODE || function_code == FunctionCode::FNFILE_RENAME_CODE )
            {
                // don't allow wildcard characters in the target of these functions
                if( next_token_helper_result == NextTokenHelperResult::StringLiteral && PathHasWildcardCharacters(Tokstr) )
                    IssueError(MGF_TODO::m_33056);

                if( IsCurrentTokenString() )
                {
                    expression = CompileStringExpression();
                }

                else if( Tkn == TOKFILE )
                {
                    GetSymbolLogicFile(Tokstindex).SetUsed();
                    expression = -1 * Tokstindex;
                    NextToken();
                }

                else
                {
                    IssueError(MGF_TODO::m_33055);
                }
            }

            else if( function_code == FunctionCode::FNFILE_CONCAT_CODE )
            {
                if( next_token_helper_result == NextTokenHelperResult::List )
                {
                    const LogicList& logic_list = GetSymbolLogicList(Tokstindex);

                    if( !logic_list.IsString() )
                        IssueError(MGF::List_not_correct_data_type_961, ToString(DataType::String));

                    expression = -1 * logic_list.GetSymbolIndex();
                    NextToken();
                }

                else if( IsCurrentTokenString() )
                {
                    expression = CompileStringExpression();
                }

                else
                {
                    IssueError(MGF::string_expression_expected_45006);
                }
            }

            else
            {
                ASSERT(false);
            }

            subsequent_arguments.emplace_back(expression);
        }
    }

    if( check_for_right_parenthesis )
    {
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();
    }

    // check that a sufficient number of arguments have been provided
    if( ( number_expected_arguments >= 0 && static_cast<size_t>(number_expected_arguments) != ( subsequent_arguments.size() + 1 ) ) ||
        ( number_expected_arguments < 0 && static_cast<size_t>(-1 * number_expected_arguments) > ( subsequent_arguments.size() + 1 ) ) )
    {
        IssueError(MGF::function_call_arguments_count_mismatch_50);
    }

    file_node.elements_list_node = CreateListNode(subsequent_arguments);

    return GetProgramIndex(file_node);
}


int LogicCompiler::CompileSetFileFunction()
{
    auto& setfile_node = CreateNode<Nodes::SetFile>(CurrentToken.function_details->code);
    Symbol* symbol = CurrentToken.symbol;
    ASSERT(symbol == nullptr || symbol->IsA(SymbolType::File));

    setfile_node.mode = Nodes::SetFile::Mode::Update;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    if( symbol == nullptr )
    {
        if( Tkn == TOKDICT || Tkn == TOKDICT_PRE80 || Tkn == TOKFILE )
        {
            symbol = &NPT_Ref(Tokstindex);
        }

        else
        {
            IssueError(MGF::dictionary_or_file_expected_930);
        }

        if( symbol->IsA(SymbolType::Dictionary) )
        {
            EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(*symbol);
            VerifyEngineDataRepository(&engine_dictionary, VerifyDictionaryFlag::NotWorkingStorage);
            engine_dictionary.GetEngineDataRepository().SetHasDynamicFileManagement();
        }

        else if( symbol->IsA(SymbolType::Pre80Dictionary) )
        {
            DICT* pDicT = assert_cast<DICT*>(symbol);
            VerifyDictionary(pDicT, VerifyDictionaryFlag::NotWorkingStorage);
            pDicT->SetHasDynamicFileManagement();
        }

        NextToken();
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();
    }

    setfile_node.symbol_index = symbol->GetSymbolIndex();

    // read the filename
    setfile_node.filename_expression = CompileStringExpression();

    if( Tkn == TOKCOMMA )
    {
        switch( NextKeywordOrError({ _T("update"), _T("create"), _T("append") }) )
        {
            case 1:
            {
                setfile_node.mode = Nodes::SetFile::Mode::Update;
                break;
            }

            case 2:
            {
                setfile_node.mode = Nodes::SetFile::Mode::Create;

                if( symbol->IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary) )
                {
                    // the input dictionary can only be used in entry
                    if( symbol->GetSubType() == SymbolSubType::Input && GetEngineAppType() != EngineAppType::Entry )
                        IssueError(MGF::setfile_invalid_use_of_create_in_non_entry_931);
                }

                break;
            }

            case 3:
            {
                setfile_node.mode = Nodes::SetFile::Mode::Append;
                break;
            }
        }

        NextToken();
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(setfile_node);
}
