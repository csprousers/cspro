#include "StdAfx.h"
#include "DocSetBuilder.h"
#include "CSDocCompilerSettings.h"
#include "CSDocCompilerWorker.h"
#include "DocSetBuilderCache.h"
#include "GenerateTaskProcessRunner.h"
#include <zToolsO/File.h>


namespace
{
    constexpr const char* HhcDisplayText = "Microsoft HTML Help Compiler";
    constexpr const char* ChmDisplayText = "Compiled HTML Help";
}


// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuildingChm
// --------------------------------------------------------------------------

std::string CSDocCompilerSettingsForBuildingChm::CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::string& path) const
{
    ASSERT(m_buildSettings.GetProjectLinkageAction() == DocBuildSettings::ProjectLinkageAction::Link);

    const std::string project_output_file_path = project_settings.GetDocSetBuildOutputFilePath();

    return SO::Concatenate(CreateRelativeUrlForPath(project_output_file_path),
                           "::",
                           Encoders::ToUri(GetBuiltHtmlFilename(path)));
}


std::string CSDocCompilerSettingsForBuildingChm::GetDefaultDocumentFilePath() const
{
    return GetDocSetBuilderCache().GetDefaultDocumentFilePath(*m_docSetSpec, true);
}


void CSDocCompilerSettingsForBuildingChm::RunPreCompilationTasks(DocSetBuilderChmGenerateTask& generate_task)
{
    m_generateTask = &generate_task;
}


std::string CSDocCompilerSettingsForBuildingChm::GetStylesheetsHtml()
{
    if( m_buildSettings.GetStylesheetAction() == DocBuildSettings::StylesheetAction::Embed )
        return CSDocCompilerSettingsForBuilding::GetStylesheetsHtml();

    // if not embedded, the stylesheet will be linked as if it were in the same location as the compiled HTML
    if( m_generateTask->m_nonEmbeddedStylesheetHtml.empty() )
    {
        const std::string& source_css_file_path = m_generateTask->AddChmInput(GetStylesheetCssFilePath(CSDocStylesheetFilename));
        m_generateTask->m_nonEmbeddedStylesheetHtml = GetStylesheetLinkHtml(Encoders::ToUri(PortableFunctions::PathGetFilename(source_css_file_path)));
    }

    return m_generateTask->m_nonEmbeddedStylesheetHtml;
}


std::string CSDocCompilerSettingsForBuildingChm::EvaluateBuildExtra(const std::string& path)
{
    std::string evaluated_path = CSDocCompilerSettings::EvaluateBuildExtra(path);

    CreatePathAndCopyFileToDirectory(evaluated_path, m_generateTask->GetTempOutputDirectory());

    return evaluated_path;
}


std::string CSDocCompilerSettingsForBuildingChm::CreateUrlForImageFile(const std::string& path)
{
    if( m_buildSettings.GetImageAction() == DocBuildSettings::ImageAction::DataUrl )
        return CSDocCompilerSettingsForBuilding::CreateUrlForImageFile(path);

    // if not a data URL, the image will be linked as if it were in the same location as the compiled HTML
    m_generateTask->AddChmInput(path);

    return Encoders::ToUri(PortableFunctions::PathGetFilename(path));
}


std::optional<unsigned> CSDocCompilerSettingsForBuildingChm::GetContextId(const std::string& context, const bool use_if_exists)
{
    std::optional<unsigned> context_id = CSDocCompilerSettingsForBuilding::GetContextId(context, use_if_exists);

    if( context_id.has_value() )
    {
        const auto& lookup = m_generateTask->m_contextMap.find(*context_id);

        if( lookup != m_generateTask->m_contextMap.cend() )
        {
            throw CSProException("The context '%s' (%d) has already been used for: %s",
                                 context.c_str(),
                                 static_cast<int>(*context_id),
                                 lookup->second.c_str());
        }

        m_generateTask->m_contextMap.try_emplace(*context_id, GetCompilationFilePath());
    }

    else if( !use_if_exists )
    {
        throw CSProException("The context '%s' is unknown.", context.c_str());
    }

    return context_id;
}



