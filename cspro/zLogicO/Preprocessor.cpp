#include "stdafx.h"
#include "Preprocessor.h"
#include "BasicTokenCompiler.h"
#include "KeywordTable.h"
#include "StringLiteralParser.h"
#include <zToolsO/Special.h>
#include <zToolsO/VarFuncs.h>
#include <zEngineO/Messages/EngineMessages.h>

using namespace Logic;


namespace
{
    constexpr const TCHAR* CommandIf          = _T("if");
    constexpr const TCHAR* CommandElseIf      = _T("elseif");
    constexpr const TCHAR* CommandElse        = _T("else");
    constexpr const TCHAR* CommandEndif       = _T("endif");

    constexpr const TCHAR* CommandSetProperty = _T("setProperty");

    constexpr const TCHAR* FunctionExists     = _T("exists");
    constexpr const TCHAR* FunctionAppType    = _T("AppType");
}


Preprocessor::Preprocessor(BasicTokenCompiler& compiler)
    :   m_compiler(compiler),
        m_nextBasicTokenIndexOnError(0)
{
}


void Preprocessor::ProcessBuffer()
{
    m_nextBasicTokenIndexOnError = 0;

    try
    {
        std::vector<BasicToken>& basic_tokens = m_compiler.m_basicTokens;
        auto basic_token_itr = basic_tokens.cbegin();

        enum class IncludeType { No, Include, WaitingInclude };
        std::vector<IncludeType> include_types;

        enum class CurrentBlockType { If, Else };
        std::vector<CurrentBlockType> current_block_types;

        size_t previous_line_number = SIZE_MAX;

        while( basic_token_itr != basic_tokens.cend() )
        {
            // the # must appear as the first token on a line
            if( basic_token_itr->line_number != previous_line_number )
            {
                previous_line_number = basic_token_itr->line_number;

                // if the line started with #, then we are in the preprocessor
                if( basic_token_itr->type == BasicToken::Type::Operator && basic_token_itr->token_code == TokenCode::TOKHASH )
                {
                    // get all tokens on this line
                    const auto [command, arguments_token_itr_begin, arguments_token_itr_end] = GetCommandAndLineTokens(basic_token_itr);
                    bool preprocessor_handled_command = true;

                    // #if -> start a new block
                    if( command == CommandIf )
                    {
                        IncludeType include_type = include_types.empty() ? IncludeType::Include : include_types.back();

                        if( include_type == IncludeType::Include )
                        {
                            if( !EvaluateCondition(arguments_token_itr_begin, arguments_token_itr_end) )
                                include_type = IncludeType::WaitingInclude;
                        }

                        else
                        {
                            include_type = IncludeType::No;
                        }

                        include_types.emplace_back(include_type);
                        current_block_types.emplace_back(CurrentBlockType::If);
                    }

                    // #elseif -> a new condition for the current block
                    else if( command == CommandElseIf )
                    {
                        // no matching #if block or an #elseif following an #else
                        if( current_block_types.empty() || current_block_types.back() == CurrentBlockType::Else )
                            IssueError(63, CommandElseIf);

                        IncludeType& include_type = include_types.back();

                        if( include_type == IncludeType::Include )
                        {
                            include_type = IncludeType::No;
                        }

                        else if( include_type == IncludeType::WaitingInclude && EvaluateCondition(arguments_token_itr_begin, arguments_token_itr_end) )
                        {
                            include_type = IncludeType::Include;
                        }
                    }

                    // #else -> the last condition for the current block
                    else if( command == CommandElse )
                    {
                        // no matching #if block
                        if( current_block_types.empty() )
                            IssueError(63, CommandElse);

                        // and #else following an #else
                        CurrentBlockType& current_block_type = current_block_types.back();

                        if( current_block_type == CurrentBlockType::Else )
                            IssueError(64);

                        IncludeType& include_type = include_types.back();

                        if( include_type == IncludeType::WaitingInclude )
                        {
                            include_type = IncludeType::Include;
                        }

                        else
                        {
                            include_type = IncludeType::No;
                        }

                        current_block_type = CurrentBlockType::Else;
                    }

                    // #endif -> end the current block
                    else if( command == CommandEndif )
                    {
                        // no matching #if block
                        if( current_block_types.empty() )
                            IssueError(63, CommandEndif);

                        include_types.pop_back();
                        current_block_types.pop_back();
                    }

                    // #setProperty
                    else if( command == CommandSetProperty )
                    {
                        preprocessor_handled_command = ProcessSetProperty(true, arguments_token_itr_begin, arguments_token_itr_end);
                    }

                    else
                    {
                        ASSERT(false);
                    }

                    if( preprocessor_handled_command )
                    {
                        // erase the tokens on the preprocessed line
                        basic_token_itr = basic_tokens.erase(*m_lastHashTokenPosition, arguments_token_itr_end);
                    }

                    // reset this because the erased tokens invalidate the iterator
                    m_lastHashTokenPosition.reset();

                    // don't proceed in the loop because the current state should apply to the next token
                    continue;
                }
            }


            // handle the non-preprocessor token
            if( include_types.empty() || include_types.back() == IncludeType::Include )
            {
                ++basic_token_itr;
            }

            else
            {
                basic_token_itr = basic_tokens.erase(basic_token_itr);
            }
        }


        // check that every #if block ends with #endif
        if( !include_types.empty() )
            IssueError(65);
    }

    catch( const PreprocessorException& )
    {
    }
}


