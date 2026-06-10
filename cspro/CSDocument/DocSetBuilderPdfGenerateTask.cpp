#include "StdAfx.h"
#include "DocSetBuilder.h"
#include "CSDocCompilerSettings.h"
#include "PdfCreator.h"
#include <zToolsO/Hash.h>


// --------------------------------------------------------------------------
// CSDocCompilerSettingsForBuildingPdf
// --------------------------------------------------------------------------

void CSDocCompilerSettingsForBuildingPdf::RunPreCompilationTasksForDocSetBuild(DocSetBuilderPdfGenerateTask& generate_task)
{
    m_generateTaskForDocSetBuild = &generate_task;
}


bool CSDocCompilerSettingsForBuildingPdf::AddHtmlHeader() const
{
    if( m_generateTaskForDocSetBuild == nullptr )
        return CSDocCompilerSettingsForBuilding::AddHtmlHeader();

    // add the header for the cover page or the first document
    return ( m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex <= m_generateTaskForDocSetBuild->m_csdocFirstNonCoverPageCompilationIndex );
}


bool CSDocCompilerSettingsForBuildingPdf::AddHtmlFooter() const
{
    if( m_generateTaskForDocSetBuild == nullptr )
        return CSDocCompilerSettingsForBuilding::AddHtmlHeader();

    // add the header for the cover page or the last document
    return ( ( m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex < m_generateTaskForDocSetBuild->m_csdocFirstNonCoverPageCompilationIndex ) ||
             ( ( m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex + 1 ) == m_generateTaskForDocSetBuild->m_csdocFilePathsInCompilationOrder.size() ) );
}


std::string CSDocCompilerSettingsForBuildingPdf::GetHtmlHeaderTitle(const std::string& csdoc_file_path)
{
    if( m_generateTaskForDocSetBuild == nullptr )
        return CSDocCompilerSettingsForBuilding::GetHtmlHeaderTitle(csdoc_file_path);

    ASSERT(m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex <= m_generateTaskForDocSetBuild->m_csdocFirstNonCoverPageCompilationIndex);
    return GetDocSetSpec().GetTitleOrFilenameWithoutExtension();
}


std::tuple<std::string, std::string> CSDocCompilerSettingsForBuildingPdf::GetHtmlToWrapDocument()
{
    // construct HTML for any chapter titles that preceed the current document
    std::string titles_html;

    if( m_generateTaskForDocSetBuild != nullptr )
    {
        for( const auto& [csdoc_compilation_index, title] : m_generateTaskForDocSetBuild->m_csdocCompilationIndexWithPreceedingTitles )
        {
            if( csdoc_compilation_index == m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex )
            {
                if( titles_html.empty() )
                {
                    titles_html = "<h1>";
                }

                else
                {
                    titles_html.append("<h1 style=\"page-break-before: avoid;\">");
                }

                titles_html.append(Encoders::ToHtml(title));
                titles_html.append("</h1>");
            }

            else if( csdoc_compilation_index > m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex )
            {
                break;
            }
        }
    }

    const std::string anchor_id = CreateHtmlAnchorId(GetCompilationFilePath(), *m_docSetSpec);

    return std::make_tuple(SO::Concatenate(std::move(titles_html), "<div id=\"", Encoders::ToHtmlTagValue(anchor_id), "\">"),
                           "</div>");
}


std::string CSDocCompilerSettingsForBuildingPdf::EvaluateBuildExtra(const std::string& path)
{
    std::string evaluated_path = CSDocCompilerSettings::EvaluateBuildExtra(path);

    if( m_generateTaskForDocSetBuild != nullptr )
        CreatePathAndCopyFileToDirectory(evaluated_path, m_generateTaskForDocSetBuild->GetTempOutputDirectory());

    return evaluated_path;
}


std::string CSDocCompilerSettingsForBuildingPdf::CreateHtmlAnchorId(const std::string& csdoc_file_path, const DocSetSpec& doc_set_spec)
{
    constexpr size_t hash_length = 4;
    constexpr std::string_view salt_sv = "CSDocument";

    // the anchor ID will be a hash of the relative path of the CSPro Document to the Document Set
    const std::string relative_path = GetRelativePathForDisplay(doc_set_spec.GetFilePath(), csdoc_file_path);

    return Hash::Create(relative_path, hash_length, salt_sv);
}


