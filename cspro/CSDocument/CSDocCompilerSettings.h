#pragma once

#include <CSDocument/TitleManager.h>

enum class CompilerMessageType;
class DocSetBuilderCache;
class DocSetBuilderChmGenerateTask;
class DocSetBuilderHtmlWebsiteGenerateTask;
class DocSetBuilderPdfGenerateTask;
class DocSetSpec;


// --------------------------------------------------------------------------
// CSDocCompilerSettings
// --------------------------------------------------------------------------

class CSDocCompilerSettings
{
public:
    CSDocCompilerSettings(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec);
    virtual ~CSDocCompilerSettings() { }

    // gets or sets the filename of the CSPro Document to be compiled
    const std::wstring& GetCompilationFilename() const;
    [[nodiscard]] RAII::PushOnVectorAndPopOnDestruction<std::wstring> SetCompilationFilename(std::wstring csdoc_filename);

    // evaluates the path, making it an absolute path
    std::wstring EvaluatePath(std::wstring path) const;

    // returns the definition for the key, throwing an exception if not found
    const std::wstring& GetDefinition(const std::wstring& key) const;

    // returns the definition for the key (in the special definition set), throwing an exception if not found
    std::wstring GetSpecialDefinition(const std::wstring& domain, const std::wstring& key) const;

    // overridable TitleManager wrappers
    // --------------------------------------------------------------------------

    virtual std::wstring GetTitle(const std::wstring& csdoc_filename)      { return m_titleManager.GetTitle(csdoc_filename); }
    virtual void SetTitleForCompilationFilename(const std::wstring& title) { m_titleManager.SetTitle(GetCompilationFilename(), title); }
    virtual void ClearTitleForCompilationFilename()                        { m_titleManager.ClearTitle(GetCompilationFilename()); }

    // overridable methods
    // --------------------------------------------------------------------------

    // adds a compiler message to the build window
    virtual void AddCompilerMessage(CompilerMessageType compiler_message_type, const std::wstring& text);

    // when true, preprocessor exceptions will be suppressed
    virtual bool SuppressPreprocessorExceptions() const { return false; }

    // when true, HTML headers and footers will be added
    virtual bool AddHtmlHeader() const { return true; }
    virtual bool AddHtmlFooter() const { return true; }

    // returns the title to insert into the HTML header
    virtual std::wstring GetHtmlHeaderTitle(const std::wstring& csdoc_filename) { return GetTitle(csdoc_filename); }

    // returns the HTML to insert in the head section that includes any stylesheet(s), as links or embedded;
    // the base implementation embeds the CSPro Document stylesheet
    virtual std::wstring GetStylesheetsHtml();

    // returns any extra HTML to include at the start and end of the document
    virtual std::tuple<std::wstring, std::wstring> GetHtmlToWrapDocument() { return std::tuple<std::wstring, std::wstring>(); }

    // if true, documents without titles will be considered incomplete
    virtual bool TitleIsRequired() const { return false; }

    // returns whether the document is being compiled for a .chm file
    virtual bool CompilingForCompiledHtmlHelp() const { return false; }

    // evaluates the path, making it an absolute path
    virtual std::wstring EvaluateTopicPath(const std::wstring& path);
    virtual std::wstring EvaluateTopicPath(const std::wstring& project, const std::wstring& path);
    virtual std::wstring EvaluateImagePath(const std::wstring& path);

    // evaluates and processes the path for a build extra;
    // the base implementation returns the evaluated path, throwing an exception if not found
    virtual std::wstring EvaluateBuildExtra(const std::wstring& path);

    // if non-blank, the title will be wrapped in a link to the URL;
    // the base implementation returns blank
    virtual std::wstring CreateUrlForTitle(const std::wstring& path);

    // returns a URL for the topic;
    // the base implementation returns blank, which means that the URL will not have a target;
    // if the URL begins with a !, everything beyond the ! will be set as the onclick
    virtual std::wstring CreateUrlForTopic(const std::wstring& project, const std::wstring& path);

