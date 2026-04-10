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

    // gets or sets the file path of the CSPro Document to be compiled
    const std::string& GetCompilationFilePath() const;
    [[nodiscard]] RAII::PushOnVectorAndPopOnDestruction<std::string> SetCompilationFilePath(std::string csdoc_file_path);

    // evaluates the path, making it an absolute path
    std::string EvaluatePath(std::string path) const;

    // returns the definition for the key, throwing an exception if not found
    const std::string& GetDefinition(const std::string& key) const;

    // returns the definition for the key (in the special definition set), throwing an exception if not found
    std::string GetSpecialDefinition(const std::string& domain, const std::string& key) const;

    // overridable TitleManager wrappers
    // --------------------------------------------------------------------------

    virtual std::string GetTitle(const std::string& csdoc_file_path)      { return m_titleManager.GetTitle(csdoc_file_path); }
    virtual void SetTitleForCompilationFilePath(const std::string& title) { m_titleManager.SetTitle(GetCompilationFilePath(), title); }
    virtual void ClearTitleForCompilationFilePath()                       { m_titleManager.ClearTitle(GetCompilationFilePath()); }

    // overridable methods
    // --------------------------------------------------------------------------

    // called when processing the metadata tag;
    // the base implementation does nothing
    virtual void AddMetadata(std::string_view attribute_sv, std::string_view value_sv);

    // adds a compiler message to the build window
    virtual void AddCompilerMessage(CompilerMessageType compiler_message_type, const std::string& text);

    // when true, preprocessor exceptions will be suppressed
    virtual bool SuppressPreprocessorExceptions() const { return false; }

    // when true, HTML headers and footers will be added
    virtual bool AddHtmlHeader() const { return true; }
    virtual bool AddHtmlFooter() const { return true; }

    // returns the title to insert into the HTML header
    virtual std::string GetHtmlHeaderTitle(const std::string& csdoc_file_path) { return GetTitle(csdoc_file_path); }

    // returns the HTML to insert in the head section that includes any stylesheet(s), as links or embedded;
    // the base implementation embeds the CSPro Document stylesheet
    virtual std::string GetStylesheetsHtml();

    // returns any extra HTML to include at the start and end of the document
    virtual std::tuple<std::string, std::string> GetHtmlToWrapDocument() { return std::tuple<std::string, std::string>(); }

    // if true, documents without titles will be considered incomplete
    virtual bool TitleIsRequired() const { return false; }

    // if false, the title will not be written to the document where the tag exists;
    // this setting does not impact whether the title is inserted into the HTML header
    virtual bool AddTitleToDocument() const { return true; }

    // returns whether the document is being compiled for a .chm file
    virtual bool CompilingForCompiledHtmlHelp() const { return false; }

    // evaluates the path, making it an absolute path
    virtual std::string EvaluateTopicPath(const std::string& path);
    virtual std::string EvaluateTopicPath(const std::string& project, const std::string& path);
    virtual std::string EvaluateImagePath(const std::string& path);

    // evaluates and processes the path for a build extra;
    // the base implementation returns the evaluated path, throwing an exception if not found
    virtual std::string EvaluateBuildExtra(const std::string& path);

    // if non-blank, the title will be wrapped in a link to the URL;
    // the base implementation returns blank
    virtual std::string CreateUrlForTitle(const std::string& path);

    // returns a URL for the topic;
    // the base implementation returns blank, which means that the URL will not have a target;
    // if the URL begins with a !, everything beyond the ! will be set as the onclick
    virtual std::string CreateUrlForTopic(const std::string& project, const std::string& path);

    // if non-blank, the URL will be inserted into colorized logic;
    // the base implementation links to the CSPro Users online help
    virtual std::string CreateUrlForLogicTopic(const char* help_topic_filename);

    // returns a URL for the image;
    // the base implementation returns a data URL
    virtual std::string CreateUrlForImageFile(const std::string& path);

    // returns a URL for the specified resource;
    // the base implementation returns a blank string
    virtual std::string CreateUrlForResource(const std::string& resource);

    // if true, external links will open in a new window
    virtual bool OpenExternalLinksInSeparateWindow() const { return true; }

    // processes context-sensitive help entries, potentially issuing errors or warnings if the entry is not found;
    // the base implementation ignores entries not found
    virtual std::optional<unsigned> GetContextId(const std::string& context, bool use_if_exists);