// --------------------------------------------------------------------------
// DocSetBuilderChmGenerateTask
// --------------------------------------------------------------------------

DocSetBuilderChmGenerateTask::DocSetBuilderChmGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                           const DocBuildSettings& base_build_settings, std::string build_name,
                                                           const bool throw_exceptions_for_serious_issues_when_validating_build_settings)
    :   DocSetBuilderBaseGenerateTask(CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(std::move(doc_set_spec), base_build_settings,
                                      DocBuildSettings::BuildType::Chm, std::move(build_name),
                                      throw_exceptions_for_serious_issues_when_validating_build_settings))
{
}


CSDocCompilerSettingsForBuildingChm& DocSetBuilderChmGenerateTask::GetSettings()
{
    return assert_cast<CSDocCompilerSettingsForBuildingChm&>(*m_csdocCompilerSettingsForBuilding);
}


const std::string& DocSetBuilderChmGenerateTask::AddChmInput(std::string file_path)
{
    const std::string filename = PortableFunctions::PathGetFilename(file_path);

    for( const std::string& previously_added_file_path : m_chmInputFilePaths )
    {
        if( SO::EqualsNoCase(filename, PortableFunctions::PathGetFilename(previously_added_file_path)) )
        {
            // when a file has the same name (but is not the same file, as the path is different),
            // issue an error when the contents are different
            if( !SO::EqualsNoCase(file_path, previously_added_file_path) &&
                PortableFunctions::FileSizeAndModifiedTime(file_path) != PortableFunctions::FileSizeAndModifiedTime(previously_added_file_path) &&
                PortableFunctions::FileMd5(file_path) != PortableFunctions::FileMd5(previously_added_file_path) )
            {
                throw CSProException("Multiple files with the same name cannot be built into a %s: %s",
                                     ChmDisplayText, file_path.c_str());
            }

            return previously_added_file_path;
        }
    }

    return m_chmInputFilePaths.emplace_back(std::move(file_path));
}


void DocSetBuilderChmGenerateTask::ValidateInputs()
{
    DocSetBuilderBaseGenerateTask::ValidateInputs();

    if( IsInterfaceSet() && !PortableFunctions::FileIsRegular(GetInterface().GetGlobalSettings().html_help_compiler_path) )
    {
        throw CSProException("The program %s must be installed to create %s files. "
                             "Install the software and then add a reference to it in the Global Settings.",
                             HhcDisplayText, ChmDisplayText);
    }
}


void DocSetBuilderChmGenerateTask::ValidateInputsPostDocSetCompilation()
{
    CSDocCompilerSettingsForBuildingChm& settings = GetSettings();

    if( !GetDocSetSpec().GetTitle().has_value() )
    {
        throw CSProException("You cannot create a %s file without defining a title.",
                             ChmDisplayText);
    }

    // this will throw an exception is there is no default document
    const std::string default_document_file_path = settings.GetDefaultDocumentFilePath();
    m_defaultDocumentBuiltHtmlFilename = settings.GetBuiltHtmlFilename(default_document_file_path);

    // make sure that the button links are valid
    m_evaluatedButtonValues.clear();

    for( const auto& [link, text] : settings.GetBuildSettings().GetChmButtons() )
    {
        if( m_evaluatedButtonValues.size() == 4 )
        {
            throw CSProException("Only two buttons can be added to a %s file so the button with text '%s' cannot be processed.",
                                 ChmDisplayText, text.c_str());
        }

        std::string& evaluated_link = m_evaluatedButtonValues.emplace_back(link);

        if( !SO::StartsWithNoCase(link, "http") )
        {
            // evaluating links requires an output file path, so use the default document to set one
            m_csdocCompilerSettingsForBuilding->SetOutputFilePath(GetCSDocOutputFilePath(default_document_file_path));

            evaluated_link = CSDocCompilerWorker::EvaluateAndCreateUrlForTopicComponent(settings, evaluated_link);
        }

        m_evaluatedButtonValues.emplace_back(text);
    }

    m_evaluatedButtonValues.resize(4);
}


