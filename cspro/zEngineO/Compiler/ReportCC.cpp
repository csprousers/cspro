#include "stdafx.h"
#include "IncludesCC.h"
#include "Report.h"
#include "ReportTokenizer.h"
#include "Nodes/Report.h"


namespace
{
    constexpr const TCHAR* ReportWriteTypeNamedArgument = _T("rwt");
}


void LogicCompiler::CheckReportIsCurrentlyWriteable(const Report& report)
{
    // report.write can only be called from a report or a user-defined function
    if( !IsCompiling(report) && !IsCompiling(SymbolType::UserFunction) )
        IssueError(MGF::Report_write_in_invalid_locatation_48104, report.GetName().c_str());
}


int LogicCompiler::CompileReportFunctions()
{
    // compiling report_name.save(filename);
    //           report_name.view([viewer options]);
    //           report_name.write(...);
    const FunctionCode function_code = CurrentToken.function_details->code;
    const Report& report = assert_cast<const Report&>(*CurrentToken.symbol);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    bool on_completion_check_for_right_parenthesis_and_read_next_token = true;
    int program_index;

    auto initialize_node = [&](auto& node) -> auto&
    {
        ASSERT(node.function_code == function_code);
        node.symbol_index = report.GetSymbolIndex();
        program_index = GetProgramIndex(node);
        return node;
    };


    // report.save expects a filename
    if( function_code == FunctionCode::REPORTFN_SAVE_CODE )
    {
        auto& report_save_node = initialize_node(CreateNode<Nodes::Report::Save>(function_code));

        NextToken();
        report_save_node.filename_expression = CompileStringExpression();
    }


    // report.view can take optional viewer options
    else if( function_code == FunctionCode::REPORTFN_VIEW_CODE )
    {
        auto& report_view_node = initialize_node(CreateNode<Nodes::Report::View>(function_code));

        report_view_node.viewer_options_node_index = CompileViewerOptions(true);

        if( report_view_node.viewer_options_node_index == -1 )
            NextToken();
    }


    // report.write can be invoked directly by a user to output filewrite-style text
    // to the report, and it can be called under-the-hood to output the report contents
    else
    {
        ASSERT(function_code == FunctionCode::REPORTFN_WRITE_CODE);

        CheckReportIsCurrentlyWriteable(report);

        auto& report_write_node = initialize_node(CreateNode<Nodes::Report::Write>(function_code));
        report_write_node.encode_text = 0;

        // named arguments will be used for the under-the-hood content specification mode
        OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);
        int dummy_report_write_type_argument = -1;

        optional_named_arguments_compiler.AddArgument(ReportWriteTypeNamedArgument, dummy_report_write_type_argument,
            [&]()
            {
                ASSERT(IsCompiling(report));

                NextToken();

                auto get_constant_int = [&]()
                {
                    if( Tkn != TOKCTE || !IsNumericConstantInteger() )
                        IssueError(MGF::Report_unsupported_functionality_48103);

                    return static_cast<int>(Tokvalue);
                };

                report_write_node.type = static_cast<Nodes::Report::Write::Type>(get_constant_int());

                NextToken();
                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                if( report_write_node.type == Nodes::Report::Write::Type::ReportText )
                {
                    NextToken();
                    report_write_node.expression = get_constant_int();

                    NextToken();
                }

                else if( report_write_node.type == Nodes::Report::Write::Type::TextFill )
                {
                    NextToken();
                    report_write_node.encode_text = get_constant_int();

                    NextToken();
                    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                    NextToken();
                    report_write_node.expression = CompileFillText();
                }

                else
                {
                    IssueError(MGF::Report_unsupported_functionality_48103);
                }

                return 1;
            });

        // if not using named arguments, the user is supplying the text directly using a report.write function call
        if( optional_named_arguments_compiler.Compile(true) == 0 )
        {
            report_write_node.type = Nodes::Report::Write::Type::Write;
            report_write_node.expression = CompileMessageFunction(function_code);

            on_completion_check_for_right_parenthesis_and_read_next_token = false;
        }
    }


    if( on_completion_check_for_right_parenthesis_and_read_next_token )
    {
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();
    }

    return program_index;
}


void LogicCompiler::CompileReports()
{
    if( m_engineData->application == nullptr )
    {
        ASSERT(false);
        return;
    }

    for( const NamedTextSource& report_named_text_source : VI_V(m_engineData->application->GetReportNamedTextSources()) )
        CompileReport(report_named_text_source);
}


