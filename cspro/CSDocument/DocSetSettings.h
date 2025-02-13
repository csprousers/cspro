#pragma once

#include <CSDocument/DocBuildSettings.h>


class DocSetSettings
{
public:
    void Reset();

    bool HasCustomSettings() const;

    bool IsDocSetPartOfProject() const { return !m_projectRootDirectory.empty(); }

    // throws an exception if not found
    std::wstring FindProjectDocSetSpecFilename(const std::wstring& project) const;

    // returns blank if not found
    std::wstring GetProjectNameFromDocSetSpecFilename(wstring_view project_doc_set_spec_filename_sv) const; 

    // throws an exception if not found
    const std::wstring* FindInImageDirectories(const std::wstring& image_name) const;

    const std::optional<DocBuildSettings>& GetDefaultBuildSettings() const                       { return m_defaultBuildSettings; }
    const std::vector<std::tuple<std::wstring, DocBuildSettings>>& GetNamedBuildSettings() const { return m_namedBuildSettings; }

    DocBuildSettings GetEvaluatedBuildSettings(const DocBuildSettings& build_settings) const;

    // throws an exception if not found
    DocBuildSettings GetEvaluatedBuildSettings(const std::wstring& name) const;

    // if the name is non-blank, build settings matching that name are returned;
    // if no build settings match the name, then the first build settings matching the build type are returned;
    // if no build settings match the build type, then default settings for that build type are returned;
    // the tuple includes the name of the matched build settings
    std::tuple<DocBuildSettings, std::wstring> GetEvaluatedBuildSettings(DocBuildSettings::BuildType build_type, const std::wstring& name = std::wstring()) const;

    // serialization
    void Compile(DocSetCompiler& doc_set_compiler, const JsonNode<wchar_t>& json_node, bool reset_settings);
    void WriteJson(JsonWriter& json_writer, bool write_evaluated_build_settings = true) const;

private:
    struct ImageDirectory
    {
        std::wstring directory;
        bool recursive;
    };

private:
    std::wstring m_projectRootDirectory;
    std::vector<ImageDirectory> m_imageDirectories;
    std::optional<DocBuildSettings> m_defaultBuildSettings;
    std::vector<std::tuple<std::wstring, DocBuildSettings>> m_namedBuildSettings; // name / settings
};
