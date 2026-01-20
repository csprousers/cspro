#include "StdAfx.h"
#include "DocSetBuilder.h"
#include "CSDocCompilerWorker.h"
#include "DocSetBuilderCache.h"


// --------------------------------------------------------------------------
// DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator
// (declaration -- the definition is at end of the file)
// --------------------------------------------------------------------------

class DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator : public DocSetTableOfContents::Writer
{
public:
    TableOfContentsEvaluator(DocSetBuilderHtmlWebsiteGenerateTask& generate_task);

    std::tuple<std::string, std::string> GetTableOfContentsHtml(const std::string& csdoc_file_path);

protected:
    // DocSetTableOfContents::Writer overrides
    void WriteProject(const std::string& project) override;
    void StartChapter(const std::string& title, bool write_title_to_pdf) override;
    void FinishChapter() override;
    void WriteDocument(const std::string& csdoc_file_path, const std::string* title_override) override;

private:
    struct TitleAndFilePath
    {
        std::string title;
        std::string file_path;
    };

    struct Node
    {
        TitleAndFilePath title_and_file_path; // the file path is empty for chapters
        std::string project;

        std::vector<TitleAndFilePath> documents;
        std::vector<Node> subchapters;
    };

private:
    const std::string& GetFirstDocumentFilePathInNode(const Node& node) const;

    bool NodeContainsDocument(const Node& node, const std::string& csdoc_file_path) const;

    void WriteHtmlForNode(std::string& table_of_contents_html, const Node& node, const std::string& csdoc_file_path);

    void WriteHtmlForLiTagAndLink(std::string& table_of_contents_html, const std::string& project, const std::string& csdoc_file_path,
                                  const std::string& title, bool end_li_tag, const char* li_class_text);

private:
    DocSetBuilderHtmlWebsiteGenerateTask& m_generateTask;
    std::vector<Node> m_nodes;
    std::stack<Node*> m_currentChapterNode;
};



// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuildingHtmlWebsite
// --------------------------------------------------------------------------

void CSDocCompilerSettingsForBuildingHtmlWebsite::RunPreCompilationTasks(DocSetBuilderHtmlWebsiteGenerateTask& generate_task)
{
    m_generateTask = &generate_task;

    // if defined, the Document Set title will be postpended to the title
    if( m_docSetSpec->GetTitle().has_value() )
        m_titlePostfix = " - " + *m_docSetSpec->GetTitle();

    // if using a particular directory for stylesheets, copy the stylesheet images there
    if( m_buildSettings.GetStylesheetAction() == DocBuildSettings::StylesheetAction::Directory )
    {
        const std::string stylesheet_directory = EvaluateDirectoryRelativeToOutputDirectory(m_buildSettings.GetStylesheetDirectory(), false);
        CopyStylesheetImages(stylesheet_directory);
    }
}


std::string CSDocCompilerSettingsForBuildingHtmlWebsite::GetHtmlHeaderTitle(const std::string& csdoc_file_path)
{
    return GetTitle(csdoc_file_path) + m_titlePostfix;
}


std::string CSDocCompilerSettingsForBuildingHtmlWebsite::GetStylesheetsHtml()
{
    // when embedding stylesheets, the stylesheet images must be copied to any directory where a HTML file is created
    if( m_buildSettings.GetStylesheetAction() == DocBuildSettings::StylesheetAction::Embed )
        CopyStylesheetImages(GetCSDocOutputDirectory());

    return GetStylesheetsHtmlWorker(CSDocStylesheetFilename) +
           GetStylesheetsHtmlWorker(DocSetWebStylesheetFilename);
}


void CSDocCompilerSettingsForBuildingHtmlWebsite::CopyStylesheetImages(const std::string& directory)
{
    // keep track of the directories where stylesheet images have been copied to avoid copying them over and over
    if( m_copiedStylesheetImageDirectories.find(directory) != m_copiedStylesheetImageDirectories.cend() )
        return;

    for( const std::string& source_file_path : GetDocSetBuilderCache().GetStylesheetImageFilePaths() )
    {
        const std::string destination_file_path = Path::Combine(directory, PortableFunctions::PathGetFilename(source_file_path));
        CopyFileToDirectory(source_file_path, destination_file_path);
    }

    m_copiedStylesheetImageDirectories.emplace(directory);
}


