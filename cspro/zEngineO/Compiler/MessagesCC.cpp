#include "stdafx.h"
#include "IncludesCC.h"
#include "Nodes/Messages.h"
#include <engine/VarT.h>
#include <zMessageO/MessageEvaluator.h>
#include <zMessageO/MessageManager.h>


namespace
{
    constexpr size_t MaximumNumberSelectButtons = 5;
}


int LogicCompiler::CompileMessageFunctions()
{
    FunctionCode function_code = CurrentToken.function_details->code;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    if( function_code == FunctionCode::FNDISPLAY_CODE )
    {
        IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, MGF_TODO::m_95001, _T("DISPLAY"), _T("ERRMSG"));
    }

    else if( function_code == FunctionCode::FNWRITE_CODE )
    {
        if( m_engineData->application != nullptr )
            m_engineData->application->SetHasWriteStatements();
    }

    return CompileMessageFunction(function_code);
}


int LogicCompiler::CompileMessageFunction(FunctionCode function_code)
{
    bool compile_errmsg_node = false;

    // three functions use the message compilation, simulated as maketext, but have already read in the left parenthesis
    if( function_code == FunctionCode::FNTRACE_CODE ||
        function_code == FunctionCode::FNFILE_WRITE_CODE ||
        function_code == FunctionCode::REPORTFN_WRITE_CODE )
    {
        function_code = FunctionCode::FNMAKETEXT_CODE;
    }

    else
    {
        compile_errmsg_node = ( function_code == FunctionCode::FNDISPLAY_CODE ||
                                function_code == FunctionCode::FNERRMSG_CODE ||
                                function_code == FunctionCode::FNWARNING_CODE );

        ASSERT(compile_errmsg_node || function_code == FunctionCode::FNLOGTEXT_CODE ||
                                      function_code == FunctionCode::FNMAKETEXT_CODE ||
                                      function_code == FunctionCode::FNWRITE_CODE);
    }

    MessageManager& user_message_manager = GetUserMessageManager();

    // the first argument is one of:
    // - constant number (from the message file)
    // - variable number (from the message file) -- cannot do argument type checking
    // - string literal
    // - evaluated string                        -- cannot do argument type checking
    std::optional<int> message_number;
    std::optional<int> message_expression;
    std::optional<std::wstring> unformatted_message_text;

    NextTokenHelperResult next_token_helper_result = CheckNextTokenHelper();
    NextToken();

    // at least one argument is required
    if( Tkn == TOKRPAREN )
        IssueError(MGF_TODO::m_771);

    // constant number
    if( next_token_helper_result == NextTokenHelperResult::NumericConstantNonNegative )
    {
        // the message number must be a positive integer
        if( !IsNumericConstantInteger() || Tokvalue < 1 || Tokvalue > INT_MAX )
            IssueError(MGF_TODO::m_772, INT_MAX, Tokvalue);

        message_number = static_cast<int>(Tokvalue);

        // lookup the message in the message file and warn about invalid message numbers
        const std::wstring* unformatted_message_text_lookup = user_message_manager.GetMessageFile().GetMessageTextWithNoDefaultMessage(*message_number);

        if( unformatted_message_text_lookup != nullptr )
        {
            unformatted_message_text = *unformatted_message_text_lookup;
        }

        else
        {
            IssueWarning(MGF_TODO::m_775, *message_number);
        }

        NextToken();
    }

    // variable number
    else if( !IsCurrentTokenString() )
    {
        message_expression = exprlog();
    }

    // string literal
    else if( next_token_helper_result == NextTokenHelperResult::StringLiteral )
    {
        unformatted_message_text = Tokstr;
        message_number = user_message_manager.CreateMessageNumberForUnnumberedMessage(GetCurrentBasicTokenLineNumber(), unformatted_message_text);

        // ignore the results of the string compilation because we will use the text from the message file
        CompileStringExpression();
    }

    // evaluated string
    else
    {
        message_number = user_message_manager.CreateMessageNumberForUnnumberedMessage(GetCurrentBasicTokenLineNumber());
        message_expression = CompileStringExpression();
    }

    // process the arguments
    std::vector<int> arguments;
    std::vector<DataType> arguments_data_types;

    while( Tkn == TOKCOMMA )
    {
        NextToken();

        int variable_index = ( IsCurrentTokenVART(*this) && VPT(Tokstindex)->GetDictItem() != nullptr ) ? Tokstindex :
                                                                                                          -1;

        DataType argument_data_type = GetCurrentTokenDataType();
        arguments_data_types.emplace_back(argument_data_type);

        int argument_expression = CompileExpression(argument_data_type);

        // for variables given as arguments, store the variable's index
        if( variable_index != -1 )
        {
            auto& variable_value_node = CreateNode<Nodes::VariableValue>(FunctionCode::FN_VARIABLE_VALUE_CODE);

            variable_value_node.symbol_index = variable_index;
            variable_value_node.expression = argument_expression;

            argument_expression = GetProgramIndex(variable_value_node);
        }

        // store the argument data type along with the argument
        arguments.emplace_back(static_cast<int>(argument_data_type));
        arguments.emplace_back(argument_expression);
    }

    // check if the formats are valid
    if( unformatted_message_text.has_value() )
    {
        std::vector<MessageFormat> message_formats = GetUserMessageEvaluator().GetMessageFormats(*unformatted_message_text, false);

        if( arguments_data_types.size() != message_formats.size() )
        {
            IssueWarning(MGF_TODO::m_90003, static_cast<int>(arguments_data_types.size()), static_cast<int>(message_formats.size()));
        }

        else
        {
            for( size_t i = 0; i < message_formats.size(); ++i )
            {
                MessageFormat::Type message_type = message_formats[i].type;
                std::optional<DataType> required_data_type;

                if( message_type == MessageFormat::Type::Integer || message_type == MessageFormat::Type::Double )
                {
                    required_data_type = DataType::Numeric;
                }

                else if( message_type == MessageFormat::Type::String || message_type == MessageFormat::Type::Char )
                {
                    required_data_type = DataType::String;
                }

                if( required_data_type.has_value() && *required_data_type != arguments_data_types[i] )
                {
                    std::wstring formatter = unformatted_message_text->substr(message_formats[i].formatter_start_position,
                        message_formats[i].formatter_end_position - message_formats[i].formatter_start_position);

                    IssueWarning(MGF_TODO::m_90004, static_cast<int>(i + 1), ToString(arguments_data_types[i]), formatter.c_str());
                }
            }
        }
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    // fill the compilation node
    auto& message_node = CreateNode<Nodes::Message>(function_code);

    message_node.message_number = message_number.value_or(-1);
    message_node.message_expression = message_expression.value_or(-1);
    message_node.argument_list = CreateListNode(arguments);
    message_node.extended_message_node_index = -1;

    int message_node_program_index = GetProgramIndex(message_node);

    // we are done if not compiling an errmsg-style function
    if( !compile_errmsg_node )
    {
        NextToken();
        return message_node_program_index;
    }


    std::optional<int> denominator_symbol_index;
    std::optional<Nodes::ExtendedMessage::DisplayType> display_type;
    std::vector<int> select_button_texts;
    std::vector<int> select_movements;
    std::optional<int> select_default_button_expression;

    bool display_function = ( function_code == FunctionCode::FNDISPLAY_CODE );

    if( display_function )
        display_type = Nodes::ExtendedMessage::DisplayType::Case;

    bool read_next_token = true;
    bool extended_message_node_necessary = display_function;

    while( true )
    {
        static const std::vector<const TCHAR*> additional_clauses = { _T("DENOM"), _T("CASE"), _T("SUMMARY"), _T("SELECT") };

        size_t additional_clauses_selection = NextKeyword(additional_clauses);

        if( additional_clauses_selection == 0 )
        {
            // allow commas to separate the clauses
            NextToken();

            if( Tkn == TOKCOMMA )
                continue;

            read_next_token = false;
            break;
        }


        // denom
        else if( additional_clauses_selection == 1 )
        {
            // don't allow multiple specifications
            if( denominator_symbol_index.has_value() )
                IssueError(MGF_TODO::m_776);

            // invalid for display functions
            if( display_function )
                IssueError(MGF_TODO::m_777);

            // invalid along with case
            if( display_type == Nodes::ExtendedMessage::DisplayType::Case )
                IssueError(MGF_TODO::m_773);

            NextToken();
            IssueErrorOnTokenMismatch(TOKEQOP, MGF_TODO::m_779);

            NextToken();
            IssueErrorOnTokenMismatch(TOKVAR, MGF_TODO::m_780);

            denominator_symbol_index = Tokstindex;

            // make sure the variable is numeric and singly occurring (and not part of a non-working dictionary)
            const Symbol& symbol = NPT_Ref(Tokstindex);

            if( symbol.IsA(SymbolType::Variable) )
            {
                const VART* pVarT = assert_cast<const VART*>(&symbol);

                if( !( pVarT->IsNumeric() && pVarT->GetNumDim() == 0 && pVarT->GetSubType() == SymbolSubType::Work ) )
                    IssueError(MGF_TODO::m_781);
            }
        }


        // case + summary
        else if( bool specifying_case = ( additional_clauses_selection == 2 ); specifying_case || additional_clauses_selection == 3 )
        {
            // invalid for display functions
            if( display_function )
                IssueError(specifying_case ? MGF_TODO::m_784 : MGF_TODO::m_787);

            Nodes::ExtendedMessage::DisplayType new_display_type = specifying_case ? Nodes::ExtendedMessage::DisplayType::Case :
                                                                                     Nodes::ExtendedMessage::DisplayType::Summary;

            // don't allow multiple specifications
            if( display_type == new_display_type )
                IssueError(specifying_case ? MGF_TODO::m_782 : MGF_TODO::m_785);

            // don't allow case if summary and vice versa
            if( display_type.has_value() )
                IssueError(specifying_case ? MGF_TODO::m_783 : MGF_TODO::m_786);

            // invalid along with case
            if( specifying_case && denominator_symbol_index.has_value() )
                IssueError(MGF_TODO::m_773);

            display_type = new_display_type;

            // set the message to be generated to the summary
            if( !specifying_case && message_number.has_value() )
                user_message_manager.ShowMessageInSummary(*message_number);
        }


        // select
        else if( additional_clauses_selection == 4 )
        {
            // don't allow multiple specifications
            if( !select_button_texts.empty() )
                IssueError(MGF_TODO::m_776);

            bool has_path_for_warning_function = false;

            NextToken();
            IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

            NextToken();

            // read each button information
            while( true )
            {
                if( select_button_texts.size() == MaximumNumberSelectButtons )
                    IssueError(MGF_TODO::m_789, static_cast<int>(MaximumNumberSelectButtons));

                if( IsCurrentTokenString() )
                {
                    select_button_texts.emplace_back(CompileStringExpression());
                }

                // for some reason the original compilation allowed blank button text
                else if( Tkn == TOKCOMMA )
                {
                    select_button_texts.emplace_back(CreateStringLiteralNode(std::wstring()));
                }

                else
                {
                    IssueError(MGF_TODO::m_770);
                }

                // move past the optional comma to the movement specification
                if( Tkn == TOKCOMMA )
                    NextToken();

                if( Tkn == TOKCONTINUE || Tkn == TOKNEXT )
                {
                    has_path_for_warning_function = true;
                    select_movements.emplace_back(-1);
                    NextToken();
                }

                else if( Tkn == TOKREENTER )
                {
                    select_movements.emplace_back(CompileReenterStatement_COMPILER_DLL_TODO(false));
                    NextToken();
                }

                else if( Tkn == TOKVAR || Tkn == TOKBLOCK || Tkn == TOKGROUP || Tkn == TOKWORKSTRING )
                {
                    select_movements.emplace_back(CompileMoveStatement_COMPILER_DLL_TODO(true));
                }

                // expecting a variable
                else
                {
                    IssueError(MGF::expecting_variable_11);
                }

                if( Tkn != TOKCOMMA )
                    break;

                NextToken();
            }

            IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

            // compile the optional button index
            if( NextKeyword({ _T("DEFAULT") }) == 1 )
            {
                has_path_for_warning_function = true;

                NextToken();
                IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

                next_token_helper_result = CheckNextTokenHelper();
                NextToken();

                // verify that the index is valid
                if( next_token_helper_result == NextTokenHelperResult::NumericConstantNonNegative )
                {
                    if( !IsNumericConstantInteger() || Tokvalue < 1 || Tokvalue > select_movements.size() )
                    {
                        IssueError(MGF_TODO::m_790, ( select_movements.size() == 1 ) ? _T("1") :
                                                                                       FormatText(_T("1-%d"), static_cast<int>(select_movements.size())).GetString());
                    }
                }

                select_default_button_expression = exprlog();

                IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);
            }

            if( function_code == FunctionCode::FNWARNING_CODE && !has_path_for_warning_function )
                IssueError(MGF_TODO::m_788);
        }

        extended_message_node_necessary = true;
    }

    // insert the denominator into the message file if possible; otherwise keep it in the compilation
    // node to be processed during runtime
    if( denominator_symbol_index.has_value() && message_number.has_value() )
    {
        user_message_manager.AddDenominator(*message_number, *denominator_symbol_index);
        user_message_manager.ShowMessageInSummary(*message_number);
        denominator_symbol_index.reset();
    }


    if( read_next_token )
        NextToken();

    // fill the compilation node
    if( extended_message_node_necessary )
    {
        auto& extended_message_node = CreateNode<Nodes::ExtendedMessage>();

        extended_message_node.denominator_symbol_index = denominator_symbol_index.value_or(-1);
        extended_message_node.display_type = display_type.value_or(Nodes::ExtendedMessage::DisplayType::Default);
        extended_message_node.select_button_texts_list = CreateListNode(select_button_texts);
        extended_message_node.select_movements_list = CreateListNode(select_movements);
        extended_message_node.select_default_button_expression = select_default_button_expression.value_or(-1);

        message_node.extended_message_node_index = GetProgramIndex(extended_message_node);
    }

    return message_node_program_index;
}
