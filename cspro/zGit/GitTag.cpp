#include "StdAfx.h"
#include "GitTag.h"


GitTag::GitTag(const git_oid& oid, std::string name) noexcept
    :   GitObjectId(oid),
        m_name(std::move(name))
{
}


std::string GitTag::GetDisplayName() const noexcept
{
    if( SO::StartsWith(m_name, RefsTagPrefix_sv) )
        return m_name.substr(RefsTagPrefix_sv.length());

    return m_name;
}
