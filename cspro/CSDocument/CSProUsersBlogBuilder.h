#pragma once

class CSDocCompilerSettingsForCSProUsersBlog;
class CSProUsersBlogHtmlTagModifier;


class CSProUsersBlogBuilder
{
public:
    CSProUsersBlogBuilder(GlobalSettings& global_settings, std::string blog_doc_set_spec_file_path, std::string output_directory);
    ~CSProUsersBlogBuilder();

    void Build();

private:
    void Build(const std::string& blog_file_path, const std::string& blog_text, const std::string& output_file_path);

private:
    std::string m_outputDirectory;
    std::unique_ptr<CSProUsersBlogHtmlTagModifier> m_htmlTagModifier;

    DocSetSpec m_docSetSpec;
    std::unique_ptr<CSDocCompilerSettingsForCSProUsersBlog> m_settings;
};
