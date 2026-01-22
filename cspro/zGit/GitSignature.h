#pragma once

#include <zGit/zGit.h>
#include <zGit/GitTime.h>

struct git_signature;


// --------------------------------------------------------------------------
// GitSignature
//
// Stores the values of git_signature: "an action signature."
// --------------------------------------------------------------------------

class ZGIT_API GitSignature
{
private:
    GitSignature(std::string name, std::string email, const git_time& when);

public:
    // GitSignature creates a copy of the git_signature object.
    GitSignature(const git_signature& signature);

    // Creates a new signature.
    static GitSignature Create(std::string name, std::string email);

    bool operator==(const GitSignature& rhs) const noexcept;
    bool operator!=(const GitSignature& rhs) const noexcept { return !operator==(rhs); }

    // Returns the non-null git_signature object that GitSignature wraps.
    operator const git_signature*() const noexcept { return reinterpret_cast<const git_signature*>(&m_wrapper); }

    const std::string& GetName() const noexcept { return m_name; }

    const std::string& GetEmail() const noexcept { return m_email; }

    const GitTime& GetWhen() const noexcept    { return m_wrapper.when; }
    void SetWhen(const GitTime& when) noexcept { m_wrapper.when = when; }

    // Returns a string such as "John Doe <john.doe@csprousers.org>".
    std::string GetDisplayString() const noexcept;

private:
    std::string m_name;
    std::string m_email;

    struct git_signature_wrapper { const char* name; const char* email; GitTime when; };
    git_signature_wrapper m_wrapper;
};
