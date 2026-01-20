#include "StdAfx.h"
#include "FilteredCommitsMessageCreatorView.h"
#include <numeric>


namespace
{
    constexpr std::string_view FilteredRepositoryDirectoryKey_sv = "directory-filtered";
    constexpr std::string_view FilteredRepositoryCommitKey_sv    = "commit";
    constexpr std::string_view DestinationDirectoryKey_sv        = "directory-destination";
    constexpr std::string_view DestinationBranchNameKey_sv       = "branch";
}


IMPLEMENT_DYNCREATE(FilteredCommitsMessageCreatorView, CFormView)


BEGIN_MESSAGE_MAP(FilteredCommitsMessageCreatorView, CFormView)
    ON_BN_CLICKED(IDC_CREATE_FILTERED_COMMIT_TEXT, OnCreateCommitTextForFilteredCommits)
END_MESSAGE_MAP()


FilteredCommitsMessageCreatorView::FilteredCommitsMessageCreatorView()
    :   CFormView(IDD_FILTERED_COMMITS_MESSAGE_CREATOR),
        m_settingsDb("Stygitan.db", "FilteredCommitsMessageCreator"),
        m_filteredRepositoryDirectory(m_settingsDb.ReadOrDefault<std::string>(FilteredRepositoryDirectoryKey_sv)),
        m_filteredRepositoryCommit(m_settingsDb.ReadOrDefault<std::string>(FilteredRepositoryCommitKey_sv)),
        m_destinationDirectory(m_settingsDb.ReadOrDefault<std::string>(DestinationDirectoryKey_sv)),
        m_destinationBranchName(m_settingsDb.ReadOrDefault<std::string>(DestinationBranchNameKey_sv))
{
}


void FilteredCommitsMessageCreatorView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_FILTERED_REPO, m_filteredRepositoryDirectory, true);
    DDX_Text(pDX, IDC_FILTERED_COMMIT, m_filteredRepositoryCommit, true);
    DDX_Text(pDX, IDC_DESTINATION_REPO, m_destinationDirectory, true);
    DDX_Text(pDX, IDC_DESTINATION_BRANCH, m_destinationBranchName, true);
}


void FilteredCommitsMessageCreatorView::OnCreateCommitTextForFilteredCommits()
{
    UpdateData(TRUE);

    m_settingsDb.Write<std::string>(FilteredRepositoryDirectoryKey_sv, m_filteredRepositoryDirectory);
    m_settingsDb.Write<std::string>(FilteredRepositoryCommitKey_sv, m_filteredRepositoryCommit);
    m_settingsDb.Write<std::string>(DestinationDirectoryKey_sv, m_destinationDirectory);
    m_settingsDb.Write<std::string>(DestinationBranchNameKey_sv, m_destinationBranchName);

    try
    {
        const std::vector<GitCommit> commits = ReadCommitHistory();
        const std::vector<std::string> original_shas = LookupCommitSHAs(commits);
        ASSERT(commits.size() == original_shas.size());

        std::vector<std::string> authors;

        std::string message = "Squashed commits:";

        for( size_t i = 0; i < commits.size(); ++i )
        {
            const GitCommit& commit = commits[i];
            const GitSignature& author = commit.GetAuthor();

            std::string author_display_string = author.GetDisplayString();

            message.append("\n\n")
                   .append(SO::Trim(commit.GetMessage()))
                   .append("\n[").append(author_display_string)
                   .append(", ").append(author.GetWhen().GetRfc2822String())
                   .append(", ").append(original_shas[i])
                   .append("]");

            if( std::find(authors.cbegin(), authors.cend(), author_display_string) == authors.cend() )
                authors.emplace_back(std::move(author_display_string));
        }

        // add the Co-authored-by trailer
        message.push_back('\n');

        for( const std::string& author : authors )
            message.append("\nCo-authored-by: ").append(author);

        // save the message
        const std::string message_filename = FormatText("Filtered-Commits-Comment-%s.txt", IntToString(GetTimestamp()).c_str());
        FileIO::WriteText(Path::Combine(m_destinationDirectory, message_filename), message, false);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


std::vector<GitCommit> FilteredCommitsMessageCreatorView::ReadCommitHistory() const
{
    GitRepository repo;
    repo.OpenBare(Path::Combine(m_filteredRepositoryDirectory, ".git"));

    const GitCommit end_commit = repo.LookupCommit(m_filteredRepositoryCommit);

    GitRevisionWalker walker(repo);

    return walker.GetCommitsFromHead(end_commit);
}


std::vector<std::string> FilteredCommitsMessageCreatorView::LookupCommitSHAs(const std::vector<GitCommit>& commits) const
{
    GitRepository repo;
    repo.OpenBare(Path::Combine(m_destinationDirectory, ".git"));

    const GitBranch branch = repo.LookupBranch(m_destinationBranchName);

    // get the commit that the branch points to
    const GitObjectId branch_target = branch.GetTarget();

    std::vector<size_t> commits_to_match(commits.size());
    std::iota(commits_to_match.begin(), commits_to_match.end(), 0);

    std::vector<std::string> commit_shas(commits_to_match.size());

    // walk the commits in the branch, finding the ones that match
    GitRevisionWalker walker(repo);

    walker.Walk(branch_target,
        [&](GitCommit commit)
        {
            auto lookup = std::find_if(commits_to_match.cbegin(), commits_to_match.cend(),
                [&](const size_t index)
                {
                    const GitCommit& comparison_commit = commits[index];
                    return commit.Equals(comparison_commit);
                });

            if( lookup == commits_to_match.cend() )
            {
                const GitSignature& author = commit.GetAuthor();

                // search for a commit that has the same author but a different commit message
                lookup = std::find_if(commits_to_match.cbegin(), commits_to_match.cend(),
                    [&](const size_t index)
                    {
                        const GitCommit& comparison_commit = commits[index];
                        return ( author == comparison_commit.GetAuthor() );
                    });

                if( lookup != commits_to_match.cend() )
                {
                    const std::string message = FormatText("Should these commit messages be considered identical?\n\n%s\n\n%s",
                                                           commit.GetMessage().c_str(), commits[*lookup].GetMessage().c_str());

                    if( AfxMessageBox(message, MB_YESNO) != IDYES )
                        lookup = commits_to_match.cend();
                }
            }

            if( lookup != commits_to_match.cend() )
            {
                ASSERT(commit_shas[*lookup].empty());
                commit_shas[*lookup] = commit.GetObjectId().GetHexHash();
                commits_to_match.erase(lookup);
            }

            // stop processing when all commits have been matched
            return !commits_to_match.empty();
        });

    if( !commits_to_match.empty() )
    {
        const std::string unmatched_text = SO::CreateSingleStringUsingCallback(
            commits_to_match,
            [&](const size_t index) { return commits[index].GetMessage(); },
            "\n\n"
        );

        throw CSProException("Not all commits were matched:\n\n" + unmatched_text);
    }

    ASSERT(std::find(commit_shas.cbegin(), commit_shas.cend(), std::string()) == commit_shas.cend());

    return commit_shas;
}
