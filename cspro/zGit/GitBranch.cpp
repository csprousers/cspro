#include "StdAfx.h"
#include "GitBranch.h"


GitBranch::GitBranch(git_reference& branch_ref, std::string name) noexcept
    :   m_branchRef(&branch_ref),
        m_name(std::move(name))
{
    ASSERT(!m_name.empty());
}


GitBranch::GitBranch(git_reference& branch_ref) noexcept
    :   GitBranch(branch_ref, GetBranchName(branch_ref))
{
}


GitBranch::GitBranch(GitBranch&& rhs) noexcept
    :   m_branchRef(rhs.m_branchRef),
        m_name(std::move(rhs.m_name))
{
    rhs.m_branchRef = nullptr;
}


GitBranch::~GitBranch() noexcept
{
    if( m_branchRef != nullptr )
        git_reference_free(m_branchRef);
}


GitBranch& GitBranch::operator=(GitBranch&& rhs) noexcept
{
    // swapping the branch references ensures that this object's
    // branch reference will be deleted in rhs' destructor
    std::swap(m_branchRef, rhs.m_branchRef);

    m_name = std::move(rhs.m_name);

    return *this;
}


bool GitBranch::operator==(const GitBranch& rhs) const noexcept
{
    const git_oid* oid;
    const git_oid* rhs_oid;

    return ( ( ( oid = git_reference_target(m_branchRef) ) != nullptr ) &&
             ( ( rhs_oid = git_reference_target(rhs.m_branchRef) ) != nullptr ) &&
             ( memcmp(oid, rhs_oid, sizeof(git_oid)) == 0 ) );
}


std::string GitBranch::GetBranchName(git_reference& branch_ref)
{
    const char* name;

    if( git_branch_name(&name, &branch_ref) != 0 )
        throw GitException();

    return name;
}


void GitBranch::Refresh(GitRepository& repo)
{
    git_reference_free(m_branchRef);

    if( git_branch_lookup(&m_branchRef, repo, m_name.c_str(), GIT_BRANCH_ALL) != 0 )
    {
        m_branchRef = nullptr;
        throw GitException();
    }
}


std::unique_ptr<GitBranch> GitBranch::GetUpstreamBranch() const
{
    git_reference* upstream_branch_ref;

    switch( git_branch_upstream(&upstream_branch_ref, m_branchRef) )
    {
        case 0:
            return std::make_unique<GitBranch>(*upstream_branch_ref);

        case GIT_ENOTFOUND:
            return nullptr;

        default:
            throw GitException();
    }
}


GitObjectId GitBranch::GetTarget() const
{
    const git_oid* const oid = git_reference_target(m_branchRef);

    if( oid == nullptr )
        throw GitException("The latest commit for branch '%s' is unknown.", GetName().c_str());

    return GitObjectId(*oid);
}


void GitBranch::Delete()
{
    if( git_branch_delete(m_branchRef) != 0 )
        throw GitException();

    git_reference_free(m_branchRef);
    m_branchRef = nullptr;
}
