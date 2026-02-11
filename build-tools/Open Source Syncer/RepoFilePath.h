#pragma once


struct RepoFilePath
{
    std::string repo_path;
    std::string file_path;

    static RepoFilePath CreateFromFilePath(std::string file_path_, const std::string& repo_working_directory);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline RepoFilePath RepoFilePath::CreateFromFilePath(std::string file_path_, const std::string& repo_working_directory)
{
    ASSERT(repo_working_directory.back() == Path::NativeSlashChar);

    return RepoFilePath
    {
        Path::ToForwardSlash(file_path_.substr(repo_working_directory.length())),
        std::move(file_path_)
    };
}
