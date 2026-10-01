#pragma once

#include <CSDocument/CSDocCompilerSettings.h>

class PdfCreator;
namespace FileIO { class TextFile; }


// --------------------------------------------------------------------------
// DocSetBuilderBaseGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderBaseGenerateTask : public GenerateTask
{
protected:
    DocSetBuilderBaseGenerateTask(std::unique_ptr<CSDocCompilerSettingsForBuilding> settings);

public:
    ~DocSetBuilderBaseGenerateTask();

    static std::unique_ptr<DocSetBuilderBaseGenerateTask> CreateForBuild(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                                         const DocBuildSettings& base_build_settings, std::string build_name,
                                                                         bool throw_exceptions_for_serious_issues_when_validating_build_settings);

    void SetDocSetBuilderCache(std::shared_ptr<DocSetBuilderCache> doc_set_builder_cache);

    void ValidateInputs() override;

    virtual void ValidateInputsPostDocSetCompilation() { }

protected:
    DocSetSpec& GetDocSetSpec() { return m_csdocCompilerSettingsForBuilding->GetDocSetSpec(); }

    void CreateTempOutputDirectory();
    const std::string& GetTempOutputDirectory() const { ASSERT(!m_tempOutputDirectory.empty()); return m_tempOutputDirectory; }

    void RunBuild();

    // base class implementations assume nothing is done pre- or post-compilation
    virtual std::tuple<double, double> GetPreAndPostCompilationProgressPercents(size_t num_csdocs);

    // base class implementation does nothing
    virtual void OnPreCSDocCompilation();

    // base class implementation returns the file paths in no particular order
    virtual const std::vector<std::string>& GetCSDocFilePathsInCompilationOrder();

    // base class implementation calls CSDocCompilerSettingsForBuilding::CreateHtmlOutputFilePath
    virtual std::string GetCSDocOutputFilePath(const std::string& csdoc_file_path);

    // base class implementation saves the HTML to the output file
    virtual void OnCSDocCompilationResult(const std::string& csdoc_file_path, const std::string& output_file_path, const std::string& html);

    // base class implementation throws the exception
    virtual void OnCSDocCompilationResult(const std::string& csdoc_file_path, const std::string& output_file_path, const CSProException& exception);

    // base class implementation does nothing
    virtual void OnPostCSDocCompilation();

private:
    void GetTextAndModifiedIterationForOpenDocuments();
    const std::tuple<SharableString, int64_t>* GetTextAndModifiedIteration(const std::string& file_path) const;
    SharableString GetFileText(const std::string& file_path) const;

    void IncrementAndUpdateProgress(double progress_increase);

    void CompileCSDocs(double progress_for_each_document);

protected:
    std::unique_ptr<CSDocCompilerSettingsForBuilding> m_csdocCompilerSettingsForBuilding;

private:
    std::map<std::string, std::tuple<SharableString, int64_t>, cs::case_insensitive_less> m_textAndModifiedIterationForOpenDocuments;
    std::optional<RAII::PushOnVectorAndPopOnDestruction<DocSetCompiler::GetFileTextOrModifiedIterationCallback>> m_getFileTextOrModifiedIterationCallbackHolder;
    double m_progress;
    std::vector<std::string> m_csdocFilePaths;
    std::string m_tempOutputDirectory;
};



// --------------------------------------------------------------------------
// DocSetBuilderCompileAllGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderCompileAllGenerateTask : public DocSetBuilderBaseGenerateTask
{
public:
    DocSetBuilderCompileAllGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec);

    const std::vector<std::tuple<std::string, std::string>>& GetDocumentsWithCompilationErrors() const { return m_documentsWithCompilationErrors; }

    void ValidateInputs() override { }

protected:
    void OnRun() override;

    std::string GetCSDocOutputFilePath(const std::string& /*csdoc_file_path*/) override { return std::string(); }

    void OnCSDocCompilationResult(const std::string& /*csdoc_file_path*/, const std::string& /*output_file_path*/, const std::string& /*html*/) override { }

    void OnCSDocCompilationResult(const std::string& csdoc_file_path, const std::string& output_file_path, const CSProException& exception) override;

private:
    std::vector<std::tuple<std::string, std::string>> m_documentsWithCompilationErrors; // file path, error
};



// --------------------------------------------------------------------------
// DocSetBuilderHtmlPagesGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderHtmlPagesGenerateTask : public DocSetBuilderBaseGenerateTask
{
public:
    DocSetBuilderHtmlPagesGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                       const DocBuildSettings& base_build_settings, std::string build_name,
                                       bool throw_exceptions_for_serious_issues_when_validating_build_settings);

protected:
    void OnRun() override;
};



