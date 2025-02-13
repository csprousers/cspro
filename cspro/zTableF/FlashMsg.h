#pragma once

#include <zTableF/zTableF.h>

#define FLASH_TIMER    1

// CFlashMsgDlg dialog

class CLASS_DECL_ZTABLEF CFlashMsgDlg : public CDialog
{
    DECLARE_DYNAMIC(CFlashMsgDlg)

public:
    CFlashMsgDlg(CWnd* pParent = NULL, int iSeconds=2);   // standard constructor
    virtual ~CFlashMsgDlg();

// Dialog Data
    enum { IDD = IDD_FLASHMSG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnTimer(UINT nIDEvent);
    virtual BOOL OnInitDialog();

// Attributes
    CString m_sFlashMsg;
    int m_iSeconds;
};
