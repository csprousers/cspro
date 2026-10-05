#pragma once

#include <zDocument/zDocument.h>

class DocSetBuilderCache;
class GenerateTask;
class GlobalSettings;


// --------------------------------------------------------------------------
// BuildToolDocSetBuilder
//
// This class builds Document Sets for the build tools.
// --------------------------------------------------------------------------

class ZDOCUMENT_API BuildToolDocSetBuilder
{
public:
    BuildToolDocSetBuilder();
    ~BuildToolDocSetBuilder() noexcept;

    const GlobalSettings& GetGlobalSettings() const noexcept { return *m_globalSettings; }

    void SetLogger(std::function<void(SharableString text)> logger) noexcept { m_logger = std::move(logger); }

    void BuildDocSet(std::string csdocset_file_path, std::string output_directory, const std::string& build_name);

private:
    class BuildToolInterface;
    std::unique_ptr<BuildToolInterface> CreateInterface();

    void RunGenerateTask(GenerateTask& generate_task);

private:
    std::unique_ptr<GlobalSettings> m_globalSettings;
    std::shared_ptr<DocSetBuilderCache> m_docSetBuilderCache;
    std::function<void(SharableString text)> m_logger;
};
