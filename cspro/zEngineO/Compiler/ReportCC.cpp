#include "stdafx.h"
#include "IncludesCC.h"
#include "Report.h"
#include "Nodes/Report.h"


int LogicCompiler::CompileReportFunctions()
{
    // compiling report_name.save(filename);
    //           report_name.view([viewer options]);
    const FunctionCode function_code = CurrentToken.function_details->code;
    const Report& report = assert_cast<const Report&>(*CurrentToken.symbol);

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

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

    else
    {
        ASSERT(false);
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return program_index;
}


void LogicCompiler::CompileReports()
{
    if( m_engineData->application == nullptr )
    {
        ASSERT(false);
        return;
    }

    for( const ReportFile& report_files : m_engineData->application->GetReportFiles() )
        CompileReport(report_files);
}


void LogicCompiler::CompileReport(const ReportFile& report_file)
{
    Report* report = nullptr;
    std::unique_ptr<Logic::SourceBuffer> source_buffer;

    ClearSourceBuffer();
    SetCompilationUnitName(report_file.GetFilePath());

    try
    {
        // find the report symbol and tokenize the report
        report = &assert_cast<Report&>(GetSymbolTable().FindSymbolOfType(report_file.GetName(), SymbolType::Report));

        const std::string& report_text = report_file.GetTextSource().GetText();
        source_buffer = ConvertTextTemplateToSourceBuffer(report_text, true);
    }

    catch( const CSProException& exception )
    {
        // report any errors reading the report
        ReportError(MGF:: FileIO_error_163, ToString(SymbolType::Report), exception.what());
    }

    if( source_buffer == nullptr )
        return;

    SetSourceBuffer(std::move(source_buffer));

    // set the compilation unit name again because SetSourceBuffer will have cleared what was set
    // at the beginning of the method
    SetCompilationUnitName(report_file.GetFilePath());

    try
    {
        // compile the report as if it were part of PROC GLOBAL (simulating a user-defined function)
        // because we don't know when the report will be executed
        rutasync_as_global_compilation_COMPILER_DLL_TODO(*report,
            [&]()
            {
                NextToken();

                const int program_index = instruc_COMPILER_DLL_TODO();

                // if the entire buffer was not processed, issue an error
                if( Tkn != TOKEOP )
                    IssueError(MGF::statement_invalid_1);

                report->SetProgramIndex(program_index);
            });
    }
    catch(...) { ASSERT(false); }
}
