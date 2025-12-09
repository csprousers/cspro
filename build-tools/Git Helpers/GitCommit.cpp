#include "StdAfx.h"
#include "GitCommit.h"


GitCommit::GitCommit(std::string message, const git_time when, std::string author_name, std::string author_email)
    :   m_message(std::move(message)),
        m_when(when),
        m_authorName(std::move(author_name)),
        m_authorEmail(std::move(author_email))
{
}


GitCommit::GitCommit(const git_commit* const commit, const git_signature* const author)
    :   GitCommit(git_commit_message(commit), author->when, author->name, author->email)
{
}


GitCommit::GitCommit(const git_commit* const commit)
    :   GitCommit(commit, git_commit_author(commit))
{
}


bool GitCommit::operator==(const GitCommit& rhs) const
{
    return ( m_message == rhs.m_message &&
             m_when == rhs.m_when &&
             m_authorName == rhs.m_authorName &&
             m_authorEmail == rhs.m_authorEmail );
}


std::string GitCommit::GetAuthorString() const
{
    return m_authorName + " <" + m_authorEmail + ">";
}