    // if non-blank, the URL will be inserted into colorized logic;
    // the base implementation links to the CSPro Users online help
    virtual std::wstring CreateUrlForLogicTopic(const TCHAR* help_topic_filename);

    // returns a URL for the image;
    // the base implementation returns a data URL
    virtual std::wstring CreateUrlForImageFile(const std::wstring& path);

    // if true, external links will open in a new window
    virtual bool OpenExternalLinksInSeparateWindow() const { return true; }

    // processes context-sensitive help entries, potentially issuing errors or warnings if the entry is not found;
    // the base implementation ignores entries not found
    virtual std::optional<unsigned> GetContextId(const std::wstring& context, bool use_if_exists);

protected:
    static constexpr const TCHAR* CSDocStylesheetFilename     = _T("csdoc.css");
    static constexpr const TCHAR* DocSetWebStylesheetFilename = _T("docset.css");

    static std::wstring GetStylesheetCssPath(const TCHAR* css_filename);
    static std::wstring GetStylesheetLinkHtml(const std::wstring& css_url);
    static std::wstring GetStylesheetEmbeddedHtml(std::wstring css);

    static std::wstring CreateUrlForLogicTopicOnCSProUsersForum(const TCHAR* help_topic_filename);
    std::wstring CreateUrlForLogicHelpTopicInCSProProject(const TCHAR* help_topic_filename);

#ifdef _DEBUG
    virtual const DocBuildSettings* GetBuildSettingsDebug() { return nullptr; }
#endif

protected:
    cs::non_null_shared_or_raw_ptr<DocSetSpec> m_docSetSpec;
    std::vector<std::wstring> m_compilationFilenames;
    TitleManager m_titleManager;
};



// --------------------------------------------------------------------------
// CSDocCompilerSettingsForTitleManager
// --------------------------------------------------------------------------

class CSDocCompilerSettingsForTitleManager : public CSDocCompilerSettings
{
public:
    using CSDocCompilerSettings::CSDocCompilerSettings;

    bool SuppressPreprocessorExceptions() const override { return true; }
};



// --------------------------------------------------------------------------
// CSDocCompilerSettingsForCSDocumentPreview
// --------------------------------------------------------------------------

class CSDocCompilerSettingsForCSDocumentPreview : public CSDocCompilerSettings
{
public:
    CSDocCompilerSettingsForCSDocumentPreview(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, BuildWnd* build_wnd);

    void AddCompilerMessage(CompilerMessageType compiler_message_type, const std::wstring& text) override;

    std::wstring GetStylesheetsHtml() override;

    std::wstring CreateUrlForTitle(const std::wstring& path) override;
    std::wstring CreateUrlForTopic(const std::wstring& project, const std::wstring& path) override;
    std::wstring CreateUrlForLogicTopic(const TCHAR* help_topic_filename) override;
    std::wstring CreateUrlForImageFile(const std::wstring& path) override;

    std::optional<unsigned> GetContextId(const std::wstring& context, bool use_if_exists) override;

private:
    SharedHtmlLocalFileServer& m_fileServer;
    BuildWnd* const m_buildWnd;
    std::optional<bool> m_logicHelpsArePartOfProject;
};



// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuilding
// --------------------------------------------------------------------------

class CSDocCompilerSettingsForBuilding : public CSDocCompilerSettings
{
public:
    CSDocCompilerSettingsForBuilding(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, DocBuildSettings build_settings, std::wstring build_name);

    static std::unique_ptr<CSDocCompilerSettingsForBuilding> CreateForDocSetBuild(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                                                  const DocBuildSettings& base_build_settings,
                                                                                  DocBuildSettings::BuildType build_type, std::wstring build_name,
                                                                                  bool throw_exceptions_for_serious_issues_when_validating_build_settings);

    const DocSetSpec& GetDocSetSpec() const { return *m_docSetSpec; }
    DocSetSpec& GetDocSetSpec()             { return *m_docSetSpec; }

