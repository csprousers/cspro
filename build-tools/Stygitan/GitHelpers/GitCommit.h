#pragma once


// --------------------------------------------------------------------------
// GitCommit
// --------------------------------------------------------------------------

class GitCommit
{
public:
    GitCommit(std::string message, git_time when, std::string author_name, std::string author_email);
    GitCommit(const git_commit* commit, const git_signature* author);
    GitCommit(const git_commit* commit);

    bool operator==(const GitCommit& rhs) const;

    const std::string& GetMessage() const { return m_message; }

    const git_time& GetWhen() const { return m_when; }

    const std::string& GetAuthorName() const  { return m_authorName; }
    const std::string& GetAuthorEmail() const { return m_authorEmail; }

    std::string GetAuthorString() const;

private:
    std::string m_message;
    git_time m_when;
    std::string m_authorName;
    std::string m_authorEmail;
};



// --------------------------------------------------------------------------
// git_time comparison
// --------------------------------------------------------------------------

inline bool operator==(const git_time& gt1, const git_time& gt2)
{
    return ( memcmp(&gt1, &gt2, sizeof(git_time)) == 0 );
}
