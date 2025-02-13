#include "StdAfx.h"
#include "DocSetBuilder.h"
#include "CSDocCompilerSettings.h"
#include "CSDocCompilerWorker.h"
#include "DocSetBuilderCache.h"
#include "GenerateTaskProcessRunner.h"
#include <zUtilO/StdioFileUnicode.h>


namespace
{
    constexpr const TCHAR* HhcDisplayText = _T("Microsoft HTML Help Compiler");
    constexpr const TCHAR* ChmDisplayText = _T("Compiled HTML Help");
}


// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuildingChm
// --------------------------------------------------------------------------

std::wstring CSDocCompilerSettingsForBuildingChm::CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::wstring& path) const
{
    ASSERT(m_buildSettings.GetProjectLinkageAction() == DocBuildSettings::ProjectLinkageAction::Link);

    const std::wstring project_output_filename = project_settings.GetDocSetBuildOutputFilename();

    return CreateRelativeUrlForPath(project_output_filename) + _T("::") + Encoders::ToUri(GetBuiltHtmlFilename(path));
}


std::wstring CSDocCompilerSettingsForBuildingChm::GetDefaultDocumentPath() const
{
    return GetDocSetBuilderCache().GetDefaultDocumentPath(*m_docSetSpec, true);
}


void CSDocCompilerSettingsForBuildingChm::RunPreCompilationTasks(DocSetBuilderChmGenerateTask& generate_task)
{
    m_generateTask = &generate_task;
}


std::wstring CSDocCompilerSettingsForBuildingChm::GetStylesheetsHtml()
{
    if( m_buildSettings.GetStylesheetAction() == DocBuildSettings::StylesheetAction::Embed )
        return CSDocCompilerSettingsForBuilding::GetStylesheetsHtml();

    // if not embedded, the stylesheet will be linked as if it were in the same location as the compiled HTML
    if( m_generateTask->m_nonEmbeddedStylesheetHtml.empty() )
    {
        const StringNoCase& source_css_path = m_generateTask->AddChmInput(GetStylesheetCssPath(CSDocStylesheetFilename));
        m_generateTask->m_nonEmbeddedStylesheetHtml = GetStylesheetLinkHtml(Encoders::ToUri(PortableFunctions::PathGetFilename(source_css_path)));
    }

    return m_generateTask->m_nonEmbeddedStylesheetHtml;
}


std::wstring CSDocCompilerSettingsForBuildingChm::EvaluateBuildExtra(const std::wstring& path)
{
    std::wstring evaluated_path = CSDocCompilerSettings::EvaluateBuildExtra(path);

    CreatePathAndCopyFileToDirectory(evaluated_path, m_generateTask->GetTempOutputDirectory());

    return evaluated_path;
}


std::wstring CSDocCompilerSettingsForBuildingChm::CreateUrlForImageFile(const std::wstring& path)
{
    if( m_buildSettings.GetImageAction() == DocBuildSettings::ImageAction::DataUrl )
        return CSDocCompilerSettingsForBuilding::CreateUrlForImageFile(path);

    // if not a data URL, the image will be linked as if it were in the same location as the compiled HTML
    m_generateTask->AddChmInput(path);

    return Encoders::ToUri(PortableFunctions::PathGetFilename(path));
}


std::optional<unsigned> CSDocCompilerSettingsForBuildingChm::GetContextId(const std::wstring& context, bool use_if_exists)
{
    std::optional<unsigned> context_id = CSDocCompilerSettingsForBuilding::GetContextId(context, use_if_exists);

    if( context_id.has_value() )
    {
        const auto& lookup = m_generateTask->m_contextMap.find(*context_id);

        if( lookup != m_generateTask->m_contextMap.cend() )
        {
            throw CSProException(_T("The context '%s' (%d) has already been used for: %s"),
                                 context.c_str(), static_cast<int>(*context_id), lookup->second.c_str());
        }

        m_generateTask->m_contextMap.try_emplace(*context_id, GetCompilationFilename());
    }

    else if( !use_if_exists )
    {
        throw CSProException(_T("The context '%s' is unknown."), context.c_str());
    }

    return context_id;
}



// --------------------------------------------------------------------------
// DocSetBuilderChmGenerateTask
// --------------------------------------------------------------------------

