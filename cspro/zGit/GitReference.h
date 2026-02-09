#pragma once

#include <zGit/zGit.h>

struct git_reference;


// --------------------------------------------------------------------------
// GitReference
//
// Wraps git_reference: "in-memory representation of a reference."
// --------------------------------------------------------------------------

class ZGIT_API GitReference
{
public:
    // GitReference assumes ownership of the git_reference object.
    GitReference(git_reference& reference) noexcept;
    GitReference(const GitReference& rhs);
    GitReference(GitReference&& rhs) noexcept;
    ~GitReference() noexcept;

    // Compares the references' target IDs.
    bool operator==(const GitReference& rhs) const noexcept;
    bool operator!=(const GitReference& rhs) const noexcept { return !operator==(rhs); }

    // Returns the non-null git_reference object that GitReference wraps.
    operator const git_reference*() const noexcept { return m_reference; }
    operator git_reference*() noexcept             { return m_reference; }

private:
    git_reference* m_reference;
};
