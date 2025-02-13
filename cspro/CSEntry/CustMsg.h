#pragma once

// CustMsg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCustMsg window
#include <CSEntry/DEEdit.h>
#include <CSEntry/MessageOverrides.h>
#include <zUtilF/MsgOpt.h>

enum eMsgType{None_Msg = -1,OutOfRange, ISSA, OutOfSeq, VerifyMsg};
class CCustMsg : public CWnd
{
// Construction
public:
    //CCustMsg();
    CCustMsg(const MessageOverrides& message_overrides = MessageOverrides(), const CMsgOptions* pMsgOptions = nullptr);

// Attributes
public:
    eMsgType m_eMsgType;
    CIMSAString m_sMessage;
    bool        m_bCanForceOutOfRange;
    CDEBaseEdit*    m_pEdit;
private:
    COLORREF    m_cColorFg;
    CFont       m_ErrorFont1;
    CFont       m_ErrorFont2;

    CRect       m_rectErr;
    CRect       m_rectLine;
    CRect       m_rectInst;
    CRect       m_rectMsg;
    CRect       m_rectWnd;
    int         m_iBorder;
    int         m_iLine;

    CIMSAString m_sMsgText;
    CIMSAString m_sMsgNum;
    UINT        m_uRetVal;

    const MessageOverrides& m_messageOverrides;
    const CMsgOptions* m_pMsgOptions;

// Operations
public:
    UINT GetRetVal() { return m_uRetVal;}

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCustMsg)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

// Implementation
public:
    void ShowMessage();

    void CalcRMsgDims();
    void DrawRMsg(CDC* pDC);
    void CalcGMsgDims();
    void DrawGMsg(CDC* pDC);
    void CalcSMsgDims();
    void DrawSMsg(CDC* pDC);
    void CalcVMsgDims();
    void DrawVMsg(CDC* pDC);

    int MaxErrWidth();
    CPoint CalcPos(CRect rectWnd);
    virtual ~CCustMsg();

    // Generated message map functions
protected:
    //{{AFX_MSG(CCustMsg)
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
