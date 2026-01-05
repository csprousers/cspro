#include "StdAfx.h"
#include "CodePurifierDoc.h"
#include <zToolsO/Encoders.h>
#include <zGit/GitTree.h>
#include <regex>


IMPLEMENT_DYNCREATE(CodePurifierDoc, CDocument)


CodePurifierDoc::CodePurifierDoc()
    :   m_wndForGitUpdates(nullptr),
        m_refreshDataCancelFlag(false),
        m_directoryChangeHandle(nullptr),
        m_directoryChangesMadeInGitDirectory(false),
        m_directoryChangesMadeInWorkingDirectory(false)
{
}


CodePurifierDoc::~CodePurifierDoc()
{
    StopGitProcessing();
}


void CodePurifierDoc::SetTitle(LPCTSTR /*lpszTitle*/)
{
    const std::string directory = PortableFunctions::PathRemoveTrailingSlash(m_repoWorkingDirectory);
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
        m_repoWorkingDirectory = m_repo.GetWorkingDirectory();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    return TRUE;
}


void CodePurifierDoc::OnCloseDocument()
{
    StopGitProcessing();

    __super::OnCloseDocument();
}


void CodePurifierDoc::StartGitProcessing(CWnd* const wnd_for_updates)
{
    ASSERT(wnd_for_updates != nullptr);
    m_wndForGitUpdates = wnd_for_updates;

    // watch the directory for changes so as to know when to refresh the data
    StartDirectoryChangeWatcher();

    // refresh all the data
    StartRefreshDataThread(RefreshStartAction::All);
}


void CodePurifierDoc::StopGitProcessing()
{
    StopRefreshDataThread(ThreadStopType::Cancel);
    StopDirectoryChangeWatcher();
    m_wndForGitUpdates = nullptr;
}


void CodePurifierDoc::ToggleGitProcessingUpdates(const bool activate)
{
    // on activation, refresh data (as informed by the directory watcher)
    if( activate )
    {
        if( m_directoryChangesMadeInGitDirectory )
        {
            m_directoryChangesMadeInGitDirectory = false;

            StartRefreshDataThread(m_directoryChangesMadeInWorkingDirectory ? RefreshStartAction::All :
                                                                              RefreshStartAction::AllGitRelated);
        }

        else if( m_directoryChangesMadeInWorkingDirectory )
        {
            StartRefreshDataThread(RefreshStartAction::IdentifyModifiedFiles);
        }
    }
}


void CodePurifierDoc::StartRefreshDataThread(const RefreshStartAction action,
                                             std::function<void(const CP::RefreshDataChanges& changes)> post_refresh_action/* = { }*/)
{
    if( m_refreshDataThread.has_value() )
    {
        StopRefreshDataThread(ThreadStopType::Cancel);
        ASSERT(!m_refreshDataThread.has_value());
    }

    ASSERT(!m_refreshDataCancelFlag);

    m_refreshDataThread.emplace(
        [this, action, post_refresh_action_ = std::move(post_refresh_action)]()
        {
            try
            {
                const CP::RefreshDataChanges changes = RefreshData(action);

                if( post_refresh_action_ )
                    post_refresh_action_(changes);
            }

            catch( const CSProException& exception )
            {
                // display the error and close the Code Purifier on the UI thread
                RunOnUIThreadAsync(
                    [this, message = std::string(exception.what())]()
                    {
                        ErrorMessage::Display(message);
                        WithParentFrame(*this, [](CFrameWnd& frame_wnd) { frame_wnd.PostMessage(WM_CLOSE); });
                    });
            }
        });
}


void CodePurifierDoc::StopRefreshDataThread(const ThreadStopType thread_stop_type)
{
    if( !m_refreshDataThread.has_value() )
        return;

    if( m_refreshDataThread->joinable() )
    {
        if( thread_stop_type == ThreadStopType::Cancel )
            m_refreshDataCancelFlag = true;

        m_refreshDataThread->join();

        m_refreshDataCancelFlag = false;
    }

    m_refreshDataThread.reset();
}