void LogicCompiler::CompileReport(const NamedTextSource& report_named_text_source)
{
    Report* report = nullptr;
    std::unique_ptr<Logic::SourceBuffer> source_buffer;

    ClearSourceBuffer();
    SetCompilationUnitName(report_named_text_source.text_source->GetFilename());

    try
    {
        // find the report symbol and tokenize the report
        report = &assert_cast<Report&>(GetSymbolTable().FindSymbolOfType(report_named_text_source.name, SymbolType::Report));

        const std::wstring& report_text = report_named_text_source.text_source->GetText();

        source_buffer = ConvertReportToSourceBuffer(report_text);
    }

    catch( const CSProException& exception )
    {
        // report any errors reading the report
        ReportError(MGF:: FileIO_error_163, ToString(SymbolType::Report), exception.GetErrorMessage().c_str());
    }

    if( source_buffer == nullptr )
        return;

    SetSourceBuffer(std::move(source_buffer));

    // set the compilation unit name again because SetSourceBuffer will have cleared what was set
    // at the beginning of the method
    SetCompilationUnitName(report_named_text_source.text_source->GetFilename());

    try
    {
        // compile the report as if it were part of PROC GLOBAL (simulating a user-defined function)
        // because we don't know when the report will be executed
        rutasync_as_global_compilation_COMPILER_DLL_TODO(*report,
            [&]()
            {
                NextToken();

                int program_index = instruc_COMPILER_DLL_TODO();

                // if the entire buffer was not processed, issue an error
                if( Tkn != TOKEOP )
                    IssueError(MGF::statement_invalid_1);

                report->SetProgramIndex(program_index);
            });
    }

    catch(...)
    {
        ASSERT(false);
    }
}


namespace
{
    class EngineReportTokenizer : public ReportTokenizer
    {
    public:
        EngineReportTokenizer(LogicCompiler& logic_compiler)
            :   m_compiler(logic_compiler)
        {
        }

        void OnErrorUnbalancedEscapes(size_t line_number) override
        {
            m_compiler.ReportError(MGF::Report_unbalanced_escapes_48101, static_cast<int>(line_number));
        }

        void OnErrorTokenNotEnded(const ReportToken& report_token) override
        {
            m_compiler.ReportError(MGF::Report_end_reached_while_in_logic_or_fill_48102,
                                   ( report_token.type == ReportToken::Type::Logic ) ? _T("logic") : _T("a fill"),
                                   static_cast<int>(report_token.section_line_number_start));
        }

    private:
        LogicCompiler& m_compiler;
    };


    // maintain a mapping of the report line numbers to the CSPro logic
    struct ReportLineAdjuster : Logic::SourceBuffer::LineAdjuster
    {
        std::map<size_t, size_t> line_map;

        size_t GetLineNumber(size_t line_number) override
        {
            for( size_t i = line_number; i > 0; --i )
            {
                if( line_map.count(i) != 0 )
                {
                    // only the first output line in each section is stored, so the exact line number has to be calculated
                    return line_map.find(i)->second + ( line_number - i );
                }
            }

            return 0;
        }
    };
}


std::unique_ptr<Logic::SourceBuffer> LogicCompiler::ConvertReportToSourceBuffer(const wstring_view report_text_sv)
{
    EngineReportTokenizer report_tokenizer(*this);

    if( !report_tokenizer.Tokenize(report_text_sv, GetLogicSettings()) )
        return nullptr;

    // create the logic to run this report
    std::wstring report_logic;

    auto report_line_adjuster = std::make_unique<ReportLineAdjuster>();
    size_t source_line = 1;
    size_t output_line = 1;

    for( const ReportToken& report_token : report_tokenizer.GetReportTokens() )
    {
        size_t token_text_newlines = CountNewlines(report_token.text);

        report_line_adjuster->line_map.try_emplace(output_line, source_line);
        source_line += token_text_newlines;

        if( report_token.type == ReportToken::Type::ReportText )
        {
            // the report text will be added to the string literal conserver
            SO::AppendFormat(report_logic, _T("$.write(%s := %d, %d);"),
                                           ReportWriteTypeNamedArgument,
                                           static_cast<int>(Nodes::Report::Write::Type::ReportText),
                                           ConserveConstant(report_token.text));

            ++output_line;
        }

        else if( report_token.type == ReportToken::Type::DoubleTilde || report_token.type == ReportToken::Type::TripleTilde )
        {
            SO::AppendFormat(report_logic, _T("$.write(%s := %d, %d, %s);"),
                                           ReportWriteTypeNamedArgument,
                                           static_cast<int>(Nodes::Report::Write::Type::TextFill),
                                           ( report_token.type == ReportToken::Type::DoubleTilde ) ? 1 : 0,
                                           report_token.text.c_str());

            output_line += token_text_newlines + 1;
        }

        else
        {
            ASSERT(report_token.type == ReportToken::Type::Logic);

            report_logic.append(report_token.text);

            output_line += token_text_newlines + 1;
        }

        report_logic.push_back('\n');
    }


    auto source_buffer = std::make_unique<Logic::SourceBuffer>(std::move(report_logic));
    source_buffer->SetLineAdjuster(std::move(report_line_adjuster));

    return source_buffer;
}
