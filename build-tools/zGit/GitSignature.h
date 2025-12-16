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
public:
    GitSignature(const git_signature& signature) noexcept;

    bool operator==(const GitSignature& rhs) const noexcept;
    bool operator!=(const GitSignature& rhs) const noexcept { return !operator==(rhs); }

    const std::string& GetName() const noexcept { return m_name; }

    const std::string& GetEmail() const noexcept { return m_email; }

    const GitTime& GetWhen() const noexcept { return m_when; }

    // Returns a string such as "John Doe <john.doe@csprousers.org>".
    std::string GetDisplayString() const noexcept;

private:
    std::string m_name;
    std::string m_email;
    GitTime m_when;
};
