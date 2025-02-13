#pragma once

#include <zEdit2O/LogicCtrl.h>

// CEdtLogicDlg dialog
class CEdtLogicDlg : public CDialog
{
    DECLARE_DYNAMIC(CEdtLogicDlg)

public:
    CEdtLogicDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CEdtLogicDlg();
// Dialog Data
    enum { IDD = IDD_EDTLOGIC_DLG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    BOOL PreTranslateMessage(MSG* pMsg);

    DECLARE_MESSAGE_MAP()
public:
    bool m_bIsPostCalc;    // BMD 05 Jun 2006
    CLogicCtrl m_edtLogicCtrl;
    CIMSAString m_sLogic;
    virtual BOOL OnInitDialog();
    virtual void OnOK();
};