void Preprocessor::ProcessLineDuringCompilation()
{
    m_nextBasicTokenIndexOnError = m_compiler.m_nextBasicTokenIndex;

    ASSERT(m_nextBasicTokenIndexOnError > 0 && m_compiler.m_basicTokens[m_nextBasicTokenIndexOnError - 1].GetTextSV() == _T("#"));

    try
    {
        const auto hash_token_position = m_compiler.m_basicTokens.cbegin() + m_nextBasicTokenIndexOnError - 1;
        const auto [command, arguments_token_itr_begin, arguments_token_itr_end] = GetCommandAndLineTokens(hash_token_position);

        // the 1 + is for the command
        m_compiler.m_nextBasicTokenIndex += 1 + ( arguments_token_itr_end - arguments_token_itr_begin );

        // process the command
        if( SO::EqualsNoCase(command, CommandSetProperty) )
        {
            ProcessSetProperty(false, arguments_token_itr_begin, arguments_token_itr_end);
        }

        else
        {
            // the preprocessor, sort of hacked together, allows errors during the buffer processing period
            // to then be processed again during compilation (though the errors are still reported),
            // so ideally we could assert here, but that isn't possible now, and hopefully the preprocessor will
            // be refactored at some point to use the main compiler's error issuing and token reading routines
            // ASSERT(false);
        }
    }

    catch( const PreprocessorException& )
    {
    }

    m_lastHashTokenPosition.reset();
}


std::tuple<const TCHAR*, std::vector<BasicToken>::const_iterator, std::vector<BasicToken>::const_iterator> Preprocessor::GetCommandAndLineTokens(const std::vector<BasicToken>::const_iterator& hash_token_position)
{
    ASSERT(hash_token_position->GetTextSV() == _T("#"));

    m_lastHashTokenPosition = hash_token_position;

    // get all tokens on this line
    auto token_itr_begin = hash_token_position + 1;
    auto token_itr_end = token_itr_begin;

    const auto& basic_token_buffer_end = m_compiler.m_basicTokens.cend();

    while( token_itr_end != basic_token_buffer_end && token_itr_end->line_number == hash_token_position->line_number )
        ++token_itr_end;

    // it is an error if there is no command (or if the command is on the next line)
    if( token_itr_begin == token_itr_end )
        IssueError(61);

    // get the preprocessor command
    const wstring_view command_text_sv = token_itr_begin->GetTextSV();
    const TCHAR* command = nullptr;

    for( const TCHAR* possible_command_text : { CommandIf, CommandElseIf, CommandElse, CommandEndif,
                                                CommandSetProperty } )
    {
        if( SO::EqualsNoCase(command_text_sv, possible_command_text) )
        {
            command = possible_command_text;
            break;
        }
    }

    // invalid command error
    if( command == nullptr )
        IssueError(62, std::wstring(command_text_sv).c_str());

    // the argument tokens begin 1 token after the command
    return std::make_tuple(command, token_itr_begin + 1, token_itr_end);
}


