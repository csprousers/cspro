#pragma once

#include <CSDocument/CSDocCompilerSettings.h>

class PdfCreator;


// --------------------------------------------------------------------------
// CSDocCompiler
// --------------------------------------------------------------------------

class CSDocCompiler
{
public:
    std::wstring CompileToHtml(CSDocCompilerSettings& settings, std::wstring csdoc_filename, wstring_view csdoc_text_sv);
};


// --------------------------------------------------------------------------
// CSDocCompilerBuildToFileGenerateTask
// --------------------------------------------------------------------------

class CSDocCompilerBuildToFileGenerateTask : public GenerateTask
{
public:
    CSDocCompilerBuildToFileGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, DocBuildSettings build_settings,
                                         std::wstring csdoc_filename, std::wstring csdoc_text, std::wstring output_filename);

    void ValidateInputs() override;

protected:
    void OnRun() override;

private:
    cs::non_null_shared_or_raw_ptr<DocSetSpec> m_docSetSpec;
    const DocBuildSettings m_baseBuildSettings;
    const std::wstring m_csdocFilename;
    const std::wstring m_csdocText;
    const std::wstring m_outputFilename;
    std::unique_ptr<CSDocCompilerSettingsForBuilding> m_csdocCompilerSettingsForBuilding;
    std::unique_ptr<PdfCreator> m_pdfCreator;
};
