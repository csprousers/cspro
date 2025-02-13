#include "StdAfx.h"
#include "VSLabelRenamerDlg.h"


VSLabelRenamerDlg::VSLabelRenamerDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_VSET_LABEL_RENAMER, pParent),
        m_template(_T("%s"))
{
}


void VSLabelRenamerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_VSET_LABEL_TEMPLATE, m_template);
}
