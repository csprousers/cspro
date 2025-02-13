#pragma once

#include <CSDocument/CSDocCompilerSettings.h>

class CStdioFileUnicode;
class PdfCreator;


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
                                                                         const DocBuildSettings& base_build_settings, std::wstring build_name,
                                                                         bool throw_exceptions_for_serious_issues_when_validating_build_settings);

    void SetDocSetBuilderCache(std::shared_ptr<DocSetBuilderCache> doc_set_builder_cache);

    void ValidateInputs() override;

    virtual void ValidateInputsPostDocSetCompilation() { }

protected:
    DocSetSpec& GetDocSetSpec() { return m_csdocCompilerSettingsForBuilding->GetDocSetSpec(); }

    void CreateTempOutputDirectory();
    const std::wstring& GetTempOutputDirectory() const { ASSERT(!m_tempOutputDirectory.empty()); return m_tempOutputDirectory; }

    void RunBuild();

    // base class implementations assume nothing is done pre- or post-compilation
    virtual std::tuple<double, double> GetPreAndPostCompilationProgressPercents(size_t num_csdocs);

    // base class implementation does nothing
    virtual void OnPreCSDocCompilation();

    // base class implementation returns the filenames in no particular order
    virtual const std::vector<std::wstring>& GetCSDocFilenamesInCompilationOrder();

    // base class implementation calls CSDocCompilerSettingsForBuilding::CreateHtmlOutputFilename
    virtual std::wstring GetCSDocOutputFilename(const std::wstring& csdoc_filename);

    // base class implementation saves the HTML to the output file
    virtual void OnCSDocCompilationResult(const std::wstring& csdoc_filename, const std::wstring& output_filename, const std::wstring& html);

    // base class implementation throws the exception
    virtual void OnCSDocCompilationResult(const std::wstring& csdoc_filename, const std::wstring& output_filename, const CSProException& exception);

    // base class implementation does nothing
    virtual void OnPostCSDocCompilation();

private:
    void GetTextAndModifiedIterationForOpenDocuments();
    const std::tuple<std::wstring, int64_t>* GetTextAndModifiedIteration(const std::wstring& filename) const;
    std::wstring GetFileText(const std::wstring& filename) const;

    void IncrementAndUpdateProgress(double progress_increase);

    void CompileCSDocs(double progress_for_each_document);

protected:
    std::unique_ptr<CSDocCompilerSettingsForBuilding> m_csdocCompilerSettingsForBuilding;

private:
    std::map<StringNoCase, std::tuple<std::wstring, int64_t>> m_textAndModifiedIterationForOpenDocuments;
    std::optional<RAII::PushOnVectorAndPopOnDestruction<DocSetCompiler::GetFileTextOrModifiedIterationCallback>> m_getFileTextOrModifiedIterationCallbackHolder;
    double m_progress;
    std::vector<std::wstring> m_csdocFilenames;
    std::wstring m_tempOutputDirectory;
};



// --------------------------------------------------------------------------
// DocSetBuilderCompileAllGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderCompileAllGenerateTask : public DocSetBuilderBaseGenerateTask
{
public:
    DocSetBuilderCompileAllGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec);

    const std::vector<std::tuple<std::wstring, std::wstring>>& GetDocumentsWithCompilationErrors() const { return m_documentsWithCompilationErrors; }

    void ValidateInputs() override { }

protected:
    void OnRun() override;

    std::wstring GetCSDocOutputFilename(const std::wstring& /*csdoc_filename*/) override { return std::wstring(); }

    void OnCSDocCompilationResult(const std::wstring& /*csdoc_filename*/, const std::wstring& /*output_filename*/, const std::wstring& /*html*/) override { }

    void OnCSDocCompilationResult(const std::wstring& csdoc_filename, const std::wstring& output_filename, const CSProException& exception) override;

private:
    std::vector<std::tuple<std::wstring, std::wstring>> m_documentsWithCompilationErrors; // filename, error
};



// --------------------------------------------------------------------------
// DocSetBuilderHtmlPagesGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderHtmlPagesGenerateTask : public DocSetBuilderBaseGenerateTask
{
public:
    DocSetBuilderHtmlPagesGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                       const DocBuildSettings& base_build_settings, std::wstring build_name,
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
                                         const DocBuildSettings& base_build_settings, std::wstring build_name,
                                         bool throw_exceptions_for_serious_issues_when_validating_build_settings);
    ~DocSetBuilderHtmlWebsiteGenerateTask();

    void ValidateInputsPostDocSetCompilation() override;

protected:
    void OnRun() override;

    std::tuple<double, double> GetPreAndPostCompilationProgressPercents(size_t num_csdocs) override;

    void OnPreCSDocCompilation() override;

