#pragma once

#include <CSDocument/CSDocCompilerSettings.h>

class PdfCreator;


// --------------------------------------------------------------------------
// CSDocCompiler
// --------------------------------------------------------------------------

class CSDocCompiler
{
public:
    std::string CompileToHtml(CSDocCompilerSettings& settings, std::string csdoc_file_path, std::string_view csdoc_text_sv);
};


// --------------------------------------------------------------------------
// CSDocCompilerBuildToFileGenerateTask
// --------------------------------------------------------------------------

class CSDocCompilerBuildToFileGenerateTask : public GenerateTask
{
public:
    CSDocCompilerBuildToFileGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, DocBuildSettings base_build_settings,
                                         std::string csdoc_file_path, std::string csdoc_text, std::string output_file_path);

    void ValidateInputs() override;

protected:
    void OnRun() override;

private:
    cs::non_null_shared_or_raw_ptr<DocSetSpec> m_docSetSpec;
    DocBuildSettings m_baseBuildSettings;
    std::string m_csdocFilePath;
    std::string m_csdocText;
    std::string m_outputFilePath;
    std::unique_ptr<CSDocCompilerSettingsForBuilding> m_csdocCompilerSettingsForBuilding;
    std::unique_ptr<PdfCreator> m_pdfCreator;
};
