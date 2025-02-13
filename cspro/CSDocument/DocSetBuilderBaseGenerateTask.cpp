#include "StdAfx.h"
#include "DocSetBuilder.h"
#include "CSDocCompiler.h"


DocSetBuilderBaseGenerateTask::DocSetBuilderBaseGenerateTask(std::unique_ptr<CSDocCompilerSettingsForBuilding> settings)
    :   m_csdocCompilerSettingsForBuilding(std::move(settings)),
        m_progress(0)
{
    ASSERT(m_csdocCompilerSettingsForBuilding != nullptr);

    GetTextAndModifiedIterationForOpenDocuments();
}


DocSetBuilderBaseGenerateTask::~DocSetBuilderBaseGenerateTask()
{
    if( !m_tempOutputDirectory.empty() )
        PortableFunctions::DirectoryDelete(m_tempOutputDirectory, true);
}


std::unique_ptr<DocSetBuilderBaseGenerateTask> DocSetBuilderBaseGenerateTask::CreateForBuild(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                                                             const DocBuildSettings& base_build_settings, std::wstring build_name,
                                                                                             bool throw_exceptions_for_serious_issues_when_validating_build_settings)
{
    if( !base_build_settings.GetBuildType().has_value() )
        throw CSProException("You cannot build a Document Set without specifying a build type.");

    switch( *base_build_settings.GetBuildType() )
    {
        case DocBuildSettings::BuildType::HtmlPages:
            return std::make_unique<DocSetBuilderHtmlPagesGenerateTask>(std::move(doc_set_spec), base_build_settings, std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings);

        case DocBuildSettings::BuildType::HtmlWebsite:
            return std::make_unique<DocSetBuilderHtmlWebsiteGenerateTask>(std::move(doc_set_spec), base_build_settings, std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings);

        case DocBuildSettings::BuildType::Chm:
            return std::make_unique<DocSetBuilderChmGenerateTask>(std::move(doc_set_spec), base_build_settings, std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings);

        case DocBuildSettings::BuildType::Pdf:
            return std::make_unique<DocSetBuilderPdfGenerateTask>(std::move(doc_set_spec), base_build_settings, std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings);

        default:
            throw ProgrammingErrorException();
    }
}


void DocSetBuilderBaseGenerateTask::SetDocSetBuilderCache(std::shared_ptr<DocSetBuilderCache> doc_set_builder_cache)
{
    m_csdocCompilerSettingsForBuilding->SetDocSetBuilderCache(std::move(doc_set_builder_cache));
}


void DocSetBuilderBaseGenerateTask::GetTextAndModifiedIterationForOpenDocuments()
{
    // get the text sources for open documents
    std::map<StringNoCase, TextSourceEditable*> text_sources_for_open_documents;
    WindowsDesktopMessage::Send(UWM::CSDocument::GetOpenTextSourceEditables, &text_sources_for_open_documents);

    // because TextSourceEditable::GetText can try to get the text from the CLogicCtrl, which must be done on the UI-thread,
    // we will get all details about the open documents here
    for( const auto& [filename, text_source] : text_sources_for_open_documents )
    {
        try
        {
            m_textAndModifiedIterationForOpenDocuments.try_emplace(filename, std::make_tuple(text_source->GetText(),
                                                                                             text_source->GetModifiedIteration()));
        }
        catch(...) { }
    }

    // the DocSetCompiler can also send messages to the UI-thread, so this will override that functionality
    m_getFileTextOrModifiedIterationCallbackHolder.emplace(DocSetCompiler::OverrideGetFileTextOrModifiedIteration(
        [&](const std::wstring& filename)
        {
            return GetTextAndModifiedIteration(filename);
        }));
}


const std::tuple<std::wstring, int64_t>* DocSetBuilderBaseGenerateTask::GetTextAndModifiedIteration(const std::wstring& filename) const
{
    const auto& lookup = m_textAndModifiedIterationForOpenDocuments.find(filename);

    if( lookup != m_textAndModifiedIterationForOpenDocuments.cend() )
        return &lookup->second;

    return nullptr;
}


std::wstring DocSetBuilderBaseGenerateTask::GetFileText(const std::wstring& filename) const
{
    const std::tuple<std::wstring, int64_t>* text_and_modified_iteration = GetTextAndModifiedIteration(filename);

    return ( text_and_modified_iteration != nullptr ) ? std::get<0>(*text_and_modified_iteration) :
                                                        FileIO::ReadText(filename);
}


void DocSetBuilderBaseGenerateTask::IncrementAndUpdateProgress(double progress_increase)
{
    m_progress += progress_increase;
    GetInterface().UpdateProgress(m_progress);
}


void DocSetBuilderBaseGenerateTask::ValidateInputs()
{
    const DocBuildSettings& build_settings = m_csdocCompilerSettingsForBuilding->GetBuildSettings();

    if( build_settings.GetOutputDirectory().empty() )
        throw CSProException("You must specify an output directory.");
}


void DocSetBuilderBaseGenerateTask::CreateTempOutputDirectory()
{
    // when creating the temporary directory, use "CSDocument" instead of the ".CS" default because 
    // names with a dot led to issues with the HTML Help Compiler
    ASSERT(m_tempOutputDirectory.empty());

    m_tempOutputDirectory = PortableFunctions::GetUniqueFilenameInDirectory(GetTempDirectory(), wstring_view(), _T("CSDocument"));
    FileIO::CreateDirectories(m_tempOutputDirectory);
}


