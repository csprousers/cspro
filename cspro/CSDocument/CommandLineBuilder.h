#pragma once

class CommandLineParser;
class ConsoleWrapper;
class DocSetBuilderCache;


class CommandLineBuilder : public GenerateTask::Interface
{
public:
    CommandLineBuilder(GlobalSettings& global_settings);

    void Build(const CommandLineParser& command_line_parser);

    void CreateNotepadPlusPlusColorizer();

protected:
    // GenerateTask::Interface overrides
    void SetTitle(const std::wstring& title) override;
    void LogText(std::wstring text) override;
    void UpdateProgress(double percent) override;
    void SetOutputText(const std::wstring& text) override;
    void OnCreatedOutput(std::wstring output_title, std::wstring path) override;
    void OnException(const CSProException& exception) override;
    void OnCompletion(GenerateTask::Status status) override;
    const GlobalSettings& GetGlobalSettings() override;

private:
    std::unique_ptr<GenerateTask> CreateGenerateTask(const CommandLineParser& command_line_parser, const std::wstring& input_filename);

    static std::tuple<DocBuildSettings, std::wstring> GetBuildSettings(const DocSetSettings* doc_set_settings, const std::wstring& build_name_or_type,
                                                                       bool require_that_doc_set_settings_has_build_settings);

    static std::unique_ptr<GenerateTask> CreateGenerateTaskForCSDoc(std::shared_ptr<DocSetSpec> doc_set_spec, DocBuildSettings build_settings,
                                                                    const std::wstring& csdoc_filename, const std::wstring& output_path);

    std::unique_ptr<GenerateTask> CreateGenerateTaskForDocumentSet(std::shared_ptr<DocSetSpec> doc_set_spec, DocBuildSettings build_settings,
                                                                   std::wstring build_name, const std::wstring& output_path);

private:
    GlobalSettings& m_globalSettings;

    // for Build
    ConsoleWrapper* m_console;
    std::optional<GenerateTask::Status> m_completionStatus;
    std::vector<std::tuple<std::wstring, std::wstring>> m_finalOutputs;
    std::shared_ptr<DocSetBuilderCache> m_docSetBuilderCache;
};