    const DocBuildSettings& GetBuildSettings() const { return m_buildSettings; }
    const std::wstring& GetBuildName() const         { return m_buildName; }
    
    void SetDocSetBuilderCache(std::shared_ptr<DocSetBuilderCache> doc_set_builder_cache);

    std::wstring GetDocSetBuildOutputDirectory() const { return GetDocSetBuildOutputDirectoryOrFilename(true); }
    std::wstring GetDocSetBuildOutputFilename() const  { return GetDocSetBuildOutputDirectoryOrFilename(false); }

    const std::wstring& GetOutputDirectoryForRelativeEvaluation(bool use_evaluated_output_directory) const;

    const std::wstring& GetDefaultDocumentPath() const;

    std::wstring CreateHtmlOutputFilename(const std::wstring& csdoc_filename) const;

    static std::wstring GetBuiltHtmlFilename(const std::wstring& path);
    static std::wstring GetBuiltHtmlPathInSourceDirectory(const std::wstring& path);

    void SetOutputFilename(std::wstring output_filename);

    const CSDocCompilerSettingsForBuilding& GetProjectSettings(const std::wstring& project) const;

    // WriteTextToFile and CopyFileToDirectory ensure that every file written/copied is unique
    void WriteTextToFile(const std::wstring& destination_path, wstring_view text_content_sv, bool write_utf8_bom) const;
    void CopyFileToDirectory(const std::wstring& source_path, const std::wstring& destination_path) const;

    std::wstring GetTitle(const std::wstring& csdoc_filename) override;
    void SetTitleForCompilationFilename(const std::wstring& title) override;
    void ClearTitleForCompilationFilename() override;

    std::wstring GetStylesheetsHtml() override;

    // this implementation copies the file to m_csdocOutputDirectory, if defined
    std::wstring EvaluateBuildExtra(const std::wstring& path) override;

    std::wstring CreateUrlForTitle(const std::wstring& path) override;
    std::wstring CreateUrlForTopic(const std::wstring& project, const std::wstring& path) override;
    std::wstring CreateUrlForLogicTopic(const TCHAR* help_topic_filename) override;
    std::wstring CreateUrlForImageFile(const std::wstring& path) override;

protected:
    DocSetBuilderCache& GetDocSetBuilderCache() const   { return *m_docSetBuilderCache; }
    const std::wstring& GetCSDocOutputDirectory() const { return m_csdocOutputDirectory; }

    virtual std::wstring CreateUrlForDocSetTopic(const std::wstring& path) const;
    virtual std::wstring CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::wstring& path) const;
    virtual std::wstring CreateUrlForExternalTopic(const std::wstring& path) const;

protected:
    static void EnsurePathIsRelative(const std::wstring& path);

    std::wstring GetPathWithPathAdjustments(const std::wstring& path) const;

    std::wstring EvaluateDirectoryRelativeToOutputDirectory(const std::wstring& directory, bool use_evaluated_output_directory) const;

    static std::wstring CreatePathIfCopiedToDirectory(const std::wstring& source_path, const std::wstring& destination_directory);
    std::wstring CreatePathIfCopiedRelativeToFile(const std::wstring& source_path, const std::wstring& relative_to_file, const std::wstring& output_directory) const;
    std::wstring CreatePathIfCopiedRelativeToOutput(const std::wstring& source_path) const;

    std::wstring CreatePathAndCopyFileToDirectory(const std::wstring& source_path, const std::wstring& destination_directory) const;
    std::wstring CreatePathAndCopyFileIfCopiedRelativeToOutput(const std::wstring& source_path) const;

    static std::wstring CreateAbsoluteUrlForPath(const std::wstring& path);
    std::wstring CreateRelativeUrlForPath(const std::wstring& path) const;

    std::wstring GetStylesheetsHtmlWorker(const TCHAR* css_filename) const;

private:
    std::wstring GetDocSetBuildOutputDirectoryOrFilename(bool directory) const;