void CodePurifierDoc::StartDirectoryChangeWatcher()
{
    if( m_directoryChangeThread.has_value() )
        return;

    ASSERT(m_directoryChangeHandle == nullptr);

    // FILE_FLAG_BACKUP_SEMANTICS is required for ReadDirectoryChangesW
    m_directoryChangeHandle = CreateFile(TC::ToWide(m_repoWorkingDirectory).c_str(),
                                         FILE_LIST_DIRECTORY,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                         nullptr,
                                         OPEN_EXISTING,
                                         FILE_FLAG_BACKUP_SEMANTICS,
                                         nullptr);

    if( m_directoryChangeHandle == INVALID_HANDLE_VALUE )
        throw CSProException("Unable to watch the directory.");

    m_directoryChangeThread.emplace(
        [this]()
        {
            auto buffer = std::make_unique<BinaryBlock>(sizeof(DWORD) * 1024);
            DWORD bytes_returned;

            while( ReadDirectoryChangesW(m_directoryChangeHandle,
                                         buffer->data(), static_cast<DWORD>(buffer->size()),
                                         TRUE, // watch subdirectories
                                         FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                         &bytes_returned,
                                         nullptr, nullptr) )
            {
                if( bytes_returned == 0 )
                {
                    // the buffer was not big enough so resize it
                    buffer = std::make_unique<BinaryBlock>(buffer->size() * 2);
                }

                else
                {
                    ProcessDirectoryChange(reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer->data()));
                }
            }
        });
}


void CodePurifierDoc::StopDirectoryChangeWatcher()
{
    if( !m_directoryChangeThread.has_value() )
    {
        ASSERT(m_directoryChangeHandle == nullptr);
        return;
    }

    if( m_directoryChangeThread->joinable() )
    {
        CancelIoEx(m_directoryChangeHandle, nullptr);
        m_directoryChangeThread->join();
    }

    m_directoryChangeThread.reset();

    VERIFY(CloseHandle(m_directoryChangeHandle));
    m_directoryChangeHandle = nullptr;
}


void CodePurifierDoc::ProcessDirectoryChange(const FILE_NOTIFY_INFORMATION* fni)
{
    ASSERT(fni != nullptr);

    while( true )
    {
        const std::wstring_view filename_sv(fni->FileName, fni->FileNameLength / sizeof(wchar_t));

        if( filename_sv._Starts_with(L".git") )
        {
            m_directoryChangesMadeInGitDirectory = true;
        }

        else
        {
            m_directoryChangesMadeInWorkingDirectory = true;
        }

        if( fni->NextEntryOffset == 0 )
            break;

        fni = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(reinterpret_cast<const char*>(fni) + fni->NextEntryOffset);
    }
}


CP::RefreshDataChanges CodePurifierDoc::RefreshData(RefreshStartAction action)
{
    CP::RefreshDataChanges changes { false };
    bool identify_modified_files = false;

    const bool process_all_git_related_actions = ( action == RefreshStartAction::All ||
                                                   action == RefreshStartAction::AllGitRelated );

    if( process_all_git_related_actions ||
        action == RefreshStartAction::UpdateBranches )
    {
        // when there are changes to the local or remote branches, we must also...
        if( RefreshBranchDetails() )
        {
            changes.branch_details = true;

            if( m_refreshDataCancelFlag )
                return changes;

            // ...enumerate the temporary copies of this branch
            EnumerateBranchCopies();

            action = RefreshStartAction::LocateCleanCommit;
            identify_modified_files = true;

            if( m_refreshDataCancelFlag )
                return changes;
        }
    }

    // ...potentially find the "clean commit"
    if( process_all_git_related_actions ||
        action == RefreshStartAction::LocateCleanCommit )
    {
        if( LocateCleanCommit() )
        {
            changes.clean_commit = true;
            action = RefreshStartAction::LoadRecentCommits;
            identify_modified_files = true;
        }
    }

    if( m_refreshDataCancelFlag )
        return changes;

    // ...potentially load recent commits
    if( process_all_git_related_actions ||
        action == RefreshStartAction::LoadRecentCommits )
    {
        if( LoadRecentCommits() )
            changes.recent_commits = true;
    }

    if( m_refreshDataCancelFlag )
        return changes;

    // ...potentially load the files modified since the clean commit
    if( identify_modified_files ||
        action == RefreshStartAction::All ||
        action == RefreshStartAction::IdentifyModifiedFiles )
    {
        changes.modified_files = IdentifyModifiedFiles();
    }

    return changes;
}


