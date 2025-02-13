#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "EngineExecutor.h"
#include <zEngineO/Report.h>
#include <zEngineO/Nodes/Report.h>
#include <zToolsO/Encoders.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/ValueConserver.h>
#include <zUtilO/TemporaryFile.h>


std::wstring* CIntDriver::GetReportTextBuilderWithValidityCheck(Report& report)
{
    std::wstring* report_text_builder = report.GetReportTextBuilder();

    if( report_text_builder == nullptr )
        issaerror(MessageType::Error, 48111, report.GetName().c_str(), _T("The report creation has not yet been initiated."));

    return report_text_builder;
}


double CIntDriver::exreport_save(int iExpr)
{
    const auto& report_save_node = GetNode<Nodes::Report::Save>(iExpr);
    Report& report = GetSymbolReport(report_save_node.symbol_index);
    std::wstring report_filename = EvalFullPathFileName(report_save_node.filename_expression);

    return ( GenerateReport(report, report_filename) != nullptr ) ? 1 : 0;
}


double CIntDriver::exReport_view(const int program_index)
{
    const auto& report_view_node = GetNode<Nodes::Report::View>(program_index);
    Report& report = GetSymbolReport(report_view_node.symbol_index);
    std::unique_ptr<const ViewerOptions> viewer_options = EvaluateViewerOptions(report_view_node.viewer_options_node_index);

    return exReport_view(report, viewer_options.get());
}


double CIntDriver::exReport_view(Report& report, const ViewerOptions* viewer_options)
{
    // if not creating a HTML report, which can be shown in the embedded browser,
    // save the report to a temporary file that will be deleted when the program ends
    std::optional<std::wstring> report_filename;

    if( !report.IsHtmlType() )
    {
        report_filename = GetUniqueTempFilename(PortableFunctions::PathGetFilename(report.GetFilename()));
        TemporaryFile::RegisterFileForDeletion(*report_filename);
    }

    const std::unique_ptr<std::wstring> report_text_builder = GenerateReport(report, report_filename);

    if( report_text_builder == nullptr )
        return 0;

    Viewer viewer;
    viewer.UseEmbeddedViewer();

    // view HTML contents...
    if( report.IsHtmlType() )
    {
        // in case the report uses resources specified using relative paths, set the
        // local file server root directory to where the report would have existed on the disk
        viewer.UseSharedHtmlLocalFileServer()
              .SetOptions(viewer_options)
              .ViewHtmlContent(*report_text_builder, PortableFunctions::PathGetDirectory(report.GetFilename()));
    }

    // ...or a file
    else
    {
        viewer.ViewFile(*report_filename);
    }

    return 1;
}


double CIntDriver::exreport_write(int iExpr)
{
    const auto& report_write_node = GetNode<Nodes::Report::Write>(iExpr);
    Report& report = GetSymbolReport(report_write_node.symbol_index);

    std::wstring* report_text_builder = GetReportTextBuilderWithValidityCheck(report);

    if( report_text_builder == nullptr )
        return 0;

    // write out direct report text...
    if( report_write_node.type == Nodes::Report::Write::Type::ReportText )
    {
        report_text_builder->append(GetStringLiteral(report_write_node.expression));
    }

    // ...or the results of a text fill...
    else if( report_write_node.type == Nodes::Report::Write::Type::TextFill )
    {
        std::wstring fill_text = EvaluateTextFill(report_write_node.expression);

        if( report_write_node.encode_text == 1 && report.IsHtmlType() )
            fill_text = Encoders::ToHtml(fill_text);

        report_text_builder->append(fill_text);
    }

    // ...or the results of a report.write call
    else
    {
        ASSERT(report_write_node.type == Nodes::Report::Write::Type::Write);

        report_text_builder->append(EvaluateUserMessage(report_write_node.expression, FunctionCode::REPORTFN_WRITE_CODE));
    }

    return 1;
}


std::unique_ptr<std::wstring> CIntDriver::GenerateReport(Report& report, const std::optional<std::wstring>& output_filename)
{
    auto report_text_builder = std::make_unique<std::wstring>();

    try
    {
        if( report.GetReportTextBuilder() != nullptr )
            throw CSProException(_T("Multiple instances of the %s report cannot be generated at the same time."), report.GetName().c_str());

        report.SetReportTextBuilder(report_text_builder.get());

        bool program_control_executed = Execute(
            [&]()
            {
                // run the code to generate the report
                ValueConserver field_symbol_index_conserver(m_FieldSymbol, m_iExSymbol);
                ValueConserver execution_symbol_index_conserver(m_iExSymbol, report.GetSymbolIndex());

                ExecuteProgramStatements(report.GetProgramIndex());
            });

        report.SetReportTextBuilder(nullptr);

        // if there was a program control statement executed, act as though the report could not be generated
        if( program_control_executed )
        {
            report_text_builder.reset();
        }

        // save the report if necessary
        else if( output_filename.has_value() )
        {
            FileIO::WriteText(*output_filename, *report_text_builder, true);
        }
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 48111, report.GetName().c_str(), exception.GetErrorMessage().c_str());
        report_text_builder.reset();
    }

    RethrowProgramControlExceptions();

    return report_text_builder;
}
