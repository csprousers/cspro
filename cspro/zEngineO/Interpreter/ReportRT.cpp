#include "stdafx.h"
#include "IncludesRT.h"
#include "Report.h"
#include "Nodes/Report.h"
#include <zUtilO/TemporaryFile.h>
#include <zViewO/MarkdownViewInput.h>


Engine::Value LogicInterpreter::ex_Report_save(const int program_index)
{
    const auto& report_save_node = GetNode<Nodes::Report::Save>(program_index);
    Report& report = GetSymbolReport(report_save_node.symbol_index);
    const std::string report_file_path = EvaluatePath(report_save_node.filename_expression);

    return Engine::Value::Bool(
        ( GenerateReport(report, &report_file_path) != nullptr )
    );
}


Engine::Value LogicInterpreter::ex_Report_view(const int program_index)
{
    const auto& report_view_node = GetNode<Nodes::Report::View>(program_index);
    Report& report = GetSymbolReport(report_view_node.symbol_index);
    const std::unique_ptr<const ViewerOptions> viewer_options = EvaluateViewerOptions(report_view_node.viewer_options_node_index);

    return ex_Report_view(report, viewer_options.get());
}


Engine::Value LogicInterpreter::ex_Report_view(Report& report, const ViewerOptions* const viewer_options)
{
    // if not creating a HTML or Markdown report, which can be shown in the embedded browser,
    // save the report to a temporary file that will be deleted when the program ends
    const FileExtensionAnalyzer report_extension_analyser(report.GetFilePath());
    std::unique_ptr<std::string> report_file_path;

    if( !report_extension_analyser.IsTypeHtmlOrDerivable() )
    {
        report_file_path = std::make_unique<std::string>(GetUniqueTempFilePath(PortableFunctions::PathGetFilename(report.GetFilePath())));
        TemporaryFile::RegisterFileForDeletion(*report_file_path);
    }

    const std::unique_ptr<std::string> report_text_builder = GenerateReport(report, report_file_path.get());

    if( report_text_builder == nullptr )
        return Engine::Value::Bool(false);

    Viewer viewer;
    viewer.UseEmbeddedViewer();

    // view HTML contents...
    if( report_extension_analyser.IsTypeHtmlOrDerivable() )
    {
        // if Markdown, convert to HTML
        if( report_extension_analyser.IsTypeMarkdown() )
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

    return Engine::Value::Bool(true);
}


std::unique_ptr<std::string> LogicInterpreter::GenerateReport(Report& report, const std::string* const output_file_path)
{
    auto report_text_builder = std::make_unique<std::string>();

    try
    {
        if( report.GetReportTextBuilder() != nullptr )
        {
            throw CSProException("Multiple instances of the %s report cannot be generated at the same time.",
                                 report.GetName().c_str());
        }

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
            // if a Markdown report is being saved to HTML, save the converted version
            if( FileExtensions::IsFileHtml(*output_file_path) &&
                SO::EqualsNoCase(Path::GetExtension(report.GetFilePath()), FileExtensions::Markdown) )
            {
                const std::string html = MarkdownViewInput::ToSaveableHtml(report.GetFilePath(), *report_text_builder);
                FileIO::WriteText(*output_file_path, html, true);
            }

            else
            {
                FileIO::WriteText(*output_file_path, *report_text_builder, true);
            }
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
