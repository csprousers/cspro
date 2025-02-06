#include "StdAfx.h"
#include "OpenSourceReleaseCreatorDlg.h"
#include <zUtilO/DataExchange.h>
#include <zUtilO/WindowHelpers.h>


BEGIN_MESSAGE_MAP(OpenSourceReleaseCreatorDlg, ResizableDlg)
    ON_CBN_SELCHANGE(IDC_TAGS, OnTagChange)
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


void OpenSourceReleaseCreatorDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( SO::IsBlank(m_commit) )
            throw CSProException("Specify a commit.");

        if( !PortableFunctions::FileIsDirectory(m_outputDirectory) )
            throw CSProException("Specify a valid output directory.");

        m_creator->CreateRelease(m_loggingListBox, m_commit, m_outputDirectory);

        m_settingsDb.Write<std::string>(OutputDirectoryKey_sv, m_outputDirectory);

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
