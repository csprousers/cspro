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
    ON_MESSAGE(UWM::OpenSourceSyncer::OperationComplete, OnOperationComplete)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
END_MESSAGE_MAP()


namespace
{
    constexpr std::string_view OpenSourceDirectoryKey_sv = "open-source-directory";
}


OpenSourceSyncerDlg::OpenSourceSyncerDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_SYNCER, pParent),
        m_settingsDb("OpenSourceSyncer.db"),
        m_openSourceDirectory(m_settingsDb.ReadOrDefault<std::string>(OpenSourceDirectoryKey_sv))
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


bool OpenSourceSyncerDlg::InitializeOperation() noexcept
{
    UpdateData(TRUE);

    m_settingsDb.Write<std::string>(OpenSourceDirectoryKey_sv, m_openSourceDirectory);

    m_loggingListBox.Clear();

    try
    {
        if( m_openSourceDirectory.empty() )
            throw CSProException("Specify the open source directory.");

        m_syncer->SetOpenSourceDirectory(m_openSourceDirectory);

        return true;
    }

    catch( const CSProException& exception )
    {
        m_loggingListBox.AddText("\n\nError: %s", exception.what());
        ErrorMessage::Display(exception);
        return false;
    }
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
