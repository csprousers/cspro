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
             ( ( m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex + 1 ) == m_generateTaskForDocSetBuild->m_csdocFilenamesInCompilationOrder.size() ) );
}


std::wstring CSDocCompilerSettingsForBuildingPdf::GetHtmlHeaderTitle(const std::wstring& csdoc_filename)
{
    if( m_generateTaskForDocSetBuild == nullptr )
        return CSDocCompilerSettingsForBuilding::GetHtmlHeaderTitle(csdoc_filename);

    ASSERT(m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex <= m_generateTaskForDocSetBuild->m_csdocFirstNonCoverPageCompilationIndex);
    return GetDocSetSpec().GetTitleOrFilenameWithoutExtension();
}


std::tuple<std::wstring, std::wstring> CSDocCompilerSettingsForBuildingPdf::GetHtmlToWrapDocument()
{
    // construct HTML for any chapter titles that preceed the current document
    std::wstring titles_html;

    if( m_generateTaskForDocSetBuild != nullptr )
    {
        for( const auto& [csdoc_compilation_index, title] : m_generateTaskForDocSetBuild->m_csdocCompilationIndexWithPreceedingTitles )
        {
            if( csdoc_compilation_index == m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex )
            {
                if( titles_html.empty() )
                {
                    titles_html = _T("<h1>");
                }

                else
                {
                    titles_html.append(_T("<h1 style=\"page-break-before: avoid;\">"));
                }

                titles_html.append(Encoders::ToHtml(title));
                titles_html.append(_T("</h1>"));
            }

            else if( csdoc_compilation_index > m_generateTaskForDocSetBuild->m_csdocCurrentCompilationIndex )
            {
                break;
            }
        }
    }

    const std::wstring anchor_id = CreateHtmlAnchorId(GetCompilationFilename(), *m_docSetSpec);
        
    return std::make_tuple(titles_html + _T("<div id=\"") + Encoders::ToHtmlTagValue(anchor_id) + _T("\">"),
                                         _T("</div>"));
}


std::wstring CSDocCompilerSettingsForBuildingPdf::EvaluateBuildExtra(const std::wstring& path)
{
    std::wstring evaluated_path = CSDocCompilerSettings::EvaluateBuildExtra(path);

    if( m_generateTaskForDocSetBuild != nullptr )
        CreatePathAndCopyFileToDirectory(evaluated_path, m_generateTaskForDocSetBuild->GetTempOutputDirectory());

    return evaluated_path;
}


std::wstring CSDocCompilerSettingsForBuildingPdf::CreateHtmlAnchorId(const std::wstring& csdoc_filename, const DocSetSpec& doc_set_spec)
{
    constexpr size_t hash_length = 4;
    constexpr wstring_view salt_sv = _T("CSDocument");

    // the anchor ID will be a hash of the relative path of the CSPro Document to the Document Set
    const std::wstring relative_path = GetRelativeFNameForDisplay(doc_set_spec.GetFilename(), csdoc_filename);

    return Hash::Hash(relative_path, hash_length, salt_sv);
}


std::wstring CSDocCompilerSettingsForBuildingPdf::CreateUrlForDocSetTopic(const std::wstring& path) const
{
    ASSERT(m_buildSettings.GetDocSetLinkageAction() == DocBuildSettings::DocSetLinkageAction::Link);

    // the link will be an anchor ID
    return _T("#") + CreateHtmlAnchorId(path, *m_docSetSpec);
}


std::wstring CSDocCompilerSettingsForBuildingPdf::CreateUrlForProjectTopic(const CSDocCompilerSettingsForBuilding& project_settings, const std::wstring& path) const
{
    ASSERT(m_buildSettings.GetProjectLinkageAction() == DocBuildSettings::ProjectLinkageAction::Link);

    // the link will be the built filename of the project along with the anchor ID
    const std::wstring project_output_filename = project_settings.GetDocSetBuildOutputFilename();

    return CreateRelativeUrlForPath(project_output_filename) + _T("#") + CreateHtmlAnchorId(path, project_settings.GetDocSetSpec());
}



// --------------------------------------------------------------------------
// DocSetBuilderPdfGenerateTask
// --------------------------------------------------------------------------

DocSetBuilderPdfGenerateTask::DocSetBuilderPdfGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                           const DocBuildSettings& base_build_settings, std::wstring build_name,
                                                           bool throw_exceptions_for_serious_issues_when_validating_build_settings)
    :   DocSetBuilderBaseGenerateTask(CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(std::move(doc_set_spec), base_build_settings, DocBuildSettings::BuildType::Pdf, std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings)),
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
        throw CSProException(_T("You cannot build a PDF without defining a %s."), ToString(DocSetComponent::Type::TableOfContents));
}


