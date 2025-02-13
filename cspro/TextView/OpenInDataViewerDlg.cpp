#include "StdAfx.h"
#include "OpenInDataViewerDlg.h"

IMPLEMENT_DYNAMIC(COpenInDataViewerDlg, CDialog)

COpenInDataViewerDlg::COpenInDataViewerDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_DATA_VIEWER_QUERY, pParent),
        m_OpenInDataViewer(false),
        m_bRememberSetting(false)
{
}

BOOL COpenInDataViewerDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_pDataViewerRadioButton = (CButton*)GetDlgItem(IDC_OPEN_IN_DATA_VIEWER_RADIOBUTTON);
    m_pDataViewerRadioButton->SetCheck(BST_CHECKED);

    return TRUE;
}

void COpenInDataViewerDlg::OnOK()
{
    m_OpenInDataViewer = ( m_pDataViewerRadioButton->GetCheck() == BST_CHECKED );
    m_bRememberSetting = ( ((CButton*)GetDlgItem(IDC_REMEMBER_DATA_VIEWER_SETTING_CHECKBOX))->GetCheck() == BST_CHECKED );

    CDialog::OnOK();
}