DocSetBuilderChmGenerateTask::DocSetBuilderChmGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                           const DocBuildSettings& base_build_settings, std::wstring build_name,
                                                           bool throw_exceptions_for_serious_issues_when_validating_build_settings)
    :   DocSetBuilderBaseGenerateTask(CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(std::move(doc_set_spec), base_build_settings, DocBuildSettings::BuildType::Chm, std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings))
{
}


CSDocCompilerSettingsForBuildingChm& DocSetBuilderChmGenerateTask::GetSettings()
{
    return assert_cast<CSDocCompilerSettingsForBuildingChm&>(*m_csdocCompilerSettingsForBuilding);
}


const std::wstring& DocSetBuilderChmGenerateTask::AddChmInput(std::wstring filename)
{
    const std::wstring filename_only = PortableFunctions::PathGetFilename(filename);

    for( const std::wstring& previously_added_filename : m_chmInputFilenames )
    {
        if( SO::EqualsNoCase(filename_only, PortableFunctions::PathGetFilename(previously_added_filename)) )
        {
            // when a file has the same name (but is not the same file, as the path is different),
            // issue an error when the contents are different
            if( !SO::EqualsNoCase(filename, previously_added_filename) &&
                PortableFunctions::FileSizeAndModifiedTime(filename) != PortableFunctions::FileSizeAndModifiedTime(previously_added_filename) &&
                PortableFunctions::FileMd5(filename) != PortableFunctions::FileMd5(previously_added_filename) )
            {
                throw CSProException(_T("Multiple files with the same name cannot be built into a %s: %s"), ChmDisplayText, filename.c_str());
            }

            return previously_added_filename;
        }
    }

    return m_chmInputFilenames.emplace_back(std::move(filename));
}


void DocSetBuilderChmGenerateTask::ValidateInputs()
{
    DocSetBuilderBaseGenerateTask::ValidateInputs();

    if( IsInterfaceSet() && !PortableFunctions::FileIsRegular(GetInterface().GetGlobalSettings().html_help_compiler_path) )
    {
        throw CSProException(_T("The program %s must be installed to create %s files. Install the software and then add a reference to it in the Global Settings."),
                             HhcDisplayText, ChmDisplayText);
    }
}


void DocSetBuilderChmGenerateTask::ValidateInputsPostDocSetCompilation()
{
    CSDocCompilerSettingsForBuildingChm& settings = GetSettings();

    if( !GetDocSetSpec().GetTitle().has_value() )
        throw CSProException(_T("You cannot create a %s file without defining a title."), ChmDisplayText);

    // this will throw an exception is there is no default document
    const std::wstring default_document_path = settings.GetDefaultDocumentPath();
    m_defaultDocumentBuiltHtmlFilename = settings.GetBuiltHtmlFilename(default_document_path);

    // make sure that the button links are valid
    m_evaluatedButtonValues.clear();

    for( const auto& [link, text] : settings.GetBuildSettings().GetChmButtons() )
    {
        if( m_evaluatedButtonValues.size() == 4 )
        {
            throw CSProException(_T("Only two buttons can be added to a %s file so the button with text '%s' cannot be processed."),
                                 ChmDisplayText, text.c_str());
        }

        std::wstring& evaluated_link = m_evaluatedButtonValues.emplace_back(link);

        if( !SO::StartsWithNoCase(link, _T("http")) )
        {
            // evaluating links requires an output filename, so use the default document to set one
            m_csdocCompilerSettingsForBuilding->SetOutputFilename(GetCSDocOutputFilename(default_document_path));

            evaluated_link = CSDocCompilerWorker::EvaluateAndCreateUrlForTopicComponent(settings, evaluated_link);
        }

        m_evaluatedButtonValues.emplace_back(text);
    }

    m_evaluatedButtonValues.resize(4);
}


void DocSetBuilderChmGenerateTask::OnRun()
{
    const int64_t start_timestamp = GetTimestamp<int64_t>();

    GetInterface().SetTitle(FormatTextCS2WS(_T("Building Document Set to a %s file: %s"), ChmDisplayText, GetDocSetSpec().GetFilename().c_str()));

    // all compiled files will be saved to a temporary directory
    CreateTempOutputDirectory();

    RunBuild();

    GetInterface().LogText(FormatTextCS2WS(_T("\nBuild completed in %s."), GetElapsedTimeText(start_timestamp).c_str()));

    GetInterface().OnCreatedOutput(PortableFunctions::PathGetFilename(m_chmOutputFilename), m_chmOutputFilename);
}


