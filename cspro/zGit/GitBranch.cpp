#include "StdAfx.h"
#include "GitBranch.h"


GitBranch::GitBranch(GitRepository& repo, std::string name) noexcept
    :   m_repo(&repo),
        m_name(std::move(name))
{
}


GitBranch::GitBranch(GitRepository& repo, const GitReference& branch_ref)
    :   GitBranch(repo, GetBranchName(branch_ref))
{
}


std::string GitBranch::GetBranchName(const GitReference& branch_ref)
{
    const char* name;

    if( git_branch_name(&name, branch_ref) != 0 )
        throw GitException();

    return name;
}


bool GitBranch::operator==(const GitBranch& rhs) const
{
    return ( m_name == rhs.m_name &&
             GetReference() == rhs.GetReference() );
}


GitReference GitBranch::GetReference() const
{
    m_repo->EnsureRepositoryIsOpen();

    git_reference* branch_ref;

    if( git_branch_lookup(&branch_ref, *m_repo, m_name.c_str(), GIT_BRANCH_ALL) != 0 )
        throw GitException();

    return GitReference(*branch_ref);
}


std::unique_ptr<GitBranch> GitBranch::GetUpstreamBranch() const
{
    const GitReference branch_ref = GetReference();
    git_reference* upstream_branch_ref;

    switch( git_branch_upstream(&upstream_branch_ref, branch_ref) )
    {
        case 0:
            return std::make_unique<GitBranch>(*m_repo, GitReference(*upstream_branch_ref));

        case GIT_ENOTFOUND:
            return nullptr;

        default:
            throw GitException();
    }
}


GitObjectId GitBranch::GetTarget() const
{
    const GitReference branch_ref = GetReference();
    const git_oid* const oid = git_reference_target(branch_ref);

    if( oid == nullptr )
        throw GitException("The latest commit for branch '%s' is unknown.", GetName().c_str());

    return GitObjectId(*oid);
}


void GitBranch::Delete()
{
    GitReference branch_ref = GetReference();

    if( git_branch_delete(branch_ref) != 0 )
        throw GitException();
}
