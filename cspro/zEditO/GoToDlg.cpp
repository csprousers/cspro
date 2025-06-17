#include "stdafx.h"
#include "GoToDlg.h"


IMPLEMENT_DYNAMIC(GoToDlg, CDialog)


GoToDlg::GoToDlg(CLogicCtrl& logic_ctrl, CWnd* pParent/* = nullptr*/)
    :   CDialog(GoToDlg::IDD, pParent),
        m_logicCtrl(logic_ctrl)
{
}


void GoToDlg::OnOK()
{
    CString line_text;
    GetDlgItemText(IDC_GOTO_LINE, line_text);
    int line_number = _ttoi(line_text) - 1;

    CDialog::OnOK();

    m_logicCtrl.GotoLine(line_number);
    m_logicCtrl.SetFocus();
}
