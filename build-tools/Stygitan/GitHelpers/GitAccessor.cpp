#include "StdAfx.h"
#include "GitAccessor.h"
#include <zToolsO/RaiiHelpers.h>
#include <chrono>
#include <numeric>


GitAccessor::GitAccessor()
{
    if( git_libgit2_init() < 0 )
        throw CSProException("Could not initialize libgit2.");
}


GitAccessor::~GitAccessor()
{
    git_libgit2_shutdown();
}


void GitAccessor::ThrowGitException()
{
    throw CSProException("Error interacting with libgit2: %s", git_error_last()->message);
}


std::string GitAccessor::ObjectIdToString(const git_oid& oid)
{
    char buffer[GIT_OID_MAX_HEXSIZE + 1];

    return std::string(git_oid_tostr(buffer, _countof(buffer), &oid),
                       GIT_OID_MAX_HEXSIZE);
}


std::string GitAccessor::FormatTime(const git_time& time)
{
    const int64_t unix_timestamp = time.time + time.offset * 60;
    tm tm;

    if( gmtime_s(&tm, &unix_timestamp) != 0 )
        throw ProgrammingErrorException();

    char buffer[40];
    const size_t length = std::strftime(buffer, _countof(buffer), "%a, %d %b %Y %H:%M:%S", &tm);
    ASSERT(length != 0);

    std::string time_string(buffer, length);

    // append the time zone offset
    const int offset_abs = std::abs(time.offset);
    const char* offset_sign = ( time.offset >= 0 ) ? "+" : "-";
    time_string.append(FormatText(" %c%02d%02d", offset_sign[0], offset_abs / 60, offset_abs % 60));

    return time_string;
}


std::vector<GitCommit> GitAccessor::ReadCommitHistory(const std::string& repo_directory, const std::string& commit_string) const
{
    git_repository* repo;

    if( git_repository_open_bare(&repo, repo_directory.c_str()) < 0 )
        ThrowGitException();

    const RAII::RunOnDestruction free_repo([&] { git_repository_free(repo); });

    // lookup the commit
    git_oid oldest_commit_to_process;

    if( git_oid_fromstr(&oldest_commit_to_process, commit_string.c_str()) < 0 )
        throw CSProException("The commit was not found in the repo: %s", commit_string.c_str());

    // walk the commits
    git_revwalk* walker;

    if( git_revwalk_new(&walker, repo) != 0 )
        ThrowGitException();

    git_revwalk_push(walker, &oldest_commit_to_process);
    git_revwalk_sorting(walker, GIT_SORT_TIME);
    git_revwalk_push_head(walker);

    git_oid commit_oid;

    std::vector<GitCommit> commits;

    while( git_revwalk_next(&commit_oid, walker) == 0 )
    {
        git_commit* commit;

        if( git_commit_lookup(&commit, repo, &commit_oid) != 0 )
            ThrowGitException();

        commits.emplace_back(commit);

        git_commit_free(commit);

        if( git_oid_equal(&commit_oid, &oldest_commit_to_process) )
            break;
    }

    git_revwalk_free(walker);

    return commits;
}


std::vector<std::string> GitAccessor::LookupCommitSHAs(const std::string& repo_directory, const std::string& branch_name,
                                                       const std::vector<GitCommit>& commits) const
{
    git_repository* repo;

    if( git_repository_open_bare(&repo, repo_directory.c_str()) < 0 )
        ThrowGitException();

    const RAII::RunOnDestruction free_repo([&] { git_repository_free(repo); });

    git_reference* branch_ref;

    if( git_branch_lookup(&branch_ref, repo, branch_name.c_str(), GIT_BRANCH_ALL ) != 0 )
        throw CSProException("The branch was not found in the repo: %s", branch_name.c_str());

    const RAII::RunOnDestruction free_branch_ref([&] { git_reference_free(branch_ref); });

    // get the commit that the branch points to
    const git_oid* const oid = git_reference_target(branch_ref);

    if( oid == nullptr )
        throw ProgrammingErrorException();

    std::vector<size_t> commits_to_match(commits.size());
    std::iota(commits_to_match.begin(), commits_to_match.end(), 0);

    // walk the commits in the branch, finding the ones that match
    git_revwalk* walker;

    if( git_revwalk_new(&walker, repo) != 0 )
        ThrowGitException();

    git_revwalk_push(walker, oid);
    git_revwalk_sorting(walker, GIT_SORT_TIME);

    std::vector<std::string> commit_shas(commits_to_match.size());
    git_oid commit_oid;

    while( !commits_to_match.empty() && git_revwalk_next(&commit_oid, walker) == 0 )
    {
        git_commit* commit;

        if( git_commit_lookup(&commit, repo, &commit_oid) != 0 )
            ThrowGitException();

        const GitCommit this_commit(commit);

        auto lookup = std::find_if(commits_to_match.cbegin(), commits_to_match.cend(),
            [&](const size_t index)
            {
                const GitCommit& comparison_commit = commits[index];
                return ( this_commit == comparison_commit );
            });

        if( lookup == commits_to_match.cend() )
        {
            // search for a commit that has the same author but a different commit message
            lookup = std::find_if(commits_to_match.cbegin(), commits_to_match.cend(),
                [&](const size_t index)
                {
                    const GitCommit& comparison_commit = commits[index];

                    return ( this_commit.GetWhen() == comparison_commit.GetWhen() &&
                             this_commit.GetAuthorName() == comparison_commit.GetAuthorName() &&
                             this_commit.GetAuthorEmail() == comparison_commit.GetAuthorEmail() );
                });

            if( lookup != commits_to_match.cend() )
            {
                const std::string message = FormatText("Should these commit messages be considered identical?\n\n%s\n\n%s",
                                                       this_commit.GetMessage().c_str(), commits[*lookup].GetMessage().c_str());

                if( AfxMessageBox(message, MB_YESNO) != IDYES )
                    lookup = commits_to_match.cend();
            }
        }

        if( lookup != commits_to_match.cend() )
        {
            ASSERT(commit_shas[*lookup].empty());
            commit_shas[*lookup] = ObjectIdToString(commit_oid);
            commits_to_match.erase(lookup);
        }

        git_commit_free(commit);
    }

    if( !commits_to_match.empty() )
    {
        const std::string unmatched_text = SO::CreateSingleStringUsingCallback(
            commits_to_match,
            [&](const size_t index) { return commits[index].GetMessage(); },
            "\n\n");

        throw CSProException("Not all commits were matched:\n\n" + unmatched_text);
    }

    ASSERT(std::find(commit_shas.cbegin(), commit_shas.cend(), std::string()) == commit_shas.cend());

    return commit_shas;
}
