#include "StdAfx.h"
#include "OpenSourceSyncerDlg.h"
#include "Syncer.h"


BEGIN_MESSAGE_MAP(OpenSourceSyncerDlg, ResizableDlg)
    ON_COMMAND(IDC_SYNC, OnSync)
    ON_COMMAND(IDC_COMPARE, OnCompare)
    ON_COMMAND(IDC_MIRROR_SINGLE_COMMIT, OnManualMirrorSingleCommit)
    ON_COMMAND(IDC_MIRROR_MERGE_COMMIT, OnManualMirrorMergeCommit)
    ON_MESSAGE(UWM::OpenSourceSyncer::OperationComplete, OnOperationComplete)
END_MESSAGE_MAP()


namespace
{
    constexpr std::string_view BranchNameKey_sv = "branch-name";
}


OpenSourceSyncerDlg::OpenSourceSyncerDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_SYNCER, pParent),
        m_controller(Controller::GetInstance()),
        m_branchName(m_controller.GetSettingsDb().ReadOrDefault<std::string>(BranchNameKey_sv))
{
    SerializeDialogSize("OpenSourceSyncerDlg");
}


OpenSourceSyncerDlg::~OpenSourceSyncerDlg()
{
}


void OpenSourceSyncerDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_BRANCH_NAME, m_branchName, true);
    DDX_Text(pDX, IDC_COMMIT_OLD, m_commitOld, true);
    DDX_Text(pDX, IDC_COMMIT_NEW, m_commitNew, true);

    DDX_Text(pDX, IDC_MANUAL_BRANCH_NAME, m_manualBranchName, true);
    DDX_Text(pDX, IDC_MANUAL_SINGLE_COMMIT, m_manualSingleCommit, true);

    DDX_Text(pDX, IDC_MANUAL_MERGE_COMMIT, m_manualMergeCommit, true);
    DDX_Text(pDX, IDC_MANUAL_PARENT_COMMIT1, m_manualParentCommit1, true);
    DDX_Text(pDX, IDC_MANUAL_PARENT_COMMIT2, m_manualParentCommit2, true);

    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
}


BOOL OpenSourceSyncerDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    try
    {
        m_syncer = std::make_unique<Syncer>(m_loggingListBox);
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

    m_controller.GetSettingsDb().Write<std::string>(BranchNameKey_sv, m_branchName);

    m_loggingListBox.Clear();

    try
    {
        // makes sure that the open source repository is open
        m_controller.GetOpenSourceRepo();

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

    sync_data.os_merge_branch = m_controller.GetOpenSourceRepo().LookupBranch(m_branchName);

    if( using_oldest_merge_commit )
    {
        if( m_commitOld.empty() )
            throw CSProException("Specify the feature branch oldest merge commit.");

        sync_data.cs_oldest_merge_commit = m_controller.GetPrivateRepo().LookupCommit(m_commitOld);
    }

    if( m_commitNew.empty() )
        throw CSProException("Specify the feature branch newest merge commit.");

    sync_data.cs_newest_merge_commit = m_controller.GetPrivateRepo().LookupCommit(m_commitNew);
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
        [sync_data, controller = &m_controller]()
        {
            sync_data->syncer->CompareRepositories(
                *sync_data->cs_newest_merge_commit,
                controller->GetOpenSourceRepo().LookupCommit(*sync_data->os_merge_branch),
                true
            );
        }
    );
}


void OpenSourceSyncerDlg::OnManualMirrorSingleCommit()
{
    struct Data
    {
        Syncer* syncer = nullptr;
        std::optional<GitBranch> os_branch;
        std::optional<GitCommit> cs_commit;
    };

    auto data = std::make_shared<Data>();

    RunOperation(
        // validation
        [&, data]()
        {
            data->syncer = m_syncer.get();
            ASSERT(data->syncer != nullptr);

            if( m_manualBranchName.empty() )
                throw CSProException("Specify the manual mirroring branch name.");

            try
            {
                data->os_branch = m_controller.GetOpenSourceRepo().LookupBranch(m_manualBranchName);
            }

            catch(...)
            {
                // try creating the branch if it does not exist
                const GitBranch current_branch = m_controller.GetOpenSourceRepo().GetCurrentBranch();

                data->os_branch = m_controller.GetOpenSourceRepo().CreateBranch(
                    m_manualBranchName,
                    m_controller.GetOpenSourceRepo().LookupCommit(current_branch)
                );
            }

            if( m_manualSingleCommit.empty() )
                throw CSProException("Specify the manual mirroring commit.");

            data->cs_commit = m_controller.GetPrivateRepo().LookupCommit(m_manualSingleCommit);
        },
        // operation
        [data]()
        {
            data->syncer->ManualMirrorSingleCommit(*data->os_branch, *data->cs_commit);
        }
    );
}


void OpenSourceSyncerDlg::OnManualMirrorMergeCommit()
{
    struct Data
    {
        Syncer* syncer = nullptr;
        std::optional<GitBranch> os_merge_branch;
        std::optional<GitCommit> cs_merge_commit;
        std::optional<GitCommit> os_parent_commit1;
        std::optional<GitCommit> os_parent_commit2;
    };

    auto data = std::make_shared<Data>();

    RunOperation(
        // validation
        [&, data]()
        {
            data->syncer = m_syncer.get();
            ASSERT(data->syncer != nullptr);

            if( m_branchName.empty() )
                throw CSProException("Specify the open source branch target (in the feature branch section).");

            data->os_merge_branch = m_controller.GetOpenSourceRepo().LookupBranch(m_branchName);

            if( m_manualMergeCommit.empty() )
                throw CSProException("Specify the manual merge commit.");

            data->cs_merge_commit = m_controller.GetPrivateRepo().LookupCommit(m_manualMergeCommit);

            if( m_manualParentCommit1.empty() ||
                m_manualParentCommit2.empty() )
            {
                throw CSProException("Specify the two parent commits.");
            }

            data->os_parent_commit1 = m_controller.GetOpenSourceRepo().LookupCommit(m_manualParentCommit1);
            data->os_parent_commit2 = m_controller.GetOpenSourceRepo().LookupCommit(m_manualParentCommit2);
        },
        // operation
        [data]()
        {
            data->syncer->ManualMirrorMergeCommit(*data->os_merge_branch, *data->cs_merge_commit,
                                                  *data->os_parent_commit1, *data->os_parent_commit2);
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