void DocSetBuilderBaseGenerateTask::RunBuild()
{
    ValidateInputs();

    // compile the spec
    constexpr double SpecCompilationProgressPercent = 5;

    DocSetSpec& doc_set_spec = GetDocSetSpec();

    GetInterface().LogText(_T("Compiling Document Set specification and components: ") + doc_set_spec.GetFilename());
    
    DocSetCompiler doc_set_compiler(GetInterface().GetGlobalSettings(), DocSetCompiler::ThrowErrors { });
    doc_set_compiler.CompileSpec(doc_set_spec, GetFileText(doc_set_spec.GetFilename()), DocSetCompiler::SpecCompilationType::SpecAndComponents);

    GetInterface().LogText(_T("\nValidating the Document Set's build settings."));
    ValidateInputsPostDocSetCompilation();

    IncrementAndUpdateProgress(SpecCompilationProgressPercent);

    // determine the progress bar increment rate
    for( const DocSetComponent& doc_set_component : VI_V(doc_set_spec.GetComponents()) )
    {
        if( doc_set_component.type == DocSetComponent::Type::Document )
            m_csdocFilenames.emplace_back(doc_set_component.filename);
    }

    const auto [progress_for_pre_compilation, progress_for_post_compilation] = GetPreAndPostCompilationProgressPercents(m_csdocFilenames.size());
    const double progress_for_all_documents = 100 - m_progress - progress_for_pre_compilation - progress_for_post_compilation;
    ASSERT(progress_for_all_documents > 0);

    // run pre-CSPro Document compilation routines
    OnPreCSDocCompilation();
    IncrementAndUpdateProgress(progress_for_pre_compilation);

    if( IsCanceled() )
        return;

    // compile the documents
    const std::vector<std::wstring>& csdoc_filenames_in_compilation_order = GetCSDocFilenamesInCompilationOrder();

    if( !csdoc_filenames_in_compilation_order.empty() )
    {
        const int64_t start_timestamp = GetTimestamp<int64_t>();

        GetInterface().LogText(FormatTextCS2WS(_T("\nCompiling %d CSPro Documents..."), static_cast<int>(csdoc_filenames_in_compilation_order.size())));

        const double progress_for_each_document = progress_for_all_documents / csdoc_filenames_in_compilation_order.size();
        CompileCSDocs(progress_for_each_document);

        if( IsCanceled() )
            return;

        GetInterface().LogText(FormatTextCS2WS(_T("\nCompiled %d CSPro Documents in %s."), static_cast<int>(csdoc_filenames_in_compilation_order.size()),
                                                                                           GetElapsedTimeText(start_timestamp).c_str()));
    }

    if( IsCanceled() )
        return;

    // run post-CSPro Document compilation routines
    OnPostCSDocCompilation();
    IncrementAndUpdateProgress(progress_for_post_compilation);
}


void DocSetBuilderBaseGenerateTask::CompileCSDocs(double progress_for_each_document)
{
    CSDocCompiler csdoc_compiler;

    for( const std::wstring& csdoc_filename : GetCSDocFilenamesInCompilationOrder() )
    {
        if( IsCanceled() )
            return;

        GetInterface().LogText(_T("\nCompiling CSPro Document: ") + csdoc_filename);

        const std::wstring output_filename = GetCSDocOutputFilename(csdoc_filename);
        m_csdocCompilerSettingsForBuilding->SetOutputFilename(output_filename);

        try
        {
            const std::wstring html = csdoc_compiler.CompileToHtml(*m_csdocCompilerSettingsForBuilding,
                                                                   csdoc_filename, GetFileText(csdoc_filename));

            OnCSDocCompilationResult(csdoc_filename, output_filename, html);
        }

        catch( const CSProException& exception )
        {
            OnCSDocCompilationResult(csdoc_filename, output_filename, exception);
        }

        IncrementAndUpdateProgress(progress_for_each_document);
    }
}


// --------------------------------------------------------------------------
// base class implementations of overridable methods
// --------------------------------------------------------------------------

std::tuple<double, double> DocSetBuilderBaseGenerateTask::GetPreAndPostCompilationProgressPercents(size_t /*num_csdocs*/)
{
    return { 0, 0 };
}


void DocSetBuilderBaseGenerateTask::OnPreCSDocCompilation()
{
}


const std::vector<std::wstring>& DocSetBuilderBaseGenerateTask::GetCSDocFilenamesInCompilationOrder()
{
    return m_csdocFilenames;
}


std::wstring DocSetBuilderBaseGenerateTask::GetCSDocOutputFilename(const std::wstring& csdoc_filename)
{
    return m_csdocCompilerSettingsForBuilding->CreateHtmlOutputFilename(csdoc_filename);
}


void DocSetBuilderBaseGenerateTask::OnCSDocCompilationResult(const std::wstring& /*csdoc_filename*/, const std::wstring& output_filename, const std::wstring& html)
{
    GetInterface().LogText(_T("Saving CSPro Document: ") + output_filename);
    m_csdocCompilerSettingsForBuilding->WriteTextToFile(output_filename, html, true);
}


void DocSetBuilderBaseGenerateTask::OnCSDocCompilationResult(const std::wstring& /*csdoc_filename*/, const std::wstring& /*output_filename*/, const CSProException& exception)
{
    throw exception;
}


void DocSetBuilderBaseGenerateTask::OnPostCSDocCompilation()
{
}
