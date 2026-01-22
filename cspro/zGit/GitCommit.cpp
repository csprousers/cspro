#include "StdAfx.h"
#include "GitCommit.h"
#include <zToolsO/TextEncoding.h>


GitCommit::GitCommit(git_commit& commit) noexcept
    :   m_commit(&commit)
{
}


GitCommit::GitCommit(const GitCommit& rhs)
    :   m_message(rhs.m_message),
        m_committer(rhs.m_committer),
        m_author(rhs.m_author)
{
    if( git_commit_dup(&m_commit, rhs.m_commit) != 0 )
        throw GitException();
}


GitCommit::GitCommit(GitCommit&& rhs) noexcept
    :   m_commit(rhs.m_commit),
        m_message(std::move(rhs.m_message)),
        m_author(std::move(rhs.m_author))
{
    rhs.m_commit = nullptr;
}


GitCommit::~GitCommit() noexcept
{
    if( m_commit != nullptr )
        git_commit_free(m_commit);
}


GitCommit& GitCommit::operator=(GitCommit&& rhs) noexcept
{
    m_commit = rhs.m_commit;
    m_message = std::move(rhs.m_message);
    m_author = std::move(rhs.m_author);

    rhs.m_commit = nullptr;

    return *this;
}


bool GitCommit::operator==(const GitCommit& rhs) const noexcept
{
    return git_oid_equal(operator const git_oid*(), rhs.operator const git_oid*());
}


bool GitCommit::Equals(const GitCommit& rhs) const noexcept
{
    return ( GetMessage() == rhs.GetMessage() &&
             GetAuthor() == rhs.GetAuthor() );
}


GitCommit::operator const git_oid*() const noexcept
{
    return git_commit_id(m_commit);
}


GitObjectId GitCommit::GetObjectId() const noexcept
{
    return GitObjectId(*operator const git_oid*());
}


const std::string& GitCommit::GetMessage() const noexcept
{
    if( m_message.empty() )
    {
        std::string& message = const_cast<GitCommit*>(this)->m_message;
        message = SO::Trim(std::string_view(git_commit_message(m_commit)));

        // strip UTF-8 BOMs when present
        if( SO::StartsWith(message, TextEncoding::Utf8Bom_sv) )
        {
            message.erase(0, TextEncoding::Utf8Bom_sv.length());
            SO::MakeTrim(message);
        }
    }

    return m_message;
}


const GitSignature& GitCommit::GetCommitter() const noexcept
{
    if( !m_committer.has_value() )
        const_cast<GitCommit*>(this)->m_committer.emplace(*git_commit_committer(m_commit));

    return *m_committer;
}


const GitSignature& GitCommit::GetAuthor() const noexcept
{
    if( !m_author.has_value() )
        const_cast<GitCommit*>(this)->m_author.emplace(*git_commit_author(m_commit));

    return *m_author;
}


unsigned int GitCommit::GetParentCount() const noexcept
{
    return git_commit_parentcount(m_commit);
}


GitCommit GitCommit::GetParent(const unsigned int parent_commit_index) const
{
    git_commit* commit;

    if( git_commit_parent(&commit, m_commit, parent_commit_index) != 0 )
        throw GitException();

    return GitCommit(*commit);
}


GitTree GitCommit::GetTree() const
{
    git_tree* tree;

    if( git_commit_tree(&tree, m_commit) != 0 )
        throw GitException();

    return GitTree(*tree);
}
