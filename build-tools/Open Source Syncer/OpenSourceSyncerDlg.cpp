#include "StdAfx.h"
#include "OpenSourceSyncerDlg.h"
#include <zToolsO/UWM.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/WindowHelpers.h>


namespace UWM::OpenSourceSyncer
{
    constexpr unsigned OperationComplete = UWM::Ranges::ExeStart;
}


BEGIN_MESSAGE_MAP(OpenSourceSyncerDlg, ResizableDlg)
    ON_COMMAND(IDC_SYNC, OnSync)
    ON_MESSAGE(UWM::OpenSourceSyncer::OperationComplete, OnOperationComplete)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
END_MESSAGE_MAP()


namespace
{
    constexpr std::string_view OpenSourceDirectoryKey_sv = "open-source-directory";
    constexpr std::string_view BranchNameKey_sv          = "branch-name";
}


OpenSourceSyncerDlg::OpenSourceSyncerDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_SYNCER, pParent),
        m_settingsDb("OpenSourceSyncer.db"),
        m_openSourceDirectory(m_settingsDb.ReadOrDefault<std::string>(OpenSourceDirectoryKey_sv)),
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

    DDX_Text(pDX, IDC_OPEN_SOURCE_DIRECTORY, m_openSourceDirectory, true);
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

    m_settingsDb.Write<std::string>(OpenSourceDirectoryKey_sv, m_openSourceDirectory);
    m_settingsDb.Write<std::string>(BranchNameKey_sv, m_branchName);

    m_loggingListBox.Clear();

    try
    {
        if( m_openSourceDirectory.empty() )
            throw CSProException("Specify the open source directory.");

        m_syncer->SetOpenSourceDirectory(m_openSourceDirectory);

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


void OpenSourceSyncerDlg::OnSync()
{
    struct Data
    {
        Syncer* syncer = nullptr;
        std::optional<GitBranch> os_merge_branch;
        std::optional<GitCommit> cs_oldest_merge_commit;
        std::optional<GitCommit> cs_newest_merge_commit;
    };

    auto data = std::make_shared<Data>();

    RunOperation(
        // validation
        [&, data]()
        {
            data->syncer = m_syncer.get();
            ASSERT(data->syncer != nullptr);

            if( m_branchName.empty() )
                throw CSProException("Specify the open source branch target.");

            data->os_merge_branch = data->syncer->GetOpenSourceRepo().LookupBranch(m_branchName);

            if( m_commitOld.empty() || m_commitNew.empty() )
                throw CSProException("Specify the feature branch old and new merge commits.");

            data->cs_oldest_merge_commit = data->syncer->GetPrivateRepo().LookupCommit(m_commitOld);
            data->cs_newest_merge_commit = data->syncer->GetPrivateRepo().LookupCommit(m_commitNew);
        },

        // operation
        [data]()
        {
            data->syncer->MirrorFeatureBranches(
                *data->os_merge_branch,
                *data->cs_oldest_merge_commit,
                *data->cs_newest_merge_commit
            );
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
