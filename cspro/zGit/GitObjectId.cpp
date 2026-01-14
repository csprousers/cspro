#include "StdAfx.h"
#include "GitObjectId.h"


GitObjectId::GitObjectId(const git_oid& oid) noexcept
{
    static_assert(GitOidMaxSize == GIT_OID_MAX_SIZE);
    memcpy(m_oid, oid.id, GitOidMaxSize);
}


GitObjectId::GitObjectId(const git_object& object) noexcept
    :   GitObjectId(*git_object_id(&object))
{
}


GitObjectId::GitObjectId(const cs::string_sz hex_hash)
{
    if( git_oid_fromstr(reinterpret_cast<git_oid*>(m_oid), hex_hash.c_str()) < 0 )
        throw CSProException("A Git object ID could not be created from '%s'.", hex_hash.c_str());
}


bool GitObjectId::operator==(const GitObjectId& rhs) const noexcept
{
    return ( memcmp(m_oid, rhs.m_oid, GitOidMaxSize) == 0 );
}


std::string GitObjectId::GetHexHash(const git_oid& oid) noexcept
{
    char buffer[GIT_OID_MAX_HEXSIZE + 1];

    return std::string(git_oid_tostr(buffer, _countof(buffer), &oid),
                       GIT_OID_MAX_HEXSIZE);
}


std::string GitObjectId::GetHexHash(const GitObject& object) noexcept
{
    return GitObjectId::GetHexHash(*git_object_id(object));
}