std::tuple<std::string, std::string> CSDocCompilerSettingsForBuildingHtmlWebsite::GetHtmlToWrapDocument()
{
    ASSERT(m_generateTask != nullptr && m_generateTask->m_tableOfContentsEvaluator != nullptr);
    return m_generateTask->m_tableOfContentsEvaluator->GetTableOfContentsHtml(GetCompilationFilePath());
}



// --------------------------------------------------------------------------
// DocSetBuilderHtmlWebsiteGenerateTask
// --------------------------------------------------------------------------

DocSetBuilderHtmlWebsiteGenerateTask::DocSetBuilderHtmlWebsiteGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                                           const DocBuildSettings& base_build_settings, std::string build_name,
                                                                           const bool throw_exceptions_for_serious_issues_when_validating_build_settings)
    :   DocSetBuilderBaseGenerateTask(CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(std::move(doc_set_spec), base_build_settings, DocBuildSettings::BuildType::HtmlWebsite,
                                                                                             std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings)),
        m_tableOfContentsEvaluator(std::make_unique<TableOfContentsEvaluator>(*this))
{
}


DocSetBuilderHtmlWebsiteGenerateTask::~DocSetBuilderHtmlWebsiteGenerateTask()
{
}


CSDocCompilerSettingsForBuildingHtmlWebsite& DocSetBuilderHtmlWebsiteGenerateTask::GetSettings()
{
    return assert_cast<CSDocCompilerSettingsForBuildingHtmlWebsite&>(*m_csdocCompilerSettingsForBuilding);
}


void DocSetBuilderHtmlWebsiteGenerateTask::ValidateInputsPostDocSetCompilation()
{
    if( !GetDocSetSpec().GetTableOfContents().has_value() )
        throw CSProException("You cannot build a website without defining a %s.", ToString(DocSetComponent::Type::TableOfContents));
}


void DocSetBuilderHtmlWebsiteGenerateTask::OnRun()
{
    const int64_t start_timestamp = GetTimestamp();

    GetInterface().SetTitle(FormatText("Building Document Set to an HTML Website: %s", GetDocSetSpec().GetFilePath().c_str()));

    RunBuild();

    GetInterface().LogText("\nBuild completed in %s.", GetElapsedTimeText(start_timestamp, GetTimestamp()).c_str());

    ASSERT(!m_defaultDocumentBuiltFilePath.empty());
    std::string html_website_output_name = PortableFunctions::PathGetFilename(m_csdocCompilerSettingsForBuilding->GetDocSetBuildOutputDirectory());

    GetInterface().OnCreatedOutput(std::move(html_website_output_name), m_defaultDocumentBuiltFilePath);
}


std::tuple<double, double> DocSetBuilderHtmlWebsiteGenerateTask::GetPreAndPostCompilationProgressPercents(size_t /*num_csdocs*/)
{
    constexpr double TableOfContentsGenerationProgressPercent = 5;

    return { TableOfContentsGenerationProgressPercent, 0 };
}


void DocSetBuilderHtmlWebsiteGenerateTask::OnPreCSDocCompilation()
{
    GetInterface().LogText("\nPreparing the HTML Website inputs.");

    GetSettings().RunPreCompilationTasks(*this);

    // evaluate the table of contents
    m_tableOfContentsEvaluator->Write(*GetDocSetSpec().GetTableOfContents());

    // set the default document file path, which will be used in a few places
    const std::string& default_document_file_path = m_csdocCompilerSettingsForBuilding->GetDefaultDocumentFilePath();
    m_defaultDocumentBuiltFilePath = m_csdocCompilerSettingsForBuilding->CreateHtmlOutputFilePath(default_document_file_path);

    // write out .htaccess and web.config files listing the default topic
    const std::string default_document_built_directory = PortableFunctions::PathGetDirectory(m_defaultDocumentBuiltFilePath);
    const std::string default_document_built_filename = PortableFunctions::PathGetFilename(m_defaultDocumentBuiltFilePath);

    Create_htaccess(default_document_built_directory, default_document_built_filename);
    Create_web_config(default_document_built_directory, default_document_built_filename);
}


