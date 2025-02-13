#include "StdAfx.h"
#include "CSDocCompiler.h"
#include "CSDocCompilerWorker.h"
#include "PdfCreator.h"


// --------------------------------------------------------------------------
// CSDocCompiler
// --------------------------------------------------------------------------

std::wstring CSDocCompiler::CompileToHtml(CSDocCompilerSettings& settings, std::wstring csdoc_filename, wstring_view csdoc_text_sv)
{
    RAII::PushOnVectorAndPopOnDestruction<std::wstring> filename_holder = settings.SetCompilationFilename(std::move(csdoc_filename));

    CSDocCompilerWorker worker(settings, csdoc_text_sv);
    return worker.CreateHtml();
}



// --------------------------------------------------------------------------
// CSDocCompilerBuildToFileGenerateTask
// --------------------------------------------------------------------------

CSDocCompilerBuildToFileGenerateTask::CSDocCompilerBuildToFileGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, DocBuildSettings base_build_settings,
                                                                           std::wstring csdoc_filename, std::wstring csdoc_text, std::wstring output_filename)
    :   m_docSetSpec(std::move(doc_set_spec)),
        m_baseBuildSettings(std::move(base_build_settings)),
        m_csdocFilename(std::move(csdoc_filename)),
        m_csdocText(std::move(csdoc_text)),
        m_outputFilename(std::move(output_filename))
{
}


void CSDocCompilerBuildToFileGenerateTask::ValidateInputs()
{
    if( m_outputFilename.empty() )
        throw CSProException("You must specify an output filename.");

    FileIO::CreateDirectoriesForFile(m_outputFilename);

    const std::wstring extension = PortableFunctions::PathGetFileExtension(m_outputFilename);

    const DocBuildSettings::BuildType build_type =
        SO::EqualsNoCase(extension, FileExtensions::HTML) ? DocBuildSettings::BuildType::HtmlPages :
        SO::EqualsNoCase(extension, FileExtensions::PDF)  ? DocBuildSettings::BuildType::Pdf :
        throw CSProException(_T("There is no routine for converting CSPro Documents to files with the extension: ") + extension);

    // create build settings by using the defaults for building and applying the user-specified settings on top
    DocBuildSettings default_build_settings = ( build_type == DocBuildSettings::BuildType::HtmlPages ) ? DocBuildSettings::DefaultSettingsForCSDocBuildToHtml() :
                                                                                                         DocBuildSettings::DefaultSettingsForCSDocBuildToPdf();
    DocBuildSettings build_settings = DocBuildSettings::ApplySettings(std::move(default_build_settings), m_baseBuildSettings);

    m_csdocCompilerSettingsForBuilding = CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(m_docSetSpec, build_settings, build_type, std::wstring(), true);

    if( build_type == DocBuildSettings::BuildType::Pdf )
        m_pdfCreator = std::make_unique<PdfCreator>(*this);
}


void CSDocCompilerBuildToFileGenerateTask::OnRun()
{
    GetInterface().SetTitle(_T("Building CSPro Document: ") + m_csdocFilename);

    ValidateInputs();
    ASSERT(m_csdocCompilerSettingsForBuilding != nullptr);

    GetInterface().LogText(_T("Compiling CSPro Document: ") + m_csdocFilename);

    const bool building_to_html = ( m_csdocCompilerSettingsForBuilding->GetBuildSettings().GetBuildType() == DocBuildSettings::BuildType::HtmlPages );

    ASSERT(building_to_html || m_pdfCreator != nullptr);
    const std::wstring& html_filename = building_to_html ? m_outputFilename :
                                                           m_pdfCreator->CreateTemporaryHtmlFilename(1);

    m_csdocCompilerSettingsForBuilding->SetOutputFilename(html_filename);

    CSDocCompiler csdoc_compiler;
    const std::wstring html = csdoc_compiler.CompileToHtml(*m_csdocCompilerSettingsForBuilding, m_csdocFilename, m_csdocText);

    GetInterface().UpdateProgress(50);
    
    if( building_to_html )
        GetInterface().LogText(_T("\nSaving CSPro Document: ") + m_outputFilename);

    FileIO::WriteText(html_filename, html, true);

    if( !building_to_html )
        m_pdfCreator->CreatePdf(m_csdocCompilerSettingsForBuilding->GetBuildSettings(), m_outputFilename, html_filename);

    if( !IsCanceled() )
        GetInterface().OnCreatedOutput(PortableFunctions::PathGetFilename(m_outputFilename), m_outputFilename);
}
