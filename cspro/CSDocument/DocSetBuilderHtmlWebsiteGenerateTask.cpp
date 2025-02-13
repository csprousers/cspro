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

    std::tuple<std::wstring, std::wstring> GetTableOfContentsHtml(const std::wstring& csdoc_filename);

protected:
    // DocSetTableOfContents::Writer overrides
    void WriteProject(const std::wstring& project) override;
    void StartChapter(const std::wstring& title, bool write_title_to_pdf) override;
    void FinishChapter() override;
    void WriteDocument(const std::wstring& csdoc_filename, const std::wstring* title_override) override;

private:
    struct TitleAndFilename
    {
        std::wstring title;
        std::wstring filename;
    };

    struct Node
    {
        TitleAndFilename title_and_filename; // the filename is empty for chapters
        std::wstring project;

        std::vector<TitleAndFilename> documents;
        std::vector<Node> subchapters;
    };

private:
    const std::wstring& GetFirstDocumentFilenameInNode(const Node& node) const;

    bool NodeContainsDocument(const Node& node, const std::wstring& csdoc_filename) const;

    void WriteHtmlForNode(std::wstring& table_of_contents_html, const Node& node, const std::wstring& csdoc_filename);

    void WriteHtmlForLiTagAndLink(std::wstring& table_of_contents_html, const std::wstring& project, const std::wstring& csdoc_filename,
                                  const std::wstring& title, bool end_li_tag, const TCHAR* li_class_text);

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
        m_titlePostfix = _T(" - ") + *m_docSetSpec->GetTitle();

    // if using a particular directory for stylesheets, copy the stylesheet images there
    if( m_buildSettings.GetStylesheetAction() == DocBuildSettings::StylesheetAction::Directory )
    {
        const std::wstring stylesheet_directory = EvaluateDirectoryRelativeToOutputDirectory(m_buildSettings.GetStylesheetDirectory(), false);
        CopyStylesheetImages(stylesheet_directory);
    }
}


std::wstring CSDocCompilerSettingsForBuildingHtmlWebsite::GetHtmlHeaderTitle(const std::wstring& csdoc_filename)
{
    return GetTitle(csdoc_filename) + m_titlePostfix;
}


std::wstring CSDocCompilerSettingsForBuildingHtmlWebsite::GetStylesheetsHtml()
{
    // when embedding stylesheets, the stylesheet images must be copied to any directory where a HTML file is created
    if( m_buildSettings.GetStylesheetAction() == DocBuildSettings::StylesheetAction::Embed )
        CopyStylesheetImages(GetCSDocOutputDirectory());

    return GetStylesheetsHtmlWorker(CSDocStylesheetFilename) +
           GetStylesheetsHtmlWorker(DocSetWebStylesheetFilename);
}


void CSDocCompilerSettingsForBuildingHtmlWebsite::CopyStylesheetImages(const std::wstring& directory)
{
    // keep track of the directories where stylesheet images have been copied to avoid copying them over and over
    if( m_copiedStylesheetImageDirectories.find(directory) != m_copiedStylesheetImageDirectories.cend() )
        return;

    for( const std::wstring& source_path : GetDocSetBuilderCache().GetStylesheetImagePaths() )
    {
        const std::wstring destination_path = PortableFunctions::PathAppendToPath(directory, PortableFunctions::PathGetFilename(source_path));
        CopyFileToDirectory(source_path, destination_path);
    }
    
    m_copiedStylesheetImageDirectories.emplace(directory);
}


std::tuple<std::wstring, std::wstring> CSDocCompilerSettingsForBuildingHtmlWebsite::GetHtmlToWrapDocument()
{
    ASSERT(m_generateTask != nullptr && m_generateTask->m_tableOfContentsEvaluator != nullptr);
    return m_generateTask->m_tableOfContentsEvaluator->GetTableOfContentsHtml(GetCompilationFilename());
}



// --------------------------------------------------------------------------
// DocSetBuilderHtmlWebsiteGenerateTask
// --------------------------------------------------------------------------

DocSetBuilderHtmlWebsiteGenerateTask::DocSetBuilderHtmlWebsiteGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                                           const DocBuildSettings& base_build_settings, std::wstring build_name,
                                                                           bool throw_exceptions_for_serious_issues_when_validating_build_settings)
    :   DocSetBuilderBaseGenerateTask(CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(std::move(doc_set_spec), base_build_settings, DocBuildSettings::BuildType::HtmlWebsite, std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings)),
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
        throw CSProException(_T("You cannot build a website without defining a %s."), ToString(DocSetComponent::Type::TableOfContents));
}


