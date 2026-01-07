#pragma once

#include <zGit/zGit.h>
#include <zGit/GitObjectId.h>

class GitCommit;
class GitRepository;


// --------------------------------------------------------------------------
// GitTag
// --------------------------------------------------------------------------

class ZGIT_API GitTag : public GitObjectId
{
public:
    GitTag(const git_oid& oid, std::string name) noexcept;

    // Returns tag's name.
    const std::string& GetName() const noexcept { return m_name; }

    // Returns tag's name without the refs/tags/ prefix (if applicable).
    std::string GetDisplayName() const noexcept;

private:
    std::string m_name;
};