void DocSetBuilderHtmlWebsiteGenerateTask::Create_htaccess(const std::string& directory, const std::string& default_document_built_filename)
{
    const std::string htaccess_text = "DirectoryIndex " + default_document_built_filename + "\n";

    const std::string htaccess_path = Path::Combine(directory, ".htaccess");

    GetSettings().WriteTextToFile(htaccess_path, htaccess_text, false);
}


void DocSetBuilderHtmlWebsiteGenerateTask::Create_web_config(const std::string& directory, const std::string& default_document_built_filename)
{
    constexpr std::string_view WebConfigStart_sv =
R"!(<?xml version="1.0" encoding="UTF-8"?>
<configuration>
  <system.webServer>
    <defaultDocument enabled="true">
      <files>
        <clear/>
          <add value=")!";
    constexpr std::string_view WebConfigEnd_sv = R"!("/>
      </files>
    </defaultDocument>
  </system.webServer>
</configuration>
)!";

    const std::string web_config_text = SO::Concatenate(WebConfigStart_sv, Encoders::ToHtmlTagValue(default_document_built_filename), WebConfigEnd_sv);

    const std::string web_config_path = Path::Combine(directory, "web.config");

    GetSettings().WriteTextToFile(web_config_path, web_config_text, false);
}



// --------------------------------------------------------------------------
// DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator
// --------------------------------------------------------------------------

DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::TableOfContentsEvaluator(DocSetBuilderHtmlWebsiteGenerateTask& generate_task)
    :   m_generateTask(generate_task)
{
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::WriteProject(const std::string& project)
{
    ASSERT(m_currentChapterNode.empty());

    const CSDocCompilerSettingsForBuilding& settings = m_generateTask.GetSettings();

    if( settings.GetBuildSettings().GetProjectLinkageAction() != DocBuildSettings::ProjectLinkageAction::Link )
        return;

    const CSDocCompilerSettingsForBuilding& project_settings = settings.GetProjectSettings(project);

    std::string title = "<" + project_settings.GetBuildSettings().GetEvaluatedOutputName(project_settings.GetDocSetSpec()) + ">";

    m_nodes.emplace_back(Node { TitleAndFilePath { std::move(title), project_settings.GetDefaultDocumentFilePath() }, project });
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::StartChapter(const std::string& title, bool /*write_title_to_pdf*/)
{
    std::vector<Node>& root_or_subchapter_nodes = m_currentChapterNode.empty() ? m_nodes :
                                                                                 m_currentChapterNode.top()->subchapters;

    root_or_subchapter_nodes.emplace_back(Node { TitleAndFilePath { title, std::string() } });
    m_currentChapterNode.push(&root_or_subchapter_nodes.back());
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::FinishChapter()
{
    m_currentChapterNode.pop();
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::WriteDocument(const std::string& csdoc_file_path, const std::string* const title_override)
{
    ASSERT(!m_currentChapterNode.empty());

    std::string title = ( title_override != nullptr ) ? *title_override :
                                                        m_generateTask.GetSettings().GetTitle(csdoc_file_path);

    m_currentChapterNode.top()->documents.emplace_back(TitleAndFilePath { std::move(title), csdoc_file_path });
}


const std::string& DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::GetFirstDocumentFilePathInNode(const Node& node) const
{
    if( !node.documents.empty() )
        return node.documents.front().file_path;

    if( !node.subchapters.empty() )
        return GetFirstDocumentFilePathInNode(node.subchapters.front());

    return ReturnProgrammingError(SO::Empty_string);
}


bool DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::NodeContainsDocument(const Node& node, const std::string& csdoc_file_path) const
{
    for( const TitleAndFilePath& title_and_file_path : node.documents )
    {
        if( SO::EqualsNoCase(csdoc_file_path, title_and_file_path.file_path) )
            return true;
    }

    for( const Node& subchapter_node : node.subchapters )
    {
        if( NodeContainsDocument(subchapter_node, csdoc_file_path) )
            return true;
    }

    return false;
}


std::tuple<std::string, std::string> DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::GetTableOfContentsHtml(const std::string& csdoc_file_path)
{
    constexpr std::string_view PreTableOfContentsHtml_sv =
        "<div id=\"container\">\n"
        "<div id=\"left\">\n";

    constexpr std::string_view PostTableOfContentsHtml_sv =
        "</div>\n"
        "<div id=\"middle_spacing1\"></div>\n"
        "<div id=\"middle\"></div>\n"
        "<div id=\"middle_spacing2\"></div>\n"
        "<div id=\"right\">\n";

    std::string table_of_contents_html(PreTableOfContentsHtml_sv);

    table_of_contents_html.append("<ul class=\"toc_ul\">\n");

    for( const Node& node : m_nodes )
        WriteHtmlForNode(table_of_contents_html, node, csdoc_file_path);

    table_of_contents_html.append("</ul>\n");

    table_of_contents_html.append(PostTableOfContentsHtml_sv);

    return { std::move(table_of_contents_html),
             "</div>\n</div>\n" };
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::WriteHtmlForNode(std::string& table_of_contents_html, const Node& node, const std::string& csdoc_file_path)
{
    // a project
    if( !node.project.empty() )
    {
        WriteHtmlForLiTagAndLink(table_of_contents_html, node.project, node.title_and_file_path.file_path, node.title_and_file_path.title,
                                 true, "toc_li_chapter");
    }

    // a chapter
    else
    {
        ASSERT(node.title_and_file_path.file_path.empty());

        const bool node_contains_document = NodeContainsDocument(node, csdoc_file_path);

        WriteHtmlForLiTagAndLink(table_of_contents_html, node.project, GetFirstDocumentFilePathInNode(node), node.title_and_file_path.title,
                                 false, node_contains_document ? "toc_li_chapter_current" : "toc_li_chapter");

        if( node_contains_document )
        {
            table_of_contents_html.append("\n<ul class=\"toc_ul\">\n");

            for( const TitleAndFilePath& title_and_file_path : node.documents )
            {
                WriteHtmlForLiTagAndLink(table_of_contents_html, node.project, title_and_file_path.file_path, title_and_file_path.title,
                                         true, SO::EqualsNoCase(csdoc_file_path, title_and_file_path.file_path) ? "toc_li_topic_current" : "toc_li_topic");
            }

            for( const Node& subchapter_node : node.subchapters )
                WriteHtmlForNode(table_of_contents_html, subchapter_node, csdoc_file_path);

            table_of_contents_html.append("</ul>\n");
        }

        table_of_contents_html.append("</li>\n");
    }
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::WriteHtmlForLiTagAndLink(std::string& table_of_contents_html, const std::string& project,
                                                                                              const std::string& csdoc_file_path, const std::string& title,
                                                                                              const bool end_li_tag, const char* const li_class_text)
{
    ASSERT(Encoders::ToHtmlTagValue(li_class_text) == li_class_text);

    table_of_contents_html.append("<li class=\"");
    table_of_contents_html.append(li_class_text);
    table_of_contents_html.append("\">");

    const std::string url = m_generateTask.GetSettings().CreateUrlForTopic(project, csdoc_file_path);
    table_of_contents_html.append(CSDocCompilerWorker::CreateHyperlinkStart(url));

    table_of_contents_html.append(Encoders::ToHtml(title));

    table_of_contents_html.append("</a>");

    if( end_li_tag )
        table_of_contents_html.append("</li>\n");
}
