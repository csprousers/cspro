#pragma once

class CLogicCtrl;


class GoToDlg : public CDialog
{
    DECLARE_DYNAMIC(GoToDlg)

public:
    GoToDlg(CLogicCtrl& logic_ctrl, CWnd* pParent = nullptr);

    enum { IDD = IDD_SCINTILLA_GOTO_DLG };

protected:
    void OnOK() override;

public:
    CLogicCtrl& m_logicCtrl;
};
