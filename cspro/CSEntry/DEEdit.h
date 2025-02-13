#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDEEdit window

#include <CSEntry/DEBaseEdit.h>


class CDEEdit : public CDEBaseEdit
{
    DECLARE_DYNAMIC (CDEEdit)

private:
    int             m_iArabicCaretChar; // For arabic, not all chars have same width
                                        // need to keep track of which csprochar the caret is
                                        // is on

    // Construction
public:
    CDEEdit();


    // Attributes
public:
    // Operations
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDEEdit)
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL


    // Implementation
public:
    virtual ~CDEEdit();
    void GetUnits(CSize& size)const;
    void CalcCaretPos(CPoint& point)const;
    void SetCaret();
    void SetCaretSize(int sizeX);
    bool ComputeRect(CRect& clientRect);
    bool IsValidChar(UINT nChar) const;
    void ProcessCharKey(UINT& nChar);


    //TEMP FUNCTIONS
    virtual BOOL Create(DWORD dwStyle, CRect rect, CWnd* pParent ,UINT  nID);

    virtual void SetSel(int iStart, int iEnd) { return; }
    virtual void LimitText(int nChars) { return; }

    void MoveCaretPos( int iChar );
    void SetWindowText(const CString& sString);

    void AdvanceCaretPos();
    void BackupCaretPos();
    int GetCharFromCaretPos()const;
    CPoint GetCaretPosFromChar(int iChar, int& iCaretSizeX) const;
    int GetCaretPosXForChar(int iChar, int& iCaretSizeX) const;
    void PadBlanksToDecimal(CString& sString);

    CString GetDecimalPart(const CString& sString)const;
    CString GetNonDecimalPart(const CString& sString)const;
    bool IsPosBeforeDecimal(void)const;
    CString MakeDecNumString(const CString& sNonDecimal, const CString & sDecimal)const;

    // RHF INIT Jul 05, 2007
    virtual void SetModifiedFlag(bool bFlag) {
        m_bModified = bFlag;
        if( bFlag == false ) {
            SetHasDecimalPoint(false);
        }
    }

    // Generated message map functions
protected:
    //{{AFX_MSG(CDEEdit)
    afx_msg void OnPaint();
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

private:
    bool m_bHasDecimalPoint;

public:
    void SetHasDecimalPoint( bool bHasDecimalPoint );
    void SetNewCaretPos( CPoint& cPoint );
    // RHF END Jul 05, 2007

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
