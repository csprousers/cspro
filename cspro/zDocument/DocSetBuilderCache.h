#pragma once

class CSDocCompilerSettingsForBuilding;
class TitleManager;


// the DocSetBuilderCache class manages values that are cached while building Document Sets;
// if multiple Document Sets are built in one operation, the cached values are used for all builds

class DocSetBuilderCache
{
public:
    // a routine to ensure that all written files are identical;
    // a return value of false means that the file does not need to be written as it is
    // identical to a previously written file
    bool LogWrittenFile(const std::string& file_path, std::string_view text_content_sv);

    // a routine to ensure that all copied files are identical
    void LogCopiedFile(const std::string& file_path, const std::tuple<int64_t, int64_t>& file_size_and_modified_time);

    // TitleManager wrappers that ensure that a title used during the build does not change later
    const std::string& GetTitle(TitleManager& title_manager, const std::string& csdoc_file_path);
    void SetTitle(TitleManager& title_manager, const std::string& csdoc_file_path, const std::string& title);
    void ClearTitle(TitleManager& title_manager, const std::string& csdoc_file_path);

    // an object to cache embedded CSS
    std::map<const char*, std::string>& GetEmbeddedStylesheetsHtmlCache() { return m_embeddedStylesheetsHtmlCache; }

    // a routine to get the paths of images used by stylesheets
    const std::vector<std::string>& GetStylesheetImageFilePaths();

    // a routine to determine the default CSPro Document that is part of a Document Set;
    // the Document Set must be compiled prior to being used by this method;
    // if no default document exists, an exception is thrown
    const std::string& GetDefaultDocumentFilePath(const DocSetSpec& doc_set_spec, bool path_must_be_for_default_document);

    // loads the Document Set, compiles it using the SpecCompilationType::DataForTree setting, and caches it;
    // errors during compilation result in exceptions
    const CSDocCompilerSettingsForBuilding& GetDocSetForProjectCompiledForDataForTree(const std::string& project_doc_set_spec_file_path,
                                                                                      const CSDocCompilerSettingsForBuilding& current_settings);

private:
    [[noreturn]] void IssueTitleChangeException(const std::string& csdoc_file_path, cs::string_sz new_title) const;

private:
    // file path -> content (when loaded)
    std::map<std::string, std::unique_ptr<std::string>, cs::case_insensitive_less> m_writtenFilesAndLoadedContent;

    // file path -> size + modified time
    std::map<std::string, std::tuple<int64_t, int64_t>, cs::case_insensitive_less> m_copiedFileSizesAndModifiedTimes;

    // CSPro Document file path -> title
    std::map<std::string, std::string, cs::case_insensitive_less> m_previouslyRetrievedTitles;

    // CSS filename -> embedded HTML
    std::map<const char*, std::string> m_embeddedStylesheetsHtmlCache;

    // stylesheet image file paths
    std::vector<std::string> m_stylesheetImageFilePaths;

    // Document Set file path -> CSPro Document file path + whether it is a true default document
    std::map<std::string, std::tuple<std::string, bool>, cs::case_insensitive_less> m_defaultDocumentFilePaths;

    // Document Set file path (lowercase) + build type + build name -> CSDocCompilerSettingsForBuilding
    using DocSetProjectCacheKey = std::tuple<std::string, DocBuildSettings::BuildType, std::string>;
    std::map<DocSetProjectCacheKey, std::unique_ptr<CSDocCompilerSettingsForBuilding>> m_docSetProjects;
};
