#pragma once

#include <zEdit2O/LogicCtrl.h>


class CEdtLogicDlg : public CDialog
{
public:
    CEdtLogicDlg(CWnd* pParent = NULL);   // standard constructor

    enum { IDD = IDD_EDTLOGIC_DLG };

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL PreTranslateMessage(MSG* pMsg) override;

    BOOL OnInitDialog() override;
    void OnOK() override;

public:
    bool m_bIsPostCalc;    // BMD 05 Jun 2006
    CLogicCtrl m_edtLogicCtrl;
    std::string m_logic;
};
