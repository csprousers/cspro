#include "StdAfx.h"
#include "GitTag.h"


GitTag::GitTag(const git_oid& oid, std::string name) noexcept
    :   GitObjectId(oid),
        m_name(std::move(name))
{
}


std::string GitTag::GetDisplayName() const noexcept
{
    constexpr std::string_view TagPrefix_sv = "refs/tags/";

    if( SO::StartsWith(m_name, TagPrefix_sv) )
        return m_name.substr(TagPrefix_sv.length());

    return m_name;
}
