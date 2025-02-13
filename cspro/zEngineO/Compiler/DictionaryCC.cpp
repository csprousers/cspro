#include "stdafx.h"
#include "IncludesCC.h"
#include "EngineDictionary.h"
#include "LoopStack.h"
#include "Nodes/Dictionaries.h"
#include "WorkString.h"
#include <engine/Dict.h>


namespace
{
    SelcaseQueryType ReadSelcaseQueryType(LogicCompiler& logic_compiler)
    {
        switch( logic_compiler.NextKeywordOrError({ _T("all"), _T("marked"), _T("unmarked") }) )
        {
            case 1: return SelcaseQueryType::All;
            case 2: return SelcaseQueryType::Marked;
            case 3: return SelcaseQueryType::Unmarked;
        }

        throw ReturnProgrammingError(SelcaseQueryType::Marked);
    }
}


int LogicCompiler::CompileDictionaryFunctionsVarious()
{
    FunctionCode function_code = CurrentToken.function_details->code;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);


    // --------------------------------------------------------------------------
    // countcases
    // --------------------------------------------------------------------------
    if( function_code == FunctionCode::FNCOUNTCASES_CODE )
    {
        auto& countcases_node = CreateNode<Nodes::CountCases>(function_code);

        countcases_node.where_expression = -1;
        countcases_node.dictionary_access = 0;
        countcases_node.starts_with_expression = -1;

        NextToken();

        if( Tkn != TOKDICT && Tkn != TOKDICT_PRE80 )
            IssueError(MGF::dictionary_expected_544);

        countcases_node.data_repository_dictionary_symbol_index = Tokstindex;
        countcases_node.case_dictionary_symbol_index = Tokstindex;

        if( Tkn == TOKDICT )
        {
            NextToken();

            // the data repository can be specified separate from the case
            if( Tkn == TOKIN )
            {
                NextToken();
                VerifyEngineDataRepository();

                countcases_node.data_repository_dictionary_symbol_index = Tokstindex;

                NextToken();
            }

            EngineDictionary& data_repository_engine_dictionary = GetSymbolEngineDictionary(countcases_node.data_repository_dictionary_symbol_index);
            VerifyEngineDataRepository(&data_repository_engine_dictionary, VerifyDictionaryFlag::NeedsIndex);

            if( countcases_node.data_repository_dictionary_symbol_index != countcases_node.case_dictionary_symbol_index )
            {
                VerifyEngineDataRepositoryWithEngineCase(data_repository_engine_dictionary,
                                                         GetSymbolEngineDictionary(countcases_node.case_dictionary_symbol_index));
            }

            if( Tkn == TOKLPAREN )
            {
                countcases_node.dictionary_access = CompileDictionaryAccess(data_repository_engine_dictionary, &countcases_node.starts_with_expression);
                IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

                NextToken();
            }

            if( Tkn == TOKWHERE )
            {
                VerifyEngineDataRepository(&data_repository_engine_dictionary, VerifyDictionaryFlag::External_OneLevel);

                // check that a case (not just a data source) is available when using where
                if( !GetSymbolEngineDictionary(countcases_node.case_dictionary_symbol_index).HasEngineCase() )
                    IssueError(MGF::countcases_with_where_requires_case_47311);

                LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::CountCases, &data_repository_engine_dictionary);

                NextToken();
                countcases_node.where_expression = exprlog();
            }
        }

        else
        {
            DICT* pDicT = DPT(countcases_node.data_repository_dictionary_symbol_index);

            VerifyDictionary(pDicT, VerifyDictionaryFlag::NeedsIndex);

            NextToken();

            if( Tkn == TOKLPAREN )
            {
                countcases_node.dictionary_access = CompileDictionaryAccess(pDicT, &countcases_node.starts_with_expression);
                IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

                NextToken();
            }

            if( Tkn == TOKWHERE )
            {
                VerifyDictionary(pDicT, VerifyDictionaryFlag::External_OneLevel);

                LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(LoopStackSource::CountCases, pDicT);

                NextToken();
                countcases_node.where_expression = exprlog();
            }
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(countcases_node);
    }


    // --------------------------------------------------------------------------
    // nmembers
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNNMEMBERS_CODE )
    {
        auto& nmembers_node = CreateNode<Nodes::NMembers>(function_code);

        // read the dictionary
        NextToken();

        if( Tkn != TOKDICT_PRE80 )
            IssueError(MGF::dictionary_expected_544);

        nmembers_node.dictionary_symbol_index = Tokstindex;

        DICT* pDicT = DPT(nmembers_node.dictionary_symbol_index);

        VerifyDictionary(pDicT, VerifyDictionaryFlag::External_OneLevel_NeedsIndex);

        // read the selcase query type (or default to marked)
        nmembers_node.query_type = IsNextToken(TOKRPAREN) ? SelcaseQueryType::Marked :
                                                            ReadSelcaseQueryType(*this);

        NextToken();
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(nmembers_node);
    }


    // --------------------------------------------------------------------------
    // retrieve
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNRETRIEVE_CODE )
    {
        auto& retrieve_node = CreateNode<Nodes::Retrieve>(function_code);

        NextToken();

        if( Tkn != TOKDICT && Tkn != TOKDICT_PRE80 )
            IssueError(MGF::dictionary_expected_544);

        retrieve_node.data_repository_dictionary_symbol_index = Tokstindex;
        retrieve_node.case_dictionary_symbol_index = Tokstindex;

        Symbol& symbol = NPT_Ref(retrieve_node.data_repository_dictionary_symbol_index);

        NextToken();

        if( symbol.IsA(SymbolType::Dictionary) )
        {
            EngineDictionary& data_repository_engine_dictionary = assert_cast<EngineDictionary&>(symbol);
            VerifyEngineDataRepository(&data_repository_engine_dictionary, VerifyDictionaryFlag::External_OneLevel_NeedsIndex);

            if( Tkn == TOKCOMMA )
            {
                NextToken();

                VerifyEngineCase();
                retrieve_node.case_dictionary_symbol_index = Tokstindex;

                NextToken();
            }

            VerifyEngineDataRepositoryWithEngineCase(data_repository_engine_dictionary,
                                                     GetSymbolEngineDictionary(retrieve_node.case_dictionary_symbol_index));
        }

        else
        {
            DICT* pDicT = assert_cast<DICT*>(&symbol);
            VerifyDictionary(pDicT, VerifyDictionaryFlag::External_OneLevel_NeedsIndex);
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(retrieve_node);
    }


    // --------------------------------------------------------------------------
    // unhandled function code
    // --------------------------------------------------------------------------
    else
    {
        return ReturnProgrammingError(-1);
    }
}