std::tuple<double, double> DocSetBuilderChmGenerateTask::GetPreAndPostCompilationProgressPercents(size_t /*num_csdocs*/)
{
    constexpr double PreCSDocCompilationProgressPercent = 2;
    constexpr double ChmGenerationProgressPercent       = 20;

    return { PreCSDocCompilationProgressPercent, ChmGenerationProgressPercent };
}


void DocSetBuilderChmGenerateTask::OnPreCSDocCompilation()
{
    GetSettings().RunPreCompilationTasks(*this);

    m_chmOutputFilename = m_csdocCompilerSettingsForBuilding->GetDocSetBuildOutputFilename();
    FileIO::CreateDirectoriesForFile(m_chmOutputFilename);
    PortableFunctions::FileDelete(m_chmOutputFilename);
}


std::wstring DocSetBuilderChmGenerateTask::GetCSDocOutputFilename(const std::wstring& csdoc_filename)
{
    CSDocCompilerSettingsForBuildingChm& settings = GetSettings();

    // return a fake filename (with the the right name in the ultimate output directory)
    return PortableFunctions::PathAppendToPath(settings.GetDocSetBuildOutputDirectory(),
                                               settings.GetBuiltHtmlFilename(csdoc_filename));
}


void DocSetBuilderChmGenerateTask::OnCSDocCompilationResult(const std::wstring& csdoc_filename, const std::wstring& output_filename, const std::wstring& html)
{
    const std::wstring& built_html_filename = AddChmInput(PortableFunctions::PathAppendToPath(GetTempOutputDirectory(),
                                                                                              PortableFunctions::PathGetFilename(output_filename)));

    DocSetBuilderBaseGenerateTask::OnCSDocCompilationResult(csdoc_filename, built_html_filename, html);
}


void DocSetBuilderChmGenerateTask::OnPostCSDocCompilation()
{
    const std::wstring hh_base_filename = PortableFunctions::PathAppendToPath(GetTempOutputDirectory(),
                                                                              PortableFunctions::PathGetFilenameWithoutExtension(m_chmOutputFilename));

    const std::wstring hhc_filename = GetDocSetSpec().GetTableOfContents().has_value() ? ( hh_base_filename + _T(".hhc") ) : std::wstring();
    const std::wstring hhk_filename = GetDocSetSpec().GetIndex().has_value() ? ( hh_base_filename + _T(".hhk") ) : std::wstring();
    const std::wstring hhp_filename = hh_base_filename + _T(".hhp");

    if( !hhc_filename.empty() )
    {
        auto file = OpenChmFileForOutput(hhc_filename);
        WriteChmTableOfContentsFile(*file);

        if( IsCanceled() )
            return;
    }

    if( !hhk_filename.empty() )
    {
        auto file = OpenChmFileForOutput(hhk_filename);
        WriteChmIndexFile(*file);

        if( IsCanceled() )
            return;
    }

    WriteChmProjectFile(hhp_filename, hhc_filename, hhk_filename);

    if( IsCanceled() )
        return;

    // create the CHM
    std::wstring command_line = EscapeCommandLineArgument(GetInterface().GetGlobalSettings().html_help_compiler_path) +
                                _T(" ") + EscapeCommandLineArgument(hhp_filename);

    GetInterface().LogText(FormatTextCS2WS(_T("\nCreating %s file using %s: %s"), ChmDisplayText, HhcDisplayText, command_line.c_str()));

    GenerateTaskProcessRunner process_runner(*this, HhcDisplayText, _T("hhc"), &ProcessRunner::ReadStdOut);

    process_runner.SetOutputPreprocessor(
        [&](std::wstring& output)
        {
            // for some reason lots of \r characters (without a matching \n) end up in the output
            SO::Remove(output, '\r');
            SO::ConvertTabsToSpaces(output);
        });

    process_runner.Run(std::move(command_line));

    if( IsCanceled() )
    {
        PortableFunctions::FileDelete(m_chmOutputFilename);
        return;
    }

    if( !PortableFunctions::FileIsRegular(m_chmOutputFilename) )
        throw CSProException(_T("There was a problem creating the %s file."), ChmDisplayText);
}


std::unique_ptr<CStdioFileUnicode> DocSetBuilderChmGenerateTask::OpenChmFileForOutput(const std::wstring& filename)
{
    auto file = std::make_unique<CStdioFileUnicode>();

    file->SetEncoding(Encoding::Ansi);

    if( !file->Open(filename.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::typeText) )
        throw FileIO::Exception::FileOpenError(filename);

    return file;
}