void DocSetBuilderPdfGenerateTask::OnRun()
{
    const int64_t start_timestamp = GetTimestamp<int64_t>();

    GetInterface().SetTitle(_T("Building Document Set to a PDF: ") + GetDocSetSpec().GetFilename());

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

    GetInterface().LogText(FormatTextCS2WS(_T("\nBuild completed in %s."), GetElapsedTimeText(start_timestamp).c_str()));

    GetInterface().OnCreatedOutput(PortableFunctions::PathGetFilename(m_pdfOutputFilename), m_pdfOutputFilename);
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

    m_pdfOutputFilename = m_csdocCompilerSettingsForBuilding->GetDocSetBuildOutputFilename();
    FileIO::CreateDirectoriesForFile(m_pdfOutputFilename);
}


const std::vector<std::wstring>& DocSetBuilderPdfGenerateTask::GetCSDocFilenamesInCompilationOrder()
{
    return m_csdocFilenamesInCompilationOrder;
}


std::wstring DocSetBuilderPdfGenerateTask::GetCSDocOutputFilename(const std::wstring& csdoc_filename)
{
    // return a fake filename (with the the right name in the ultimate output directory)
    return PortableFunctions::PathAppendToPath(PortableFunctions::PathGetDirectory(m_pdfOutputFilename),
                                               m_csdocCompilerSettingsForBuilding->GetBuiltHtmlFilename(csdoc_filename));
}


void DocSetBuilderPdfGenerateTask::OnCSDocCompilationResult(const std::wstring& /*csdoc_filename*/, const std::wstring& /*output_filename*/, const std::wstring& html)
{
    const bool result_is_cover_page = ( m_csdocCurrentCompilationIndex < m_csdocFirstNonCoverPageCompilationIndex );
    FILE* file = result_is_cover_page ? nullptr : m_csdocsHtmlFile;
    std::wstring& html_filename = result_is_cover_page ? m_coverPageHtmlFilename : m_csdocsHtmlFilename;

    // open the file if not yet open
    if( file == nullptr )
    {
        ASSERT(html_filename.empty());

        html_filename = m_pdfCreator->CreateTemporaryHtmlFilename(GetTempOutputDirectory(),
                                                                  result_is_cover_page ? 1 : m_csdocFilenamesInCompilationOrder.size());

        file = FileIO::OpenFileForOutput(html_filename);

        if( !result_is_cover_page )
            m_csdocsHtmlFile = file;
    }

    // write the HTML
    const std::string utf8_html = UTF8Convert::WideToUTF8(html);
    const size_t bytes_written = fwrite(utf8_html.data(), 1, utf8_html.length(), file);

    if( result_is_cover_page )
        fclose(file);

    if( bytes_written != utf8_html.length() )
        throw CSProException(_T("There was an error writing to: ") + html_filename);

    ++m_csdocCurrentCompilationIndex;
}


void DocSetBuilderPdfGenerateTask::OnPostCSDocCompilation()
{
    fclose(m_csdocsHtmlFile);
    m_csdocsHtmlFile = nullptr;

    m_pdfCreator->CreatePdf(GetSettings().GetBuildSettings(), m_pdfOutputFilename, m_csdocsHtmlFilename, m_coverPageHtmlFilename);
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
    void StartChapter(const std::wstring& title, bool write_title_to_pdf) override
    {
        if( write_title_to_pdf )
            m_generateTask.m_csdocCompilationIndexWithPreceedingTitles.emplace_back(m_generateTask.m_csdocFilenamesInCompilationOrder.size(), title);
    }

    void WriteDocument(const std::wstring& csdoc_filename, const std::wstring* /*title_override*/) override
    {
        m_generateTask.m_csdocFilenamesInCompilationOrder.emplace_back(csdoc_filename);
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
        m_csdocFilenamesInCompilationOrder.emplace_back(doc_set_spec.GetCoverPageDocument()->filename);
        m_csdocFirstNonCoverPageCompilationIndex = 1;
        ASSERT(m_csdocFirstNonCoverPageCompilationIndex == m_csdocFilenamesInCompilationOrder.size());
    }

    // add the documents from the table of contents
    TableOfContentsEvaluator table_of_contents_evaluator(*this);
    table_of_contents_evaluator.Write(*GetDocSetSpec().GetTableOfContents());
}
