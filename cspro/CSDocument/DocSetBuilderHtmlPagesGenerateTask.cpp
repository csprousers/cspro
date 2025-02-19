#include "StdAfx.h"
#include "DocSetBuilder.h"


DocSetBuilderHtmlPagesGenerateTask::DocSetBuilderHtmlPagesGenerateTask(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec,
                                                                       const DocBuildSettings& base_build_settings, std::string build_name,
                                                                       const bool throw_exceptions_for_serious_issues_when_validating_build_settings)
    :   DocSetBuilderBaseGenerateTask(CSDocCompilerSettingsForBuilding::CreateForDocSetBuild(std::move(doc_set_spec), base_build_settings, DocBuildSettings::BuildType::HtmlPages,
                                                                                             std::move(build_name), throw_exceptions_for_serious_issues_when_validating_build_settings))
{
}


void DocSetBuilderHtmlPagesGenerateTask::OnRun()
{
    const int64_t start_timestamp = GetTimestamp<int64_t>();

    GetInterface().SetTitle(FormatText("Building Document Set to HTML Pages: %s", GetDocSetSpec().GetFilePath().c_str()));

    RunBuild();

    GetInterface().LogText("\nBuild completed in %s.", GetElapsedTimeText(start_timestamp, GetTimestamp<int64_t>()).c_str());

    const std::string html_pages_directory = m_csdocCompilerSettingsForBuilding->GetDocSetBuildOutputDirectory();
    GetInterface().OnCreatedOutput(PortableFunctions::PathGetFilename(html_pages_directory), html_pages_directory);
}
