#pragma once

class LogicCtrl;


class GoToDlg : public CDialog
{
    DECLARE_DYNAMIC(GoToDlg)

public:
    GoToDlg(LogicCtrl& logic_ctrl, CWnd* pParent = nullptr);

    enum { IDD = IDD_SCINTILLA_GOTO_DLG };

protected:
    void OnOK() override;

public:
    LogicCtrl& m_logicCtrl;
};