void DocSetBuilderChmGenerateTask::OnRun()
{
    const int64_t start_timestamp = GetTimestamp<int64_t>();

    GetInterface().SetTitle(FormatText("Building Document Set to a %s file: %s", ChmDisplayText, GetDocSetSpec().GetFilePath().c_str()));

    // all compiled files will be saved to a temporary directory
    CreateTempOutputDirectory();

    RunBuild();

    GetInterface().LogText("\nBuild completed in %s.", GetElapsedTimeText(start_timestamp, GetTimestamp<int64_t>()).c_str());

    GetInterface().OnCreatedOutput(PortableFunctions::PathGetFilename(m_chmOutputFilePath), m_chmOutputFilePath);
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

    m_chmOutputFilePath = m_csdocCompilerSettingsForBuilding->GetDocSetBuildOutputFilePath();
    FileIO::CreateDirectoriesForFile(m_chmOutputFilePath);
    PortableFunctions::FileDelete(m_chmOutputFilePath);
}


std::string DocSetBuilderChmGenerateTask::GetCSDocOutputFilePath(const std::string& csdoc_file_path)
{
    CSDocCompilerSettingsForBuildingChm& settings = GetSettings();

    // return a fake file path (with the the right name in the ultimate output directory)
    return Path::Combine(settings.GetDocSetBuildOutputDirectory(),
                         settings.GetBuiltHtmlFilename(csdoc_file_path));
}


void DocSetBuilderChmGenerateTask::OnCSDocCompilationResult(const std::string& csdoc_file_path, const std::string& output_file_path, const std::string& html)
{
    const std::string& built_html_file_path = AddChmInput(Path::Combine(GetTempOutputDirectory(),
                                                                        PortableFunctions::PathGetFilename(output_file_path)));

    DocSetBuilderBaseGenerateTask::OnCSDocCompilationResult(csdoc_file_path, built_html_file_path, html);
}


void DocSetBuilderChmGenerateTask::OnPostCSDocCompilation()
{
    const std::string hh_base_file_path = Path::Combine(GetTempOutputDirectory(),
                                                        Path::GetFilenameWithoutExtension(m_chmOutputFilePath));

    const std::string hhc_file_path = GetDocSetSpec().GetTableOfContents().has_value() ? ( hh_base_file_path + ".hhc" ) : std::string();
    const std::string hhk_file_path = GetDocSetSpec().GetIndex().has_value() ? ( hh_base_file_path + ".hhk" ) : std::string();
    const std::string hhp_file_path = hh_base_file_path + ".hhp";

    if( !hhc_file_path.empty() )
    {
        FileIO::TextFile text_file = OpenChmFileForOutput(hhc_file_path);
        WriteChmTableOfContentsFile(text_file);

        if( IsCanceled() )
            return;
    }

    if( !hhk_file_path.empty() )
    {
        FileIO::TextFile text_file = OpenChmFileForOutput(hhk_file_path);
        WriteChmIndexFile(text_file);

        if( IsCanceled() )
            return;
    }

    WriteChmProjectFile(hhp_file_path, hhc_file_path, hhk_file_path);

    if( IsCanceled() )
        return;

    // create the CHM
    const std::string command_line = SO::Concatenate(EscapeCommandLineArgument(GetInterface().GetGlobalSettings().html_help_compiler_path),
                                                     " ",
                                                     EscapeCommandLineArgument(hhp_file_path));

    GetInterface().LogText("\nCreating %s file using %s: %s", ChmDisplayText, HhcDisplayText, command_line.c_str());

    GenerateTaskProcessRunner process_runner(*this, HhcDisplayText, "hhc", &ProcessRunner::ReadStdOut);

    process_runner.SetOutputPreprocessor(
        [&](std::string& output)
        {
            // for some reason lots of \r characters (without a matching \n) end up in the output
            SO::Remove(output, '\r');
            SO::ConvertTabsToSpaces(output);
        });

    process_runner.Run(command_line);

    if( IsCanceled() )
    {
        PortableFunctions::FileDelete(m_chmOutputFilePath);
        return;
    }

    if( !PortableFunctions::FileIsRegular(m_chmOutputFilePath) )
        throw CSProException("There was a problem creating the %s file.", ChmDisplayText);
}


