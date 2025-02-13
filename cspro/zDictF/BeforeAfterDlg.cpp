#include "StdAfx.h"
#include "BeforeAfterDlg.h"


BEGIN_MESSAGE_MAP(BeforeAfterDlg, CDialog)
    ON_BN_CLICKED(IDC_AFTER, OnAfter)
    ON_BN_CLICKED(IDC_BEFORE, OnBefore)
END_MESSAGE_MAP()


BeforeAfterDlg::BeforeAfterDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_BEFORE_AFTER_DLG, pParent),
        m_after(true)
{
}


void BeforeAfterDlg::OnAfter()
{
    m_after = true;
    OnOK();
}

void BeforeAfterDlg::OnBefore()
{
    m_after = false;
    OnOK();
}
