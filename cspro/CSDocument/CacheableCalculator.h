#pragma once


class CacheableCalculator
{
private:
    CacheableCalculator() { }

public:
    // resets the cache
    static void ResetCache();

    // returns null if not found; throws an exception if multiple files match
    static const std::wstring* FindFileByNameInDirectory(const std::wstring& directory, bool recursive, const std::wstring& name, const std::wstring* already_matched_file = nullptr);

    // throws an exception if not found
    static std::wstring FindProjectDocSetSpecFilename(const std::wstring& project_directory);

    // throws an exception if the file cannot be read
    static const std::vector<std::wstring>& GetDocumentFilenamesForProject(const std::wstring& project_doc_set_spec_filename);

private:
    static const std::wstring* FindFileByNameInDirectory(const std::map<StringNoCase, std::vector<std::wstring>>& files_in_directory_by_name, const std::wstring& name, const std::wstring* already_matched_file);
};
