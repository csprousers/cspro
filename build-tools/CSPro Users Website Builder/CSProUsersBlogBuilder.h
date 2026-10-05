#pragma once

class CSDocCompilerSettingsForCSProUsersBlog;
class CSProUsersBlogTagModifier;


class CSProUsersBlogBuilder
{
public:
    CSProUsersBlogBuilder();
    ~CSProUsersBlogBuilder();

    // Returns the file paths of the blog entries created.
    std::vector<std::string> Build(const std::string& cspro_directory, const std::string& blog_doc_set_spec_file_path,
                                   const std::string& posts_output_directory);

private:
    void BuildBlog(const std::string& blog_file_path, const std::string& blog_text, const std::string& output_file_path);

private:
    std::unique_ptr<CSDocCompilerSettingsForCSProUsersBlog> m_settings;
    std::unique_ptr<CSProUsersBlogTagModifier> m_tagModifier;
};
