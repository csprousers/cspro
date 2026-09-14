#include "StdAfx.h"
#include "RemovalConfirmationDlg.h"


BEGIN_MESSAGE_MAP(RemovalConfirmationDlg, ResizableDlg)
    ON_COMMAND(IDC_RECYCLE, OnRecycle)
    ON_COMMAND(IDC_DELETE, OnDelete)
END_MESSAGE_MAP()


RemovalConfirmationDlg::RemovalConfirmationDlg(const std::vector<std::string>& paths, CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_REMOVAL_CONFIRMATION, pParent),
        m_pathsText(SO::CreateSingleString(paths, SO::Newline_crlf_sv)),
        m_recycle(true)
{
    SerializeDialogSize("RemovalConfirmationDlg");
}


BOOL RemovalConfirmationDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowsUtf8::SetText(this, IDC_PATHS, m_pathsText);

    return TRUE;
}


void RemovalConfirmationDlg::OnRecycle()
{
    ASSERT(m_recycle);
    OnOK();
}


void RemovalConfirmationDlg::OnDelete()
{
    m_recycle = false;
    OnOK();
}
