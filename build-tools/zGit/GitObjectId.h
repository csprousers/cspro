#pragma once

#include <zGit/zGit.h>

struct git_object;
struct git_oid;
class GitObject;


// --------------------------------------------------------------------------
// GitObjectId
//
// Wraps git_oid: "unique identity of any object (commit, tree, blob, tag)."
// --------------------------------------------------------------------------

class ZGIT_API GitObjectId
{
public:
    GitObjectId(const git_oid& oid) noexcept;
    GitObjectId(const git_object& object) noexcept;

    // Converts the hex hash string to an object, throwing an exception on error.
    GitObjectId(cs::string_sz hex_hash);

    // Compares the object IDs.
    bool operator==(const GitObjectId& rhs) const noexcept;
    bool operator!=(const GitObjectId& rhs) const noexcept { return !operator==(rhs); }

    // Returns the non-null git_oid object that GitObjectId wraps.
    operator const git_oid*() const noexcept { return reinterpret_cast<const git_oid*>(m_oid); }

    // Returns a string with the hex hash representation of the object.
    static std::string GetHexHash(const git_oid& oid) noexcept;
    static std::string GetHexHash(const GitObject& object) noexcept;
    std::string GetHexHash() const noexcept { return GetHexHash(*operator const git_oid*()); }

private:
    static constexpr size_t GitOidMaxSize = 20;
    std::byte m_oid[GitOidMaxSize];
};