void DocSetBuilderChmGenerateTask::WriteChmProjectFile(const std::wstring& hhp_filename, const std::wstring& hhc_filename, const std::wstring& hhk_filename)
{
    CSDocCompilerSettingsForBuildingChm& settings = GetSettings();

    auto file = OpenChmFileForOutput(hhp_filename);

    file->WriteLine(_T("[OPTIONS]"));
    
    file->WriteLine(_T("Compiled File=") + m_chmOutputFilename);
    file->WriteLine(_T("Title=") + *GetDocSetSpec().GetTitle());

    if( !hhc_filename.empty() )
        file->WriteLine(_T("Contents File=") + hhc_filename);

    if( !hhk_filename.empty() )
        file->WriteLine(_T("Index File=") + hhk_filename);

    file->WriteLine(_T("Default topic=") + settings.GetBuiltHtmlFilename(settings.GetDefaultDocumentPath()));
    file->WriteLine(_T("Default Window=main"));
    file->WriteLine(_T("Auto Index=No"));
    file->WriteLine(_T("Binary Index=Yes"));
    file->WriteLine(_T("Binary TOC=No"));
    file->WriteLine(_T("Flat=No"));
    file->WriteLine(_T("Full-text search=Yes"));
    file->WriteLine(_T("Language=0x409 English (United States)"));
    file->WriteLine(_T("Display compile progress=Yes"));

    file->WriteLine(_T("[WINDOWS]"));

    constexpr int window_properties = HHWIN_PROP_TRI_PANE | HHWIN_PROP_AUTO_SYNC | HHWIN_PROP_TAB_SEARCH |
                                      HHWIN_PROP_TAB_ADVSEARCH | HHWIN_PROP_USER_POS;

    ASSERT(m_evaluatedButtonValues.size() == 4);

    const int button_properties = HHWIN_BUTTON_EXPAND | HHWIN_BUTTON_BACK | HHWIN_BUTTON_FORWARD |
                                  HHWIN_BUTTON_HOME   | HHWIN_BUTTON_SYNC | HHWIN_BUTTON_PRINT |
                                  ( m_evaluatedButtonValues.front().empty() ? 0 : HHWIN_BUTTON_JUMP1 ) |
                                  ( m_evaluatedButtonValues[2].empty()      ? 0 : HHWIN_BUTTON_JUMP2 );

    file->WriteFormattedLine(_T("main=\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%d,%d,%d,%s"),
                             _T(""),                                     // window caption (no need to specify as it is specified above)
                             hhc_filename.c_str(),                       // table of contents file
                             hhk_filename.c_str(),                       // index file
                             _T(""),                                     // default topic (no need to specify as it is specified above)
                             m_defaultDocumentBuiltHtmlFilename.c_str(), // home topic
                             m_evaluatedButtonValues.front().c_str(),    // button 1 link
                             m_evaluatedButtonValues[1].c_str(),         // button 1 text
                             m_evaluatedButtonValues[2].c_str(),         // button 2 link
                             m_evaluatedButtonValues.back().c_str(),     // button 2 link
                             window_properties,                          // HHWIN_PROP_ settings
                             0,                                          // navigation pane width
                             button_properties,                          // HHWIN_BUTTON_ settings
                             _T("[0,0,800,600]"));                       // default window position

    file->WriteLine(_T("[FILES]"));

    for( const std::wstring& filename : m_chmInputFilenames )
        file->WriteLine(filename);

    if( !m_contextMap.empty() )
        WriteChmProjectFileContextIds(*file);
}


void DocSetBuilderChmGenerateTask::WriteChmProjectFileContextIds(CStdioFileUnicode& file)
{
    CSDocCompilerSettingsForBuildingChm& settings = GetSettings();

    // write context entries that are used and map all others (that begin with valid prefixes) to the default document
    std::vector<std::tuple<unsigned, const std::wstring*>> entries_for_map_section;

    file.WriteLine(_T("[ALIAS]"));

    for( const auto& [context, context_id] : GetDocSetSpec().GetContextIds() )
    {
        const unsigned context_id_adjustment = SO::StartsWith(context, _T("ID_"))  ? 0x10000 :
                                               SO::StartsWith(context, _T("IDD_")) ? 0x20000 :
                                               SO::StartsWith(context, _T("IDR_")) ? 0x20000 :
                                                                                     0;

        if( context_id_adjustment != 0 )
        {
            entries_for_map_section.emplace_back(context_id | context_id_adjustment, &context);

            const auto& used_lookup = m_contextMap.find(context_id);

            file.WriteFormattedLine(_T("%s=%s"), context.c_str(), ( used_lookup != m_contextMap.cend() ) ? settings.GetBuiltHtmlFilename(used_lookup->second).c_str() :
                                                                                                           m_defaultDocumentBuiltHtmlFilename.c_str());
        }
    }

    file.WriteLine(_T("[MAP]"));

    for( const auto& [adjusted_context_id, context] : entries_for_map_section )
        file.WriteFormattedLine(_T("#define %s %d"), context->c_str(), static_cast<int>(adjusted_context_id));
}