// only a subset of operators will be supported by the preprocessor
namespace
{
    struct Operator
    {
        TokenCode token_code;
        int precedence;
        bool left_associative = true;
    };

    static const std::vector<Operator> Operators =
    {
        { TOKMINUS, 9,       },
        { TOKEXPOP, 8, false },
        { TOKMULOP, 7        },
        { TOKDIVOP, 7        },
        { TOKMODOP, 7        },
        { TOKADDOP, 6        },
        { TOKMINOP, 6        },
        { TOKEQOP,  5        },
        { TOKNEOP,  5        },
        { TOKLEOP,  5        },
        { TOKLTOP,  5        },
        { TOKGEOP,  5        },
        { TOKGTOP,  5        },
        { TOKNOTOP, 4        },
        { TOKANDOP, 3        },
        { TOKOROP,  2        }
    };

    const Operator* OperatorSearch(const TokenCode token_code)
    {
        const auto& operator_search = std::find_if(Operators.cbegin(), Operators.cend(),
                                                   [&](const Operator& this_operator) { return ( this_operator.token_code == token_code); });

        return ( operator_search != Operators.cend() ) ? &(*operator_search) : nullptr;
    }

    int GetPrecedence(const Preprocessor::ParsedToken parsed_token)
    {
        if( std::holds_alternative<TokenCode>(parsed_token) )
            return OperatorSearch(std::get<TokenCode>(parsed_token))->precedence;

        // use the highest precedence for functions
        ASSERT(std::holds_alternative<Preprocessor::FunctionCode>(parsed_token));
        return Operators.front().precedence + 1;
    }
}


