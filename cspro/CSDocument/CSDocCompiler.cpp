#include "StdAfx.h"
#include "CSDocCompiler.h"
#include "CSDocCompilerWorker.h"
#include "PdfCreator.h"


// --------------------------------------------------------------------------
// CSDocCompiler
// --------------------------------------------------------------------------

std::string CSDocCompiler::CompileToHtml(CSDocCompilerSettings& settings, std::string csdoc_file_path, const std::string_view csdoc_text_sv)
{
    const RAII::PushOnVectorAndPopOnDestruction<std::string> file_path_holder = settings.SetCompilationFilePath(std::move(csdoc_file_path));

    CSDocCompilerWorker worker(settings, csdoc_text_sv);
    return worker.CreateHtml();
}



// --------------------------------------------------------------------------
// CSDocCompilerBuildToFileGenerateTask
// --------------------------------------------------------------------------

CSDocCompilerBuildToFileGenerateTask::CSDocCompilerBuildToFileGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, DocBuildSettings base_build_settings,
                                                                           std::string csdoc_file_path, std::string csdoc_text, std::string output_file_path)
    :   m_docSetSpec(std::move(doc_set_spec)),
        m_baseBuildSettings(std::move(base_build_settings)),
        m_csdocFilePath(std::move(csdoc_file_path)),
        m_csdocText(std::move(csdoc_text)),
        m_outputFilePath(std::move(output_file_path))
{
}


void CSDocCompilerBuildToFileGenerateTask::ValidateInputs()
{
    if( m_outputFilePath.empty() )
        throw CSProException("You must specify an output filename.");

    FileIO::CreateDirectoriesForFile(m_outputFilePath);

    const std::string extension = PortableFunctions::PathGetFileExtension(m_outputFilePath);

    const DocBuildSettings::BuildType build_type =
        SO::EqualsNoCase(extension, FileExtensions::HTML) ? DocBuildSettings::BuildType::HtmlPages :
        SO::EqualsNoCase(extension, FileExtensions::PDF)  ? DocBuildSettings::BuildType::Pdf :
        throw CSProException("There is no routine for converting CSPro Documents to files with the extension: %s", extension.c_str());

    // create build settings by using the defaults for building and applying the user-specified settings on top
    DocBuildSettings default_build_settings = ( build_type == DocBuildSettings::BuildType::HtmlPages ) ? DocBuildSettings::DefaultSettingsForCSDocBuildToHtml() :
                                                                                                         DocBuildSettings::DefaultSettingsForCSDocBuildToPdf();
    DocBuildSettings build_settings = DocBuildSettings::ApplySettings(std::move(default_build_settings), m_baseBuildSettings);

    m_csdocCompilerSettingsForBuilding = CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(m_docSetSpec, build_settings, build_type, std::string(), true);

    if( build_type == DocBuildSettings::BuildType::Pdf )
        m_pdfCreator = std::make_unique<PdfCreator>(*this);
}


void CSDocCompilerBuildToFileGenerateTask::OnRun()
{
    GetInterface().SetTitle(FormatText("Building CSPro Document: %s", m_csdocFilePath.c_str()));

    ValidateInputs();
    ASSERT(m_csdocCompilerSettingsForBuilding != nullptr);

    GetInterface().LogText("Compiling CSPro Document: %s", m_csdocFilePath.c_str());

    const bool building_to_html = ( m_csdocCompilerSettingsForBuilding->GetBuildSettings().GetBuildType() == DocBuildSettings::BuildType::HtmlPages );

    ASSERT(building_to_html || m_pdfCreator != nullptr);
    const std::string& html_file_path = building_to_html ? m_outputFilePath :
                                                           m_pdfCreator->CreateTemporaryHtmlFilePath(1);

    m_csdocCompilerSettingsForBuilding->SetOutputFilePath(html_file_path);

    CSDocCompiler csdoc_compiler;
    const std::string html = csdoc_compiler.CompileToHtml(*m_csdocCompilerSettingsForBuilding, m_csdocFilePath, m_csdocText);

    GetInterface().UpdateProgress(50);

    if( building_to_html )
        GetInterface().LogText("\nSaving CSPro Document: " + m_outputFilePath);

    FileIO::WriteText(html_file_path, html, true);

    if( !building_to_html )
        m_pdfCreator->CreatePdf(m_csdocCompilerSettingsForBuilding->GetBuildSettings(), m_outputFilePath, html_file_path);

    if( !IsCanceled() )
        GetInterface().OnCreatedOutput(PortableFunctions::PathGetFilename(m_outputFilePath), m_outputFilePath);
}
