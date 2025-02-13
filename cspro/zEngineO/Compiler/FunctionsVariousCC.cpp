#include "stdafx.h"
#include "IncludesCC.h"
#include "AllSymbols.h"
#include "PublishDateCompilerHelper.h"
#include "Nodes/Date.h"
#include "Nodes/UserInterface.h"
#include "Nodes/Various.h"
#include <regex>


int LogicCompiler::CompileFunctionsVarious()
{
    const Logic::FunctionDetails* const function_details = CurrentToken.function_details;
    const FunctionCode function_code = CurrentToken.function_details->code;
    const Symbol* dot_notation_symbol = CurrentToken.symbol;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);


    // --------------------------------------------------------------------------
    // randomin
    // --------------------------------------------------------------------------
    if( function_code == FunctionCode::FNRANDOMIN_CODE ) // 20110721
    {
        NextToken();

        int code_expression = CompileInNodes(DataType::Numeric);

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsWithSizeNode(function_code, { code_expression } ); // NUMBER_ARGUMENTS_NOT_NEEDED_LEGACY_NODE
    }


    // --------------------------------------------------------------------------
    // randomizevs + ValueSet.randomize
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNRANDOMIZEVS_CODE ) // 20110811
    {
        std::vector<int> arguments;
        int& symbol_index = arguments.emplace_back();
        DataType data_type;

        if( dot_notation_symbol != nullptr )
        {
            ASSERT(dot_notation_symbol->IsA(SymbolType::ValueSet));

            symbol_index = dot_notation_symbol->GetSymbolIndex();
            data_type = assert_cast<const ValueSet&>(*dot_notation_symbol).GetDataType();

            NextToken();
        }

        else
        {
            NextToken();

            if( Tkn == TOKDICT_PRE80 || Tkn == TOKFORM || Tkn == TOKGROUP || Tkn == TOKBLOCK )
            {
                symbol_index = Tokstindex;
                data_type = DataType::Numeric;
            }

            else if( Tkn == TOKVALUESET || IsCurrentTokenVART(*this) )
            {
                symbol_index = Tokstindex;
                data_type = SymbolCalculator::GetDataType(NPT_Ref(symbol_index));
            }

            else
            {
                IssueError(MGF::randomizevs_invalid_element_91710);
            }

            NextToken();

            if( Tkn == TOKCOMMA )
            {
                NextToken();
                IssueErrorOnTokenMismatch(TOKEXCLUDE, MGF::randomizevs_exclude_expected_91711);
            }
        }

        if( Tkn == TOKEXCLUDE )
        {
            NextToken();

            IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

            do
            {
                NextToken();
                arguments.emplace_back(CompileExpression(data_type));

            } while( Tkn == TOKCOMMA );

            IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

            NextToken();
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsWithSizeNode(function_code, arguments);
    }


    // --------------------------------------------------------------------------
    // changekeyboard
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNCHANGEKEYBOARD_CODE ) // 20120820
    {
        IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, MGF::deprecation_use_setProperty_95015, _T("Keyboard"));

        NextToken();

        int symbol_index;
        int keyboard_id;

        if( ( Tkn == TOKDICT_PRE80 || Tkn == TOKFORM || Tkn == TOKGROUP || Tkn == TOKBLOCK ) ||
            ( Tkn == TOKVAR && VPT(Tokstindex)->SYMTfrm > 0 ) )
        {
            symbol_index = Tokstindex;
        }

        else
        {
            IssueError(MGF_TODO::m_91703);
        }

        NextToken();

        // getting
        if( Tkn == TOKRPAREN )
        {
            // getting only allowed for fields
            if( !NPT_Ref(symbol_index).IsA(SymbolType::Variable) )
                IssueError(MGF_TODO::m_91704);

            keyboard_id = -1;
        }

        // setting
        else
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();

            keyboard_id = exprlog();
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { keyboard_id, symbol_index });
    }


    // --------------------------------------------------------------------------
    // getos
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNGETOS_CODE )
    {
        auto& va_node = CreateVariableSizeNode<Nodes::VariableArguments>(function_code, 2);

        va_node.arguments[0] = -1;

        std::optional<SymbolType> next_token_symbol_type = GetNextTokenSymbolType();
        NextToken();

        // the user can specify a hashmap...
        if( next_token_symbol_type.has_value() && *next_token_symbol_type == SymbolType::HashMap )
        {
            const LogicHashMap& hashmap = GetSymbolLogicHashMap(Tokstindex);

            if( !hashmap.IsValueTypeString() ||
                hashmap.GetNumberDimensions() != 1 ||
                !hashmap.DimensionTypeHandles(0, DataType::String) )
            {
                IssueError(MGF::getos_invalid_argument_1201);
            }

            va_node.arguments[0] = -2;
            va_node.arguments[1] = hashmap.GetSymbolIndex();

            NextToken();
        }

        // ...or a string variable to store detailed information about the operating system
        else if( Tkn != TOKRPAREN )
        {
            va_node.arguments[0] = CompileDestinationVariable(DataType::String);
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(va_node);
    }


    // --------------------------------------------------------------------------
    // setvalue / getvalue + getvaluenumeric / getvaluealpha
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSETVALUE_CODE ||
             function_code == FunctionCode::FNGETVALUE_CODE ||
             function_code == FunctionCode::FNGETVALUEALPHA_CODE ) // 20140228 + 20140422
    {
        std::vector<int> arguments;

        NextToken();

        arguments.emplace_back(CompileStringExpression());

        int& value_expression = arguments.emplace_back(-1);

        if( function_code == FunctionCode::FNSETVALUE_CODE )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();

            value_expression = IsCurrentTokenString() ? ( -1 * CompileStringExpression() ) : exprlog();
        }

        // 20140422 allow occurrences to be specified as parameters (in addition to the in the string text)
        for( int i = 0; i < 2 && Tkn == TOKCOMMA; ++i )
        {
            NextToken();
            arguments.emplace_back(exprlog());
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        // 20140429 because we don't know what symbol may be passed as an alpha parameter, mark all the variables as used
        MarkAllDictionaryItemsAsUsed();

        return CreateVariableArgumentsWithSizeNode(function_code, arguments);
    }


    // --------------------------------------------------------------------------
    // accept
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNACCEPT_CODE )
    {
        std::vector<int> arguments;

        NextToken();

        arguments.emplace_back(CompileStringExpression());

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
        NextToken();

        // list
        if( next_token_helper_result == NextTokenHelperResult::List )
        {
            const LogicList& accept_list = GetSymbolLogicList(Tokstindex);

            if( !accept_list.IsString() )
                IssueError(MGF::List_not_correct_data_type_961, ToString(DataType::String));

            arguments.emplace_back(-1 * accept_list.GetSymbolIndex());

            NextToken();
        }

        // array
        else if( next_token_helper_result == NextTokenHelperResult::Array )
        {
            const LogicArray& accept_array = GetSymbolLogicArray(Tokstindex);

            if( !accept_array.IsString() )
                IssueError(MGF::Array_not_correct_data_type_955, ToString(DataType::String));

            if( accept_array.GetNumberDimensions() != 1 )
                IssueError(MGF::Array_not_correct_dimensions_956, ToString(DataType::String), _T("1"));

            arguments.emplace_back(-1 * accept_array.GetSymbolIndex());

            NextToken();
        }

        // the original accept list, comprised of alpha strings
        else
        {
            while( true )
            {
                arguments.emplace_back(CompileStringExpression());

                if( Tkn != TOKCOMMA )
                    break;

                NextToken();
            }
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsWithSizeNode(function_code, arguments);
    }


    // --------------------------------------------------------------------------
    // length
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNLENGTH_CODE )
    {
        // length can return the length of a list or array, in addition to the
        // more common use of returning the length of a string
        std::vector<int> arguments;

        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();

        NextToken();

        // list
        if( next_token_helper_result == NextTokenHelperResult::List )
        {
            arguments.emplace_back(-1 * Tokstindex);
            NextToken();
        }

        // array
        else if( next_token_helper_result == NextTokenHelperResult::Array )
        {
            const LogicArray& logic_array = GetSymbolLogicArray(Tokstindex);

            arguments.emplace_back(-1 * Tokstindex);
            NextToken();

            // the dimension of an array can be specified
            if( Tkn != TOKCOMMA )
            {
                // and must be specified for multidimensional arrays
                if( logic_array.GetNumberDimensions() > 1  )
                    IssueError(MGF::Array_specify_dimension_8226);

                // if not specified, default to the first dimension
                arguments.emplace_back(CreateNumericConstantNode(1));
            }

            else
            {
                NextToken();
                arguments.emplace_back(exprlog());
            }
        }

        // string
        else
        {
            arguments.emplace_back(CompileStringExpression());
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsWithSizeNode(function_code, arguments); // NUMBER_ARGUMENTS_NOT_NEEDED_LEGACY_NODE
    }


    // --------------------------------------------------------------------------
    // publishdate
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNPUBLISHDATE_CODE )
    {
        // there is no interpreted function associated with pubishdate; we will simply insert the value as a double
        double publish_date = LogicCompiler::GetCompilerHelper<PublishDateCompilerHelper>().GetPublishDate();

        NextToken();
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateNumericConstantNode(publish_date);
    }


    // --------------------------------------------------------------------------
    // connection
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNCONNECTION_CODE )
    {
        auto& connection_node = CreateNode<Nodes::Connection>(function_code);

        if( IsNextToken(TOKRPAREN) )
        {
            connection_node.connection_type = Nodes::Connection::Any;
        }

        else
        {
            size_t specified_connection_type = NextKeywordOrError({ _T("Mobile"), _T("Wifi") });
            connection_node.connection_type = ( specified_connection_type == 1 ) ? Nodes::Connection::Mobile :
                                                                                   Nodes::Connection::WiFi;
        }

        NextToken();
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(connection_node);
    }


    // --------------------------------------------------------------------------
    // prompt
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNPROMPT_CODE )
    {
        // arguments: title (string expression), optional initial value (string expression), optional flags
        auto& prompt_node = CreateNode<Nodes::Prompt>(function_code);

        NextToken();

        prompt_node.title_expression = CompileStringExpression();
        prompt_node.initial_value_expression = -1;
        prompt_node.flags = 0;

        if( Tkn != TOKRPAREN )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            const std::vector<const TCHAR*> PromptCommands = { _T("Numeric"),   _T("Password"),
                                                               _T("Uppercase"), _T("Multiline") };
            size_t prompt_type = NextKeyword(PromptCommands);

            if( prompt_type == 0 )
            {
                NextToken();
                prompt_node.initial_value_expression = CompileStringExpression();
            }

            while( Tkn != TOKRPAREN )
            {
                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                if( prompt_type == 0 )
                    prompt_type = NextKeywordOrError(PromptCommands);

                prompt_node.flags |= ( prompt_type == 1 ) ? Nodes::Prompt::NumericFlag :
                                     ( prompt_type == 2 ) ? Nodes::Prompt::PasswordFlag :
                                     ( prompt_type == 3 ) ? Nodes::Prompt::UppercaseFlag:
                                                            Nodes::Prompt::MultilineFlag;

                NextToken();

                prompt_type = 0;
            }
        }

        if( ( prompt_node.flags & Nodes::Prompt::PasswordFlag ) != 0 &&
            ( prompt_node.flags & Nodes::Prompt::MultilineFlag ) != 0 )
        {
            IssueError(MGF::prompt_invalid_combination_51102);
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(prompt_node);
    }


    // --------------------------------------------------------------------------
    // savepartial
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSAVEPARTIAL_CODE )
    {
        if( !IsNoLevelCompilation() )
        {
            if( GetCompilationLevelNumber_base1() < 1 )
                IssueError(MGF::savepartial_invalid_in_level0_8050);

            // savepartial can only be called from within a field
            if( GetCompilationSymbolType() != SymbolType::Variable )
                IssueError(MGF::savepartial_invalid_in_proc_8054);
        }

        bool clear_skipped = ( NextKeyword({ _T("CLEAR") }) == 1 );

        NextToken();
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { clear_skipped ? 1 : 0 });
    }


    // --------------------------------------------------------------------------
    // savesetting / loadsetting
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSAVESETTING_CODE ||
             function_code == FunctionCode::FNLOADSETTING_CODE )
    {
        std::vector<int> arguments;

        if( function_code == FunctionCode::FNSAVESETTING_CODE && NextKeyword({ _T("CLEAR") }) == 1 )
            arguments.emplace_back(-1);

        NextToken();

        if( arguments.empty() )
        {
            // read the key
            arguments.emplace_back(CompileStringExpression());

            if( function_code == FunctionCode::FNSAVESETTING_CODE && Tkn != TOKCOMMA )
                IssueError(MGF::function_call_comma_expected_528);

            // read the savesetting value, or the loadsetting default value
            if( Tkn == TOKCOMMA )
            {
                NextToken();

                DataType value_data_type = GetCurrentTokenDataType();
                arguments.emplace_back(static_cast<int>(value_data_type));
                arguments.emplace_back(CompileExpression(value_data_type));
            }

            else
            {
                arguments.emplace_back(-1);
            }
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, arguments);
    }


    // --------------------------------------------------------------------------
    // compress
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNCOMPRESS_CODE )
    {
        NextToken();

        int zip_filename_expression = CompileStringExpression();

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();

        int files_expression;

        if( Tkn == TOKLIST )
        {
            if( !GetSymbolLogicList(Tokstindex).IsString() )
                IssueError(MGF::List_not_correct_data_type_961, ToString(DataType::String));

            files_expression = -1 * Tokstindex;

            NextToken();
        }

        else
        {
            files_expression = CompileStringExpression();
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { zip_filename_expression, files_expression });
    }


    // --------------------------------------------------------------------------
    // uuid
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNUUID_CODE )
    {
        int dictionary_symbol_index = -1;

        NextToken();

        if( Tkn != TOKRPAREN )
        {
            if( Tkn == TOKDICT )
            {
                VerifyEngineCase();
            }

            else if( Tkn != TOKDICT_PRE80 )
            {
                IssueError(MGF::dictionary_expected_544);
            }

            dictionary_symbol_index = Tokstindex;

            NextToken();
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { dictionary_symbol_index });
    }


    // --------------------------------------------------------------------------
    // sqlquery
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSQLQUERY_CODE )
    {
        NextToken();

        return CompileSqlQueryFunction();
    }


    // --------------------------------------------------------------------------
    // tr
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNTR_CODE )
    {
        NextToken();

        DataType value_data_type = GetCurrentTokenDataType();
        int expression = CompileExpression(value_data_type);

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { static_cast<int>(value_data_type), expression });
    }


    // --------------------------------------------------------------------------
    // timestring
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNTIMESTRING_CODE )
    {
        int timestamp_expression = -1;
        int format_expression = -1;

        NextToken();

        if( Tkn != TOKRPAREN )
        {
            for( int i = 0; i < 2; ++i )
            {
                if( format_expression == -1 && IsCurrentTokenString() ) // the formatting string
                {
                    format_expression = CompileStringExpression();
                }

                else if( timestamp_expression == -1 ) // the timestamp
                {
                    timestamp_expression = exprlog();
                }

                else
                {
                    IssueError(MGF::function_call_arguments_count_mismatch_50);
                }

                if( Tkn != TOKCOMMA )
                    break;

                NextToken();
            }
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { timestamp_expression, format_expression });
    }


    // --------------------------------------------------------------------------
    // getvaluelabel
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNGETVALUELABEL_CODE )
    {
        // read in a dictionary item
        NextToken();

        const VART* pVarT = IsCurrentTokenVART(*this) ? VPT(Tokstindex) : nullptr;

        if( pVarT == nullptr || pVarT->GetDictItem() == nullptr )
            IssueError(MGF::dictionary_item_expected_8100);

        // get the item's current value
        int value_expression = CompileExpression(pVarT->GetDataType());

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { pVarT->GetSymbolIndex(), value_expression });
    }


    // --------------------------------------------------------------------------
    // regexmatch
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNREGEXMATCH_CODE )
    {
        NextToken();

        int target_expression = CompileStringExpression();

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();

        int regex_expression = CompileStringExpressionWithStringLiteralCheck(
            [&](const std::wstring& text)
            {
                try
                {
                    // throws an exception if the regular expression syntax is invalid
                    std::regex regex_tester(UTF8Convert::WideToUTF8(text));
                }

                catch( const std::regex_error& )
                {
                    IssueError(MGF::regex_invalid_100260, text.c_str());
                }
            });

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { target_expression, regex_expression });
    }


    // --------------------------------------------------------------------------
    // // protect
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNPROTECT_CODE )
    {
        if( CheckNextTokenHelper() != NextTokenHelperResult::DictionaryRelatedSymbol )
            IssueError(MGF::dictionary_form_symbol_expected_1108);

        NextToken();
        int symbol_index = Tokstindex;

        NextToken();
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();
        int protect_expression = exprlog();

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { symbol_index, protect_expression });
    }


    // --------------------------------------------------------------------------
    // hash
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNHASH_CODE )
    {
        auto& hash_node = CreateNode<Nodes::Hash>(function_code);

        hash_node.length_expression = -1;
        hash_node.salt_expression = -1;

        NextToken();

        hash_node.value_data_type = GetCurrentTokenDataType();
        hash_node.value_expression = CompileExpression(hash_node.value_data_type);

        if( Tkn == TOKCOMMA )
        {
            NextToken();

            hash_node.length_expression = exprlog();

            if( Tkn == TOKCOMMA )
            {
                NextToken();

                hash_node.salt_expression = CompileStringExpression();
            }
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(hash_node);
    }


    // --------------------------------------------------------------------------
    // encode
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNENCODE_CODE )
    {
        auto& encode_node = CreateNode<Nodes::Encode>(function_code);

        // get the optional encoding type (if specified)
        encode_node.encoding_type = static_cast<Nodes::EncodeType>(NextKeyword({ _T("HTML"),
                                                                                 _T("CSV"),
                                                                                 _T("PercentEncoding"),
                                                                                 _T("URI"),
                                                                                 _T("URIComponent"),
                                                                                 _T("Slashes"),
                                                                                 _T("JsonString") }));
        encode_node.string_expression = -1;

        NextToken();

        if( encode_node.encoding_type != Nodes::EncodeType::Default && Tkn != TOKRPAREN )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
            NextToken();
        }

        if( encode_node.encoding_type == Nodes::EncodeType::Default || Tkn != TOKRPAREN )
            encode_node.string_expression = CompileStringExpression();

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(encode_node);
    }


    // --------------------------------------------------------------------------
    // view
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNVIEW_CODE )
    {
        auto& view_node = CreateNode<Nodes::View>(function_code);
        const Symbol* symbol_for_data_access_check = nullptr;

        NextToken();

        if( Tkn == TOKDICT_PRE80 || Tkn == TOKDOCUMENT || Tkn == TOKFREQ || Tkn == TOKIMAGE || Tkn == TOKREPORT ) // ENGINECR_TODO implement for non-DICT
        {
            Symbol& symbol = NPT_Ref(Tokstindex);

            view_node.symbol_index_or_source_expression = -1 * symbol.GetSymbolIndex();
            view_node.subscript_compilation = CurrentToken.symbol_subscript_compilation;

            if( Tkn == TOKDICT_PRE80 )
            {
                symbol_for_data_access_check = &symbol;
                assert_cast<DICT&>(symbol).GetCaseAccess()->SetRequiresFullAccess();
            }

            NextToken();
        }

        else
        {
            view_node.symbol_index_or_source_expression = CompileStringExpression();
        }

        view_node.viewer_options_node_index = CompileViewerOptions(false);

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        int program_index = GetProgramIndex(view_node);

        if( symbol_for_data_access_check != nullptr )
            program_index = WrapNodeAroundValidDataAccessCheck(program_index, *symbol_for_data_access_check, function_details->return_data_type);

        return program_index;
    }


    // --------------------------------------------------------------------------
    // setoutput
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSETOUTPUT_CODE )
    {
        // if using setoutput, which is only valid in batch-style applications...
        EngineAppType engine_app_type = GetEngineAppType();

        if( engine_app_type == EngineAppType::Batch || engine_app_type == EngineAppType::Tabulation )
        {
            // ...all input data must be read
            GetInputDictionary(true)->GetCaseAccess()->SetRequiresFullAccess();
        }

        else
        {
            IssueWarning(MGF::setoutput_valid_only_for_batch_29006);
        }

        int output_expression;

        NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
        NextToken();

        if( next_token_helper_result == NextTokenHelperResult::List )
        {
            const LogicList& setoutput_list = GetSymbolLogicList(Tokstindex);

            if( !setoutput_list.IsString() )
                IssueError(MGF::List_not_correct_data_type_961, ToString(DataType::String));

            output_expression = -1 * setoutput_list.GetSymbolIndex();

            NextToken();
        }

        else
        {
            output_expression = CompileStringExpression();
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { output_expression });
    }


    // --------------------------------------------------------------------------
    // htmldialog
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNHTMLDIALOG_CODE )
    {
        IssueWarning(Logic::ParserMessage::Type::DeprecationMajor, MGF::deprecation_use_CS_UI_showDialog_95006);

        // arguments: filename (string expression), optional input data (string expression), optional flags
        auto& html_dialog_node = CreateNode<Nodes::HtmlDialog>(function_code);
        InitializeNode(html_dialog_node, -1, 2);

        NextToken();

        html_dialog_node.filename_expression = CompileStringExpression();

        OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);

        optional_named_arguments_compiler.AddArgumentJsonText(JK::inputData, html_dialog_node.input_data_expression);
        optional_named_arguments_compiler.AddArgumentJsonText(JK::displayOptions, html_dialog_node.display_options_json_expression);

        // if no optional arguments were encountered, but there is another argument, it should
        // be the input data (or a JSON object with nodes for inputData and/or displayOptions)
        if( Tkn == TOKCOMMA && optional_named_arguments_compiler.Compile() == 0 )
        {
            NextToken();

            html_dialog_node.input_data_expression = CompileJsonText();
            html_dialog_node.single_input_version = 1;
        }

        optional_named_arguments_compiler.Compile();

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(html_dialog_node);
    }


    // --------------------------------------------------------------------------
    // timestamp
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNTIMESTAMP_CODE )
    {
        auto& timestamp_node = CreateNode<Nodes::Timestamp>(function_code);

        timestamp_node.type = Nodes::Timestamp::Type::Current;

        NextToken();

        // without arguments, the function returns the current timestamp; alternatively, possible arguments include:
        // - a RFC 3339 string;
        // - the year/month/day, optionally the hour/minute/second, and optionally the UTC offset (in hours)
        if( Tkn != TOKRPAREN )
        {
            if( IsCurrentTokenString() )
            {
                timestamp_node.type = Nodes::Timestamp::Type::RFC3339;
                timestamp_node.argument = CompileStringExpression();
            }

            else
            {
                timestamp_node.type = Nodes::Timestamp::Type::SpecifiedDate;

                std::vector<int> arguments;

                auto read_numeric = [&]()
                {
                    if( Tkn == TOKRPAREN )
                    {
                        IssueError(MGF::timestamp_argument_error_51111);
                    }

                    else if( !arguments.empty() )
                    {
                        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
                        NextToken();
                    }

                    arguments.emplace_back(exprlog());
                };

                // read the year/month/day
                for( int i = 0; i < 3; ++i )
                    read_numeric();

                // set up the optional named arguments compiler to allow for the setting of the UTC offset
                OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);
                int utc_offset_expression = -1;

                optional_named_arguments_compiler.AddArgument(_T("utcOffset"), utc_offset_expression, DataType::Numeric);

                // if no optional arguments were encountered, but there are additional arguments, read the the hour/minute/second
                while( Tkn == TOKCOMMA && optional_named_arguments_compiler.Compile() == 0 && arguments.size() < 6 )
                    read_numeric();

                arguments.emplace_back(utc_offset_expression);

                timestamp_node.argument = CreateListNode(arguments);
            }
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return GetProgramIndex(timestamp_node);
    }


    // --------------------------------------------------------------------------
    // inc
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNINC_CODE )
    {
        NextToken();

        int reference_expression = CompileDestinationVariable(DataType::Numeric);
        int increment_expression = -1;

        if( Tkn == TOKCOMMA )
        {
            NextToken();

            increment_expression = exprlog();
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, { reference_expression, increment_expression });
    }


    // --------------------------------------------------------------------------
    // unhandled function code
    // --------------------------------------------------------------------------
    else
    {
        return ReturnProgrammingError(-1);
    }
}