std::string CSDocCompilerSettingsForBuildingPdf::CreateUrlForDocSetTopic(const std::string& path) const
{
    ASSERT(m_buildSettings.GetDocSetLinkageAction() == DocBuildSettings::DocSetLinkageAction::Link);

    // the link will be an anchor ID
    return "#" + CreateHtmlAnchorId(path, *m_docSetSpec);
}


std::string CSDocCompilerSettingsForBuildingPdf::CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::string& path) const
{
    ASSERT(m_buildSettings.GetProjectLinkageAction() == DocBuildSettings::ProjectLinkageAction::Link);

    // the link will be the built filename of the project along with the anchor ID
    const std::string project_output_file_path = project_settings.GetDocSetBuildOutputFilePath();

    return SO::Concatenate(CreateRelativeUrlForPath(project_output_file_path), "#", CreateHtmlAnchorId(path, project_settings.GetDocSetSpec()));
}



// --------------------------------------------------------------------------
// DocSetBuilderPdfGenerateTask
// --------------------------------------------------------------------------

DocSetBuilderPdfGenerateTask::DocSetBuilderPdfGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                           const DocBuildSettings& base_build_settings, std::string build_name,
                                                           const bool throw_exceptions_for_serious_issues_when_validating_build_settings)
    :   DocSetBuilderBaseGenerateTask(CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(std::move(doc_set_spec), base_build_settings, DocBuildSettings::BuildType::Pdf,
                                                                                             std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings)),
        m_csdocFirstNonCoverPageCompilationIndex(0),
        m_csdocCurrentCompilationIndex(0),
        m_csdocsHtmlFile(nullptr)
{
}


DocSetBuilderPdfGenerateTask::~DocSetBuilderPdfGenerateTask()
{
    ASSERT(m_csdocsHtmlFile == nullptr);
}


CSDocCompilerSettingsForBuildingPdf& DocSetBuilderPdfGenerateTask::GetSettings()
{
    return assert_cast<CSDocCompilerSettingsForBuildingPdf&>(*m_csdocCompilerSettingsForBuilding);
}


void DocSetBuilderPdfGenerateTask::ValidateInputs()
{
    DocSetBuilderBaseGenerateTask::ValidateInputs();

    m_pdfCreator = std::make_unique<PdfCreator>(*this);
}


void DocSetBuilderPdfGenerateTask::ValidateInputsPostDocSetCompilation()
{
    if( !GetDocSetSpec().GetTableOfContents().has_value() )
    {
        throw CSProException("You cannot build a PDF without defining a %s.",
                             ToString(DocSetComponent::Type::TableOfContents));
    }
}


void DocSetBuilderPdfGenerateTask::OnRun()
{
    const int64_t start_timestamp = GetTimestamp();

    GetInterface().SetTitle(FormatText("Building Document Set to a PDF: %s", GetDocSetSpec().GetFilePath().c_str()));

    // all compiled files will be saved to a temporary directory
    CreateTempOutputDirectory();

    try
    {
        RunBuild();
    }

    catch(...)
    {
        if( m_csdocsHtmlFile != nullptr )
        {
            fclose(m_csdocsHtmlFile);
            m_csdocsHtmlFile = nullptr;
        }

        throw;
    }

    GetInterface().LogText("\nBuild completed in %s.", GetElapsedTimeText(start_timestamp, GetTimestamp()).c_str());

    GetInterface().OnCreatedOutput(PortableFunctions::PathGetFilename(m_pdfOutputFilePath), m_pdfOutputFilePath);
}


std::tuple<double, double> DocSetBuilderPdfGenerateTask::GetPreAndPostCompilationProgressPercents(size_t /*num_csdocs*/)
{
    constexpr double PreCSDocCompilationProgressPercent = 5;
    constexpr double PdfGenerationProgressPercent       = 20;

    return { PreCSDocCompilationProgressPercent, PdfGenerationProgressPercent };
}


void DocSetBuilderPdfGenerateTask::OnPreCSDocCompilation()
{
    GetSettings().RunPreCompilationTasksForDocSetBuild(*this);

    // evaluate the compilation order (using the Table of Contents)
    EvaluateCompilationOrder();

    m_pdfOutputFilePath = m_csdocCompilerSettingsForBuilding->GetDocSetBuildOutputFilePath();
    FileIO::CreateDirectoriesForFile(m_pdfOutputFilePath);
}


