#include "StdAfx.h"
#include "GitBranch.h"


GitBranch::GitBranch(git_reference& branch_ref) noexcept
    :   m_branchRef(&branch_ref)
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


const std::string& GitBranch::GetName() const noexcept
{
    if( m_name.empty() )
    {
        const char* name;

        if( git_branch_name(&name, m_branchRef) == 0 )
            const_cast<GitBranch*>(this)->m_name = name;

        ASSERT(!m_name.empty());
    }

    return m_name;
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
            ThrowGitException();
    }
}


GitObjectId GitBranch::GetTarget() const
{
    const git_oid* const oid = git_reference_target(m_branchRef);

    if( oid == nullptr )
        throw CSProException("The latest commit for branch '%s' is unknown.", GetName().c_str());

    return GitObjectId(*oid);
}


void GitBranch::Delete()
{
    if( git_branch_delete(m_branchRef) != 0 )
        ThrowGitException();

    git_reference_free(m_branchRef);
    m_branchRef = nullptr;
}
