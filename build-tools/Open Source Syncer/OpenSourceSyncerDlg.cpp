#include "StdAfx.h"
#include "OpenSourceSyncerDlg.h"
#include "Syncer.h"
#include <zToolsO/UWM.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/WindowHelpers.h>


namespace UWM::OpenSourceSyncer
{
    constexpr unsigned OperationComplete = UWM::Ranges::ExeStart;
}


BEGIN_MESSAGE_MAP(OpenSourceSyncerDlg, ResizableDlg)
    ON_COMMAND(IDC_SYNC, OnSync)
    ON_COMMAND(IDC_COMPARE, OnCompare)
    ON_COMMAND(IDC_REFRESH_LIBRARY_TAGS, OnRefreshLibraryTags)
    ON_COMMAND(IDC_COMMIT_LIBRARY, OnCommitLibrary)
    ON_MESSAGE(UWM::OpenSourceSyncer::OperationComplete, OnOperationComplete)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
END_MESSAGE_MAP()


namespace
{
    constexpr std::string_view OpenSourceCodeDirectoryKey_sv      = "open-source-code-directory";
    constexpr std::string_view OpenSourceLibrariesDirectoryKey_sv = "open-source-libraries-directory";
    constexpr std::string_view BranchNameKey_sv                   = "branch-name";
}


OpenSourceSyncerDlg::OpenSourceSyncerDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_SYNCER, pParent),
        m_settingsDb("OpenSourceSyncer.db"),
        m_openSourceCodeDirectory(m_settingsDb.ReadOrDefault<std::string>(OpenSourceCodeDirectoryKey_sv)),
        m_openSourceLibrariesDirectory(m_settingsDb.ReadOrDefault<std::string>(OpenSourceLibrariesDirectoryKey_sv)),
        m_branchName(m_settingsDb.ReadOrDefault<std::string>(BranchNameKey_sv))
{
    SerializeDialogSize("OpenSourceSyncerDlg");
}


OpenSourceSyncerDlg::~OpenSourceSyncerDlg()
{
}


void OpenSourceSyncerDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_OPEN_SOURCE_CODE_DIRECTORY, m_openSourceCodeDirectory, true);
    DDX_Text(pDX, IDC_OPEN_SOURCE_LIBRARIES_DIRECTORY, m_openSourceLibrariesDirectory, true);
    DDX_Text(pDX, IDC_BRANCH_NAME, m_branchName, true);
    DDX_Text(pDX, IDC_COMMIT_OLD, m_commitOld, true);
    DDX_Text(pDX, IDC_COMMIT_NEW, m_commitNew, true);
    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
}


BOOL OpenSourceSyncerDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    try
    {
        m_syncer = std::make_unique<Syncer>(m_settingsDb, m_loggingListBox);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        PostQuitMessage(0);
    }

    return TRUE;
}


void OpenSourceSyncerDlg::OnCancel()
{
    if( m_workerThread != nullptr )
    {
        ErrorMessage::Display(L"You cannot exit while an operation is in progress.");
        return;
    }

    __super::OnCancel();
}


void OpenSourceSyncerDlg::RunOperation(const std::function<void()>& validate_inputs_callback,
                                       std::function<void()> operation_callback) noexcept
{
    ASSERT(operation_callback);

    if( m_workerThread != nullptr )
    {
        ErrorMessage::Display(L"An operation is currently in progress.");
        return;
    }

    UpdateData(TRUE);

    m_settingsDb.Write<std::string>(OpenSourceCodeDirectoryKey_sv, m_openSourceCodeDirectory);
    m_settingsDb.Write<std::string>(OpenSourceLibrariesDirectoryKey_sv, m_openSourceLibrariesDirectory);
    m_settingsDb.Write<std::string>(BranchNameKey_sv, m_branchName);

    m_loggingListBox.Clear();

    try
    {
        if( m_openSourceCodeDirectory.empty() )
            throw CSProException("Specify the open source code directory.");

        m_syncer->SetOpenSourceDirectory(m_openSourceCodeDirectory);

        if( validate_inputs_callback )
            validate_inputs_callback();
    }

    catch( const CSProException& exception )
    {
        m_loggingListBox.AddText("\n\nError: %s", exception.what());
        ErrorMessage::Display(exception);
        return;
    }

    m_workerThread = std::make_unique<std::thread>(
        [this, operation_callback_ = std::move(operation_callback)]()
        {
            try
            {
                operation_callback_();
            }

            catch( const CSProException& exception )
            {
                m_loggingListBox.AddText("\n\nError: %s", exception.what());
                ErrorMessage::PostMessageForDisplay(exception);
            }

            PostMessage(UWM::OpenSourceSyncer::OperationComplete);
        });
}


