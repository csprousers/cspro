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
    static constexpr std::string_view RefsTagPrefix_sv = "refs/tags/";

    GitTag(const git_oid& oid, std::string name) noexcept;

    // Returns the tag's name.
    const std::string& GetName() const noexcept { return m_name; }

    // Returns the tag's name without the refs/tags/ prefix (if applicable).
    std::string GetDisplayName() const noexcept;

    // Returns an annotated tag's message, returning a blank string for non-annotated tags.
    std::string GetMessage(GitRepository& repo) const;

private:
    std::string m_name;
};
