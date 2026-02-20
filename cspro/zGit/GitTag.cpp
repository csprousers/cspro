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


std::string GitTag::GetMessage(GitRepository& repo) const
{
    repo.EnsureRepositoryIsOpen();

    std::string message;
    git_tag* tag;

    if( git_tag_lookup(&tag, repo, *this) == 0 )
    {
        const char* const message_ptr = git_tag_message(tag);

        if( message_ptr != nullptr )
            message = message_ptr;

        git_tag_free(tag);
    }

    return message;
}
