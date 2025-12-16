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


GitObjectId GitBranch::GetTarget() const
{
    const git_oid* const oid = git_reference_target(m_branchRef);

    if( oid == nullptr )
        throw CSProException("The latest commit for branch '%s' is unknown.", GetName().c_str());

    return GitObjectId(*oid);
}