bool CodePurifierDoc::RefreshBranchDetails()
{
    GitBranch current_branch = m_repo.GetCurrentBranch();
    std::unique_ptr<GitBranch> remote_branch = current_branch.GetUpstreamBranch();

    if( ( m_branchDetails != nullptr ) &&
        ( current_branch == m_branchDetails->current_branch ) &&
        ( ( remote_branch == nullptr ) == ( m_branchDetails->remote_branch == nullptr ) ) &&
        ( remote_branch == nullptr || *remote_branch == *m_branchDetails->remote_branch ) )
    {
        return false; // no changes
    }

    m_branchDetails.reset(new CP::BranchDetails { std::move(current_branch), std::move(remote_branch) });

    if( m_wndForGitUpdates != nullptr )
        m_wndForGitUpdates->PostMessage(UWM::Stygitan::UpdateUI, CP::Update::BranchDetails);

    return true;
}


void CodePurifierDoc::EnumerateBranchCopies()
{
    ASSERT(m_branchDetails != nullptr);

    // see CreateBranchCopy for branch naming rules
    const std::regex local_branch_regex(FormatText(R"(^\d{4}-\d{4}-CP-%s$)", Encoders::ToRegex(m_branchDetails->current_branch.GetName()).c_str()));

    auto branch_copies = std::make_unique<std::map<std::string, GitBranch>>();

    m_repo.ForeachLocalBranch(
        [&](GitBranch branch)
        {
            std::string name = branch.GetName();

            if( std::regex_match(name, local_branch_regex) )
                branch_copies->try_emplace(std::move(name), std::move(branch));

            return !m_refreshDataCancelFlag;
        });

    if( m_refreshDataCancelFlag )
        return;

    if( m_branchCopies != nullptr &&
        *branch_copies == *m_branchCopies  )
    {
        return; // no changes
    }

    m_branchCopies = std::move(branch_copies);

    if( m_wndForGitUpdates != nullptr )
        m_wndForGitUpdates->PostMessage(UWM::Stygitan::UpdateUI, CP::Update::BranchCopies);
}


bool CodePurifierDoc::LocateCleanCommit()
{
    ASSERT(m_branchDetails != nullptr);

    // the "clean commit" will be calculated in the following order:
    // - a user-selected commit
    // - the remote branch's target
    // - the "oldest" commit with two parents (likely a merged pull request)
    // - the "oldest" commit

    const GitCommit branch_commit = m_repo.LookupCommit(m_branchDetails->current_branch.GetTarget());
    std::optional<GitCommit> clean_commit;
    bool found_valid_clean_commit = false;

    try
    {
        // a user-selected commit
        if( m_cleanCommitOverride.has_value() )
        {
            clean_commit = m_repo.LookupCommit(*m_cleanCommitOverride);
        }

        // the remote branch's target
        else if( m_branchDetails->remote_branch != nullptr )
        {
            clean_commit = m_repo.LookupCommit(m_branchDetails->remote_branch->GetTarget());
        }

        if( clean_commit.has_value() && ( *clean_commit == branch_commit ||
                                           m_repo.IsCommitDescendantOf(branch_commit, *clean_commit) ) )
        {
            found_valid_clean_commit = true;
        }
    }
    catch(...) { }

    if( !found_valid_clean_commit )
    {
        // if here, there is no remote branch or the overridden commit is not a valid ancestor,
        // so find the "oldest" commit with two parents (likely a merged pull request);
        // the walk will continue even if a commit without two parents exists, which will
        // result in the clean commit being the initial commit
        GitRevisionWalker walker(m_repo);

        walker.Walk(branch_commit,
            [&](GitCommit commit)
            {
                return ( !m_refreshDataCancelFlag &&
                         clean_commit.emplace(std::move(commit)).GetParentCount() < 2 );
            });
    }

    if( m_refreshDataCancelFlag )
        return false;

    if( ( clean_commit.has_value() == ( m_cleanCommit != nullptr ) ) &&
        ( !clean_commit.has_value() || *clean_commit == *m_cleanCommit ) )
    {
        return false; // no changes
    }

    m_cleanCommit = clean_commit.has_value() ? std::make_unique<GitCommit>(std::move(*clean_commit)) :
                                               nullptr;

    if( m_wndForGitUpdates != nullptr )
        m_wndForGitUpdates->PostMessage(UWM::Stygitan::UpdateUI, CP::Update::CleanCommit);

    return true;
}