std::vector<Preprocessor::ParsedToken> Preprocessor::ParseTokens(const std::vector<BasicToken>::const_iterator& token_itr_begin,
                                                                 const std::vector<BasicToken>::const_iterator& token_itr_end)
{
    std::vector<ParsedToken> parsed_tokens;
    std::stack<ParsedToken> operator_stack;
    bool previous_token_was_part_of_an_expression = false;

    for( auto token_itr = token_itr_begin; token_itr != token_itr_end; ++token_itr )
    {
        TokenCode token_code = token_itr->token_code;
        const wstring_view token_text_sv = token_itr->GetTextSV();

        // convert keywords to their proper token code
        const KeywordDetails* keyword_details;

        if( KeywordTable::IsKeyword(token_text_sv, &keyword_details) )
            token_code = keyword_details->token_code;

        // convert the binary minus to a unary minus if necessary
        if( token_code == TOKMINOP && !previous_token_was_part_of_an_expression )
            token_code = TOKMINUS;

        previous_token_was_part_of_an_expression = true;

        // a routine to check the next token
        auto is_next_token_code = [&](TokenCode next_token_code)
        {
            const auto next_token_itr = token_itr + 1;
            return ( next_token_itr != token_itr_end && next_token_itr->token_code == next_token_code );
        };


        // parse the tokens into Reverse Polish notation using the shunting-yard algorithm
        // (modified from https://en.wikipedia.org/wiki/Shunting-yard_algorithm)


        // if the token is a numeric constant, add it to the output
        if( token_itr->type == BasicToken::Type::NumericConstant )
        {
            parsed_tokens.emplace_back(chartodval(std::wstring(token_text_sv).c_str(), token_text_sv.length(), 0));
        }


        // if the token is a string literal, add it to the output
        else if( token_itr->type == BasicToken::Type::StringLiteral )
        {
            parsed_tokens.emplace_back(token_text_sv);
        }


        // if the token is a left parenthesis, push it to the operator stack
        else if( token_code == TOKLPAREN )
        {
            // make sure that this is not an empty set of parentheses
            if( is_next_token_code(TOKRPAREN) )
                IssueError(20);

            operator_stack.push(token_code);
        }


        // if the token is a right parenthesis...
        else if( token_code == TOKRPAREN )
        {
            // pop operators off the operator stack as long as they are not left parentheses
            while( !operator_stack.empty() )
            {
                const ParsedToken& top_operator = operator_stack.top();

                if( std::holds_alternative<TokenCode>(top_operator) && std::get<TokenCode>(top_operator) == TOKLPAREN )
                    break;

                parsed_tokens.emplace_back(top_operator);
                operator_stack.pop();
            }

            // if the stack runs out without finding a left parenthesis, then there are mismatched parentheses
            if( operator_stack.empty() )
                IssueError(59);

            // otherwise pop the left parenthesis
            operator_stack.pop();
        }


        else
        {
            const Operator* this_operator = OperatorSearch(token_code);

            // if the token is plain text...
            if( token_itr->type == BasicToken::Type::Text && this_operator == nullptr )
            {
                // if this is a function, push it to the operator stack
                const std::optional<FunctionCode> function_code =
                    SO::EqualsNoCase(token_text_sv, FunctionAppType) ? FunctionCode::AppType :
                    SO::EqualsNoCase(token_text_sv, FunctionExists)  ? FunctionCode::Exists :
                                                                       std::optional<FunctionCode>();

                if( function_code.has_value() )
                {
                    operator_stack.push(*function_code);

                    // make sure that a left parenthesis follows the function name
                    if( !is_next_token_code(TOKLPAREN) )
                        IssueError(14);
                }

                // otherwise add the text as if it were a string literal (unless is a keyword constant,
                // which will be added as a numeric constant)
                else
                {
                    const std::optional<double> keyword_constant = KeywordTable::GetKeywordConstant(token_code);

                    if( keyword_constant.has_value() )
                    {
                        if( IsSpecial(*keyword_constant) )
                            IssueError(68, _T("You cannot use special values"));

                        parsed_tokens.emplace_back(*keyword_constant);
                    }

                    else
                    {
                        parsed_tokens.emplace_back(token_text_sv);
                    }
                }
            }


            // if the token is an operator...
            else if( this_operator != nullptr )
            {
                // while there is a operator at the top of the operator stack,
                // pop operators to the output when they are...
                while( !operator_stack.empty() )
                {
                    const ParsedToken& top_operator = operator_stack.top();

                    // not a left parenthesis
                    if( !std::holds_alternative<TokenCode>(top_operator) || std::get<TokenCode>(top_operator) != TOKLPAREN )
                    {
                        int precedence_difference = GetPrecedence(top_operator) - this_operator->precedence;

                        // and the operator at the top of the operator stack has greater precedence
                        // or the operator at the top of the operator stack has equal precedence and the token is left associative
                        if( ( precedence_difference > 0 ) || ( precedence_difference == 0 && this_operator->left_associative) )
                        {
                            parsed_tokens.emplace_back(top_operator);
                            operator_stack.pop();
                            continue;
                        }
                    }

                    break;
                }

                // push the current operator to the operator stack
                operator_stack.push(this_operator->token_code);

                previous_token_was_part_of_an_expression = false;
            }


            // an invalid token
            else
            {
                IssueError(67, std::wstring(token_text_sv).c_str());
            }
        }
    }

    // while there are still operators on the stack...
    while( !operator_stack.empty() )
    {
        const ParsedToken& top_operator = operator_stack.top();

        // if the token is a left parenthesis, then there are mismatched parentheses
        if( std::holds_alternative<TokenCode>(top_operator) && std::get<TokenCode>(top_operator) == TOKLPAREN )
            IssueError(19);

        // pop the operators to the output
        parsed_tokens.emplace_back(top_operator);
        operator_stack.pop();
    }

    return parsed_tokens;
}