struct OpenSourceSyncerDlg::SyncData
{
    Syncer* syncer = nullptr;
    std::optional<GitBranch> os_merge_branch;
    std::optional<GitCommit> cs_oldest_merge_commit;
    std::optional<GitCommit> cs_newest_merge_commit;
};


void OpenSourceSyncerDlg::ValidateSyncData(SyncData& sync_data, const bool using_oldest_merge_commit)
{
    sync_data.syncer = m_syncer.get();
    ASSERT(sync_data.syncer != nullptr);

    if( m_branchName.empty() )
        throw CSProException("Specify the open source branch target.");

    sync_data.os_merge_branch = sync_data.syncer->GetOpenSourceRepo().LookupBranch(m_branchName);

    if( using_oldest_merge_commit )
    {
        if( m_commitOld.empty() )
            throw CSProException("Specify the feature branch oldest merge commit.");

        sync_data.cs_oldest_merge_commit = sync_data.syncer->GetPrivateRepo().LookupCommit(m_commitOld);
    }

    if( m_commitNew.empty() )
        throw CSProException("Specify the feature branch newest merge commit.");

    sync_data.cs_newest_merge_commit = sync_data.syncer->GetPrivateRepo().LookupCommit(m_commitNew);
}


void OpenSourceSyncerDlg::OnSync()
{
    auto sync_data = std::make_shared<SyncData>();

    RunOperation(
        // validation
        [&, sync_data]()
        {
            ValidateSyncData(*sync_data, true);
        },
        // operation
        [sync_data]()
        {
            sync_data->syncer->MirrorFeatureBranches(
                *sync_data->os_merge_branch,
                *sync_data->cs_oldest_merge_commit,
                *sync_data->cs_newest_merge_commit
            );
        }
    );
}


void OpenSourceSyncerDlg::OnCompare()
{
    auto sync_data = std::make_shared<SyncData>();

    RunOperation(
        // validation
        [&, sync_data]()
        {
            ValidateSyncData(*sync_data, false);
        },
        // operation
        [sync_data]()
        {
            sync_data->syncer->CompareRepositories(
                *sync_data->cs_newest_merge_commit,
                sync_data->syncer->GetOpenSourceRepo().LookupCommit(*sync_data->os_merge_branch),
                true
            );
        }
    );
}


struct OpenSourceSyncerDlg::LibraryData
{
    Syncer* syncer = nullptr;
    GitRepository repo;
    std::optional<GitCommit> cs_commit;
};


void OpenSourceSyncerDlg::ValidateLibraryData(LibraryData& library_data, const bool creating_commit)
{
    library_data.syncer = m_syncer.get();
    ASSERT(library_data.syncer != nullptr);

    if( m_openSourceLibrariesDirectory.empty() )
        throw CSProException("Specify the open source libraries directory.");

    if( creating_commit )
    {
        if( m_commitNew.empty() )
            throw CSProException("Specify the commit (as the feature branch newest merge commit).");

        library_data.cs_commit = library_data.syncer->GetPrivateRepo().LookupCommit(m_commitNew);
    }

    const bool bare = !creating_commit;
    library_data.repo.Open(Path::Combine(m_openSourceLibrariesDirectory, ".git"), bare);
}


void OpenSourceSyncerDlg::OnRefreshLibraryTags()
{
    auto library_data = std::make_shared<LibraryData>();

    RunOperation(
        // validation
        [&, library_data]()
        {
            ValidateLibraryData(*library_data, false);
        },
        // operation
        [library_data]()
        {
            library_data->syncer->RefreshLibraryTags(library_data->repo);
        }
    );
}


void OpenSourceSyncerDlg::OnCommitLibrary()
{
    auto library_data = std::make_shared<LibraryData>();

    RunOperation(
        // validation
        [&, library_data]()
        {
            ValidateLibraryData(*library_data, true);
        },
        // operation
        [library_data]()
        {
            library_data->syncer->CreateLibraryCommit(library_data->repo, *library_data->cs_commit);
        }
    );
}


LRESULT OpenSourceSyncerDlg::OnOperationComplete(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_workerThread != nullptr);

    if( m_workerThread->joinable() )
        m_workerThread->join();

    m_workerThread.reset();

    return 1;
}


LRESULT OpenSourceSyncerDlg::OnDisplayErrorMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ErrorMessage::DisplayPostedMessages();
    return 0;
}