bool CodePurifierDoc::LoadRecentCommits()
{
    constexpr size_t NumberAdditionalCommitsToLoad = 9;
    std::optional<size_t> additional_commits_to_load;

    auto recent_commits = std::make_unique<std::vector<GitCommit>>();

    GitRevisionWalker walker(m_repo);

    walker.WalkFromHead(
        [&](GitCommit commit)
        {
            if( additional_commits_to_load.has_value() )
            {
                --(*additional_commits_to_load);
            }

            else if( m_cleanCommit != nullptr && commit == *m_cleanCommit )
            {
                additional_commits_to_load = NumberAdditionalCommitsToLoad;
            }

            recent_commits->emplace_back(std::move(commit));

            return ( !m_refreshDataCancelFlag &&
                     additional_commits_to_load != 0 );
        });

    if( m_refreshDataCancelFlag )
        return false;

    if( m_recentCommits != nullptr &&
        *recent_commits == *m_recentCommits  )
    {
        return false; // no changes
    }

    m_recentCommits = std::move(recent_commits);

    if( m_wndForGitUpdates != nullptr )
        m_wndForGitUpdates->PostMessage(UWM::Stygitan::UpdateUI, CP::Update::RecentCommits);

    return true;
}


bool CodePurifierDoc::IdentifyModifiedFiles()
{
    ASSERT(m_branchDetails != nullptr);

    m_directoryChangesMadeInWorkingDirectory = false;

    auto modified_files = std::make_unique<std::vector<CP::ModifiedFile>>();

    if( m_cleanCommit != nullptr )
    {
        m_repo.ForeachDifferenceInWorkingDirectory(*m_cleanCommit,
            [&](std::string path, const unsigned int diff_flag)
            {
                modified_files->emplace_back(std::move(path), diff_flag);
                return !m_refreshDataCancelFlag;
            });

        if( m_refreshDataCancelFlag )
            return false;

        // sort case-insensitively and so that files at directory roots are listed before subdirectory files
        std::sort(modified_files->begin(), modified_files->end(),
            [&](const CP::ModifiedFile& mf1, const CP::ModifiedFile& mf2)
            {
                return Helpers::CompareFilePathsByDirectory(mf1.git_path, mf2.git_path);
            });
    }

    if( m_refreshDataCancelFlag )
        return false;

    if( m_modifiedFiles != nullptr &&
        *modified_files == *m_modifiedFiles  )
    {
        return false; // no changes
    }

    m_modifiedFiles  = std::move(modified_files);

    if( m_wndForGitUpdates != nullptr )
        m_wndForGitUpdates->PostMessage(UWM::Stygitan::UpdateUI, CP::Update::ModifiedFiles);

    return true;
}


