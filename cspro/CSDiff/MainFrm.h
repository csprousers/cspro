#pragma once

#include <zUtilO/BCMenu.h>


class CMainFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CMainFrame)

protected: 
    CMainFrame() { } // create from serialization only

public:
    BCMenu& GetMenu() { return m_menu; }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreCreateWindow(CREATESTRUCT& cs) override;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);

private:
    BCMenu m_menu;
    CStatusBar m_wndStatusBar;
    CToolBar m_wndToolBar;
};
