#pragma once
#include <CSEntry/DEBaseEdit.h>
// CDETextEdit

class CDETextEdit : public CDEBaseEdit
{
    DECLARE_DYNAMIC(CDETextEdit)

public:
    CDETextEdit();
    virtual ~CDETextEdit();

protected:
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

public:
    virtual BOOL Create(DWORD dwStyle, CRect rect, CWnd* pParent ,UINT  nID);
    virtual bool IsValidChar(UINT nChar) const;
    virtual void ProcessCharKey(UINT& nChar);
    virtual void SetWindowText(const CString& sString);
    virtual void GetWindowText(CString& rString) const;
    virtual void SetSel(int iStart, int iEnd) {return;/*change this if you want the default behavior*/}
    void RefreshEditStyles();
};


