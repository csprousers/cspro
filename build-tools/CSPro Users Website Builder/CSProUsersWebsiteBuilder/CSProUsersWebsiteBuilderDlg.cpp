#include "StdAfx.h"
#include "CSProUsersWebsiteBuilderDlg.h"
#include <zUtilO/WindowHelpers.h>


BEGIN_MESSAGE_MAP(CSProUsersWebsiteBuilderDlg, ResizableDlg)
END_MESSAGE_MAP()


CSProUsersWebsiteBuilderDlg::CSProUsersWebsiteBuilderDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_BUILDER, pParent)
{
    SerializeDialogSize("CSProUsersWebsiteBuilderDlg");
}


void CSProUsersWebsiteBuilderDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
}


BOOL CSProUsersWebsiteBuilderDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    return TRUE;
}