// --------------------------------------------------------------------------
// DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter
// --------------------------------------------------------------------------

class DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter
{
public:
    IndexTableOfContentsBaseWriter(CSDocCompilerSettingsForBuilding& settings, CStdioFileUnicode& file);
    ~IndexTableOfContentsBaseWriter();

protected:
    void WriteObject(const TCHAR* type, std::initializer_list<std::tuple<const TCHAR*, const TCHAR*>> params);

    void WriteSitemapEntry(const std::wstring& csdoc_filename, const std::wstring* title_override);

    struct Tags
    {
        static constexpr const TCHAR* ul_start = _T("<ul>");
        static constexpr const TCHAR* ul_end   = _T("</ul>");
        static constexpr const TCHAR* li_start = _T("<li>");
        static constexpr const TCHAR* li_end   = _T("</li>");
    };

protected:
    CSDocCompilerSettingsForBuilding& m_settings;
    CStdioFileUnicode& m_file;
};


DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter::IndexTableOfContentsBaseWriter(CSDocCompilerSettingsForBuilding& settings, CStdioFileUnicode& file)
    :   m_settings(settings),
        m_file(file)
{
    m_file.WriteLine(_T("<html>"));
}


DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter::~IndexTableOfContentsBaseWriter()
{
    m_file.WriteLine(_T("</html>"));
}


void DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter::WriteObject(const TCHAR* type, std::initializer_list<std::tuple<const TCHAR*, const TCHAR*>> params)
{
    ASSERT(Encoders::ToHtmlTagValue(type) == type);

    m_file.WriteString(_T("<object type=\"text/"));
    m_file.WriteString(type);
    m_file.WriteLine(_T("\">"));

    for( const auto& [name, value] : params )
    {
        ASSERT(Encoders::ToHtmlTagValue(name) == name);

        m_file.WriteString(_T("<param name=\""));
        m_file.WriteString(name);
        m_file.WriteString(_T("\" value=\""));
        m_file.WriteString(Encoders::ToHtmlTagValue(value));
        m_file.WriteLine(_T("\"/>"));
    }

    m_file.WriteLine(_T("</object>"));
}


void DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter::WriteSitemapEntry(const std::wstring& csdoc_filename, const std::wstring* title_override)
{
    const std::wstring title = ( title_override != nullptr ) ? *title_override :
                                                               m_settings.GetTitle(csdoc_filename);
    const std::wstring built_filename = m_settings.GetBuiltHtmlFilename(csdoc_filename);

    m_file.WriteString(Tags::li_start); // <li> cannot be followed by a newline

    WriteObject(_T("sitemap"),
        {
            { _T("Name"),  title.c_str() },
            { _T("Local"), built_filename.c_str() }
        });

    m_file.WriteLine(Tags::li_end);
}



// --------------------------------------------------------------------------
// DocSetBuilderChmGenerateTask::TableOfContentsWriter
// --------------------------------------------------------------------------

class DocSetBuilderChmGenerateTask::TableOfContentsWriter : public DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter, public DocSetTableOfContents::Writer
{
public:
    using IndexTableOfContentsBaseWriter::IndexTableOfContentsBaseWriter;

protected:
    void StartWriting(size_t num_root_nodes) override;

    void WriteProject(const std::wstring& project)override;

    void StartChapter(const std::wstring& title, bool write_title_to_pdf) override;
    void FinishChapter() override;

    void WriteDocument(const std::wstring& csdoc_filename, const std::wstring* title_override) override;

private:
    size_t m_chapterLevel = 0;
};



