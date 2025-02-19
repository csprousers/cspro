#include "StdAfx.h"
#include "DocSetBuilder.h"


DocSetBuilderCompileAllGenerateTask::DocSetBuilderCompileAllGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec)
    :   DocSetBuilderBaseGenerateTask(std::make_unique<CSDocCompilerSettingsForBuilding>(std::move(doc_set_spec), DocBuildSettings::SettingsForCSDocQuickCompilation(), std::string()))
{
}


void DocSetBuilderCompileAllGenerateTask::OnRun()
{
    GetInterface().SetTitle(FormatText("Compiling Document Set: %s", GetDocSetSpec().GetFilePath().c_str()));
    GetInterface().SetOutputText("Compiling...");

    RunBuild();

    SharableString result_message_with_newline =
        IsCanceled()                             ? "\nCompilation Canceled!" :
        m_documentsWithCompilationErrors.empty() ? "\nCompilation Successful!" :
                                                   FormatText("\n%d CSPro Document%s %s compilation errors.",
                                                              static_cast<int>(m_documentsWithCompilationErrors.size()),
                                                              PluralizeWord(m_documentsWithCompilationErrors.size()),
                                                              PluralizeWord(m_documentsWithCompilationErrors.size(), "has", "have"));

    if( m_documentsWithCompilationErrors.empty() )
    {
        GetInterface().LogText(result_message_with_newline);
    }

    else
    {
        GetInterface().LogText(*result_message_with_newline + " The compilation errors will be shown in the Build window when the dialog is closed.");
    }

    GetInterface().SetOutputText(result_message_with_newline->substr(1));
}


void DocSetBuilderCompileAllGenerateTask::OnCSDocCompilationResult(const std::string& csdoc_file_path, const std::string& /*output_file_path*/, const CSProException& exception)
{
    const std::tuple<std::string, std::string>& file_path_and_error = m_documentsWithCompilationErrors.emplace_back(csdoc_file_path, exception.what());

    GetInterface().LogText("Compilation error (%s): %s",
                           PortableFunctions::PathGetFilename(std::get<0>(file_path_and_error)).c_str(),
                           std::get<1>(file_path_and_error).c_str());

    GetInterface().SetOutputText(FormatText("Compiling with %d errors...",
                                            static_cast<int>(m_documentsWithCompilationErrors.size())));
}
