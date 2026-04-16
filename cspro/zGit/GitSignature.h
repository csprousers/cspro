#pragma once

#include <zGit/zGit.h>
#include <zGit/GitTime.h>

struct git_signature;
class GitRepository;


// --------------------------------------------------------------------------
// GitSignature
//
// Stores the values of git_signature: "an action signature."
// --------------------------------------------------------------------------

class ZGIT_API GitSignature
{
private:
    GitSignature(SharableString name, SharableString email, const git_time& when) noexcept;

public:
    // GitSignature creates a copy of the git_signature object.
    GitSignature(const git_signature& signature) noexcept;

    // Creates a new signature.
    static GitSignature Create(SharableString name, SharableString email) noexcept;

    // Creates a new default signature (looking at local, global, and system signatures).
    // An exception is thrown if no default signature is defined.
    static GitSignature CreateDefault(GitRepository& repo);

    bool operator==(const GitSignature& rhs) const noexcept;
    bool operator!=(const GitSignature& rhs) const noexcept { return !operator==(rhs); }

    // Returns the non-null git_signature object that GitSignature wraps.
    operator const git_signature*() const noexcept { return reinterpret_cast<const git_signature*>(&m_wrapper); }

    const std::string& GetName() const noexcept { return *m_name; }

    const std::string& GetEmail() const noexcept { return *m_email; }

    const GitTime& GetWhen() const noexcept    { return m_wrapper.when; }
    void SetWhen(const GitTime& when) noexcept { m_wrapper.when = when; }

    // Returns a string such as "John Doe <john.doe@csprousers.org>".
    std::string GetDisplayString() const noexcept;

private:
    SharableString m_name;
    SharableString m_email;

    struct git_signature_wrapper { const char* name; const char* email; GitTime when; };
    git_signature_wrapper m_wrapper;
};