int LogicCompiler::CompileDictionaryFunctionsCaseSearch()
{
    // find(external_dict_name, relational_operator, key)
    // locate(external_dict_name, relational_operator, key)
    auto& case_search_node = CreateNode<Nodes::CaseSearch>(CurrentToken.function_details->code);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    NextToken();

    if( Tkn != TOKDICT && Tkn != TOKDICT_PRE80 )
        IssueError(MGF::dictionary_expected_544);

    case_search_node.dictionary_symbol_index = Tokstindex;

    Symbol& symbol = NPT_Ref(case_search_node.dictionary_symbol_index);

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);

        if( engine_dictionary.GetSubType() == SymbolSubType::External )
        {
            VerifyEngineDataRepository(&engine_dictionary, VerifyDictionaryFlag::OneLevel | VerifyDictionaryFlag::NeedsIndex);
        }

        // don't allow find/locate for special output dictionaries
        else if( engine_dictionary.GetSubType() == SymbolSubType::Output )
        {
            IssueError(MGF::dictionary_special_output_not_allowed_4018);
        }

        else if( engine_dictionary.GetSubType() == SymbolSubType::Input && case_search_node.function_code == FunctionCode::FNLOCATE_CODE )
        {
            // the input dictionary cannot be used in locate in an entry application
            if( GetEngineAppType() == EngineAppType::Entry )
                IssueError(MGF::locate_invalid_on_input_dict_in_entry_4016);
        }

        VerifyEngineDataRepository(&engine_dictionary, VerifyDictionaryFlag::NeedsIndex);
    }

    else
    {
        DICT* pDicT = assert_cast<DICT*>(&symbol);

        if( pDicT->GetSubType() == SymbolSubType::External )
        {
            VerifyDictionary(pDicT, VerifyDictionaryFlag::OneLevel | VerifyDictionaryFlag::NeedsIndex);
        }

        // don't allow find/locate for special output dictionaries
        else if( pDicT->GetSubType() == SymbolSubType::Output )
        {
            IssueError(MGF::dictionary_special_output_not_allowed_4018);
        }

        else if( pDicT->GetSubType() == SymbolSubType::Input && case_search_node.function_code == FunctionCode::FNLOCATE_CODE )
        {
            // the input dictionary cannot be used in locate in an entry application
            if( GetEngineAppType() == EngineAppType::Entry )
                IssueError(MGF::locate_invalid_on_input_dict_in_entry_4016);
        }

        VerifyDictionary(pDicT, VerifyDictionaryFlag::NeedsIndex);
    }

    NextToken();

    // secondary indices are no longer supported
    if( Tkn == TOKLPAREN )
        IssueError(MGF::secondary_indices_no_longer_supported_543);

    // optional comma
    if( Tkn == TOKCOMMA )
        NextToken();

    // allow lookups by uuid
    if( Tkn == TOKFUNCTION && CurrentToken.function_details->code == FunctionCode::FNUUID_CODE )
    {
        case_search_node.operator_code = Nodes::CaseSearch::SearchByUuidCode;
    }

    // or key prefix
    else if( Tkn == TOKFUNCTION && CurrentToken.function_details->code == FunctionCode::FNSTARTSWITH_CODE )
    {
        case_search_node.operator_code = Nodes::CaseSearch::SearchByKeyPrefixCode;
    }

    // mandatory relational operator
    else if( Tkn == TOKNEOP || !IsRelationalOperator(Tkn) )
    {
        IssueError(MGF::case_search_invalid_relational_operator_4014);
    }

    else
    {
        ASSERT(Tkn == TOKEQOP || Tkn == TOKLEOP || Tkn == TOKGEOP || Tkn == TOKGTOP || Tkn == TOKLTOP);
        case_search_node.operator_code = Tkn;
    }

    NextToken();

    if( Tkn == TOKCOMMA )
        NextToken();

    // mandatory key expression
    case_search_node.key_expression = CompileStringExpression();

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(case_search_node);
}