private:
    CSDocCompilerSettingsForBuildingHtmlWebsite& GetSettings();

    void Create_htaccess(const std::wstring& directory, const std::wstring& default_document_built_filename);
    void Create_web_config(const std::wstring& directory, const std::wstring& default_document_built_filename);

private:
    class TableOfContentsEvaluator;
    std::unique_ptr<TableOfContentsEvaluator> m_tableOfContentsEvaluator;
    std::wstring m_defaultDocumentBuiltPath;
};



// --------------------------------------------------------------------------
// DocSetBuilderChmGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderChmGenerateTask : public DocSetBuilderBaseGenerateTask
{
    friend CSDocCompilerSettingsForBuildingChm;

public:
    DocSetBuilderChmGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                 const DocBuildSettings& base_build_settings, std::wstring build_name,
                                 bool throw_exceptions_for_serious_issues_when_validating_build_settings);

    void ValidateInputs() override;
    void ValidateInputsPostDocSetCompilation() override;

protected:
    void OnRun() override;

    std::tuple<double, double> GetPreAndPostCompilationProgressPercents(size_t num_csdocs) override;

    void OnPreCSDocCompilation() override;

    std::wstring GetCSDocOutputFilename(const std::wstring& csdoc_filename) override;

    void OnCSDocCompilationResult(const std::wstring& csdoc_filename, const std::wstring& output_filename, const std::wstring& html) override;

    void OnPostCSDocCompilation() override;

private:
    CSDocCompilerSettingsForBuildingChm& GetSettings();

    const std::wstring& AddChmInput(std::wstring filename);

    std::unique_ptr<CStdioFileUnicode> OpenChmFileForOutput(const std::wstring& filename);

    class IndexTableOfContentsBaseWriter;
    class IndexWriter;
    class TableOfContentsWriter;

    void WriteChmProjectFile(const std::wstring& hhp_filename, const std::wstring& hhc_filename, const std::wstring& hhk_filename);
    void WriteChmProjectFileContextIds(CStdioFileUnicode& file);
    void WriteChmTableOfContentsFile(CStdioFileUnicode& file);
    void WriteChmIndexFile(CStdioFileUnicode& file);

private:
    std::wstring m_chmOutputFilename;
    std::wstring m_defaultDocumentBuiltHtmlFilename;
    std::vector<std::wstring> m_evaluatedButtonValues;
    std::vector<std::wstring> m_chmInputFilenames;
    std::wstring m_nonEmbeddedStylesheetHtml;
    std::map<unsigned, std::wstring> m_contextMap; // context ID -> document where used
};



// --------------------------------------------------------------------------
// DocSetBuilderPdfGenerateTask
// --------------------------------------------------------------------------

class DocSetBuilderPdfGenerateTask : public DocSetBuilderBaseGenerateTask
{
    friend class CSDocCompilerSettingsForBuildingPdf;

public:
    DocSetBuilderPdfGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                 const DocBuildSettings& base_build_settings, std::wstring build_name,
                                 bool throw_exceptions_for_serious_issues_when_validating_build_settings);
    ~DocSetBuilderPdfGenerateTask();

    void ValidateInputs() override;
    void ValidateInputsPostDocSetCompilation() override;

protected:
    void OnRun() override;

    std::tuple<double, double> GetPreAndPostCompilationProgressPercents(size_t num_csdocs) override;

    void OnPreCSDocCompilation() override;

    const std::vector<std::wstring>& GetCSDocFilenamesInCompilationOrder() override;

    std::wstring GetCSDocOutputFilename(const std::wstring& csdoc_filename) override;

    void OnCSDocCompilationResult(const std::wstring& csdoc_filename, const std::wstring& output_filename, const std::wstring& html) override;

    void OnPostCSDocCompilation() override;

private:
    CSDocCompilerSettingsForBuildingPdf& GetSettings();

    class TableOfContentsEvaluator;
    void EvaluateCompilationOrder();

private:
    std::unique_ptr<PdfCreator> m_pdfCreator;
    std::wstring m_pdfOutputFilename;

    std::vector<std::tuple<std::size_t, std::wstring>> m_csdocCompilationIndexWithPreceedingTitles;
    std::vector<std::wstring> m_csdocFilenamesInCompilationOrder;
    size_t m_csdocFirstNonCoverPageCompilationIndex;
    size_t m_csdocCurrentCompilationIndex;

    std::wstring m_csdocsHtmlFilename;
    FILE* m_csdocsHtmlFile;
    std::wstring m_coverPageHtmlFilename;
};
