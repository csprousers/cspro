#include "stdafx.h"
#include "IncludesCC.h"
#include "Array.h"
#include "EngineDictionary.h"
#include "List.h"
#include "Nodes/Dictionaries.h"
#include "Nodes/Query.h"
#include <engine/Dict.h>
#include <Zissalib/SecT.h>


int LogicCompiler::CompileParadataFunction()
{
    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    std::vector<int> arguments;

    size_t action =  NextKeywordOrError({ _T("open"), _T("close"), _T("flush"), _T("concat"), _T("query") });

    NextToken();

    // process optional arguments

    // open
    if( action == 1 )
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();

        arguments.emplace_back(CompileStringExpression()); // the filename
    }

    // concat
    else if( action == 4 )
    {
        while( Tkn == TOKCOMMA )
        {
            bool next_token_is_list = ( CheckNextTokenHelper() == NextTokenHelperResult::List );
            NextToken();

            if( next_token_is_list )
            {
                // the output filename cannot be a list
                if( arguments.empty() )
                    IssueError(MGF::Query_paradata_concat_invalid_output_argument_8275);

                if( !GetSymbolLogicList(Tokstindex).IsString() )
                    IssueError(MGF::List_not_correct_data_type_961, ToString(DataType::String));

                arguments.emplace_back(1); // list type
                arguments.emplace_back(Tokstindex);
                NextToken();
            }

            else
            {
                arguments.emplace_back(0); // filename type
                arguments.emplace_back(CompileStringExpression()); // the filename (first the output, then the inputs)
            }
        }

        if( arguments.size() < 4 )
            IssueError(MGF::Query_paradata_concat_invalid_arguments_8274);

        arguments.insert(arguments.begin(), static_cast<int>(arguments.size())); // the number of filenames
    }


    // query
    else if( action == 5 )
    {
        return CompileSqlQueryFunction(true);
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    arguments.insert(arguments.begin(), action);

    return CreateVariableArgumentsNode(FunctionCode::FNPARADATA_CODE, arguments);
}


int LogicCompiler::CompileSqlQueryFunction(bool from_paradata_function/* = false*/)
{
    auto& sqlquery_node = CreateNode<Nodes::SqlQuery>(FunctionCode::FNSQLQUERY_CODE);

    // compile the input source
    if( from_paradata_function )
    {
        sqlquery_node.source_type = Nodes::SqlQuery::Type::Paradata;
    }

    else
    {
        if( Tkn == TOKFUNCTION && CurrentToken.function_details->code == FNPARADATA_CODE )
        {
            sqlquery_node.source_type = Nodes::SqlQuery::Type::Paradata;

            NextToken();
        }

        else if( Tkn == TOKDICT || Tkn == TOKDICT_PRE80 )
        {
            sqlquery_node.source_type = Nodes::SqlQuery::Type::Dictionary;
            sqlquery_node.source_symbol_index_or_expression = Tokstindex;

            Symbol& symbol = NPT_Ref(sqlquery_node.source_symbol_index_or_expression);

            if( symbol.IsA(SymbolType::Dictionary) )
            {
                VerifyEngineDataRepository(assert_cast<EngineDictionary*>(&symbol), VerifyDictionaryFlag::NotWorkingStorage);
            }

            else
            {
                VerifyDictionary(assert_cast<DICT*>(&symbol), VerifyDictionaryFlag::NotWorkingStorage);
            }

            NextToken();
        }

        else
        {
            sqlquery_node.source_type = Nodes::SqlQuery::Type::File;
            sqlquery_node.source_symbol_index_or_expression = CompileStringExpression(); // the filename
        }
    }

    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

    NextTokenWithPreference(SymbolType::Section);

    // compile the destination
    sqlquery_node.destination_symbol_index = -1;

    if( Tkn == TOKARRAY )
    {
        sqlquery_node.destination_symbol_index = Tokstindex;

        const LogicArray& logic_array = GetSymbolLogicArray(sqlquery_node.destination_symbol_index);

        if( logic_array.GetNumberDimensions() > 2 )
            IssueError(MGF::Query_invalid_array_8272);
    }

    else if( Tkn == TOKLIST )
    {
        sqlquery_node.destination_symbol_index = Tokstindex;
    }

    else if( Tkn == TOKSECT )
    {
        sqlquery_node.destination_symbol_index = Tokstindex;

        SECT* pSecT = SPT(sqlquery_node.destination_symbol_index);

        // only records in the working storage dictionary can be assigned to
        if( pSecT->GetDicT()->GetSubType() != SymbolSubType::Work )
            IssueError(MGF::Query_record_must_be_working_storage_8273);

        MarkAllInSectionUsed(pSecT);
    }

    if( sqlquery_node.destination_symbol_index >= 0 )
    {
        NextToken();
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();
    }

    sqlquery_node.sql_query_expression = CompileStringExpression();

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(sqlquery_node);
}