FileIO::TextFile DocSetBuilderChmGenerateTask::OpenChmFileForOutput(const std::string& file_path)
{
    FileIO::TextFile text_file;
    text_file.SetTextEncoding(TextEncoding::Type::Ansi);

    text_file.OpenForTextWritingCreate(file_path);

    return text_file;
}


void DocSetBuilderChmGenerateTask::WriteChmProjectFile(const std::string& hhp_file_path, const std::string& hhc_file_path, const std::string& hhk_file_path)
{
    CSDocCompilerSettingsForBuildingChm& settings = GetSettings();

    FileIO::TextFile text_file = OpenChmFileForOutput(hhp_file_path);

    text_file.WriteLine("[OPTIONS]");

    text_file.WriteLine("Compiled File=" + m_chmOutputFilePath);
    text_file.WriteLine("Title=" + *GetDocSetSpec().GetTitle());

    if( !hhc_file_path.empty() )
        text_file.WriteLine("Contents File=" + hhc_file_path);

    if( !hhk_file_path.empty() )
        text_file.WriteLine("Index File=" + hhk_file_path);

    text_file.WriteLine("Default topic=" + settings.GetBuiltHtmlFilename(settings.GetDefaultDocumentFilePath()));
    text_file.WriteLine("Default Window=main");
    text_file.WriteLine("Auto Index=No");
    text_file.WriteLine("Binary Index=Yes");
    text_file.WriteLine("Binary TOC=No");
    text_file.WriteLine("Flat=No");
    text_file.WriteLine("Full-text search=Yes");
    text_file.WriteLine("Language=0x409 English (United States)");
    text_file.WriteLine("Display compile progress=Yes");

    text_file.WriteLine("[WINDOWS]");

    constexpr int window_properties = HHWIN_PROP_TRI_PANE | HHWIN_PROP_AUTO_SYNC | HHWIN_PROP_TAB_SEARCH |
                                      HHWIN_PROP_TAB_ADVSEARCH | HHWIN_PROP_USER_POS;

    ASSERT(m_evaluatedButtonValues.size() == 4);

    const int button_properties = HHWIN_BUTTON_EXPAND | HHWIN_BUTTON_BACK | HHWIN_BUTTON_FORWARD |
                                  HHWIN_BUTTON_HOME   | HHWIN_BUTTON_SYNC | HHWIN_BUTTON_PRINT |
                                  ( m_evaluatedButtonValues.front().empty() ? 0 : HHWIN_BUTTON_JUMP1 ) |
                                  ( m_evaluatedButtonValues[2].empty()      ? 0 : HHWIN_BUTTON_JUMP2 );

    text_file.WriteFormattedLine("main=\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%d,%d,%d,%s",
                                 "",                                         // window caption (no need to specify as it is specified above)
                                 hhc_file_path.c_str(),                      // table of contents file
                                 hhk_file_path.c_str(),                      // index file
                                 "",                                         // default topic (no need to specify as it is specified above)
                                 m_defaultDocumentBuiltHtmlFilename.c_str(), // home topic
                                 m_evaluatedButtonValues.front().c_str(),    // button 1 link
                                 m_evaluatedButtonValues[1].c_str(),         // button 1 text
                                 m_evaluatedButtonValues[2].c_str(),         // button 2 link
                                 m_evaluatedButtonValues.back().c_str(),     // button 2 link
                                 window_properties,                          // HHWIN_PROP_ settings
                                 0,                                          // navigation pane width
                                 button_properties,                          // HHWIN_BUTTON_ settings
                                 "[0,0,800,600]");                           // default window position

    text_file.WriteLine("[FILES]");

    for( const std::string& file_path : m_chmInputFilePaths )
        text_file.WriteLine(file_path);

    if( !m_contextMap.empty() )
        WriteChmProjectFileContextIds(text_file);
}