std::vector<Preprocessor::FunctionArgument> Preprocessor::ParseFunctionArguments(std::vector<BasicToken>::const_iterator token_itr, std::vector<BasicToken>::const_iterator token_itr_end)
{
    // make sure parentheses surround the arguments
    if( token_itr == token_itr_end || token_itr->token_code != TOKLPAREN )
        IssueError(MGF::left_parenthesis_expected_in_function_call_14);

    --token_itr_end;

    // allow a semicolon to follow the right parenthesis
    if( token_itr_end->token_code == TOKSEMICOLON )
        --token_itr_end;

    ASSERT(token_itr_end >= token_itr);

    if( token_itr_end->token_code != TOKRPAREN )
        IssueError(MGF::right_parenthesis_expected_in_function_call_17);

    ++token_itr;

    // this crude implementation returns numeric constant, string literal, and symbol arguments
    std::vector<FunctionArgument> function_arguments;
    bool next_token_must_be_comma = false;

    for( ; token_itr != token_itr_end; ++token_itr )
    {
        if( next_token_must_be_comma )
        {
            if( token_itr->token_code != TOKCOMMA )
                IssueError(MGF::function_call_comma_expected_528);

            next_token_must_be_comma = false;
            continue;
        }

        if( token_itr->type == BasicToken::Type::NumericConstant )
        {
            function_arguments.emplace_back(chartodval(token_itr->GetText(), 0));
        }

        else if( token_itr->type == BasicToken::Type::StringLiteral )
        {
            std::wstring& parsed_string_literal = std::get<std::wstring>(function_arguments.emplace_back(std::wstring()));
            StringLiteralParser::Parse(*this, parsed_string_literal, token_itr->GetTextSV());
        }

        else if( token_itr->type == BasicToken::Type::Text )
        {
            // support numeric constants that are keywords
            const KeywordDetails* keyword_details;
            std::optional<double> keyword_constant;

            if( KeywordTable::IsKeyword(token_itr->GetTextSV(), &keyword_details) )
                keyword_constant = KeywordTable::GetKeywordConstant(keyword_details->token_code);

            if( keyword_constant.has_value() )
            {
                function_arguments.emplace_back(*keyword_constant);
            }

            else
            {
                Symbol* symbol = FindSymbol(token_itr->GetText(), false);

                if( symbol == nullptr )
                    IssueError(93000, token_itr->GetText().c_str());

                function_arguments.emplace_back(symbol);
            }
        }

        else
        {
            IssueError(69, _T("only numeric constants, string literals, and symbols are supported"));
        }

        next_token_must_be_comma = true;
    }

    if( !next_token_must_be_comma )
        IssueError(MGF::function_call_missing_arguments_49);

    return function_arguments;
}


