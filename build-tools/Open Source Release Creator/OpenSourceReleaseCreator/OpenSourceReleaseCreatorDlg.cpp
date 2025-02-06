#include "StdAfx.h"
#include "OpenSourceReleaseCreatorDlg.h"
#include <zUtilO/DataExchange.h>
#include <zUtilO/UWMRanges.h>
#include <zUtilO/WindowHelpers.h>


BEGIN_MESSAGE_MAP(OpenSourceReleaseCreatorDlg, ResizableDlg)
    ON_CBN_SELCHANGE(IDC_TAGS, OnTagChange)
    ON_COMMAND(IDC_CREATE, OnCreate)
    ON_MESSAGE(UWM::Ranges::ExeStart, OnCreateComplete)
END_MESSAGE_MAP()


OpenSourceReleaseCreatorDlg::OpenSourceReleaseCreatorDlg(CWnd* const pParent /*=nullptr*/)
    :   ResizableDlg(IDD_CREATOR, pParent),
        m_settingsDb("OpenSourceReleaseCreator.db"),
        m_outputDirectory(m_settingsDb.ReadOrDefault<std::string>(OutputDirectoryKey_sv))
{
    SerializeDialogSize("OpenSourceReleaseCreatorDlg");
}


OpenSourceReleaseCreatorDlg::~OpenSourceReleaseCreatorDlg()
{
}


void OpenSourceReleaseCreatorDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_TAGS, m_tagsComboBox);
    DDX_Text(pDX, IDC_COMMIT, m_commit);
    DDX_Text(pDX, IDC_OUTPUT_DIRECTORY, m_outputDirectory);
    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
}


BOOL OpenSourceReleaseCreatorDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    try
    {
        m_creator = std::make_unique<Creator>();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        PostQuitMessage(0);
    }

    // populate the tags
    m_tags = m_creator->GetTags();

    for( const Git::Tag& tag : m_tags )
    {
        constexpr std::string_view TagPrefix_sv = "refs/tags/";

        std::string_view tag_name_sv = tag.name;

        if( SO::StartsWith(tag_name_sv, TagPrefix_sv) )
            tag_name_sv.remove_prefix(TagPrefix_sv.length());

        m_tagsComboBox.AddString(TC::ToWide(tag_name_sv).c_str());
    }

    m_tagsComboBox.AddString(L"Custom");
    m_tagsComboBox.SetCurSel(m_tagsComboBox.GetCount() - 1);

    return TRUE;
}


void OpenSourceReleaseCreatorDlg::OnCancel()
{
    if( !GetDlgItem(IDC_CREATE)->IsWindowEnabled() )
    {
        ErrorMessage::Display(L"You cannot exit while the creation is in progress.");
        return;
    }

    __super::OnCancel();
}


void OpenSourceReleaseCreatorDlg::OnTagChange()
{
    const size_t tag_index = static_cast<size_t>(m_tagsComboBox.GetCurSel());

    if( tag_index < m_tags.size() )
    {
        m_commit = m_tags[tag_index].id;
        UpdateData(FALSE);
        GetDlgItem(IDC_COMMIT)->EnableWindow(FALSE);
    }

    else
    {
        GetDlgItem(IDC_COMMIT)->EnableWindow(TRUE);
    }
}


void OpenSourceReleaseCreatorDlg::OnCreate()
{
    UpdateData(TRUE);

    try
    {
        if( SO::IsBlank(m_commit) )
            throw CSProException("Specify a commit.");

        if( !PortableFunctions::FileIsDirectory(m_outputDirectory) )
            throw CSProException("Specify a valid output directory.");

        m_settingsDb.Write<std::string>(OutputDirectoryKey_sv, m_outputDirectory);

        // disable the Create button while the thread is running
        GetDlgItem(IDC_CREATE)->EnableWindow(FALSE);

        m_createThread = std::make_unique<std::thread>([&]() { CreateWorker(); });
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void OpenSourceReleaseCreatorDlg::CreateWorker()
{
    try
    {
        m_creator->CreateRelease(m_loggingListBox, m_commit, m_outputDirectory);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    PostMessage(UWM::Ranges::ExeStart);
}


LRESULT OpenSourceReleaseCreatorDlg::OnCreateComplete(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_createThread != nullptr);

    if( m_createThread->joinable() )
        m_createThread->join();

    m_createThread.reset();

    GetDlgItem(IDC_CREATE)->EnableWindow(TRUE);

    return 1;
}
