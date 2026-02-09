#include "StdAfx.h"
#include "GitReference.h"


GitReference::GitReference(git_reference& reference) noexcept
    :   m_reference(&reference)
{
}


GitReference::GitReference(const GitReference& rhs)
    :   m_reference(rhs.m_reference)
{
    if( git_reference_dup(&m_reference, rhs.m_reference) != 0 )
        throw GitException();
}


GitReference::GitReference(GitReference&& rhs) noexcept
    :   m_reference(rhs.m_reference)
{
    rhs.m_reference = nullptr;
}


GitReference::~GitReference() noexcept
{
    if( m_reference != nullptr )
        git_reference_free(m_reference);
}


bool GitReference::operator==(const GitReference& rhs) const noexcept
{
    const git_oid* oid;
    const git_oid* rhs_oid;

    return ( ( ( oid = git_reference_target(m_reference) ) != nullptr ) &&
             ( ( rhs_oid = git_reference_target(rhs.m_reference) ) != nullptr ) &&
             ( memcmp(oid, rhs_oid, sizeof(git_oid)) == 0 ) );
}
