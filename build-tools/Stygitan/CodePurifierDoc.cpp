#include "StdAfx.h"
#include "CodePurifierDoc.h"
#include <zToolsO/Encoders.h>
#include <regex>


IMPLEMENT_DYNCREATE(CodePurifierDoc, CDocument)


CodePurifierDoc::CodePurifierDoc()
{
}


void CodePurifierDoc::SetTitle(LPCTSTR /*lpszTitle*/)
{
    const std::string directory = PortableFunctions::PathRemoveTrailingSlash(m_repo.GetWorkingDirectory());
    const std::wstring title = L"Code Purifier: " + TC::ToWide(directory);
    __super::SetTitle(title.c_str());
}


void CodePurifierDoc::SetPathName(LPCTSTR lpszPathName, BOOL /*bAddToMRU = TRUE*/)
{
    __super::SetPathName(lpszPathName, FALSE);
}


BOOL CodePurifierDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    try
    {
        m_repo.Open(TC::ToUtf8(lpszPathName));
        RefreshData();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    return TRUE;
}


void CodePurifierDoc::RefreshData()
{
    const std::optional<std::string> previously_loaded_branch_name = m_currentBranch.has_value()
        ? std::make_optional(m_currentBranch->GetName())
        : std::nullopt;

    m_currentBranch.emplace(m_repo.GetCurrentBranch());

    // only load (or refresh) some data when the branch has changed
    if( !previously_loaded_branch_name.has_value() ||
        *previously_loaded_branch_name != m_currentBranch->GetName() )
    {
        m_remoteBranch = m_currentBranch->GetUpstreamBranch();

        m_initialNumberOfBranchCopies.reset();
    }

    // enumerate the temporary copies of this branch
    EnumerateBranchCopies();

    // find the "clean commit"
    LocateCleanCommit();

    // load recent commits
    LoadRecentCommits();
}


void CodePurifierDoc::EnumerateBranchCopies()
{
    // (see CodePurifierView::OnCreateBranchCopy for branch naming rules)
    const std::regex local_branch_regex(FormatText(R"(^\d{8}-CP-%s$)", Encoders::ToRegex(m_currentBranch->GetName()).c_str()));

    m_branchCopies.clear();

    m_repo.ForeachLocalBranch(
        [&](GitBranch branch)
        {
            std::string name = branch.GetName();

            if( std::regex_match(name, local_branch_regex) )
                m_branchCopies.try_emplace(std::move(name), std::move(branch));

            return true;
        });

    if( !m_initialNumberOfBranchCopies.has_value() )
        m_initialNumberOfBranchCopies = m_branchCopies.size();
}


void CodePurifierDoc::LocateCleanCommit()
{
    // the "clean commit" will be calculated in the following order:
    // - a user-selected commit
    // - the remote branch's target
    // - the "oldest" commit with two parents (likely a merged pull request)
    // - the "oldest" commit

    m_cleanCommit.reset();

    const GitCommit branch_commit = m_repo.LookupCommit(m_currentBranch->GetTarget());

    try
    {
        // a user-selected commit
        if( m_cleanCommitOverride.has_value() )
        {
            m_cleanCommit = m_repo.LookupCommit(*m_cleanCommitOverride);
        }

        // the remote branch's target
        else if( m_remoteBranch != nullptr )
        {
            m_cleanCommit = m_repo.LookupCommit(m_remoteBranch->GetTarget());
        }

        if( m_cleanCommit.has_value() && ( *m_cleanCommit == branch_commit ||
                                           m_repo.IsCommitDescendantOf(branch_commit, *m_cleanCommit) ) )
        {
            return;
        }
    }

    catch(...)
    {
        m_cleanCommit.reset();
    }

    ASSERT(!m_cleanCommit.has_value());

    // if here, there is no remote branch or the overridden commit is not a valid ancestor,
    // so find the "oldest" commit with two parents (likely a merged pull request);
    // the walk will continue even if a commit without two parents exists, which will
    // result in the clean commit being the initial commit
    GitRevisionWalker walker(m_repo);

    walker.Walk(branch_commit,
        [&](GitCommit commit)
        {
            return ( m_cleanCommit.emplace(std::move(commit)).GetParentCount() < 2 );
        });
}


void CodePurifierDoc::LoadRecentCommits()
{
    constexpr size_t NumberAdditionalCommitsToLoad = 4;
    std::optional<size_t> additional_commits_to_load;

    m_recentCommits.clear();

    GitRevisionWalker walker(m_repo);

    walker.WalkFromHead(
        [&](GitCommit commit)
        {
            if( m_cleanCommit == commit )
            {
                ASSERT(!additional_commits_to_load.has_value());
                additional_commits_to_load = NumberAdditionalCommitsToLoad;
            }

            else if( additional_commits_to_load.has_value() )
            {
                --(*additional_commits_to_load);
            }

            m_recentCommits.emplace_back(std::move(commit));

            return ( additional_commits_to_load != 0 );
        });
}
