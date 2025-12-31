#pragma once


namespace CP { struct BranchDetails;
               struct ModifiedFile;
               struct RefreshDataChanges; }


struct CP::BranchDetails
{
    GitBranch current_branch;
    std::unique_ptr<GitBranch> remote_branch;
};


struct CP::ModifiedFile
{
    std::string git_path;
    unsigned int diff_flag;

    ModifiedFile(std::string git_path_, unsigned int diff_flag_) noexcept;
    bool operator==(const ModifiedFile& rhs) const noexcept;
    const wchar_t* GetStatus() const;
};


struct CP::RefreshDataChanges
{
    bool branch_details;
    bool clean_commit;
    bool recent_commits;
    bool modified_files;
};


namespace CP::Update
{
    constexpr WPARAM BranchDetails = 1;
    constexpr WPARAM BranchCopies  = 2;
    constexpr WPARAM CleanCommit   = 3;
    constexpr WPARAM RecentCommits = 4;
    constexpr WPARAM ModifiedFiles = 5;
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CP::ModifiedFile::ModifiedFile(std::string git_path_, const unsigned int diff_flag_) noexcept
    :   git_path(std::move(git_path_)),
        diff_flag(diff_flag_)
{
}


inline bool CP::ModifiedFile::operator==(const ModifiedFile& rhs) const noexcept
{
    return ( git_path == rhs.git_path &&
             diff_flag == rhs.diff_flag );
}


inline const wchar_t* CP::ModifiedFile::GetStatus() const
{
    switch( diff_flag )
    {
        case GIT_DELTA_ADDED:     return L"Added";
        case GIT_DELTA_DELETED:   return L"Deleted";
        case GIT_DELTA_MODIFIED:  return L"Modified";
        case GIT_DELTA_UNTRACKED: return L"Untracked";
        default:                  return ReturnProgrammingError(L"<unknown status>");
    }
}