const std::vector<std::string>& DocSetBuilderPdfGenerateTask::GetCSDocFilePathsInCompilationOrder()
{
    return m_csdocFilePathsInCompilationOrder;
}


std::string DocSetBuilderPdfGenerateTask::GetCSDocOutputFilePath(const std::string& csdoc_file_path)
{
    // return a fake filename (with the the right name in the ultimate output directory)
    return PortableFunctions::PathReplaceFilename(m_pdfOutputFilePath,
                                                  m_csdocCompilerSettingsForBuilding->GetBuiltHtmlFilename(csdoc_file_path));
}


void DocSetBuilderPdfGenerateTask::OnCSDocCompilationResult(const std::string& /*csdoc_file_path*/, const std::string& /*output_file_path*/, const std::string& html)
{
    const bool result_is_cover_page = ( m_csdocCurrentCompilationIndex < m_csdocFirstNonCoverPageCompilationIndex );
    FILE* file = result_is_cover_page ? nullptr : m_csdocsHtmlFile;
    std::string& html_file_path = result_is_cover_page ? m_coverPageHtmlFilePath : m_csdocsHtmlFilePath;

    // open the file if not yet open
    if( file == nullptr )
    {
        ASSERT(html_file_path.empty());

        html_file_path = m_pdfCreator->CreateTemporaryHtmlFilePath(GetTempOutputDirectory(),
                                                                   result_is_cover_page ? 1 : m_csdocFilePathsInCompilationOrder.size());

        file = FileIO::OpenFileForOutput(html_file_path);

        if( !result_is_cover_page )
            m_csdocsHtmlFile = file;
    }

    // write the HTML
    const size_t bytes_written = fwrite(html.data(), 1, html.length(), file);

    if( result_is_cover_page )
        fclose(file);

    if( bytes_written != html.length() )
        throw CSProException("There was an error writing to: %s", html_file_path.c_str());

    ++m_csdocCurrentCompilationIndex;
}


void DocSetBuilderPdfGenerateTask::OnPostCSDocCompilation()
{
    fclose(m_csdocsHtmlFile);
    m_csdocsHtmlFile = nullptr;

    m_pdfCreator->CreatePdf(GetSettings().GetBuildSettings(), m_pdfOutputFilePath, m_csdocsHtmlFilePath, m_coverPageHtmlFilePath);
}



// --------------------------------------------------------------------------
// DocSetBuilderPdfGenerateTask::TableOfContentsEvaluator
// --------------------------------------------------------------------------

class DocSetBuilderPdfGenerateTask::TableOfContentsEvaluator : public DocSetTableOfContents::Writer
{
public:
    TableOfContentsEvaluator(DocSetBuilderPdfGenerateTask& generate_task)
        :   m_generateTask(generate_task)
    {
    }

protected:
    void StartChapter(const std::string& title, const bool write_title_to_pdf) override
    {
        if( write_title_to_pdf )
            m_generateTask.m_csdocCompilationIndexWithPreceedingTitles.emplace_back(m_generateTask.m_csdocFilePathsInCompilationOrder.size(), title);
    }

    void WriteDocument(const std::string& csdoc_file_path, const std::string* /*title_override*/) override
    {
        m_generateTask.m_csdocFilePathsInCompilationOrder.emplace_back(csdoc_file_path);
    }

private:
    DocSetBuilderPdfGenerateTask& m_generateTask;
};


void DocSetBuilderPdfGenerateTask::EvaluateCompilationOrder()
{
    const DocSetSpec& doc_set_spec = GetDocSetSpec();

    // add the cover page
    if( doc_set_spec.GetCoverPageDocument() != nullptr )
    {
        m_csdocFilePathsInCompilationOrder.emplace_back(doc_set_spec.GetCoverPageDocument()->file_path);
        m_csdocFirstNonCoverPageCompilationIndex = 1;
        ASSERT(m_csdocFirstNonCoverPageCompilationIndex == m_csdocFilePathsInCompilationOrder.size());
    }

    // add the documents from the table of contents
    TableOfContentsEvaluator table_of_contents_evaluator(*this);
    table_of_contents_evaluator.Write(*GetDocSetSpec().GetTableOfContents());
}
