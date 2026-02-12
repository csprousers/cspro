#pragma once


class FileReplacer
{
public:
    FileReplacer(Controller& controller, const std::string& overrides_directory);
    ~FileReplacer();

    // Returns true when the file in the open source repository
    // has a replacement file defined in the replacements.json file.
    bool HasReplacement(const std::string& cs_file_path);

    // Returns a non-empty string containing the replacement data when the file in the open
    // source repository has a replacement file defined in the replacements.json file.
    std::string GetReplacement(const git_diff_file& new_file);

private:
    template<typename T>
    T HasReplacementWorker(const std::string& cs_file_path);

    // Returns the appropriate version of SQLite without the SQLite Encryption Extension (SEE).
    std::string CreateSqliteWithoutSEE(const git_diff_file& new_file);

private:
    Controller& m_controller;

    struct Replacement;
    std::map<std::string, Replacement> m_replacements;
};