void DocSetBuilderChmGenerateTask::TableOfContentsWriter::StartWriting(size_t /*num_root_nodes*/)
{
    WriteObject(_T("site properties"),
        {
            { _T("SiteType"),    _T("toc") },
            { _T("Image Width"), _T("16") }
        });
}


void DocSetBuilderChmGenerateTask::TableOfContentsWriter::WriteProject(const std::wstring& project)
{
    const CSDocCompilerSettingsForBuilding& project_settings = m_settings.GetProjectSettings(project);
    const std::wstring project_built_path = project_settings.GetDocSetBuildOutputFilename();

    if( !project_settings.GetDocSetSpec().GetTableOfContents().has_value() )
    {
        throw CSProException(_T("The %s cannot link to a project that does not have a %s itself: %s"),
                             ToString(DocSetComponent::Type::TableOfContents),
                             ToString(DocSetComponent::Type::TableOfContents),
                             project_built_path.c_str());
    }

    const std::wstring project_hhc_url = FormatTextCS2WS(_T("%s::/%s.hhc"),
                                                         PortableFunctions::PathGetFilename(project_built_path),
                                                         PortableFunctions::PathGetFilenameWithoutExtension(project_built_path).c_str());

    WriteObject(_T("sitemap"),
        {
            { _T("Name"),  project_hhc_url.c_str() },
            { _T("Merge"), project_hhc_url.c_str() }
        });
}


void DocSetBuilderChmGenerateTask::TableOfContentsWriter::StartChapter(const std::wstring& title, bool /*write_title_to_pdf*/)
{
    if( ++m_chapterLevel == 1 )
        m_file.WriteLine(Tags::ul_start);

    m_file.WriteString(Tags::li_start); // <li> cannot be followed by a newline

    WriteObject(_T("sitemap"),
        {
            { _T("Name"), title.c_str() }
        });

    m_file.WriteLine(Tags::ul_start);
}


void DocSetBuilderChmGenerateTask::TableOfContentsWriter::FinishChapter()
{
    m_file.WriteLine(Tags::ul_end);
    m_file.WriteLine(Tags::li_end);

    if( m_chapterLevel-- == 1 )
        m_file.WriteLine(Tags::ul_end);
}


void DocSetBuilderChmGenerateTask::TableOfContentsWriter::WriteDocument(const std::wstring& csdoc_filename, const std::wstring* title_override)
{
    WriteSitemapEntry(csdoc_filename, title_override);
}


void DocSetBuilderChmGenerateTask::WriteChmTableOfContentsFile(CStdioFileUnicode& file)
{
    ASSERT(GetDocSetSpec().GetTableOfContents().has_value());

    TableOfContentsWriter table_of_contents_writer(GetSettings(), file);
    table_of_contents_writer.Write(*GetDocSetSpec().GetTableOfContents());
}



// --------------------------------------------------------------------------
// DocSetBuilderChmGenerateTask::IndexWriter
// --------------------------------------------------------------------------

class DocSetBuilderChmGenerateTask::IndexWriter : public DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter, public DocSetIndex::Writer
{
public:
    using IndexTableOfContentsBaseWriter::IndexTableOfContentsBaseWriter;

protected:
    void StartWriting() override;

    void StartEntries() override;
    void WriteEntry(const std::wstring& csdoc_filename, const std::wstring* title_override, const void* subentries_tag) override;
    void FinishEntries() override;
};


void DocSetBuilderChmGenerateTask::IndexWriter::StartWriting()
{
    WriteObject(_T("site properties"),
        {
            { _T("SiteType"), _T("index") }
        });
}


void DocSetBuilderChmGenerateTask::IndexWriter::StartEntries()
{
    m_file.WriteLine(Tags::ul_start);
}


void DocSetBuilderChmGenerateTask::IndexWriter::WriteEntry(const std::wstring& csdoc_filename, const std::wstring* title_override, const void* subentries_tag)
{
    WriteSitemapEntry(csdoc_filename, title_override);

    if( subentries_tag != nullptr )
        WriteSubentries(subentries_tag);
}


void DocSetBuilderChmGenerateTask::IndexWriter::FinishEntries()
{
    m_file.WriteLine(Tags::ul_end);
}


void DocSetBuilderChmGenerateTask::WriteChmIndexFile(CStdioFileUnicode& file)
{
    ASSERT(GetDocSetSpec().GetIndex().has_value());

    IndexWriter index_writer(GetSettings(), file);
    index_writer.Write(*GetDocSetSpec().GetIndex());
}