int LogicCompiler::CompileDictionaryFunctionsCaseIO()
{
    // ENGINECR_TODO eventually allow loadcase/forcase to work on an input dictionary as long as a case is supplied

    FunctionCode function_code = CurrentToken.function_details->code;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    // process the dictionary
    NextToken();

    if( Tkn == TOKDICT_PRE80 )
        return CompileDictionaryFunctionsCaseIO_pre80(function_code);

    IssueErrorOnTokenMismatch(TOKDICT, MGF::dictionary_expected_544);

    auto& case_io_node = CreateNode<Nodes::CaseIO>(function_code);

    case_io_node.data_repository_dictionary_symbol_index = Tokstindex;
    case_io_node.case_dictionary_symbol_index = -1;

    EngineDictionary& engine_dictionary = GetSymbolEngineDictionary(case_io_node.data_repository_dictionary_symbol_index);
    VerifyEngineDataRepository(&engine_dictionary, VerifyDictionaryFlag::External_OneLevel);

    if( function_code != FunctionCode::FNLOADCASE_CODE )
        VerifyEngineDataRepository(&engine_dictionary, VerifyDictionaryFlag::Writeable);

    NextToken();

    bool needs_index = true;

    if( Tkn == TOKLPAREN )
    {
        if( function_code == FNWRITECASE_CODE )
        {
            NextKeywordOrError({ _T("NOINDEX") });

            needs_index = false;

            NextToken();
            IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

            NextToken();
        }

        // secondary indices are no longer supported
        else
        {
            IssueError(MGF::secondary_indices_no_longer_supported_543);
        }
    }

    VerifyEngineDataRepository(&engine_dictionary, needs_index ? VerifyDictionaryFlag::NeedsIndex : VerifyDictionaryFlag::CannotHaveIndex);


    // process each argument
    std::vector<int> key_arguments;
    std::wstring key_argument_types;
    bool key_length_can_be_calculated = true;

    while( Tkn != TOKRPAREN )
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
        NextToken();

        // if a dictionary/case is provided, its values can be used to:
        //    - provide the key of the case to delete for delcase
        //    - provide the destination case to fill in loadcase
        //    - provide the source case to write for writecase
        if( Tkn == TOKDICT && case_io_node.case_dictionary_symbol_index == -1 && key_arguments.empty() )
        {
            VerifyEngineCase();
            case_io_node.case_dictionary_symbol_index = Tokstindex;
            VerifyEngineDataRepositoryWithEngineCase(engine_dictionary, GetSymbolEngineDictionary(case_io_node.case_dictionary_symbol_index));

            NextToken();

            if( function_code == FunctionCode::FNLOADCASE_CODE )
                continue;

            break;
        }

        std::optional<std::tuple<TCHAR, int>> argument_type_and_length;

        // give priority to reading dictionary items as their settings can be checked against the case IDs;
        // the check against DictionaryRelatedSymbol ensures that this is not an expression like MY_ALPHA_VAR[1:18]
        if( IsCurrentTokenVART(*this) )
        {
            VART* pVarT = VPT(Tokstindex);
            TCHAR argument_type = pVarT->IsAlpha() ? ' ' : ( pVarT->GetZeroFill() ? 'Z' : '9' );
            argument_type_and_length.emplace(argument_type, pVarT->GetLength());
            key_arguments.emplace_back(varsanal_COMPILER_DLL_TODO(pVarT->GetFmt()));
        }

        else
        {
            if( next_token_helper_result == NextTokenHelperResult::WorkString )
            {
                const WorkString& work_string = GetSymbolWorkString(Tokstindex);

                if( work_string.GetSubType() == SymbolSubType::WorkAlpha )
                    argument_type_and_length.emplace(' ', assert_cast<const WorkAlpha&>(work_string).GetLength());
            }

            else if( next_token_helper_result == NextTokenHelperResult::StringLiteral )
            {
                argument_type_and_length.emplace(' ', Tokstr.length());
            }

            key_arguments.emplace_back(-1 * CompileStringExpression());
        }

        if( argument_type_and_length.has_value() )
        {
            key_argument_types.append(SO::GetRepeatingCharacterString(std::get<0>(*argument_type_and_length), std::get<1>(*argument_type_and_length)));
        }

        else
        {
            key_length_can_be_calculated = false;
        }
    }


    // if working with a DataSource, a Case is required
    if( engine_dictionary.IsDataRepositoryObject() && case_io_node.case_dictionary_symbol_index == -1 )
        IssueError(MGF::dictionary_Case_required_to_receive_data_47308, engine_dictionary.GetName().c_str());


    // check the arguments
    if( !key_arguments.empty() )
    {
        const CDataDict& dictionary = engine_dictionary.GetDictionary();

        if( function_code == FunctionCode::FNWRITECASE_CODE )
            IssueError(MGF::deprecation_writecase_with_ids_95011, dictionary.GetName().GetString());

        size_t key_length = dictionary.GetKeyLength();

        if( ( key_argument_types.length() > key_length ) ||
            ( key_length_can_be_calculated && key_argument_types.length() != key_length ) )
        {
            IssueError(MGF::dictionary_specified_key_length_mismatch_547, static_cast<int>(key_argument_types.length()), static_cast<int>(key_length));
        }

        if( key_length_can_be_calculated )
        {
            const CDictRecord* pIdRec = dictionary.GetLevel(0).GetIdItemsRec();
            const TCHAR* key_argument_type = key_argument_types.c_str();
            bool keep_processing = true;

            for( int i = 0; keep_processing && i < pIdRec->GetNumItems(); ++i )
            {
                const CDictItem* dict_item = pIdRec->GetItem(i);
                TCHAR argument_type = ( dict_item->GetContentType() == ContentType::Alpha ) ? ' ' :
                                      ( dict_item->GetZeroFill() )                          ? 'Z' :
                                                                                              '9';

                for( UINT j = 0; keep_processing && j < dict_item->GetLen(); ++j )
                {
                    bool key_is_zero_filled_dictionary_is_not = ( *key_argument_type == 'Z' && argument_type == '9' );

                    if( key_is_zero_filled_dictionary_is_not || ( *key_argument_type == '9' && argument_type == 'Z' ) )
                    {
                        IssueWarning(MGF::dictionary_specified_key_zero_fill_mismatch_548,
                                     static_cast<int>(key_argument_type - key_argument_types.c_str() + 1),
                                     key_is_zero_filled_dictionary_is_not ? _T("") : _T("not "),
                                     dict_item->GetName().GetString());

                        keep_processing = false;
                    }

                    ++key_argument_type;
                }
            }
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    case_io_node.key_arguments_list_node = CreateListNode(key_arguments);

    return GetProgramIndex(case_io_node);
}


int LogicCompiler::CompileDictionaryFunctionsCaseIO_pre80(FunctionCode function_code)
{
    ASSERT(Tkn == TOKDICT_PRE80);

    DICT* pDicT = DPT(Tokstindex);
    VerifyDictionary(pDicT, VerifyDictionaryFlag::External_OneLevel);

    if( function_code != FNLOADCASE_CODE )
        VerifyDictionary(pDicT, VerifyDictionaryFlag::Writeable);

    NextToken();

    bool needs_index = true;

    if( Tkn == TOKLPAREN )
    {
        if( function_code == FNWRITECASE_CODE )
        {
            NextKeywordOrError({ _T("NOINDEX") });

            needs_index = false;

            NextToken();
            IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

            NextToken();
        }

        // secondary indices are no longer supported
        else
        {
            IssueError(MGF::secondary_indices_no_longer_supported_543);
        }
    }

    VerifyDictionary(pDicT, needs_index ? VerifyDictionaryFlag::NeedsIndex : VerifyDictionaryFlag::CannotHaveIndex);

    // process each argument
    std::vector<int> arguments;
    std::wstring key_argument_types;
    bool key_length_can_be_calculated = true;

    while( Tkn != TOKRPAREN )
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
        NextToken();

        std::optional<std::tuple<TCHAR, int>> argument_type_and_length;

        // give priority to reading dictionary items as their settings can be checked against the case IDs;
        // the check against DictionaryRelatedSymbol ensures that this is not an expression like MY_ALPHA_VAR[1:18]
        if( IsCurrentTokenVART(*this) )
        {
            VART* pVarT = VPT(Tokstindex);
            TCHAR argument_type = pVarT->IsAlpha() ? ' ' : ( pVarT->GetZeroFill() ? 'Z' : '9' );
            argument_type_and_length.emplace(argument_type, pVarT->GetLength());
            arguments.emplace_back(varsanal_COMPILER_DLL_TODO(pVarT->GetFmt()));
        }

        else
        {
            if( next_token_helper_result == NextTokenHelperResult::WorkString )
            {
                const WorkString& work_string = GetSymbolWorkString(Tokstindex);

                if( work_string.GetSubType() == SymbolSubType::WorkAlpha )
                    argument_type_and_length.emplace(' ', assert_cast<const WorkAlpha&>(work_string).GetLength());
            }

            else if( next_token_helper_result == NextTokenHelperResult::StringLiteral )
            {
                argument_type_and_length.emplace(' ', Tokstr.length());
            }

            arguments.emplace_back(-1 * CompileStringExpression());
        }

        if( argument_type_and_length.has_value() )
        {
            key_argument_types.append(SO::GetRepeatingCharacterString(std::get<0>(*argument_type_and_length), std::get<1>(*argument_type_and_length)));
        }

        else
        {
            key_length_can_be_calculated = false;
        }
    }

    if( !arguments.empty() )
    {
        if( function_code == FunctionCode::FNWRITECASE_CODE )
            IssueError(MGF::deprecation_writecase_with_ids_95011, pDicT->GetName().c_str());

        size_t key_length = pDicT->GetLevelsIdLen();

        if( ( key_argument_types.length() > key_length ) ||
            ( key_length_can_be_calculated && key_argument_types.length() != key_length ) )
        {
            IssueError(MGF::dictionary_specified_key_length_mismatch_547, static_cast<int>(key_argument_types.length()), static_cast<int>(key_length));
        }

        if( key_length_can_be_calculated )
        {
            const CDictRecord* pIdRec = pDicT->GetDataDict()->GetLevel(0).GetIdItemsRec();
            const TCHAR* key_argument_type = key_argument_types.c_str();
            bool keep_processing = true;

            for( int i = 0; keep_processing && i < pIdRec->GetNumItems(); ++i )
            {
                const CDictItem* dict_item = pIdRec->GetItem(i);
                TCHAR argument_type = ( dict_item->GetContentType() == ContentType::Alpha ) ? ' ' :
                                      ( dict_item->GetZeroFill() )                          ? 'Z' :
                                                                                              '9';

                for( UINT j = 0; keep_processing && j < dict_item->GetLen(); ++j )
                {
                    bool key_is_zero_filled_dictionary_is_not = ( *key_argument_type == 'Z' && argument_type == '9' );

                    if( key_is_zero_filled_dictionary_is_not || ( *key_argument_type == '9' && argument_type == 'Z' ) )
                    {
                        IssueWarning(MGF::dictionary_specified_key_zero_fill_mismatch_548,
                                     static_cast<int>(key_argument_type - key_argument_types.c_str() + 1),
                                     key_is_zero_filled_dictionary_is_not ? _T("") : _T("not "),
                                     dict_item->GetName().GetString());

                        keep_processing = false;
                    }

                    ++key_argument_type;
                }
            }
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    arguments.insert(arguments.begin(), pDicT->GetSymbolIndex());

    return CreateVariableArgumentsWithSizeNode(function_code, arguments);
}


int LogicCompiler::CompileForDictionaryLoop(TokenCode token_code)
{
    // forcase DICT_NAME [where condition] do
    // for DICT_NAME [(query_type)] do
    bool forcase = ( token_code == TOKFORCASE );
    ASSERT(forcase || token_code == TOKFOR);

    auto& for_dictionary_node = CreateNode<Nodes::ForDictionary>(forcase ? FORCASE_CODE : FOR_DICT_CODE);

    for_dictionary_node.next_st = -1;
    for_dictionary_node.query_type_or_where_expression = forcase ? -1 : static_cast<int>(SelcaseQueryType::Marked);
    for_dictionary_node.dictionary_access = 0;
    for_dictionary_node.starts_with_expression = -1;

    NextToken();

    if( Tkn != TOKDICT && Tkn != TOKDICT_PRE80 )
        IssueError(MGF::dictionary_external_expected_525);

    for_dictionary_node.data_repository_dictionary_symbol_index = Tokstindex;
    for_dictionary_node.case_dictionary_symbol_index = Tokstindex;

    if( Tkn == TOKDICT )
    {
        VerifyEngineCase();

        NextToken();

        // the data repository can be specified separate from the case
        if( Tkn == TOKIN )
        {
            NextToken();
            VerifyEngineDataRepository();

            for_dictionary_node.data_repository_dictionary_symbol_index = Tokstindex;

            NextToken();
        }

        EngineDictionary& data_repository_engine_dictionary = GetSymbolEngineDictionary(for_dictionary_node.data_repository_dictionary_symbol_index);
        VerifyEngineDataRepository(&data_repository_engine_dictionary, VerifyDictionaryFlag::External_OneLevel_NeedsIndex);

        VerifyEngineDataRepositoryWithEngineCase(data_repository_engine_dictionary,
                                                 GetSymbolEngineDictionary(for_dictionary_node.case_dictionary_symbol_index));
    }

    else
    {
        DICT* pDicT = DPT(for_dictionary_node.data_repository_dictionary_symbol_index);
        VerifyDictionary(pDicT, VerifyDictionaryFlag::External_OneLevel_NeedsIndex);

        NextToken();
    }

    Symbol& symbol = NPT_Ref(for_dictionary_node.data_repository_dictionary_symbol_index);

    LoopStackEntry loop_stack_entry = GetLoopStack().PushOnLoopStack(forcase ? LoopStackSource::ForCase : LoopStackSource::ForDictionary, &symbol);

    if( Tkn == TOKLPAREN )
    {
        if( forcase )
        {
            if( symbol.IsA(SymbolType::Dictionary) )
            {
                for_dictionary_node.dictionary_access = CompileDictionaryAccess(assert_cast<EngineDictionary&>(symbol), &for_dictionary_node.starts_with_expression);
            }

            else
            {
                for_dictionary_node.dictionary_access = CompileDictionaryAccess(assert_cast<DICT*>(&symbol), &for_dictionary_node.starts_with_expression);
            }
        }

        else
        {
            // read the selcase query type
            for_dictionary_node.query_type_or_where_expression = static_cast<int>(ReadSelcaseQueryType(*this));

            NextToken();
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();
    }

    if( forcase && Tkn == TOKWHERE )
    {
        NextToken();
        for_dictionary_node.query_type_or_where_expression = exprlog();
    }

    IssueErrorOnTokenMismatch(TOKDO, MGF::expecting_do_keyword_9);

    NextToken();

    for_dictionary_node.block_expression = instruc_COMPILER_DLL_TODO();

    if( Tkn != TOKENDDO && Tkn != TOKENDFOR )
        IssueError(MGF::expecting_enddo_keyword_10);

    NextToken();

    // ideally we would check for a semicolon but this was allowed
    // without a semicolon for a long time so it will not be checked

    return GetProgramIndex(for_dictionary_node);
}


int LogicCompiler::CompileSetAccessFirstLast(SetAction set_action)
{
    ASSERT(set_action == SetAction::Access || set_action == SetAction::First || set_action == SetAction::Last);

    auto& set_access_first_last_node = CreateNode<Nodes::SetAccessFirstLast>(FunctionCode::SET_DICT_ACCESS_CODE);

    set_access_first_last_node.next_st = -1;
    set_access_first_last_node.set_action = set_action;
    set_access_first_last_node.dictionary_access = 0;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    // dictionary name expected
    NextToken();

    if( Tkn != TOKDICT && Tkn != TOKDICT_PRE80 )
        IssueError(MGF::dictionary_expected_544);

    set_access_first_last_node.dictionary_symbol_index = Tokstindex;

    if( NPT_Ref(set_access_first_last_node.dictionary_symbol_index).IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = GetSymbolEngineDictionary(set_access_first_last_node.dictionary_symbol_index);

        if( engine_dictionary.GetSubType() == SymbolSubType::External )
        {
            VerifyEngineDataRepository(&engine_dictionary, VerifyDictionaryFlag::OneLevel);
        }

        else if( engine_dictionary.GetSubType() == SymbolSubType::Input )
        {
            // cannot use on input dictionary
            if( GetEngineAppType() == EngineAppType::Entry )
                IssueError(MGF::dictionary_set_access_use_invalid_in_entry_4015);
        }

        else
        {
            // only external and input dictionaries are allowed
            IssueError(MGF::dictionary_special_output_not_allowed_4018);
        }

        VerifyEngineDataRepository(&engine_dictionary, VerifyDictionaryFlag::NeedsIndex);

        NextToken();

        // secondary indices are no longer supported
        if( Tkn == TOKLPAREN )
            IssueError(MGF::secondary_indices_no_longer_supported_543);

        if( set_action == SetAction::Access )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            set_access_first_last_node.dictionary_access = CompileDictionaryAccess(engine_dictionary);
        }
    }

    else
    {
        DICT* pDicT = DPT(set_access_first_last_node.dictionary_symbol_index);

        if( pDicT->GetSubType() == SymbolSubType::External )
        {
            VerifyDictionary(pDicT, VerifyDictionaryFlag::OneLevel);
        }

        else if( pDicT->GetSubType() == SymbolSubType::Input )
        {
            // cannot use on input dictionary
            if( GetEngineAppType() == EngineAppType::Entry )
                IssueError(MGF::dictionary_set_access_use_invalid_in_entry_4015);
        }

        else
        {
            // only external and input dictionaries are allowed
            IssueError(MGF::dictionary_special_output_not_allowed_4018);
        }

        VerifyDictionary(pDicT, VerifyDictionaryFlag::NeedsIndex);

        NextToken();

        // secondary indices are no longer supported
        if( Tkn == TOKLPAREN )
            IssueError(MGF::secondary_indices_no_longer_supported_543);

        if( set_action == SetAction::Access )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            set_access_first_last_node.dictionary_access = CompileDictionaryAccess(pDicT);
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(set_access_first_last_node);
}


int LogicCompiler::CompileDictionaryAccess(EngineDictionary& engine_dictionary, int* starts_with_expression/* = nullptr*/)
{
    bool allow_starts_with = ( starts_with_expression != nullptr );

    std::optional<int> starts_with;
    std::optional<CaseIterationMethod> iteration_method;
    std::optional<CaseIterationOrder> iteration_order;
    std::optional<CaseIterationCaseStatus> iteration_status;

    VerifyEngineDataRepository(&engine_dictionary);

    do
    {
        if( allow_starts_with && NextKeyword({ _T("startswith") }) == 1 )
        {
            if( starts_with.has_value() )
                IssueError(MGF::option_defined_more_than_once_7017);

            NextToken();
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();
            starts_with = CompileStringExpression();
        }

        else
        {
            size_t keyword_type = NextKeywordOrError({ _T("OrderType"), _T("Order"), _T("CaseStatus") });

            auto read_next_keyword = [&](auto& value, const std::vector<const TCHAR*>& keywords)
            {
                // can't define the same type more than once
                if( value.has_value() )
                    IssueError(MGF::option_defined_more_than_once_7017);

                NextToken();
                IssueErrorOnTokenMismatch(TOKPERIOD, MGF::dot_required_to_separate_options_7018);

                value = static_cast<typename std::remove_reference_t<decltype(value)>::value_type>(NextKeywordOrError(keywords) - 1);
            };

            if( keyword_type == 1 )
            {
                read_next_keyword(iteration_method, { _T("Indexed"), _T("Sequential") });
            }

            else if( keyword_type == 2 )
            {
                read_next_keyword(iteration_order, { _T("Ascending"), _T("Descending") });
            }

            else
            {
                read_next_keyword(iteration_status, { _T("All"), _T("NotDeleted"), _T("Partial"), _T("Duplicate") });

                if( *iteration_status == CaseIterationCaseStatus::PartialsOnly )
                    engine_dictionary.GetCaseAccess()->SetUsesStatuses();
            }

            NextToken();
        }

    } while( Tkn == TOKCOMMA );

    if( allow_starts_with )
        *starts_with_expression = starts_with.value_or(-1);

    // bytes: first bit indicates if the value is set, then the value: method || order || status
    int dictionary_access = 0;

    if( iteration_method.has_value() )
        dictionary_access += ( 0x80 + static_cast<int>(*iteration_method) ) << 16;

    if( iteration_order.has_value() )
        dictionary_access += ( 0x80 + static_cast<int>(*iteration_order) ) << 8;

    if( iteration_status.has_value() )
        dictionary_access += ( 0x80 + static_cast<int>(*iteration_status) );

    return dictionary_access;
}


int LogicCompiler::CompileDictionaryAccess(DICT* pDicT, int* starts_with_expression/* = nullptr*/)
{
    bool allow_starts_with = ( starts_with_expression != nullptr );

    std::optional<int> starts_with;
    std::optional<CaseIterationMethod> iteration_method;
    std::optional<CaseIterationOrder> iteration_order;
    std::optional<CaseIterationCaseStatus> iteration_status;

    do
    {
        if( allow_starts_with && NextKeyword({ _T("startswith") }) == 1 )
        {
            if( starts_with.has_value() )
                IssueError(MGF::option_defined_more_than_once_7017);

            NextToken();
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();
            starts_with = CompileStringExpression();
        }

        else
        {
            size_t keyword_type = NextKeywordOrError({ _T("OrderType"), _T("Order"), _T("CaseStatus") });

            auto read_next_keyword = [&](auto& value, const std::vector<const TCHAR*>& keywords)
            {
                // can't define the same type more than once
                if( value.has_value() )
                    IssueError(MGF::option_defined_more_than_once_7017);

                NextToken();
                IssueErrorOnTokenMismatch(TOKPERIOD, MGF::dot_required_to_separate_options_7018);

                value = static_cast<typename std::remove_reference_t<decltype(value)>::value_type>(NextKeywordOrError(keywords) - 1);
            };

            if( keyword_type == 1 )
            {
                read_next_keyword(iteration_method, { _T("Indexed"), _T("Sequential") });
            }

            else if( keyword_type == 2 )
            {
                read_next_keyword(iteration_order, { _T("Ascending"), _T("Descending") });
            }

            else
            {
                read_next_keyword(iteration_status, { _T("All"), _T("NotDeleted"), _T("Partial"), _T("Duplicate") });

                if( *iteration_status == CaseIterationCaseStatus::PartialsOnly )
                    pDicT->GetCaseAccess()->SetUsesStatuses();
            }

            NextToken();
        }

    } while( Tkn == TOKCOMMA );

    if( allow_starts_with )
        *starts_with_expression = starts_with.value_or(-1);

    // bytes: first bit indicates if the value is set, then the value: method || order || status
    int dictionary_access = 0;

    if( iteration_method.has_value() )
        dictionary_access += ( 0x80 + static_cast<int>(*iteration_method) ) << 16;

    if( iteration_order.has_value() )
        dictionary_access += ( 0x80 + static_cast<int>(*iteration_order) ) << 8;

    if( iteration_status.has_value() )
        dictionary_access += ( 0x80 + static_cast<int>(*iteration_status) );

    return dictionary_access;
}


void LogicCompiler::VerifyDictionaryObject(const EngineDictionary* engine_dictionary/* = nullptr*/)
{
    if( engine_dictionary != nullptr )
    {
        if( Tkn != TOKDICT )
            IssueError(MGF::dictionary_expected_47301);

        engine_dictionary = &GetSymbolEngineDictionary(Tokstindex);
    }

    if( !engine_dictionary->IsDictionaryObject() )
        IssueError(MGF::dictionary_expected_not_Case_or_DataSource_name_47302, engine_dictionary->GetDictionary().GetName().GetString());
}


void LogicCompiler::VerifyEngineCase(const EngineDictionary* engine_dictionary/* = nullptr*/)
{
    if( engine_dictionary == nullptr )
    {
        if( Tkn != TOKDICT )
            IssueError(MGF::dictionary_or_Case_expected_47303);

        engine_dictionary = &GetSymbolEngineDictionary(Tokstindex);
    }

    if( !engine_dictionary->HasEngineCase() )
        IssueError(MGF::dictionary_or_Case_not_DataSource_expected_47304, engine_dictionary->GetName().c_str());
}


void LogicCompiler::VerifyEngineDataRepository(EngineDictionary* engine_dictionary/* = nullptr*/, int flags/* = 0*/)
{
    if( engine_dictionary == nullptr )
    {
        if( Tkn != TOKDICT )
            IssueError(MGF::dictionary_or_DataSource_expected_47305);

        engine_dictionary = &GetSymbolEngineDictionary(Tokstindex);
    }

    if( !engine_dictionary->HasEngineDataRepository() )
        IssueError(MGF::dictionary_or_DataSource_not_Case_expected_47306, engine_dictionary->GetName().c_str());

    EngineDataRepository& engine_data_repository = engine_dictionary->GetEngineDataRepository();

    if( ( flags & VerifyDictionaryFlag::External ) != 0 )
    {
        if( engine_dictionary->GetSubType() != SymbolSubType::External )
            IssueError(MGF::dictionary_external_expected_525);
    }

    if( ( flags & VerifyDictionaryFlag::OneLevel ) != 0 )
    {
        if( engine_dictionary->GetDictionary().GetNumLevels() > 1 )
            IssueError(MGF::dictionary_one_level_expected_545);
    }

    if( ( flags & VerifyDictionaryFlag::Writeable ) != 0 )
        engine_data_repository.SetIsWriteable();

    if( ( flags & VerifyDictionaryFlag::NeedsIndex ) != 0 )
    {
        if( engine_data_repository.GetCannotHaveIndex() )
            IssueError(MGF::dictionary_use_not_possible_with_NoIndex_549);

        engine_data_repository.SetNeedsIndex();
    }

    if( ( flags & VerifyDictionaryFlag::CannotHaveIndex ) != 0 )
    {
        if( engine_data_repository.GetNeedsIndex() )
            IssueError(MGF::dictionary_use_not_possible_because_of_NoIndex_550);

        engine_data_repository.SetCannotHaveIndex();
    }

    if( ( flags & VerifyDictionaryFlag::NotWorkingStorage ) != 0 )
    {
        if( engine_dictionary->GetSubType() == SymbolSubType::Work )
            IssueError(MGF::dictionary_use_not_valid_for_working_storage_551);
    }
}


void LogicCompiler::VerifyEngineDataRepositoryWithEngineCase(const EngineDictionary& data_repository_engine_dictionary,
                                                             const EngineDictionary& case_engine_dictionary)
{
    VerifyEngineDataRepository(const_cast<EngineDictionary*>(&data_repository_engine_dictionary));
    VerifyEngineCase(&case_engine_dictionary);

    if( !data_repository_engine_dictionary.DictionaryMatches(case_engine_dictionary) )
    {
        IssueError(MGF::dictionary_Case_dictionary_does_not_match_47307,
                   case_engine_dictionary.GetName().c_str(),
                   data_repository_engine_dictionary.GetName().c_str(),
                   data_repository_engine_dictionary.GetDictionary().GetName().GetString());
    }
}


void LogicCompiler::VerifyDictionary(DICT* pDicT, int iFlags)
{
    if( iFlags & VerifyDictionaryFlag::External )
    {
        if( pDicT->GetSubType() != SymbolSubType::External )
            IssueError(MGF::dictionary_external_expected_525);
    }

    if( iFlags & VerifyDictionaryFlag::OneLevel )
    {
        if( pDicT->maxlevel != 1 )
            IssueError(MGF::dictionary_one_level_expected_545);
    }

    if( iFlags & VerifyDictionaryFlag::Writeable )
        pDicT->SetWriteable();

    if( iFlags & VerifyDictionaryFlag::NeedsIndex )
    {
        if( pDicT->GetCannotHaveIndex() )
            IssueError(MGF::dictionary_use_not_possible_with_NoIndex_549);

        pDicT->SetNeedsIndex();
    }

    if( iFlags & VerifyDictionaryFlag::CannotHaveIndex )
    {
        if( pDicT->GetNeedsIndex() )
            IssueError(MGF::dictionary_use_not_possible_because_of_NoIndex_550);

        pDicT->SetCannotHaveIndex();
    }

    if( iFlags & VerifyDictionaryFlag::NotWorkingStorage )
    {
        if( pDicT->GetSubType() == SymbolSubType::Work )
            IssueError(MGF::dictionary_use_not_valid_for_working_storage_551);
    }
}
