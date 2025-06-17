#pragma once
//***************************************************************************
//  File name: WinFocSw.h
//
//  Description:
//       Utility for switching focus between main application windows
//       using keyboard (Alt+F6 and Shift+Alt+F6).
//
// Usage:
// Add a CWindowFocusMgr instance as member var of CWinApp derived class.
// In constructor or InitInstance, add in the appropriate CWindowFocusSwitcher
// derived classes for your applications main windows.
// In the PreTranslateMessage of CWinApp derived class call the PreTranslateMessage
// method of CWindowFocusMgr to check for teh keystroke and switch the focus.
//
// If your main app windows consist only of visible CView derived windows
// you can just use the CViewFocusSwitcher.  If however, your main app windows
// are embedded in a propery sheet or CTabViewContainer you will need to derive
// your own CWindowFocusSwitcher for those windows and an instance of it to the
// CWindowFocusMgr.
//***************************************************************************
#include <zUtilO/zUtilO.h>

class CWindowFocusMgr;

// interface for windows that can have focused switched to them by manager
struct CLASS_DECL_ZUTILO CWindowFocusSwitcher
{
    // given a pointer to main app window (CMainFrame), find
    // all the windows of the appropriate type and them to the CWindowFocusMgr.
    // Can either add each window individually via CWindowFocusMgr::AddWindow
    // or call CWindowFocusMgr::AddWindowsRec to traverse child windows
    // and any window for MatchWindow returns true.
    virtual void FindWindows(CWindowFocusMgr* pMgr, CWnd* pAppMainWnd) = 0;

    // return true if the window passed in should be a candidate for having
    // focus sent to it via keystroke
    virtual bool MatchWindow(CWnd* pWnd) const = 0;

    // set the focus to the window passed in
    // this will be a window for MatchWindow returned true
    // can simply call pWnd->SetFocus() or can do other processing
    // like setting active tab or active view...
    virtual void SetFocus(CWnd* pWnd) = 0;

    // return true if the window can only be a candidate if it is visible
    // would return false for windows in tabs that may be hidden.
    // this is only used by CWindowFocusMgr::AddWindowsRec
    virtual bool MustBeVisible() const = 0;

    virtual ~CWindowFocusSwitcher() {}
};

// manager for handling cycling focus through main application windows
// works off a collection of CWindowFocusSwitcher derived classes to
// first determine all possible candidate windows for focus, which of them
// currently has focus, and which one should get it next.
class CLASS_DECL_ZUTILO CWindowFocusMgr
{
public:

    // destructor
    ~CWindowFocusMgr();

    // add a new switcher.  Switcher will be deleted when manager is deleted.
    void AddSwitcher(CWindowFocusSwitcher* pSwitcher);

    // PreTranslateMessage checks pMsg for keystroke and changes focus appropriately.
    BOOL PreTranslateMessage(CWnd* pMainWnd, MSG* pMsg);

    // add a focus candidate window and associated switcher to the list
    // of candidates.  Usually called from CWindowFocusSwitcher::FindWindows
    void AddWindow(CWnd* pWnd, CWindowFocusSwitcher* pSwitcher);

    // recursively iterate through all children of pStartWnd and add any focus
    // candidate windows found to the list of candidates.  Valid candidates are
    // those for which pSwitcher->MatchWindow returns true.
    // Usually called from CWindowFocusSwitcher::FindWindows
    void AddWindowsRec(CWnd* pStartWnd, CWindowFocusSwitcher* pSwitcher);

protected:
    void OnNextWindow();
    void OnPrevWindow();

    int GetFocusWndIndex() const;

    void SetNewFocusWnd(int i);

    void FindFocusCandidatesRec(CWnd* pWnd, CWindowFocusSwitcher* pSwitcher);

private:

  CArray<CWnd*> m_aFocusWindows;
  CArray<int> m_aWndSwitcherIndices;
  CArray<CWindowFocusSwitcher*> m_aSwitchers;
};

// implementation of CWindowFocusSwitcher for Cview derived classes.
struct CLASS_DECL_ZUTILO CViewFocusSwitcher : public CWindowFocusSwitcher
{
    virtual void FindWindows(CWindowFocusMgr* pMgr, CWnd* pAppMainWnd)
    {
        // find all CView derived windows that are descendants of
        // pAppMainWnd and add them to candidate list.
        CFrameWnd* pMainFrame = DYNAMIC_DOWNCAST(CFrameWnd, pAppMainWnd);
        pMgr->AddWindowsRec(pMainFrame->GetActiveFrame(), this);
    }

    virtual bool MatchWindow(CWnd* pWnd) const
    {
        // match any window derived from CView
        return pWnd->IsKindOf(RUNTIME_CLASS(CView)) != FALSE;
    }

    virtual void SetFocus(CWnd* pWnd)
    {
        ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(CView)));
        CView* pNewFocusView = DYNAMIC_DOWNCAST(CView, pWnd);

        // find first CFrameWnd ancestor for the view (not always a direct parent)
        CWnd* pParent = pWnd->GetParent();
        while (pParent && !pParent->IsKindOf(RUNTIME_CLASS(CFrameWnd))) {
            pParent = pParent->GetParent();
        }

        // make the the active view for the CFrameWnd found.
        if (pParent && pParent->IsKindOf(RUNTIME_CLASS(CFrameWnd))) {
            CFrameWnd* pParentFrame = DYNAMIC_DOWNCAST(CFrameWnd, pParent);
            pParentFrame->SetActiveView(pNewFocusView, FALSE);
        }

        // set focus to the view
        pNewFocusView->SetFocus();
    }

    virtual bool MustBeVisible() const
    {
        return true;
    }
};