void CodePurifierDoc::CreateBranchCopy()
{
    StopRefreshDataThread(ThreadStopType::Wait);

    if( m_branchCopies == nullptr )
        throw CSProException("You cannot create a copy when there is no current branch.");

    ASSERT(m_branchDetails != nullptr);

    const GitCommit commit = m_repo.LookupCommit(m_branchDetails->current_branch.GetTarget());

    // the branch name will be: [commit date]-[commit time]-CP-[branch name]
    std::string branch_name = SO::Concatenate(
        commit.GetAuthor().GetWhen().GetLocalDateTimeString("%m%d-%H%M"),
        "-CP-",
        m_branchDetails->current_branch.GetName()
    );

    // make sure no such branch already exists
    if( m_branchCopies->find(branch_name) != m_branchCopies->cend() )
        return;

    m_branchCopies->try_emplace(branch_name, m_repo.CreateBranch(branch_name, commit));
    m_createdBranchCopyNames.emplace(std::move(branch_name));

    if( m_wndForGitUpdates != nullptr )
        m_wndForGitUpdates->PostMessage(UWM::Stygitan::UpdateUI, CP::Update::BranchCopies);
}


void CodePurifierDoc::DeleteBranchCopies()
{
    StopRefreshDataThread(ThreadStopType::Wait);

    if( m_branchCopies == nullptr || m_branchCopies->empty() )
        return;

    const size_t branches_previously_created = m_branchCopies->size() - m_createdBranchCopyNames.size();

    if( branches_previously_created > 0 )
    {
        const std::string query = FormatText(
            "There %s %d copied branch%s created prior to loading the Code Purifier.\n\n"
            "Do you want to continue deleting the branch copies?",
            PluralizeWord(branches_previously_created, "was", "were"),
            static_cast<int>(branches_previously_created),
            PluralizeWord(branches_previously_created)
        );

        if( AfxMessageBox(query, MB_YESNO | MB_DEFBUTTON1) == IDNO )
            return;
    }

    while( !m_branchCopies->empty() )
    {
        auto name_and_branch = m_branchCopies->begin();
        name_and_branch->second.Delete();
        m_createdBranchCopyNames.erase(name_and_branch->first);
        m_branchCopies->erase(name_and_branch);
    }

    if( m_wndForGitUpdates != nullptr )
        m_wndForGitUpdates->PostMessage(UWM::Stygitan::UpdateUI, CP::Update::BranchCopies);
}


void CodePurifierDoc::SetCleanCommitOverride(const GitCommit& commit)
{
    StopRefreshDataThread(ThreadStopType::Wait);

    m_cleanCommitOverride = commit.GetObjectId();

    StartRefreshDataThread(RefreshStartAction::LocateCleanCommit,
        [&](const CP::RefreshDataChanges& changes)
        {
            // when the clean commit changed but the commits did not, still update them because
            // CodePurifierView's colorization of recent commits depends on the clean commit
            if( m_wndForGitUpdates != nullptr &&
                changes.clean_commit &&
                !changes.recent_commits )
            {
                m_wndForGitUpdates->PostMessage(UWM::Stygitan::UpdateUI, CP::Update::RecentCommits);
            }
        });
}


void CodePurifierDoc::ResetBranchToCleanCommit(const bool create_branch_copy_before_reset)
{
    StopRefreshDataThread(ThreadStopType::Wait);

    if( m_cleanCommit == nullptr )
        throw CSProException("There is no clean commit.");

    if( create_branch_copy_before_reset )
        CreateBranchCopy();

    m_repo.ResetBranchMixed(*m_cleanCommit);

    StartRefreshDataThread(RefreshStartAction::LoadRecentCommits);
}


void CodePurifierDoc::SaveFileFromCleanCommit(const std::string& git_path, const std::string& file_path_for_save)
{
    StopRefreshDataThread(ThreadStopType::Wait);

    if( m_cleanCommit == nullptr )
        throw ProgrammingErrorException();

    const GitTree tree = m_cleanCommit->GetTree();
    const GitTreeEntry tree_entry = tree.GetEntryByPath(git_path);

    const GitObject object = tree_entry.GetObject();
    ASSERT(object.GetType() == GitObjectType::Blob);

    object.DoAsBlob(
        [&](const void* const data, const size_t size)
        {
            FileIO::Write(file_path_for_save, data, size);
        });
}
