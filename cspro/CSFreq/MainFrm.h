#pragma once

#include <CSFreq/FrqOptV.h>
#include <CSFreq/FreqView.h>
#include <zUtilO/BCMenu.h>
#include <zInterfaceF/LangDlgBar.h>


class CMainFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CMainFrame)

protected: 
    CMainFrame(); // create from serialization only

public:
    CReBar& GetReBar()          { return m_wndReBar; }
    LangDlgBar& GetLangDlgBar() { return m_wndLangDlgBar; }

    CSFreqView* GetFreqTreeView();
    CFrqOptionsView* GetFrqOptionsView();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);

    BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;
    void OnUpdateFrameTitle(BOOL bAddToTitle) override;

    LRESULT OnSelectLanguage(WPARAM wParam, LPARAM);
    LRESULT OnGetLexerLanguage(WPARAM wParam, LPARAM lParam);

public:
    BCMenu m_menu;

private:
    CReBar m_wndReBar;
    CStatusBar m_wndStatusBar;
    CToolBar m_wndToolBar;
    CSplitterWnd m_wndSplitter;
    LangDlgBar m_wndLangDlgBar;
};
