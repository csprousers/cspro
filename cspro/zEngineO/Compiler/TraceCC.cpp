#include "stdafx.h"
#include "IncludesCC.h"
#include "Nodes/Trace.h"


void LogicCompiler::CompileSetTrace()
{
    NextToken();

    // set trace;
    if( Tkn == TOKSEMICOLON )
    {
        m_tracingLogic = true;
    }

    // set trace(on);
    // set trace(off);
    else
    {
        IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

        m_tracingLogic = ( NextKeywordOrError({ _T("on"), _T("off") }) == 1 );

        NextToken();
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();
        IssueErrorOnTokenMismatch(TOKSEMICOLON, MGF::expecting_semicolon_30);
    }
}


int LogicCompiler::CompileTraceFunction()
{
    auto& trace_node = CreateNode<Nodes::Trace>(FunctionCode::FNTRACE_CODE);

    trace_node.action = Nodes::Trace::Action::UserText;
    trace_node.argument = -1;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    size_t trace_type = NextKeyword({ _T("on"), _T("off") });

    if( trace_type != 0 )
    {
        NextToken();

        if( trace_type == 1 ) // turning trace on
        {
            trace_node.action = Nodes::Trace::Action::WindowOn;

            if( Tkn == TOKCOMMA ) // then the user is passing a filename (instead of or in addition to using the console window)
            {
                trace_node.action = Nodes::Trace::Action::FileOn;

                NextToken();
                trace_node.argument = CompileStringExpression();

                if( Tkn == TOKCOMMA ) // the user wants to create a new file instead of appending to the old file
                {
                    trace_node.action = Nodes::Trace::Action::FileOnClear;

                    if( NextKeyword({ _T("clear") }) != 1 )
                        IssueError(MGF::trace_filename_argument_error_7103);

                    NextToken();
                }
            }
        }

        else // turning trace off
        {
            ASSERT(trace_type == 2);
            trace_node.action = Nodes::Trace::Action::TurnOff;
        }

        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();
    }

    else
    {
        // for the user to pass a string to trace, as in trace("we're in this %s", "function");
        trace_node.argument = CompileMessageFunction(FunctionCode::FNTRACE_CODE);
    }

    return GetProgramIndex(trace_node);
}


namespace
{
    std::wstring GetCompilerTextForTrace(TokenCode token_code, cs::span<const Logic::BasicToken> basic_tokens)
    {
        const Logic::BasicToken* basic_tokens_itr = basic_tokens.begin();
        const Logic::BasicToken* basic_tokens_end = basic_tokens.end();

        if( basic_tokens_itr == basic_tokens_end )
            return ReturnProgrammingError(std::wstring());

        std::wstring trace_text = FormatTextCS2WS(_T("%-5d:  %s"), basic_tokens_itr->line_number, basic_tokens_itr->GetText().c_str());

        bool process_one_line = false;

        // these statements don't end in a semicolon so for simplicity we'll just print one line
        switch( token_code )
        {
            case TOKIF:
            case TOKELSE:
            case TOKELSEIF:
            case TOKDO:
            case TOKWHILE:
            case TOKFOR:
            case TOKFORCASE:
                process_one_line = true;
        }

        // read the tokens and contruct the line of code
        size_t last_line_number = basic_tokens_itr->line_number;
        size_t last_next_position_in_line = basic_tokens_itr->position_in_line + basic_tokens_itr->token_length;

        for( ++basic_tokens_itr; basic_tokens_itr != basic_tokens_end; ++basic_tokens_itr )
        {
            const Logic::BasicToken& basic_token = *basic_tokens_itr;

            if( basic_token.line_number != last_line_number )
            {
                // break on a newline
                if( process_one_line )
                    break;

                // or format the trace text, adding the newline character and spacing the line out to match the line number formatting
                trace_text.append(_T("\r\n        "));
            }

            // add spacing to match the source spacing
            else
            {
                size_t spaces_to_add = basic_token.position_in_line - last_next_position_in_line;

                while( spaces_to_add-- > 0 )
                    trace_text.push_back(' ');
            }

            // add the text
            trace_text.append(basic_token.GetTextSV());

            // break on a semicolon
            if( basic_token.token_code == TokenCode::TOKSEMICOLON )
                break;

            last_line_number = basic_token.line_number;
            last_next_position_in_line = basic_token.position_in_line + basic_token.token_length;
        }

        return trace_text;
    }
}


int LogicCompiler::CreateTraceStatement()
{
    ASSERT(m_tracingLogic);

    // no need to display the code on a trace statement line
    if( Tkn == TOKFUNCTION && CurrentToken.function_details->code == FunctionCode::FNTRACE_CODE )
        return -1;

    // create the trace function node
    std::wstring trace_text = GetCompilerTextForTrace(Tkn, GetBasicTokensSpanFromCurrentToken());

    auto& trace_node = CreateNode<Nodes::Trace>(FunctionCode::FNTRACE_CODE);

    trace_node.action = Nodes::Trace::Action::LogicText;
    trace_node.argument = CreateStringLiteralNode(std::move(trace_text));

    // create the function call statement to call the trace function node
    return CompileFunctionCall(GetProgramIndex(trace_node));
}
