#pragma once

#include "GitCommit.h"


class GitAccessor
{
public:
    GitAccessor();
    ~GitAccessor();

    // Returns the time formatted as RFC 2822; e.g.,: Thu, 03 Jul 2025 15:00:00 -0400
    static std::string FormatTime(const git_time& time);

    // Returns the commit history from HEAD up to (and including) the commit specified.
    std::vector<GitCommit> ReadCommitHistory(const std::string& repo_directory, const std::string& commit_sha) const;

    // Returns the commit SHAs that match the commits, matched by author time and email.
    std::vector<std::string> LookupCommitSHAs(const std::string& repo_directory, const std::string& branch_name,
                                              const std::vector<GitCommit>& commits) const;

private:
    [[noreturn]] static void ThrowGitException();

    static std::string ObjectIdToString(const git_oid& oid);
};