protected:
    const DocBuildSettings m_buildSettings;

private:
    const std::wstring m_buildName;
    std::shared_ptr<DocSetBuilderCache> m_docSetBuilderCache;

    std::wstring m_docSetBuildOutputDirectory;

    std::wstring m_csdocOutputFilename;
    std::wstring m_csdocOutputDirectory;
};



// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuildingHtmlPages
// --------------------------------------------------------------------------

class CSDocCompilerSettingsForBuildingHtmlPages : public CSDocCompilerSettingsForBuilding
{
public:
    using CSDocCompilerSettingsForBuilding::CSDocCompilerSettingsForBuilding;

    bool OpenExternalLinksInSeparateWindow() const override { return false; }
};



// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuildingHtmlWebsite
// --------------------------------------------------------------------------

class CSDocCompilerSettingsForBuildingHtmlWebsite : public CSDocCompilerSettingsForBuilding
{
public:
    using CSDocCompilerSettingsForBuilding::CSDocCompilerSettingsForBuilding;

    void RunPreCompilationTasks(DocSetBuilderHtmlWebsiteGenerateTask& generate_task);

    std::wstring GetHtmlHeaderTitle(const std::wstring& csdoc_filename) override;

    std::wstring GetStylesheetsHtml() override;

    std::tuple<std::wstring, std::wstring> GetHtmlToWrapDocument() override;

    bool TitleIsRequired() const override { return true; }

    bool OpenExternalLinksInSeparateWindow() const override { return false; }

private:
    void CopyStylesheetImages(const std::wstring& directory);

private:
    DocSetBuilderHtmlWebsiteGenerateTask* m_generateTask = nullptr;
    std::wstring m_titlePostfix;
    std::set<StringNoCase> m_copiedStylesheetImageDirectories;
};



// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuildingChm
// --------------------------------------------------------------------------

class CSDocCompilerSettingsForBuildingChm : public CSDocCompilerSettingsForBuilding
{
public:
    using CSDocCompilerSettingsForBuilding::CSDocCompilerSettingsForBuilding;

    std::wstring GetDefaultDocumentPath() const;

    void RunPreCompilationTasks(DocSetBuilderChmGenerateTask& generate_task);

    std::wstring GetStylesheetsHtml() override;

    bool TitleIsRequired() const override { return true; }

    bool CompilingForCompiledHtmlHelp() const { return true; }

    std::wstring EvaluateBuildExtra(const std::wstring& path) override;

    std::wstring CreateUrlForImageFile(const std::wstring& path) override;

    std::optional<unsigned> GetContextId(const std::wstring& context, bool use_if_exists) override;

protected:
    std::wstring CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::wstring& path) const override;

private:
    DocSetBuilderChmGenerateTask* m_generateTask = nullptr;
};



// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuildingPdf
// --------------------------------------------------------------------------

class CSDocCompilerSettingsForBuildingPdf: public CSDocCompilerSettingsForBuilding
{
public:
    using CSDocCompilerSettingsForBuilding::CSDocCompilerSettingsForBuilding;

    void RunPreCompilationTasksForDocSetBuild(DocSetBuilderPdfGenerateTask& generate_task);

    bool AddHtmlHeader() const override;
    bool AddHtmlFooter() const override;

    std::wstring GetHtmlHeaderTitle(const std::wstring& csdoc_filename) override;

    std::tuple<std::wstring, std::wstring> GetHtmlToWrapDocument() override;

    bool TitleIsRequired() const override { return true; }

    std::wstring EvaluateBuildExtra(const std::wstring& path) override;

protected:
    std::wstring CreateUrlForDocSetTopic(const std::wstring& path) const override;
    std::wstring CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::wstring& path) const override;

private:
    static std::wstring CreateHtmlAnchorId(const std::wstring& csdoc_filename, const DocSetSpec& doc_set_spec);

private:
    DocSetBuilderPdfGenerateTask* m_generateTaskForDocSetBuild = nullptr;
};
