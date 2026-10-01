#pragma once


class CacheableCalculator
{
private:
    CacheableCalculator() { }

public:
    // resets the cache
    static void ResetCache();

    // returns null if not found; throws an exception if multiple files match
    static const std::string* FindFileByNameInDirectory(const std::string& directory, bool recursive, const std::string& filename, const std::string* already_matched_file_path = nullptr);

    // throws an exception if not found
    static std::string FindProjectDocSetSpecFilePath(const std::string& project_directory);

    // throws an exception if the file cannot be read
    static const std::vector<std::string>& GetDocumentFilePathsForProject(const std::string& project_doc_set_spec_file_path);

private:
    static const std::string* FindFileByNameInDirectory(const std::map<std::string, std::vector<std::string>, cs::case_insensitive_less>& file_paths_in_directory_by_name,
                                                        const std::string& filename, const std::string* already_matched_file_path);
};
