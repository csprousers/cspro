#include "StdAfx.h"
#include "OpenSourceSyncerDlg.h"
#include <zToolsO/UWM.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/UWMRanges.h>
#include <zUtilO/WindowHelpers.h>


BEGIN_MESSAGE_MAP(OpenSourceSyncerDlg, ResizableDlg)
    ON_CBN_SELCHANGE(IDC_TAGS, OnTagChange)
    ON_COMMAND(IDC_CREATE, OnCreate)
    ON_COMMAND(IDC_VALIDATE, OnValidate)
    ON_COMMAND(IDC_GENERATE_FILE_LIST, OnGenerateFileList)
    ON_MESSAGE(UWM::Ranges::ExeStart, OnCreateValidateComplete)
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
    DDX_Control(pDX, IDC_TAGS, m_tagsComboBox);
    DDX_Text(pDX, IDC_COMMIT, m_commit, true);
    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
}


BOOL OpenSourceSyncerDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    try
    {
        m_syncer = std::make_unique<Syncer>(m_settingsDb, m_loggingListBox);

        // populate the tags
        m_tags = m_syncer->GetTags();

        for( const GitTag& tag : m_tags )
            m_tagsComboBox.AddString(TC::ToWide(tag.GetDisplayName()).c_str());

        m_tagsComboBox.AddString(L"Custom");
        m_tagsComboBox.SetCurSel(m_tagsComboBox.GetCount() - 1);
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
    if( !GetDlgItem(IDC_CREATE)->IsWindowEnabled() )
    {
        ErrorMessage::Display(L"You cannot exit while the creation is in progress.");
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


void OpenSourceSyncerDlg::EnableButtons(const bool enable)
{
    for( const int resource_id : { IDC_CREATE, IDC_VALIDATE, IDC_GENERATE_FILE_LIST })
        GetDlgItem(resource_id)->EnableWindow(enable);
}


void OpenSourceSyncerDlg::OnTagChange()
{
    const size_t tag_index = static_cast<size_t>(m_tagsComboBox.GetCurSel());

    if( tag_index < m_tags.size() )
    {
        m_commit = m_tags[tag_index].GetHexHash();
        UpdateData(FALSE);
        GetDlgItem(IDC_COMMIT)->EnableWindow(FALSE);
    }

    else
    {
        GetDlgItem(IDC_COMMIT)->EnableWindow(TRUE);
    }
}


void OpenSourceSyncerDlg::OnCreateValidate(const bool create)
{
    if( !InitializeOperation() )
        return;

    try
    {
        if( SO::IsBlank(m_commit) )
            throw CSProException("Specify a commit.");

        // disable the buttons while the thread is running
        EnableButtons(false);

        m_workerThread = std::make_unique<std::thread>([&, create]() { CreateValidateWorker(create); });
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void OpenSourceSyncerDlg::CreateValidateWorker(const bool create)
{
    try
    {
        create ? m_syncer->CreateRelease(m_commit) :
                 m_syncer->ValidateRelease(m_commit);
    }

    catch( const CSProException& exception )
    {
        m_loggingListBox.AddText("\n\nError: %s", exception.what());
        ErrorMessage::PostMessageForDisplay(exception);
    }

    PostMessage(UWM::Ranges::ExeStart);
}


LRESULT OpenSourceSyncerDlg::OnCreateValidateComplete(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_workerThread != nullptr);

    if( m_workerThread->joinable() )
        m_workerThread->join();

    m_workerThread.reset();

    EnableButtons(true);

    return 1;
}


void OpenSourceSyncerDlg::OnGenerateFileList()
{
    if( !InitializeOperation() )
        return;

    try
    {
        m_syncer->GenerateFileList(m_commit);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


LRESULT OpenSourceSyncerDlg::OnDisplayErrorMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ErrorMessage::DisplayPostedMessages();
    return 0;
}
