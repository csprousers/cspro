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
                                                                                             const DocBuildSettings& base_build_settings, std::string build_name,
                                                                                             const bool throw_exceptions_for_serious_issues_when_validating_build_settings)
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
    std::map<std::string, TextSourceEditable*, cs::case_insensitive_less> text_sources_for_open_documents;
    WindowsDesktopMessage::Send(UWM::CSDocument::GetOpenTextSourceEditables, &text_sources_for_open_documents);

    // because TextSourceEditable::GetText can try to get the text from the CLogicCtrl, which must be done on the UI-thread,
    // we will get all details about the open documents here
    for( const auto& [file_path, text_source] : text_sources_for_open_documents )
    {
        try
        {
            m_textAndModifiedIterationForOpenDocuments.try_emplace(file_path, std::make_tuple(text_source->GetTextAsSharableString(),
                                                                                              text_source->GetModifiedIteration()));
        }
        catch(...) { }
    }

    // the DocSetCompiler can also send messages to the UI-thread, so this will override that functionality
    m_getFileTextOrModifiedIterationCallbackHolder.emplace(DocSetCompiler::OverrideGetFileTextOrModifiedIteration(
        [&](const std::string& file_path)
        {
            return GetTextAndModifiedIteration(file_path);
        }));
}


const std::tuple<SharableString, int64_t>* DocSetBuilderBaseGenerateTask::GetTextAndModifiedIteration(const std::string& file_path) const
{
    const auto& lookup = m_textAndModifiedIterationForOpenDocuments.find(file_path);

    if( lookup != m_textAndModifiedIterationForOpenDocuments.cend() )
        return &lookup->second;

    return nullptr;
}


SharableString DocSetBuilderBaseGenerateTask::GetFileText(const std::string& file_path) const
{
    const std::tuple<SharableString, int64_t>* const text_and_modified_iteration = GetTextAndModifiedIteration(file_path);

    return ( text_and_modified_iteration != nullptr ) ? std::get<0>(*text_and_modified_iteration) :
                                                        FileIO::ReadText(file_path);
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

    m_tempOutputDirectory = PortableFunctions::GetUniqueFilePathInDirectory(GetTempDirectory(), std::string_view(), "CSDocument");
    FileIO::CreateDirectories(m_tempOutputDirectory);
}


void DocSetBuilderBaseGenerateTask::RunBuild()
{
    ValidateInputs();

    // compile the spec
    constexpr double SpecCompilationProgressPercent = 5;

    DocSetSpec& doc_set_spec = GetDocSetSpec();

    GetInterface().LogText("Compiling Document Set specification and components: " + doc_set_spec.GetFilePath());

    DocSetCompiler doc_set_compiler(GetInterface().GetGlobalSettings(), DocSetCompiler::ThrowErrors { });
    doc_set_compiler.CompileSpec(doc_set_spec, GetFileText(doc_set_spec.GetFilePath()).GetString(), DocSetCompiler::SpecCompilationType::SpecAndComponents);

    GetInterface().LogText("\nValidating the Document Set's build settings.");
    ValidateInputsPostDocSetCompilation();

    IncrementAndUpdateProgress(SpecCompilationProgressPercent);

    // determine the progress bar increment rate
    for( const DocSetComponent& doc_set_component : VI_V(doc_set_spec.GetComponents()) )
    {
        if( doc_set_component.type == DocSetComponent::Type::Document )
            m_csdocFilePaths.emplace_back(doc_set_component.file_path);
    }

    const auto [progress_for_pre_compilation, progress_for_post_compilation] = GetPreAndPostCompilationProgressPercents(m_csdocFilePaths.size());
    const double progress_for_all_documents = 100 - m_progress - progress_for_pre_compilation - progress_for_post_compilation;
    ASSERT(progress_for_all_documents > 0);

    // run pre-CSPro Document compilation routines
    OnPreCSDocCompilation();
    IncrementAndUpdateProgress(progress_for_pre_compilation);

    if( IsCanceled() )
        return;

    // compile the documents
    const std::vector<std::string>& csdoc_file_paths_in_compilation_order = GetCSDocFilePathsInCompilationOrder();

    if( !csdoc_file_paths_in_compilation_order.empty() )
    {
        const int64_t start_timestamp = GetTimestamp();

        GetInterface().LogText("\nCompiling %d CSPro Documents...", static_cast<int>(csdoc_file_paths_in_compilation_order.size()));

        const double progress_for_each_document = progress_for_all_documents / csdoc_file_paths_in_compilation_order.size();
        CompileCSDocs(progress_for_each_document);

        if( IsCanceled() )
            return;

        GetInterface().LogText("\nCompiled %d CSPro Documents in %s.", static_cast<int>(csdoc_file_paths_in_compilation_order.size()),
                                                                       GetElapsedTimeText(start_timestamp, GetTimestamp()).c_str());
    }

    if( IsCanceled() )
        return;

    // run post-CSPro Document compilation routines
    OnPostCSDocCompilation();
    IncrementAndUpdateProgress(progress_for_post_compilation);
}


void DocSetBuilderBaseGenerateTask::CompileCSDocs(const double progress_for_each_document)
{
    CSDocCompiler csdoc_compiler;

    for( const std::string& csdoc_file_path : GetCSDocFilePathsInCompilationOrder() )
    {
        if( IsCanceled() )
            return;

        GetInterface().LogText("\nCompiling CSPro Document: " + csdoc_file_path);

        const std::string output_file_path = GetCSDocOutputFilePath(csdoc_file_path);
        m_csdocCompilerSettingsForBuilding->SetOutputFilePath(output_file_path);

        try
        {
            const std::string html = csdoc_compiler.CompileToHtml(*m_csdocCompilerSettingsForBuilding,
                                                                  csdoc_file_path, GetFileText(csdoc_file_path).GetString());

            OnCSDocCompilationResult(csdoc_file_path, output_file_path, html);
        }

        catch( const CSProException& exception )
        {
            OnCSDocCompilationResult(csdoc_file_path, output_file_path, exception);
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


const std::vector<std::string>& DocSetBuilderBaseGenerateTask::GetCSDocFilePathsInCompilationOrder()
{
    return m_csdocFilePaths;
}


std::string DocSetBuilderBaseGenerateTask::GetCSDocOutputFilePath(const std::string& csdoc_file_path)
{
    return m_csdocCompilerSettingsForBuilding->CreateHtmlOutputFilePath(csdoc_file_path);
}


void DocSetBuilderBaseGenerateTask::OnCSDocCompilationResult(const std::string& /*csdoc_file_path*/, const std::string& output_file_path, const std::string& html)
{
    GetInterface().LogText("Saving CSPro Document: " + output_file_path);
    m_csdocCompilerSettingsForBuilding->WriteTextToFile(output_file_path, html, true);
}


void DocSetBuilderBaseGenerateTask::OnCSDocCompilationResult(const std::string& /*csdoc_file_path*/, const std::string& /*output_file_path*/, const CSProException& exception)
{
    throw exception;
}


void DocSetBuilderBaseGenerateTask::OnPostCSDocCompilation()
{
}