bool Preprocessor::EvaluateCondition(const std::vector<BasicToken>::const_iterator& token_itr_begin, const std::vector<BasicToken>::const_iterator& token_itr_end)
{
    const std::vector<ParsedToken> parsed_tokens = ParseTokens(token_itr_begin, token_itr_end);

    // there must be a condition specified
    if( parsed_tokens.empty() )
        IssueError(66);

    // process the tokens with a very simple interpreter
    std::vector<std::variant<double, std::wstring>> values_stack;

    auto get_value = [&]()
    {
        if( values_stack.empty() )
            IssueError(68, _T("A value was expected but none was provided"));

        std::variant<double, std::wstring> value = values_stack.back();
        values_stack.pop_back();
        return value;
    };

    auto get_numeric_value = [&]()
    {
        std::variant<double, std::wstring> value = get_value();

        if( !std::holds_alternative<double>(value) )
            IssueError(68, FormatText(_T("A numeric value was expected but the string value \"%s\" was provided"), std::get<std::wstring>(value).c_str()).GetString());

        return std::get<double>(value);
    };

    auto get_string_value = [&](bool string_literal)
    {
        std::variant<double, std::wstring> value = get_value();

        if( !std::holds_alternative<std::wstring>(value) )
            IssueError(68, FormatText(_T("A string value was expected but the numeric value %s was provided"), DoubleToString(std::get<double>(value)).c_str()).GetString());

        const std::wstring& string_value = std::get<std::wstring>(value);
        const bool is_string_literal = StringLiteralParser::IsStringLiteralStart(string_value);

        if( string_literal != is_string_literal )
        {
            if( string_literal )
            {
                IssueError(68, FormatText(_T("A string literal was expected but \"%s\" was provided"), string_value.c_str()).GetString());
            }

            else
            {
                IssueError(68, FormatText(_T("Text was expected but the string literal %s was provided"), string_value.c_str()).GetString());
            }
        }

        if( string_literal )
        {
            std::wstring parsed_string_literal;
            StringLiteralParser::Parse(*this, parsed_string_literal, string_value);
            return parsed_string_literal;
        }

        return string_value;
    };


    for( const ParsedToken& parsed_token : parsed_tokens )
    {
        // add numbers to the values stack
        if( std::holds_alternative<double>(parsed_token) )
        {
            values_stack.emplace_back(std::get<double>(parsed_token));
        }


        // add strings to the values stack
        else if( std::holds_alternative<std::wstring>(parsed_token) )
        {
            values_stack.emplace_back(std::get<std::wstring>(parsed_token));
        }


        // process functions, all of which have a single string argument
        else if( std::holds_alternative<FunctionCode>(parsed_token) )
        {
            const FunctionCode function_code = std::get<FunctionCode>(parsed_token);
            const std::wstring value = get_string_value(false);
            bool result;

            if( function_code == FunctionCode::AppType )
            {
                result = SO::EqualsNoCase(value, GetAppType());
            }

            else
            {
                ASSERT(std::get<FunctionCode>(parsed_token) == FunctionCode::Exists);

                // only count a symbol as existing if it was part of the base symbols, not symbols created in logic
                result = ( FindSymbol(value, true) != nullptr );
            }

            values_stack.emplace_back(result ? 1.0 : 0.0);
        }


        // process operators
        else
        {
            ASSERT(std::holds_alternative<TokenCode>(parsed_token));
            const TokenCode token_code = std::get<TokenCode>(parsed_token);

            // string operators
            if( values_stack.size() > 1 && std::holds_alternative<std::wstring>(values_stack[values_stack.size() - 2]) )
            {
                const std::wstring right_value = get_string_value(true);
                const std::wstring left_value = get_string_value(true);

                if( token_code == TOKADDOP )
                {
                    values_stack.emplace_back(SO::Concatenate(_T("\""), left_value, right_value, _T("\"")));
                }

                else
                {
                    const int string_comparison = left_value.compare(right_value);

                    const std::optional<bool> result =
                        ( token_code == TOKEQOP ) ? ( string_comparison == 0 ) :
                        ( token_code == TOKNEOP ) ? ( string_comparison != 0 ) :
                        ( token_code == TOKLEOP ) ? ( string_comparison <= 0 ) :
                        ( token_code == TOKLTOP ) ? ( string_comparison < 0 )  :
                        ( token_code == TOKGEOP ) ? ( string_comparison >= 0 ) :
                        ( token_code == TOKGTOP ) ? ( string_comparison > 0 )  :
                                                    std::optional<bool>();

                    if( result.has_value() )
                    {
                        values_stack.emplace_back(*result ? 1.0 : 0.0);
                    }

                    else
                    {
                        IssueError(68, _T("An operator was used with string values when only numeric values are allowed"));
                    }
                }
            }

            // numeric operators with two arguments
            else
            {
                const double right_value = get_numeric_value();
                const double left_value = ( token_code == TOKNOTOP || token_code == TOKMINUS ) ? 0 : get_numeric_value();
                double result;

                switch( token_code )
                {
                    case TOKMINUS:
                        result = -1 * right_value;
                        break;

                    case TOKEXPOP:
                    {
                        result = std::pow(left_value, right_value);

                        if( isnan(result) || !std::isfinite(result) )
                            IssueError(68, _T("The ^ operator was used with invalid values"));

                        break;
                    }

                    case TOKMULOP:
                        result = left_value * right_value;
                        break;

                    case TOKDIVOP:
                    {
                        if( right_value == 0 )
                            IssueError(68, _T("The / operator cannot be used with 0 as a denominator"));

                        result = left_value / right_value;
                        break;
                    }

                    case TOKMODOP:
                    {
                        if( right_value == 0 )
                            IssueError(68, _T("The % operator cannot be used with 0 as a denominator"));

                        result = fmod(left_value, right_value);
                        break;
                    }

                    case TOKADDOP:
                        result = left_value + right_value;
                        break;

                    case TOKMINOP:
                        result = left_value - right_value;
                        break;

                    case TOKEQOP:
                        result = ( left_value == right_value ) ? 1 : 0;
                        break;

                    case TOKNEOP:
                        result = ( left_value != right_value ) ? 1 : 0;
                        break;

                    case TOKLEOP:
                        result = ( left_value <= right_value ) ? 1 : 0;
                        break;

                    case TOKLTOP:
                        result = ( left_value < right_value ) ? 1 : 0;
                        break;

                    case TOKGEOP:
                        result = ( left_value >= right_value ) ? 1 : 0;
                        break;

                    case TOKGTOP:
                        result = ( left_value > right_value ) ? 1 : 0;
                        break;

                    case TOKNOTOP:
                        result = ( right_value == 0 ) ? 1 : 0;
                        break;

                    case TOKANDOP:
                        result = ( left_value != 0 && right_value != 0 ) ? 1 : 0;
                        break;

                    case TOKOROP:
                        result = ( left_value != 0 || right_value != 0 ) ? 1 : 0;
                        break;

                    default:
                        ASSERT(false);
                }

                values_stack.emplace_back(result);
            }
        }
    }

    if( values_stack.size() != 1 || !std::holds_alternative<double>(values_stack.front()) )
        IssueError(68, _T("The expression must evaluate to a single numeric value"));

    return ( std::get<double>(values_stack.front()) != 0 );
}


