#include "StdAfx.h"
#include "GitRevisionWalker.h"


GitRevisionWalker::GitRevisionWalker(GitRepository& repo)
    :   m_repo(repo)
{
    repo.EnsureRepositoryIsOpen();

    if( git_revwalk_new(&m_walker, m_repo) != 0 )
        throw GitException();
}


GitRevisionWalker::~GitRevisionWalker()
{
    git_revwalk_free(m_walker);
}


void GitRevisionWalker::WalkFromHead(const std::function<bool(GitCommit)>& callback_function)
{
    git_revwalk_sorting(m_walker, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);
    git_revwalk_push_head(m_walker);

    const RAII::RunOnDestruction reset_walker([&]() { git_revwalk_reset(m_walker); });

    git_oid oid;

    while( git_revwalk_next(&oid, m_walker) == 0 &&
           callback_function(m_repo.LookupCommit(oid)) )
    {
    }
}


void GitRevisionWalker::WalkFromHead(const GitCommit& end_commit, const std::function<void(GitCommit)>& callback_function)
{
    WalkFromHead(
        [&](GitCommit commit)
        {
            const bool process_more = ( commit != end_commit );
            callback_function(std::move(commit));
            return process_more;
        });
}


void GitRevisionWalker::Walk(const GitObjectId& start_oid, const unsigned int sort_mode_extras,
                             const std::function<bool(GitCommit)>& callback_function)
{
    git_revwalk_sorting(m_walker, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME | sort_mode_extras);
    git_revwalk_push(m_walker, start_oid);

    const RAII::RunOnDestruction reset_walker([&]() { git_revwalk_reset(m_walker); });

    git_oid oid;

    while( git_revwalk_next(&oid, m_walker) == 0 &&
           callback_function(m_repo.LookupCommit(oid)) )
    {
    }
}


void GitRevisionWalker::Walk(const GitObjectId& start_oid, const std::function<bool(GitCommit)>& callback_function)
{
    Walk(start_oid, GIT_SORT_NONE, callback_function);
}


void GitRevisionWalker::Walk(const GitCommit& start_commit, const std::function<bool(GitCommit)>& callback_function)
{
    Walk(start_commit.GetObjectId(), callback_function);
}


void GitRevisionWalker::Walk(const GitCommit& start_commit, const GitCommit& end_commit, const unsigned int sort_mode_extras,
                             const std::function<void(GitCommit)>& callback_function)
{
    git_revwalk_hide(m_walker, end_commit);

    Walk(start_commit.GetObjectId(), sort_mode_extras,
        [&](GitCommit commit)
        {
            ASSERT(commit != end_commit);
            callback_function(std::move(commit));
            return true;
        });
}


void GitRevisionWalker::Walk(const GitCommit& start_commit, const GitCommit& end_commit, const std::function<void(GitCommit)>& callback_function)
{
    Walk(start_commit, end_commit, 0, callback_function);
}


void GitRevisionWalker::ReverseWalk(const GitCommit& start_commit, const GitCommit& end_commit, const std::function<void(GitCommit)>& callback_function)
{
    Walk(start_commit, end_commit, GIT_SORT_REVERSE, callback_function);
}


std::vector<GitCommit> GitRevisionWalker::GetCommitsFromHead()
{
    std::vector<GitCommit> commits;

    WalkFromHead(
        [&](GitCommit commit)
        {
            commits.emplace_back(std::move(commit));
            return true;
        });

    return commits;
}


std::vector<GitCommit> GitRevisionWalker::GetCommitsFromHead(const GitCommit& end_commit)
{
    std::vector<GitCommit> commits;

    WalkFromHead(end_commit,
        [&](GitCommit commit)
        {
            commits.emplace_back(std::move(commit));
        });

    return commits;
}
