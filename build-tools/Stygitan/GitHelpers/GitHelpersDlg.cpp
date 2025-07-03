#include "StdAfx.h"
#include "GitHelpersDlg.h"


BEGIN_MESSAGE_MAP(GitHelpersDlg, CDialog)
END_MESSAGE_MAP()


GitHelpersDlg::GitHelpersDlg(CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_GIT_HELPERS, pParent)
{
}


GitHelpersDlg::~GitHelpersDlg()
{
}


void GitHelpersDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);
}