bool Preprocessor::ProcessSetProperty(const bool currently_preprocessing, const std::vector<BasicToken>::const_iterator& token_itr_begin,
                                                                          const std::vector<BasicToken>::const_iterator& token_itr_end)
{
    // for now everything can be handled during normal compilation
    if( currently_preprocessing )
        return false;

    const std::vector<FunctionArgument> function_arguments = ParseFunctionArguments(token_itr_begin, token_itr_end);

    if( function_arguments.size() < 2 || function_arguments.size() > 3 )
        IssueError(69, _T("setProperty requires two arguments, or three if specifying a symbol as the first argument"));

    auto function_arguments_itr = function_arguments.begin();

    Symbol* symbol = nullptr;

    if( function_arguments.size() == 3 )
    {
        if( !std::holds_alternative<Symbol*>(*function_arguments_itr) )
            IssueError(69, _T("setProperty requires a symbol as the first argument when specifying three arguments"));

        symbol = std::get<Symbol*>(*function_arguments_itr);
        ++function_arguments_itr;
    }

    if( !std::holds_alternative<std::wstring>(*function_arguments_itr) )
        IssueError(69, _T("setProperty requires a string literal for the attribute"));

    const std::wstring& attribute = std::get<std::wstring>(*function_arguments_itr);
    ++function_arguments_itr;

    const std::variant<double, std::wstring> value =
        [&]() -> std::variant<double, std::wstring>
        {
            if( std::holds_alternative<double>(*function_arguments_itr) )            return std::get<double>(*function_arguments_itr);
            else if( std::holds_alternative<std::wstring>(*function_arguments_itr) ) return std::get<std::wstring>(*function_arguments_itr);
            else                                                                     IssueError(69, _T("setProperty requires a numeric constant or string literal for the value"));
        }();

    SetProperty(symbol, attribute, value);

    return true;
}