// --------------------------------------------------------------------------
// DocSetBuilderHtmlWebsiteGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderHtmlWebsiteGenerateTask : public DocSetBuilderBaseGenerateTask
{
    friend CSDocCompilerSettingsForBuildingHtmlWebsite;

public:
    DocSetBuilderHtmlWebsiteGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                         const DocBuildSettings& base_build_settings, std::string build_name,
                                         bool throw_exceptions_for_serious_issues_when_validating_build_settings);
    ~DocSetBuilderHtmlWebsiteGenerateTask();

    void ValidateInputsPostDocSetCompilation() override;

protected:
    void OnRun() override;

    std::tuple<double, double> GetPreAndPostCompilationProgressPercents(size_t num_csdocs) override;

    void OnPreCSDocCompilation() override;

private:
    CSDocCompilerSettingsForBuildingHtmlWebsite& GetSettings();

    void Create_htaccess(const std::string& directory, const std::string& default_document_built_filename);
    void Create_web_config(const std::string& directory, const std::string& default_document_built_filename);

private:
    class TableOfContentsEvaluator;
    std::unique_ptr<TableOfContentsEvaluator> m_tableOfContentsEvaluator;
    std::string m_defaultDocumentBuiltFilePath;
};



// --------------------------------------------------------------------------
// DocSetBuilderChmGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderChmGenerateTask : public DocSetBuilderBaseGenerateTask
{
    friend CSDocCompilerSettingsForBuildingChm;

public:
    DocSetBuilderChmGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                 const DocBuildSettings& base_build_settings, std::string build_name,
                                 bool throw_exceptions_for_serious_issues_when_validating_build_settings);

    void ValidateInputs() override;
    void ValidateInputsPostDocSetCompilation() override;

protected:
    void OnRun() override;

    std::tuple<double, double> GetPreAndPostCompilationProgressPercents(size_t num_csdocs) override;

    void OnPreCSDocCompilation() override;

    std::string GetCSDocOutputFilePath(const std::string& csdoc_file_path) override;

    void OnCSDocCompilationResult(const std::string& csdoc_file_path, const std::string& output_file_path, const std::string& html) override;

    void OnPostCSDocCompilation() override;

private:
    CSDocCompilerSettingsForBuildingChm& GetSettings();

    const std::string& AddChmInput(std::string file_path);

    FileIO::TextFile OpenChmFileForOutput(const std::string& file_path);

    class IndexTableOfContentsBaseWriter;
    class IndexWriter;
    class TableOfContentsWriter;

    void WriteChmProjectFile(const std::string& hhp_file_path, const std::string& hhc_file_path, const std::string& hhk_file_path);
    void WriteChmProjectFileContextIds(FileIO::TextFile& text_file);
    void WriteChmTableOfContentsFile(FileIO::TextFile& text_file);
    void WriteChmIndexFile(FileIO::TextFile& text_file);

private:
    std::string m_chmOutputFilePath;
    std::string m_defaultDocumentBuiltHtmlFilename;
    std::vector<std::string> m_evaluatedButtonValues;
    std::vector<std::string> m_chmInputFilePaths;
    std::string m_nonEmbeddedStylesheetHtml;
    std::map<unsigned, std::string> m_contextMap; // context ID -> document where used
};



// --------------------------------------------------------------------------
// DocSetBuilderPdfGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderPdfGenerateTask : public DocSetBuilderBaseGenerateTask
{
    friend class CSDocCompilerSettingsForBuildingPdf;

public:
    DocSetBuilderPdfGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                 const DocBuildSettings& base_build_settings, std::string build_name,
                                 bool throw_exceptions_for_serious_issues_when_validating_build_settings);
    ~DocSetBuilderPdfGenerateTask();

    void ValidateInputs() override;
    void ValidateInputsPostDocSetCompilation() override;

protected:
    void OnRun() override;

    std::tuple<double, double> GetPreAndPostCompilationProgressPercents(size_t num_csdocs) override;

    void OnPreCSDocCompilation() override;

    const std::vector<std::string>& GetCSDocFilePathsInCompilationOrder() override;

    std::string GetCSDocOutputFilePath(const std::string& csdoc_file_path) override;

    void OnCSDocCompilationResult(const std::string& csdoc_file_path, const std::string& output_file_path, const std::string& html) override;

    void OnPostCSDocCompilation() override;

private:
    CSDocCompilerSettingsForBuildingPdf& GetSettings();

    class TableOfContentsEvaluator;
    void EvaluateCompilationOrder();

private:
    std::unique_ptr<PdfCreator> m_pdfCreator;
    std::string m_pdfOutputFilePath;

    std::vector<std::tuple<size_t, std::string>> m_csdocCompilationIndexWithPreceedingTitles;
    std::vector<std::string> m_csdocFilePathsInCompilationOrder;
    size_t m_csdocFirstNonCoverPageCompilationIndex;
    size_t m_csdocCurrentCompilationIndex;

    std::string m_csdocsHtmlFilePath;
    FILE* m_csdocsHtmlFile;
    std::string m_coverPageHtmlFilePath;
};
