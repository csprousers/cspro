#pragma once


// TitleManager caches the titles of CSPro Documents in a SettingsDb;
// if a title is requested that has not been cached, or if the CSPro Document file
// is more recent than the cache, the file will be compiled to get a title;
// an exception is thrown if the title cannot be found

class TitleManager
{
public:
    TitleManager(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec);

    std::string GetTitle(const std::string& csdoc_file_path);

    void SetTitle(const std::string& csdoc_file_path, const std::string& title) { SetTitle(csdoc_file_path, &title); }
    void ClearTitle(const std::string& csdoc_file_path)                         { SetTitle(csdoc_file_path, nullptr); }

private:
    bool GetTitleFromCache(std::string& title, const std::string& csdoc_file_path);

    void SetTitle(const std::string& csdoc_file_path, const std::string* title);

private:
    SettingsDb m_settingsDb;
    cs::non_null_shared_or_raw_ptr<DocSetSpec> m_docSetSpec;
};
