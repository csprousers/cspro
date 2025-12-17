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
