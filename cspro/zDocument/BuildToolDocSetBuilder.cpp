#include "StdAfx.h"
#include "BuildToolDocSetBuilder.h"
#include "DocSetBuilder.h"
#include "DocSetBuilderCache.h"


// --------------------------------------------------------------------------
// BuildToolDocSetBuilder
// --------------------------------------------------------------------------

BuildToolDocSetBuilder::BuildToolDocSetBuilder()
    :   m_globalSettings(std::make_unique<GlobalSettings>()),
        m_docSetBuilderCache(std::make_unique<DocSetBuilderCache>())
{
    // reset the cache before the builds
    CacheableCalculator::ResetCache();
}


BuildToolDocSetBuilder::~BuildToolDocSetBuilder() noexcept = default;


void BuildToolDocSetBuilder::BuildDocSet(std::string csdocset_file_path, std::string output_directory, const std::string& build_name)
{
    ASSERT(PortableFunctions::FileIsRegular(csdocset_file_path) && !output_directory.empty());

    // compile the doc set
    DocSetSpec doc_set_spec(std::move(csdocset_file_path));

    DocSetCompiler doc_set_compiler(*m_globalSettings, DocSetCompiler::ThrowErrors { });

    doc_set_compiler.CompileSpec(
        doc_set_spec,
        FileIO::ReadText(doc_set_spec.GetFilePath()),
        DocSetCompiler::SpecCompilationType::SpecAndComponents
    );

    DocBuildSettings doc_set_settings_for_build = doc_set_spec.GetSettings().GetEvaluatedBuildSettings(build_name);
    doc_set_settings_for_build.SetOutputDirectory(std::move(output_directory));

    // generate the build task
    const std::unique_ptr<DocSetBuilderBaseGenerateTask> generate_task = DocSetBuilderBaseGenerateTask::CreateForBuild(
        &doc_set_spec,
        doc_set_settings_for_build,
        build_name,
        true // throw_exceptions_for_serious_issues_when_validating_build_settings
    );

    // the build cache will be shared among all Document Set builds
    generate_task->SetDocSetBuilderCache(m_docSetBuilderCache);

    // run the task
    RunGenerateTask(*generate_task);
}



// --------------------------------------------------------------------------
// BuildToolDocSetBuilder::BuildToolInterface
// --------------------------------------------------------------------------

class BuildToolDocSetBuilder::BuildToolInterface : public GenerateTaskInterface
{
public:
    BuildToolInterface(BuildToolDocSetBuilder& builder, std::optional<GenerateTaskStatus>& completion_status) noexcept;

protected:
    // GenerateTaskInterface overrides
    void SetTitle(const std::string& /*title*/) override final { }

    void LogText(SharableString text) override final;

    void UpdateProgress(double /*percent*/) override final { }

    void SetOutputText(const std::string& /*text*/) override final { }

    void OnCreatedOutput(std::string /*output_title*/, std::string /*path*/) override final { }

    void OnException(const CSProException& exception) override final;

    void OnCompletion(GenerateTaskStatus status) override final { m_completionStatus = status; }

    const GlobalSettings& GetGlobalSettings() override final { return m_builder.GetGlobalSettings(); }

private:
    BuildToolDocSetBuilder& m_builder;
    const std::function<void(SharableString text)>* m_logger;
    std::optional<GenerateTaskStatus>& m_completionStatus;
};


BuildToolDocSetBuilder::BuildToolInterface::BuildToolInterface(BuildToolDocSetBuilder& builder,
                                                               std::optional<GenerateTaskStatus>& completion_status) noexcept
    :   m_builder(builder),
        m_logger(m_builder.m_logger ? &m_builder.m_logger : nullptr),
        m_completionStatus(completion_status)
{
}


void BuildToolDocSetBuilder::BuildToolInterface::LogText(SharableString text)
{
    if( m_logger != nullptr )
        (*m_logger)(std::move(text));
}


void BuildToolDocSetBuilder::BuildToolInterface::OnException(const CSProException& exception)
{
    BuildToolInterface::LogText(
        SO::Concatenate("BUILD EXCEPTION: ", exception.what())
    );
}



// --------------------------------------------------------------------------
// BuildToolDocSetBuilder::RunGenerateTask
// --------------------------------------------------------------------------

void BuildToolDocSetBuilder::RunGenerateTask(GenerateTask& generate_task)
{
    std::optional<GenerateTaskStatus> completion_status;
    BuildToolInterface build_tool_interface(*this, completion_status);

    generate_task.SetInterface(build_tool_interface);

    generate_task.Run();

    while( !completion_status.has_value() )
    {
        constexpr DWORD SleepMilliseconds = 50;
        Sleep(SleepMilliseconds);
    }

    if( completion_status != GenerateTaskStatus::Complete )
        throw CSProException("There were errors building the Document Set.");
}
