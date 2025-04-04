#pragma once

#include <CSEntry/DEBaseEdit.h>


class CDETextEdit : public CDEBaseEdit
{
    DECLARE_DYNAMIC(CDETextEdit)

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

public:
    BOOL Create(DWORD dwStyle, CRect rect, CWnd* pParent, UINT nID) override;
    bool IsValidChar(UINT nChar) const override;
    void ProcessCharKey(UINT& nChar) override;
    void SetWindowText(const CString& sString) override;
    void GetWindowText(CString& rString) const override;
    void SetSel(int iStart, int iEnd) override { return;/*change this if you want the default behavior*/ }
    void RefreshEditStyles();
};
