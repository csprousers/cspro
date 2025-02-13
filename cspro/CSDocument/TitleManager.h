#pragma once


// TitleManager caches the titles of CSPro Documents in a SettingsDb;
// if a title is requested that has not been cached, or if the CSPro Document file
// is more recent than the cache, the file will be compiled to get a title;
// an exception is thrown if the title cannot be found

class TitleManager
{
public:
    TitleManager(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec);

    std::wstring GetTitle(const std::wstring& csdoc_filename);

    void SetTitle(const std::wstring& csdoc_filename, const std::wstring& title) { SetTitle(csdoc_filename, &title); }
    void ClearTitle(const std::wstring& csdoc_filename)                          { SetTitle(csdoc_filename, nullptr); }

private:
    bool GetTitleFromCache(std::wstring& title, const std::wstring& csdoc_filename);

    void SetTitle(const std::wstring& csdoc_filename, const std::wstring* title);

private:
    SettingsDb m_settingsDb;
    cs::non_null_shared_or_raw_ptr<DocSetSpec> m_docSetSpec;
};    