void DocSetBuilderHtmlWebsiteGenerateTask::OnRun()
{
    const int64_t start_timestamp = GetTimestamp<int64_t>();

    GetInterface().SetTitle(_T("Building Document Set to an HTML Website: ") + GetDocSetSpec().GetFilename());

    RunBuild();

    GetInterface().LogText(FormatTextCS2WS(_T("\nBuild completed in %s."), GetElapsedTimeText(start_timestamp).c_str()));

    ASSERT(!m_defaultDocumentBuiltPath.empty());
    const std::wstring html_website_output_name = PortableFunctions::PathGetFilename(m_csdocCompilerSettingsForBuilding->GetDocSetBuildOutputDirectory());

    GetInterface().OnCreatedOutput(html_website_output_name, m_defaultDocumentBuiltPath);
}


std::tuple<double, double> DocSetBuilderHtmlWebsiteGenerateTask::GetPreAndPostCompilationProgressPercents(size_t /*num_csdocs*/)
{
    constexpr double TableOfContentsGenerationProgressPercent = 5;

    return { TableOfContentsGenerationProgressPercent, 0 };
}


void DocSetBuilderHtmlWebsiteGenerateTask::OnPreCSDocCompilation()
{
    GetInterface().LogText(_T("\nPreparing the HTML Website inputs."));

    GetSettings().RunPreCompilationTasks(*this);

    // evaluate the table of contents
    m_tableOfContentsEvaluator->Write(*GetDocSetSpec().GetTableOfContents());

    // set the default document path, which will be used in a few places
    const std::wstring& default_document_path = m_csdocCompilerSettingsForBuilding->GetDefaultDocumentPath();
    m_defaultDocumentBuiltPath = m_csdocCompilerSettingsForBuilding->CreateHtmlOutputFilename(default_document_path);

    // write out .htaccess and web.config files listing the default topic
    const std::wstring default_document_built_directory = PortableFunctions::PathGetDirectory(m_defaultDocumentBuiltPath);
    const std::wstring default_document_built_filename = PortableFunctions::PathGetFilename(m_defaultDocumentBuiltPath);

    Create_htaccess(default_document_built_directory, default_document_built_filename);
    Create_web_config(default_document_built_directory, default_document_built_filename);
}


void DocSetBuilderHtmlWebsiteGenerateTask::Create_htaccess(const std::wstring& directory, const std::wstring& default_document_built_filename)
{
    const std::wstring htaccess_text = _T("DirectoryIndex ") + default_document_built_filename + _T("\n");

    const std::wstring htaccess_path = PortableFunctions::PathAppendToPath(directory, _T(".htaccess"));

    GetSettings().WriteTextToFile(htaccess_path, htaccess_text, false);
}


void DocSetBuilderHtmlWebsiteGenerateTask::Create_web_config(const std::wstring& directory, const std::wstring& default_document_built_filename)
{
    constexpr const TCHAR* WebConfigStart = 
LR"!(<?xml version="1.0" encoding="UTF-8"?>
<configuration>
  <system.webServer>
    <defaultDocument enabled="true">
      <files>
        <clear/>
          <add value=")!";
constexpr const TCHAR* WebConfigEnd = LR"!("/>
      </files>      
    </defaultDocument>
  </system.webServer>
</configuration>
)!";

    const std::wstring web_config_text = WebConfigStart + Encoders::ToHtmlTagValue(default_document_built_filename) + WebConfigEnd;

    const std::wstring web_config_path = PortableFunctions::PathAppendToPath(directory, _T("web.config"));

    GetSettings().WriteTextToFile(web_config_path, web_config_text, false);
}



// --------------------------------------------------------------------------
// DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator
// --------------------------------------------------------------------------

DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::TableOfContentsEvaluator(DocSetBuilderHtmlWebsiteGenerateTask& generate_task)
    :   m_generateTask(generate_task)
{
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::WriteProject(const std::wstring& project)
{
    ASSERT(m_currentChapterNode.empty());

    const CSDocCompilerSettingsForBuilding& settings = m_generateTask.GetSettings();

    if( settings.GetBuildSettings().GetProjectLinkageAction() != DocBuildSettings::ProjectLinkageAction::Link )
        return;

    const CSDocCompilerSettingsForBuilding& project_settings = settings.GetProjectSettings(project);

    std::wstring title = _T("<") + project_settings.GetBuildSettings().GetEvaluatedOutputName(project_settings.GetDocSetSpec()) + _T(">");

    m_nodes.emplace_back(Node { TitleAndFilename { std::move(title), project_settings.GetDefaultDocumentPath() }, project });
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::StartChapter(const std::wstring& title, bool /*write_title_to_pdf*/)
{
    std::vector<Node>& root_or_subchapter_nodes = m_currentChapterNode.empty() ? m_nodes :
                                                                                 m_currentChapterNode.top()->subchapters;

    root_or_subchapter_nodes.emplace_back(Node { TitleAndFilename { title, std::wstring() } });
    m_currentChapterNode.push(&root_or_subchapter_nodes.back());
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::FinishChapter()
{
    m_currentChapterNode.pop();
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::WriteDocument(const std::wstring& csdoc_filename, const std::wstring* title_override)
{
    ASSERT(!m_currentChapterNode.empty());

    std::wstring title = ( title_override != nullptr ) ? *title_override :
                                                         m_generateTask.GetSettings().GetTitle(csdoc_filename);

    m_currentChapterNode.top()->documents.emplace_back(TitleAndFilename { std::move(title), csdoc_filename });
}


const std::wstring& DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::GetFirstDocumentFilenameInNode(const Node& node) const
{
    if( !node.documents.empty() )
        return node.documents.front().filename;

    if( !node.subchapters.empty() )
        return GetFirstDocumentFilenameInNode(node.subchapters.front());

    return ReturnProgrammingError(SO::EmptyString);
}


bool DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::NodeContainsDocument(const Node& node, const std::wstring& csdoc_filename) const
{
    for( const TitleAndFilename& title_and_filename : node.documents )
    {
        if( SO::EqualsNoCase(csdoc_filename, title_and_filename.filename) )
            return true;
    }

    for( const Node& subchapter_node : node.subchapters )
    {
        if( NodeContainsDocument(subchapter_node, csdoc_filename) )
            return true;
    }

    return false;
}


std::tuple<std::wstring, std::wstring> DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::GetTableOfContentsHtml(const std::wstring& csdoc_filename)
{
    constexpr const TCHAR* PreTableOfContentsHtml =
        _T("<div id=\"container\">\n")
        _T("<div id=\"left\">\n");

    constexpr const TCHAR* PostTableOfContentsHtml =
        _T("</div>\n")
        _T("<div id=\"middle_spacing1\"></div>\n")
        _T("<div id=\"middle\"></div>\n")
        _T("<div id=\"middle_spacing2\"></div>\n")
        _T("<div id=\"right\">\n");

    std::wstring table_of_contents_html = PreTableOfContentsHtml;

    table_of_contents_html.append(_T("<ul class=\"toc_ul\">\n"));

    for( const Node& node : m_nodes )
        WriteHtmlForNode(table_of_contents_html, node, csdoc_filename);

    table_of_contents_html.append(_T("</ul>\n"));

    table_of_contents_html.append(PostTableOfContentsHtml);

    return { std::move(table_of_contents_html),
             _T("</div>\n</div>\n") };
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::WriteHtmlForNode(std::wstring& table_of_contents_html, const Node& node, const std::wstring& csdoc_filename)
{
    // a project
    if( !node.project.empty() )
    {
        WriteHtmlForLiTagAndLink(table_of_contents_html, node.project, node.title_and_filename.filename, node.title_and_filename.title,
                                 true, _T("toc_li_chapter"));
    }

    // a chapter
    else
    {
        ASSERT(node.title_and_filename.filename.empty());

        const bool node_contains_document = NodeContainsDocument(node, csdoc_filename);

        WriteHtmlForLiTagAndLink(table_of_contents_html, node.project, GetFirstDocumentFilenameInNode(node), node.title_and_filename.title,
                                 false, node_contains_document ? _T("toc_li_chapter_current") : _T("toc_li_chapter"));

        if( node_contains_document )
        {
            table_of_contents_html.append(_T("\n<ul class=\"toc_ul\">\n"));

            for( const TitleAndFilename& title_and_filename : node.documents )
            {
                WriteHtmlForLiTagAndLink(table_of_contents_html, node.project, title_and_filename.filename, title_and_filename.title,
                                         true, SO::EqualsNoCase(csdoc_filename, title_and_filename.filename) ? _T("toc_li_topic_current") : _T("toc_li_topic"));
            }

            for( const Node& subchapter_node : node.subchapters )
                WriteHtmlForNode(table_of_contents_html, subchapter_node, csdoc_filename);

            table_of_contents_html.append(_T("</ul>\n"));
        }

        table_of_contents_html.append(_T("</li>\n"));
    }
}


void DocSetBuilderHtmlWebsiteGenerateTask::TableOfContentsEvaluator::WriteHtmlForLiTagAndLink(std::wstring& table_of_contents_html,
    const std::wstring& project, const std::wstring& csdoc_filename, const std::wstring& title, bool end_li_tag, const TCHAR* li_class_text)
{
    ASSERT(Encoders::ToHtmlTagValue(li_class_text) == li_class_text);

    table_of_contents_html.append(_T("<li class=\""));
    table_of_contents_html.append(li_class_text);
    table_of_contents_html.append(_T("\">"));

    const std::wstring url = m_generateTask.GetSettings().CreateUrlForTopic(project, csdoc_filename);
    table_of_contents_html.append(CSDocCompilerWorker::CreateHyperlinkStart(url));

    table_of_contents_html.append(Encoders::ToHtml(title));

    table_of_contents_html.append(_T("</a>"));

    if( end_li_tag )
        table_of_contents_html.append(_T("</li>\n"));
}