void DocSetBuilderChmGenerateTask::WriteChmProjectFileContextIds(FileIO::TextFile& text_file)
{
    CSDocCompilerSettingsForBuildingChm& settings = GetSettings();

    // write context entries that are used and map all others (that begin with valid prefixes) to the default document
    std::vector<std::tuple<unsigned, const std::string*>> entries_for_map_section;

    text_file.WriteLine("[ALIAS]");

    for( const auto& [context, context_id] : GetDocSetSpec().GetContextIds() )
    {
        const unsigned context_id_adjustment = SO::StartsWith(context, "ID_")  ? 0x10000 :
                                               SO::StartsWith(context, "IDD_") ? 0x20000 :
                                               SO::StartsWith(context, "IDR_") ? 0x20000 :
                                                                                 0;

        if( context_id_adjustment != 0 )
        {
            entries_for_map_section.emplace_back(context_id | context_id_adjustment, &context);

            const auto& used_lookup = m_contextMap.find(context_id);

            text_file.WriteFormattedLine("%s=%s", context.c_str(), ( used_lookup != m_contextMap.cend() ) ? settings.GetBuiltHtmlFilename(used_lookup->second).c_str() :
                                                                                                            m_defaultDocumentBuiltHtmlFilename.c_str());
        }
    }

    text_file.WriteLine("[MAP]");

    for( const auto& [adjusted_context_id, context] : entries_for_map_section )
        text_file.WriteFormattedLine("#define %s %d", context->c_str(), static_cast<int>(adjusted_context_id));
}



// --------------------------------------------------------------------------
// DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter
// --------------------------------------------------------------------------

class DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter
{
public:
    IndexTableOfContentsBaseWriter(CSDocCompilerSettingsForBuilding& settings, FileIO::TextFile& text_file);
    ~IndexTableOfContentsBaseWriter();

protected:
    void WriteObject(const char* type, std::initializer_list<std::tuple<const char*, const char*>> params);

    void WriteSitemapEntry(const std::string& csdoc_file_path, const std::string* title_override);

    struct Tags
    {
        static constexpr const char* ul_start = "<ul>";
        static constexpr const char* ul_end   = "</ul>";
        static constexpr const char* li_start = "<li>";
        static constexpr const char* li_end   = "</li>";
    };

protected:
    CSDocCompilerSettingsForBuilding& m_settings;
    FileIO::TextFile& m_textFile;
};


DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter::IndexTableOfContentsBaseWriter(CSDocCompilerSettingsForBuilding& settings, FileIO::TextFile& text_file)
    :   m_settings(settings),
        m_textFile(text_file)
{
    m_textFile.WriteLine("<html>");
}


DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter::~IndexTableOfContentsBaseWriter()
{
    m_textFile.WriteLine("</html>");
}


void DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter::WriteObject(const char* const type, const std::initializer_list<std::tuple<const char*, const char*>> params)
{
    ASSERT(Encoders::ToHtmlTagValue(type) == type);

    m_textFile.WriteString("<object type=\"text/");
    m_textFile.WriteString(type);
    m_textFile.WriteLine("\">");

    for( const auto& [name, value] : params )
    {
        ASSERT(Encoders::ToHtmlTagValue(name) == name);

        m_textFile.WriteString("<param name=\"");
        m_textFile.WriteString(name);
        m_textFile.WriteString("\" value=\"");
        m_textFile.WriteString(Encoders::ToHtmlTagValue(value));
        m_textFile.WriteLine("\"/>");
    }

    m_textFile.WriteLine("</object>");
}


void DocSetBuilderChmGenerateTask::IndexTableOfContentsBaseWriter::WriteSitemapEntry(const std::string& csdoc_file_path, const std::string* const title_override)
{
    const std::string title = ( title_override != nullptr ) ? *title_override :
                                                              m_settings.GetTitle(csdoc_file_path);
    const std::string built_filename = m_settings.GetBuiltHtmlFilename(csdoc_file_path);

    m_textFile.WriteString(Tags::li_start); // <li> cannot be followed by a newline

    WriteObject("sitemap",
        {
            { "Name",  title.c_str()          },
            { "Local", built_filename.c_str() }
        });

    m_textFile.WriteLine(Tags::li_end);
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

    void WriteProject(const std::string& project) override;

    void StartChapter(const std::string& title, bool write_title_to_pdf) override;
    void FinishChapter() override;

    void WriteDocument(const std::string& csdoc_file_path, const std::string* title_override) override;

private:
    size_t m_chapterLevel = 0;
};



