#include "StdAfx.h"
#include "OpenInDataManagerDlg.h"


OpenInDataManagerDlg::OpenInDataManagerDlg(CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_DATA_MANAGER_QUERY, pParent),
        m_openInDataManager(false),
        m_rememberSetting(false)
{
}


BOOL OpenInDataManagerDlg::OnInitDialog()
{
    __super::OnInitDialog();

    static_cast<CButton*>(GetDlgItem(IDC_OPEN_IN_DATA_MANAGER_RADIOBUTTON))->SetCheck(BST_CHECKED);

    return TRUE;
}


void OpenInDataManagerDlg::OnOK()
{
    m_openInDataManager = ( static_cast<CButton*>(GetDlgItem(IDC_OPEN_IN_DATA_MANAGER_RADIOBUTTON))->GetCheck() == BST_CHECKED );
    m_rememberSetting = ( static_cast<CButton*>(GetDlgItem(IDC_REMEMBER_DATA_MANAGER_SETTING_CHECKBOX))->GetCheck() == BST_CHECKED );

    __super::OnOK();
}
