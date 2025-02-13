#include "StdAfx.h"
#include "DocSetBuilder.h"


DocSetBuilderCompileAllGenerateTask::DocSetBuilderCompileAllGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec)
    :   DocSetBuilderBaseGenerateTask(std::make_unique<CSDocCompilerSettingsForBuilding>(std::move(doc_set_spec), DocBuildSettings::SettingsForCSDocQuickCompilation(), std::wstring()))
{
}


void DocSetBuilderCompileAllGenerateTask::OnRun()
{
    GetInterface().SetTitle(_T("Compiling Document Set: ") + GetDocSetSpec().GetFilename());
    GetInterface().SetOutputText(_T("Compiling..."));

    RunBuild();

    const std::wstring result_message_with_newline =
        IsCanceled()                             ? _T("\nCompilation Canceled!") :
        m_documentsWithCompilationErrors.empty() ? _T("\nCompilation Successful!") :
                                                   FormatTextCS2WS(_T("\n%d CSPro Document%s %s compilation errors."),
                                                                   static_cast<int>(m_documentsWithCompilationErrors.size()),
                                                                   PluralizeWord(m_documentsWithCompilationErrors.size()),
                                                                   PluralizeWord(m_documentsWithCompilationErrors.size(), _T("has"), _T("have")));

    if( m_documentsWithCompilationErrors.empty() )
    {
        GetInterface().LogText(result_message_with_newline);
    }

    else
    {
        GetInterface().LogText(result_message_with_newline + _T(" The compilation errors will be shown in the Build window when the dialog is closed."));
    }

    GetInterface().SetOutputText(result_message_with_newline.substr(1));
}


void DocSetBuilderCompileAllGenerateTask::OnCSDocCompilationResult(const std::wstring& csdoc_filename, const std::wstring& /*output_filename*/, const CSProException& exception)
{
    const std::tuple<std::wstring, std::wstring>& filename_and_error = m_documentsWithCompilationErrors.emplace_back(csdoc_filename, exception.GetErrorMessage());

    GetInterface().LogText(FormatTextCS2WS(_T("Compilation error (%s): %s"),
                                           PortableFunctions::PathGetFilename(std::get<0>(filename_and_error)),
                                           std::get<1>(filename_and_error).c_str()));

    GetInterface().SetOutputText(FormatTextCS2WS(_T("Compiling with %d errors..."),
                                                 static_cast<int>(m_documentsWithCompilationErrors.size())));
}
