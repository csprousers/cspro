#pragma once

#include <CSDocument/DocBuildSettings.h>


class DocSetSettings
{
public:
    void Reset();

    bool HasCustomSettings() const;

    const std::string& GetProjectRootDirectory() const { return m_projectRootDirectory; }

    bool IsDocSetPartOfProject() const { return !m_projectRootDirectory.empty(); }

    // throws an exception if not found
    std::string FindProjectDocSetSpecFilePath(const std::string& project) const;

    // returns blank if not found
    std::string GetProjectNameFromDocSetSpecFilePath(std::string_view project_doc_set_spec_file_path_sv) const;

    // throws an exception if not found
    const std::string* FindInImageDirectories(const std::string& image_name) const;

    const std::optional<DocBuildSettings>& GetDefaultBuildSettings() const                      { return m_defaultBuildSettings; }
    const std::vector<std::tuple<std::string, DocBuildSettings>>& GetNamedBuildSettings() const { return m_namedBuildSettings; }

    DocBuildSettings GetEvaluatedBuildSettings(const DocBuildSettings& build_settings) const;

    // throws an exception if not found
    DocBuildSettings GetEvaluatedBuildSettings(const std::string& name) const;

    // if the name is non-blank, build settings matching that name are returned;
    // if no build settings match the name, then the first build settings matching the build type are returned;
    // if no build settings match the build type, then default settings for that build type are returned;
    // the tuple includes the name of the matched build settings
    std::tuple<DocBuildSettings, std::string> GetEvaluatedBuildSettings(DocBuildSettings::BuildType build_type, const std::string& name = SO::Empty_string) const;

    // serialization
    void Compile(DocSetCompiler& doc_set_compiler, const JsonNode& json_node, bool reset_settings);
    void WriteJson(JsonWriter& json_writer, bool write_evaluated_build_settings = true) const;

private:
    struct ImageDirectory
    {
        std::string directory;
        bool recursive;
    };

private:
    std::string m_projectRootDirectory;
    std::vector<ImageDirectory> m_imageDirectories;
    std::optional<DocBuildSettings> m_defaultBuildSettings;
    std::vector<std::tuple<std::string, DocBuildSettings>> m_namedBuildSettings; // name / settings
};
