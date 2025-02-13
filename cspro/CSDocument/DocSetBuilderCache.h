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
    bool LogWrittenFile(const std::wstring& filename, wstring_view text_content_sv);

    // a routine to ensure that all copied files are identical
    void LogCopiedFile(const std::wstring& filename, const std::tuple<int64_t, time_t>& file_size_and_modified_time);

    // TitleManager wrappers that ensure that a title used during the build does not change later
    const std::wstring& GetTitle(TitleManager& title_manager, const std::wstring& csdoc_filename);
    void SetTitle(TitleManager& title_manager, const std::wstring& csdoc_filename, const std::wstring& title);
    void ClearTitle(TitleManager& title_manager, const std::wstring& csdoc_filename);

    // an object to cache embedded CSS
    std::map<const TCHAR*, std::wstring>& GetEmbeddedStylesheetsHtmlCache() { return m_embeddedStylesheetsHtmlCache; }

    // a routine to get the paths of images used by stylesheets
    const std::vector<std::wstring>& GetStylesheetImagePaths();

    // a routine to determine the default CSPro Document that is part of a Document Set;
    // the Document Set must be compiled prior to being used by this method;
    // if no default document exists, an exception is thrown
    const std::wstring& GetDefaultDocumentPath(const DocSetSpec& doc_set_spec, bool path_must_be_for_default_document);

    // loads the Document Set, compiles it using the SpecCompilationType::DataForTree setting, and caches it;
    // errors during compilation result in exceptions
    const CSDocCompilerSettingsForBuilding& GetDocSetForProjectCompiledForDataForTree(const std::wstring& project_doc_set_spec_filename,
                                                                                      const CSDocCompilerSettingsForBuilding& current_settings);

private:
    [[noreturn]] void IssueTitleChangeException(const std::wstring& csdoc_filename, const std::wstring& new_title) const;

private:
    // filename -> content (when loaded)
    std::map<StringNoCase, std::unique_ptr<std::wstring>> m_writtenFilesAndLoadedContent;

    // filename -> size + modified time
    std::map<StringNoCase, std::tuple<int64_t, time_t>> m_copiedFileSizesAndModifiedTimes;

    // CSPro Document filename -> title
    std::map<StringNoCase, std::wstring> m_previouslyRetrievedTitles;

    // CSS filename -> embedded HTML
    std::map<const TCHAR*, std::wstring> m_embeddedStylesheetsHtmlCache;

    // stylesheet image filename
    std::vector<std::wstring> m_stylesheetImagePaths;

    // Document Set filename -> CSPro Document filename + whether it is a true default document
    std::map<StringNoCase, std::tuple<std::wstring, bool>> m_defaultDocumentPaths;

    // Document Set filename + build type + build name -> CSDocCompilerSettingsForBuilding
    using DocSetProjectCacheKey = std::tuple<StringNoCase, DocBuildSettings::BuildType, std::wstring>;
    std::map<DocSetProjectCacheKey, std::unique_ptr<CSDocCompilerSettingsForBuilding>> m_docSetProjects;
};