protected:
    static constexpr const char* CSDocStylesheetFilename     = "csdoc.css";
    static constexpr const char* DocSetWebStylesheetFilename = "docset.css";

    static std::string GetStylesheetCssFilePath(const char* css_filename);
    static std::string GetStylesheetLinkHtml(const std::string& css_url);
    static std::string GetStylesheetEmbeddedHtml(std::string css);

    static std::string CreateUrlForLogicTopicOnCSProUsersWebsite(const char* help_topic_filename);
    std::string CreateUrlForLogicHelpTopicInCSProProject(const char* help_topic_filename);

#ifdef _DEBUG
    virtual const DocBuildSettings* GetBuildSettingsDebug() { return nullptr; }
#endif

private:
    static std::string CheckPathCase(std::string path, const std::string& specified_case_to_check = SO::Empty_string);

protected:
    cs::non_null_shared_or_raw_ptr<DocSetSpec> m_docSetSpec;
    std::vector<std::string> m_compilationFilePaths;
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

    void AddCompilerMessage(CompilerMessageType compiler_message_type, const std::string& text) override;

    std::string GetStylesheetsHtml() override;

    std::string CreateUrlForTitle(const std::string& path) override;
    std::string CreateUrlForTopic(const std::string& project, const std::string& path) override;
    std::string CreateUrlForLogicTopic(const char* help_topic_filename) override;
    std::string CreateUrlForImageFile(const std::string& path) override;

    std::optional<unsigned> GetContextId(const std::string& context, bool use_if_exists) override;

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
    CSDocCompilerSettingsForBuilding(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec, DocBuildSettings build_settings, std::string build_name);

    static std::unique_ptr<CSDocCompilerSettingsForBuilding> CreateForDocSetBuild(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                                                  const DocBuildSettings& base_build_settings,
                                                                                  DocBuildSettings::BuildType build_type, std::string build_name,
                                                                                  bool throw_exceptions_for_serious_issues_when_validating_build_settings);

    const DocSetSpec& GetDocSetSpec() const { return *m_docSetSpec; }
    DocSetSpec& GetDocSetSpec()             { return *m_docSetSpec; }

    const DocBuildSettings& GetBuildSettings() const { return m_buildSettings; }
    const std::string& GetBuildName() const          { return m_buildName; }

    void SetDocSetBuilderCache(std::shared_ptr<DocSetBuilderCache> doc_set_builder_cache);

    std::string GetDocSetBuildOutputDirectory() const { return GetDocSetBuildOutputDirectoryOrFilePath(true); }
    std::string GetDocSetBuildOutputFilePath() const  { return GetDocSetBuildOutputDirectoryOrFilePath(false); }

    const std::string& GetOutputDirectoryForRelativeEvaluation(bool use_evaluated_output_directory) const;

    const std::string& GetDefaultDocumentFilePath() const;

    std::string CreateHtmlOutputFilePath(const std::string& csdoc_file_path) const;

    static std::string GetBuiltHtmlFilename(const std::string& path);
    static std::string GetBuiltHtmlFilePathInSourceDirectory(const std::string& path);

    void SetOutputFilePath(std::string output_file_path);

    const CSDocCompilerSettingsForBuilding& GetProjectSettings(const std::string& project) const;

    // WriteTextToFile and CopyFileToDirectory ensure that every file written/copied is unique
    void WriteTextToFile(const std::string& file_path, std::string_view text_content_sv) const;
    void CopyFileToDirectory(const std::string& source_file_path, const std::string& destination_file_path) const;

    std::string GetTitle(const std::string& csdoc_file_path) override;
    void SetTitleForCompilationFilePath(const std::string& title) override;
    void ClearTitleForCompilationFilePath() override;

    std::string GetStylesheetsHtml() override;

    // this implementation copies the file to m_csdocOutputDirectory, if defined
    std::string EvaluateBuildExtra(const std::string& path) override;

    std::string CreateUrlForTitle(const std::string& path) override;
    std::string CreateUrlForTopic(const std::string& project, const std::string& path) override;
    std::string CreateUrlForLogicTopic(const char* help_topic_filename) override;
    std::string CreateUrlForImageFile(const std::string& path) override;