void DocSetBuilderChmGenerateTask::TableOfContentsWriter::StartWriting(size_t /*num_root_nodes*/)
{
    WriteObject("site properties",
        {
            { "SiteType",    "toc" },
            { "Image Width", "16"  }
        });
}


void DocSetBuilderChmGenerateTask::TableOfContentsWriter::WriteProject(const std::string& project)
{
    const CSDocCompilerSettingsForBuilding& project_settings = m_settings.GetProjectSettings(project);
    const std::string project_built_file_path = project_settings.GetDocSetBuildOutputFilePath();

    if( !project_settings.GetDocSetSpec().GetTableOfContents().has_value() )
    {
        throw CSProException("The %s cannot link to a project that does not have a %s itself: %s",
                             ToString(DocSetComponent::Type::TableOfContents),
                             ToString(DocSetComponent::Type::TableOfContents),
                             project_built_file_path.c_str());
    }

    const std::string project_hhc_url = FormatText("%s::/%s.hhc",
                                                   PortableFunctions::PathGetFilename(project_built_file_path).c_str(),
                                                   Path::GetFilenameWithoutExtension(project_built_file_path).c_str());

    WriteObject("sitemap",
        {
            { "Name",  project_hhc_url.c_str() },
            { "Merge", project_hhc_url.c_str() }
        });
}


void DocSetBuilderChmGenerateTask::TableOfContentsWriter::StartChapter(const std::string& title, bool /*write_title_to_pdf*/)
{
    if( ++m_chapterLevel == 1 )
        m_textFile.WriteLine(Tags::ul_start);

    m_textFile.WriteString(Tags::li_start); // <li> cannot be followed by a newline

    WriteObject("sitemap",
        {
            { "Name", title.c_str() }
        });

    m_textFile.WriteLine(Tags::ul_start);
}


void DocSetBuilderChmGenerateTask::TableOfContentsWriter::FinishChapter()
{
    m_textFile.WriteLine(Tags::ul_end);
    m_textFile.WriteLine(Tags::li_end);

    if( m_chapterLevel-- == 1 )
        m_textFile.WriteLine(Tags::ul_end);
}


void DocSetBuilderChmGenerateTask::TableOfContentsWriter::WriteDocument(const std::string& csdoc_file_path, const std::string* const title_override)
{
    WriteSitemapEntry(csdoc_file_path, title_override);
}


void DocSetBuilderChmGenerateTask::WriteChmTableOfContentsFile(FileIO::TextFile& text_file)
{
    ASSERT(GetDocSetSpec().GetTableOfContents().has_value());

    TableOfContentsWriter table_of_contents_writer(GetSettings(), text_file);
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
    void WriteEntry(const std::string& csdoc_file_path, const std::string* title_override, const void* subentries_tag) override;
    void FinishEntries() override;
};


void DocSetBuilderChmGenerateTask::IndexWriter::StartWriting()
{
    WriteObject("site properties",
        {
            { "SiteType", "index" }
        });
}


void DocSetBuilderChmGenerateTask::IndexWriter::StartEntries()
{
    m_textFile.WriteLine(Tags::ul_start);
}


void DocSetBuilderChmGenerateTask::IndexWriter::WriteEntry(const std::string& csdoc_file_path, const std::string* const title_override, const void* const subentries_tag)
{
    WriteSitemapEntry(csdoc_file_path, title_override);

    if( subentries_tag != nullptr )
        WriteSubentries(subentries_tag);
}


void DocSetBuilderChmGenerateTask::IndexWriter::FinishEntries()
{
    m_textFile.WriteLine(Tags::ul_end);
}


void DocSetBuilderChmGenerateTask::WriteChmIndexFile(FileIO::TextFile& text_file)
{
    ASSERT(GetDocSetSpec().GetIndex().has_value());

    IndexWriter index_writer(GetSettings(), text_file);
    index_writer.Write(*GetDocSetSpec().GetIndex());
}
