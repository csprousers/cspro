#include "stdafx.h"
#include "IncludesRT.h"
#include "Report.h"
#include "Nodes/Report.h"
#include <zUtilO/TemporaryFile.h>
#include <zViewO/MarkdownViewInput.h>


std::string* LogicInterpreter::GetReportTextBuilderWithValidityCheck(Report& report)
{
    std::string* const report_text_builder = report.GetReportTextBuilder();

    if( report_text_builder == nullptr )
        IssueMessage(MessageType::Error, 48111, report.GetName().c_str(), "The report creation has not yet been initiated.");

    return report_text_builder;
}


double LogicInterpreter::ex_Report_save(const int program_index)
{
    const auto& report_save_node = GetNode<Nodes::Report::Save>(program_index);
    Report& report = GetSymbolReport(report_save_node.symbol_index);
    const std::string report_file_path = EvaluatePath(report_save_node.filename_expression);

    return ( GenerateReport(report, &report_file_path) != nullptr ) ? 1 : 0;
}


double LogicInterpreter::ex_Report_view(const int program_index)
{
    const auto& report_view_node = GetNode<Nodes::Report::View>(program_index);
    Report& report = GetSymbolReport(report_view_node.symbol_index);
    const std::unique_ptr<const ViewerOptions> viewer_options = EvaluateViewerOptions(report_view_node.viewer_options_node_index);

    return ex_Report_view(report, viewer_options.get());
}


double LogicInterpreter::ex_Report_view(Report& report, const ViewerOptions* const viewer_options)
{
    // if not creating a HTML or Markdown report, which can be shown in the embedded browser,
    // save the report to a temporary file that will be deleted when the program ends
    std::unique_ptr<std::string> report_file_path;

    if( !report.IsTypeHtmlOrDerived() )
    {
        report_file_path = std::make_unique<std::string>(GetUniqueTempFilePath(PortableFunctions::PathGetFilename(report.GetFilePath())));
        TemporaryFile::RegisterFileForDeletion(*report_file_path);
    }

    const std::unique_ptr<std::string> report_text_builder = GenerateReport(report, report_file_path.get());

    if( report_text_builder == nullptr )
        return 0;

    Viewer viewer;
    viewer.UseEmbeddedViewer();

    // view HTML contents...
    if( report.IsTypeHtmlOrDerived() )
    {
        // if Markdown, convert to HTML
        if( report.IsTypeMarkdown() )
            *report_text_builder = MarkdownViewInput::ToViewableHtml(report.GetFilePath(), *report_text_builder);

        // in case the report uses resources specified using relative paths, set the
        // local file server root directory to where the report would have existed on the disk
        viewer.UseSharedHtmlLocalFileServer()
              .UseExceptionHolder(nullptr)
              .SetOptions(viewer_options)
              .ViewHtmlContent(std::move(*report_text_builder), PortableFunctions::PathGetDirectory(report.GetFilePath()));
    }

    // ...or a file
    else
    {
        viewer.ViewFile(*report_file_path);
    }

    return 1;
}


double LogicInterpreter::ex_Report_write(const int program_index)
{
    const auto& report_write_node = GetNode<Nodes::Report::Write>(program_index);
    Report& report = GetSymbolReport(report_write_node.symbol_index);

    std::string* const report_text_builder = GetReportTextBuilderWithValidityCheck(report);

    if( report_text_builder == nullptr )
        return 0;

    // write out direct report text...
    if( report_write_node.type == Nodes::Report::Write::Type::ReportText )
    {
        report_text_builder->append(*m_engineData->string_literals[report_write_node.expression]);
    }

    // ...or the results of a text fill...
    else if( report_write_node.type == Nodes::Report::Write::Type::TextFill )
    {
        const SharableString fill_text = EvaluateTextFill(report_write_node.expression);
        std::unique_ptr<std::string> escaped_fill_text;

        if( report_write_node.escape_text == 1 )
        {
            switch( report.GetEscapeType() )
            {
                case ReportFile::EscapeType::Html:
                    escaped_fill_text = Encoders::ToHtmlWorker(*fill_text);
                    break;

                case ReportFile::EscapeType::Markdown:
                    escaped_fill_text = Encoders::ToMarkdownWorker(*fill_text);
                    break;

                case ReportFile::EscapeType::Csv:
                    escaped_fill_text = Encoders::ToCsvWorker(*fill_text);
                    break;

                default:
                    ASSERT(report.GetEscapeType() == ReportFile::EscapeType::None);
                    break;
            }
        }

        report_text_builder->append(( escaped_fill_text != nullptr ) ? *escaped_fill_text :
                                                                       *fill_text);
    }

    // ...or the results of a report.write call
    else
    {
        ASSERT(report_write_node.type == Nodes::Report::Write::Type::Write);

        const SharableString fill_text = EvaluateUserMessage(report_write_node.expression, FunctionCode::REPORTFN_WRITE_CODE);
        report_text_builder->append(*fill_text);
    }

    return 1;
}


std::unique_ptr<std::string> LogicInterpreter::GenerateReport(Report& report, const std::string* const output_file_path)
{
    auto report_text_builder = std::make_unique<std::string>();

    try
    {
        if( report.GetReportTextBuilder() != nullptr )
            throw CSProException("Multiple instances of the %s report cannot be generated at the same time.", report.GetName().c_str());

        report.SetReportTextBuilder(report_text_builder.get());

#ifdef INTERPRETER_DLL_TODO
        const bool program_control_executed = Execute(
            [&]()
            {
                // run the code to generate the report
                ValueConserver field_symbol_index_conserver(m_FieldSymbol, m_iExSymbol);
                ValueConserver execution_symbol_index_conserver(m_iExSymbol, report.GetSymbolIndex());

                ExecuteProgramStatements(report.GetProgramIndex());
            });
#else
        const bool program_control_executed = Report_Evaluate_INTERPRETER_DLL_TODO(report);
#endif

        report.SetReportTextBuilder(nullptr);

        // if there was a program control statement executed, act as though the report could not be generated
        if( program_control_executed )
        {
            report_text_builder.reset();
        }

        // save the report if necessary
        else if( output_file_path != nullptr )
        {
            FileIO::WriteText(*output_file_path, *report_text_builder, true);
        }
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, 48111, report.GetName().c_str(), exception.what());
        report_text_builder.reset();
    }

    RethrowProgramControlExceptions();

    return report_text_builder;
}