protected:
    DocSetBuilderCache& GetDocSetBuilderCache() const  { return *m_docSetBuilderCache; }
    const std::string& GetCSDocOutputDirectory() const { return m_csdocOutputDirectory; }

    virtual std::string CreateUrlForDocSetTopic(const std::string& path) const;
    virtual std::string CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::string& path) const;
    virtual std::string CreateUrlForExternalTopic(const std::string& path) const;

protected:
    static void EnsurePathIsRelative(const std::string& path);

    std::string GetPathWithPathAdjustments(const std::string& path) const;

    std::string EvaluateDirectoryRelativeToOutputDirectory(const std::string& directory, bool use_evaluated_output_directory) const;

    static std::string CreatePathIfCopiedToDirectory(const std::string& source_file_path, const std::string& destination_directory);
    std::string CreatePathIfCopiedRelativeToFile(const std::string& source_file_path, const std::string& relative_to_file, const std::string& output_directory) const;
    std::string CreatePathIfCopiedRelativeToOutput(const std::string& source_file_path) const;

    std::string CreatePathAndCopyFileToDirectory(const std::string& source_file_path, const std::string& destination_directory) const;
    std::string CreatePathAndCopyFileIfCopiedRelativeToOutput(const std::string& source_file_path) const;

    static std::string CreateAbsoluteUrlForPath(std::string path);
    std::string CreateRelativeUrlForPath(const std::string& path) const;

    std::string GetStylesheetsHtmlWorker(const char* css_filename) const;

private:
    std::string GetDocSetBuildOutputDirectoryOrFilePath(bool directory) const;

protected:
    DocBuildSettings m_buildSettings;

private:
    std::string m_buildName;
    std::shared_ptr<DocSetBuilderCache> m_docSetBuilderCache;

    std::string m_docSetBuildOutputDirectory;

    std::string m_csdocOutputFilePath;
    std::string m_csdocOutputDirectory;
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

    std::string GetHtmlHeaderTitle(const std::string& csdoc_file_path) override;

    std::string GetStylesheetsHtml() override;

    std::tuple<std::string, std::string> GetHtmlToWrapDocument() override;

    bool TitleIsRequired() const override { return true; }

    bool OpenExternalLinksInSeparateWindow() const override { return false; }

private:
    void CopyStylesheetImages(const std::string& directory);

private:
    DocSetBuilderHtmlWebsiteGenerateTask* m_generateTask = nullptr;
    std::string m_titlePostfix;
    std::set<std::string, cs::case_insensitive_less> m_copiedStylesheetImageDirectories;
};



// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuildingChm
// --------------------------------------------------------------------------

class CSDocCompilerSettingsForBuildingChm : public CSDocCompilerSettingsForBuilding
{
public:
    using CSDocCompilerSettingsForBuilding::CSDocCompilerSettingsForBuilding;

    std::string GetDefaultDocumentFilePath() const;

    void RunPreCompilationTasks(DocSetBuilderChmGenerateTask& generate_task);

    std::string GetStylesheetsHtml() override;

    bool TitleIsRequired() const override { return true; }

    bool CompilingForCompiledHtmlHelp() const { return true; }

    std::string EvaluateBuildExtra(const std::string& path) override;

    std::string CreateUrlForImageFile(const std::string& path) override;

    std::optional<unsigned> GetContextId(const std::string& context, bool use_if_exists) override;

protected:
    std::string CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::string& path) const override;

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

    std::string GetHtmlHeaderTitle(const std::string& csdoc_file_path) override;

    std::tuple<std::string, std::string> GetHtmlToWrapDocument() override;

    bool TitleIsRequired() const override { return true; }

    std::string EvaluateBuildExtra(const std::string& path) override;

protected:
    std::string CreateUrlForDocSetTopic(const std::string& path) const override;
    std::string CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::string& path) const override;

private:
    static std::string CreateHtmlAnchorId(const std::string& csdoc_file_path, const DocSetSpec& doc_set_spec);

private:
    DocSetBuilderPdfGenerateTask* m_generateTaskForDocSetBuild = nullptr;
};
