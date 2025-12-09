#include "StdAfx.h"
#include "GitHelpersDlg.h"
#include "GitAccessor.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/DataExchange.h>


BEGIN_MESSAGE_MAP(GitHelpersDlg, CDialog)
    ON_BN_CLICKED(IDC_CREATE_FILTERED_COMMIT_TEXT, OnCreateCommitTextForFilteredCommits)
END_MESSAGE_MAP()


GitHelpersDlg::GitHelpersDlg(CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_GIT_HELPERS, pParent)
{
}


GitHelpersDlg::~GitHelpersDlg()
{
}


void GitHelpersDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_FILTERED_REPO, m_filteredRepositoryDirectory);
    DDX_Text(pDX, IDC_FILTERED_COMMIT, m_filteredRepositoryCommit);
    DDX_Text(pDX, IDC_DESTINATION_REPO, m_destinationDirectory);
    DDX_Text(pDX, IDC_DESTINATION_BRANCH, m_destinationBranchName);
}


void GitHelpersDlg::OnCreateCommitTextForFilteredCommits()
{
    UpdateData(TRUE);

    try
    {
        GitAccessor git;

        const std::vector<GitCommit> commits = git.ReadCommitHistory(Path::Combine(m_filteredRepositoryDirectory, ".git"),
                                                                     m_filteredRepositoryCommit);

        const std::vector<std::string> original_shas = git.LookupCommitSHAs(Path::Combine(m_destinationDirectory, ".git"),
                                                                            m_destinationBranchName, commits);
        ASSERT(commits.size() == original_shas.size());

        std::vector<std::string> authors;

        std::string message = "Squashed commits:";

        for( size_t i = 0; i < commits.size(); ++i )
        {
            const GitCommit& commit = commits[i];

            std::string author_string = commit.GetAuthorString();

            message.append("\n\n")
                   .append(SO::Trim(commit.GetMessage()))
                   .append("\n[").append(author_string)
                   .append(", ").append(GitAccessor::FormatTime(commit.GetWhen()))
                   .append(", ").append(original_shas[i])
                   .append("]");

            if( std::find(authors.cbegin(), authors.cend(), author_string) == authors.cend() )
                authors.emplace_back(std::move(author_string));
        }

        // add the Co-authored-by trailer
        message.push_back('\n');

        for( const std::string& author : authors )
            message.append("\nCo-authored-by: ").append(author);

        // save the message
        const std::string message_filename = FormatText("Filtered-Commits-Comment-%s.txt", IntToString(GetTimestamp<int64_t>()).c_str());
        FileIO::WriteText(Path::Combine(m_destinationDirectory, message_filename), message, false);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
