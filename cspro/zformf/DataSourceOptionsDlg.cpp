#include "StdAfx.h"
#include "DataSourceOptionsDlg.h"
#include <zAppO/Application.h>


IMPLEMENT_DYNAMIC(CDataSourceOptionsDlg, CDialog)

BEGIN_MESSAGE_MAP(CDataSourceOptionsDlg, CDialog)
    ON_BN_CLICKED(IDC_AUTO_PARTIAL_SAVE,OnCheckboxClicked)
    ON_BN_CLICKED(IDC_ALLOW_DELETE_OTHER_OPERATOR_NOTES,OnCheckboxClicked)
END_MESSAGE_MAP()


CDataSourceOptionsDlg::CDataSourceOptionsDlg(Application* pApplication, CWnd* pParent /*=NULL*/)
    : CDialog(IDD_DATA_SOURCE_OPTIONS_DLG, pParent)
{
    m_iAutomaticPartialSaveMinutes = pApplication->GetAutoPartialSaveMinutes();
    m_iAutomaticPartialSave = pApplication->GetAutoPartialSave() ? BST_CHECKED : BST_UNCHECKED;
    m_csAutomaticPartialSaveMinutes = IntToString(pApplication->GetAutoPartialSave() ?
        pApplication->GetAutoPartialSaveMinutes() : 5); // five minutes by default

    m_iCreateListingFile = pApplication->GetCreateListingFile() ? BST_CHECKED : BST_UNCHECKED;
    m_iCreateLogFile = pApplication->GetCreateLogFile() ? BST_CHECKED : BST_UNCHECKED;

    m_iOtherOperatorNotesDelete = pApplication->GetEditNotePermissions(EditNotePermissions::DeleteOtherOperators) ? BST_CHECKED : BST_UNCHECKED;
    m_iOtherOperatorNotesEdit = pApplication->GetEditNotePermissions(EditNotePermissions::EditOtherOperators) ? BST_CHECKED : BST_UNCHECKED;
}


BOOL CDataSourceOptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    EnableDisableSelections();

    return TRUE;
}


void CDataSourceOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Check(pDX,IDC_AUTO_PARTIAL_SAVE,m_iAutomaticPartialSave);
    DDX_Text(pDX,IDC_AUTO_PARTIAL_SAVE_MINUTES,m_csAutomaticPartialSaveMinutes);

    DDX_Check(pDX,IDC_CREATE_LISTING_FILE,m_iCreateListingFile);
    DDX_Check(pDX,IDC_CREATE_LOG_FILE,m_iCreateLogFile);

    DDX_Check(pDX,IDC_ALLOW_DELETE_OTHER_OPERATOR_NOTES,m_iOtherOperatorNotesDelete);
    DDX_Check(pDX,IDC_ALLOW_EDIT_OTHER_OPERATOR_NOTES,m_iOtherOperatorNotesEdit);
}


void CDataSourceOptionsDlg::EnableDisableSelections()
{
    GetDlgItem(IDC_AUTO_PARTIAL_SAVE_MINUTES)->EnableWindow(m_iAutomaticPartialSave == BST_CHECKED);

    GetDlgItem(IDC_ALLOW_EDIT_OTHER_OPERATOR_NOTES)->EnableWindow(m_iOtherOperatorNotesDelete == BST_CHECKED);
}


void CDataSourceOptionsDlg::OnCheckboxClicked()
{
    UpdateData(TRUE);

    // if an operator can't delete another operator's note, then they can't edit the note
    if( m_iOtherOperatorNotesDelete == BST_UNCHECKED && m_iOtherOperatorNotesEdit == BST_CHECKED )
    {
        m_iOtherOperatorNotesEdit = BST_UNCHECKED;
        UpdateData(FALSE);
    }

    EnableDisableSelections();
}


void CDataSourceOptionsDlg::OnOK()
{
    UpdateData(TRUE);

    if( m_iAutomaticPartialSave == BST_UNCHECKED )
    {
        m_iAutomaticPartialSaveMinutes = -1;
    }

    else
    {
        m_iAutomaticPartialSaveMinutes = _ttoi(m_csAutomaticPartialSaveMinutes);

        if( m_iAutomaticPartialSaveMinutes <= 0 )
        {
            AfxMessageBox(_T("The partial save interval must be a positive number"));
            return;
        }
    }

    CDialog::OnOK();
}
