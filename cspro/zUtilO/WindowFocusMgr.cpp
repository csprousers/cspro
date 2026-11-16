//***************************************************************************
//  File name: WindowFocusMgr.h
//
//  Description:
//       Utility for switching focus between main application windows
//       using keyboard (Alt+F6 and Shift+Alt+F6).
//
//***************************************************************************

#include "StdAfx.h"
#include "WindowFocusMgr.h"
#include <zUtilO/ArrUtil.h>
#include <afxtabctrl.h>


// --------------------------------------------------------------------------
// CWindowFocusMgr
// --------------------------------------------------------------------------

CWindowFocusMgr::~CWindowFocusMgr()
{
    // delete all the switchers
    for (int i = 0; i < m_aSwitchers.GetCount(); ++i) {
        delete m_aSwitchers.GetAt(i);
    }
}

// PreTranslateMessage checks pMsg for keystroke and changes focus appropriately.
BOOL CWindowFocusMgr::PreTranslateMessage(CWnd* pMainWnd, MSG* pMsg)
{
    // check for F6 with Alt
    if(pMsg->message == WM_SYSKEYDOWN &&
       pMsg->wParam == VK_F6)
    {
        // collect all the candidate windows from the switchers
        for (int i = 0; i < m_aSwitchers.GetCount(); ++i) {
            m_aSwitchers.GetAt(i)->FindWindows(this, pMainWnd);
        }

        if (GetKeyState(VK_SHIFT) & 0xFF00) {
            // shift key is down - go backwards
            OnPrevWindow();
        }
        else {
            // no shift - go forwards
            OnNextWindow();
        }

        // clean out what we found to be ready for next time
        m_aFocusWindows.RemoveAll();
        m_aWndSwitcherIndices.RemoveAll();

        return TRUE;
    }

    return FALSE;
}


// add a new switcher.  Switcher will be deleted when manager is deleted.
void CWindowFocusMgr::AddSwitcher(CWindowFocusSwitcher* pSwitcher)
{
    m_aSwitchers.Add(pSwitcher);
}

// set focus to next window in list (assumes list has been populated already)
void CWindowFocusMgr::OnNextWindow()
{
    const INT_PTR iFocus = GetFocusWndIndex();
    INT_PTR iNewFocus;
    if (iFocus == -1) {
        iNewFocus = 0;
    }
    else {
        iNewFocus = iFocus + 1;

        if (iNewFocus >= m_aFocusWindows.GetCount()) {
            iNewFocus = 0;
        }
    }

    SetNewFocusWnd(iNewFocus);
}

// set focus to previous window in list (assumes list has been populated already)
void CWindowFocusMgr::OnPrevWindow()
{
    const INT_PTR iFocus = GetFocusWndIndex();
    INT_PTR iNewFocus;
    if (iFocus == -1) {
        iNewFocus = 0;
    }
    else {
        iNewFocus = iFocus - 1;

        if (iNewFocus < 0) {
            iNewFocus = m_aFocusWindows.GetCount() - 1;
        }
    }

    SetNewFocusWnd(iNewFocus);
}

// recursively iterate through all children of pStartWnd and add any focus
// candidate windows found to the list of candidates.  Valid candidates are
// those for which pSwitcher->MatchWindow returns true.
// Usually called from CWindowFocusSwitcher::FindWindows
void CWindowFocusMgr::AddWindowsRec(CWnd* pStartWnd, CWindowFocusSwitcher* pSwitcher)
{
    FindFocusCandidatesRec(pStartWnd, pSwitcher);
}

// add a focus candidate window and associated switcher to the list
// of candidates.  Usually called from CWindowFocusSwitcher::FindWindows
void CWindowFocusMgr::AddWindow(CWnd* pWnd, CWindowFocusSwitcher* pSwitcher)
{
    ASSERT(pSwitcher->MatchWindow(pWnd));
    if (FindInArray(m_aFocusWindows, pWnd) != -1) {
        // already found this with another switcher
        return;
    }

    // add window to list of candidate windows
    m_aFocusWindows.Add(pWnd);

    // lookup switcher in list of switchers
    int iSwitcher = FindInArray(m_aSwitchers, pSwitcher);
    ASSERT(iSwitcher != -1);

    // add switcher index to list so that is associated with this window
    m_aWndSwitcherIndices.Add(iSwitcher);
}

void CWindowFocusMgr::FindFocusCandidatesRec(CWnd* pWnd, CWindowFocusSwitcher* pSwitcher)
{
    // check for hidden windows (including those whose rects are empty e.g. case tree in full screen mode)
    bool bHidden = false;
    if (!pWnd->IsWindowVisible()) {
        bHidden = true;
    }
    else {
        CRect r;
        pWnd->GetWindowRect(&r);
        if (r.Width() == 0 || r.Height() == 0) {
            bHidden = true;
        }
    }

    // check if window is a match for the switcher
    bool bMatch = pSwitcher->MatchWindow(pWnd);

    // if it is hidden and the associated switcher doesn't allow hidden windows
    // then stop searching here.
    if (bHidden && (!bMatch || pSwitcher->MustBeVisible())) {
        return;
    }

    TRACE("%p  %s\n", (LPARAM)pWnd->m_hWnd, pWnd->GetRuntimeClass()->m_lpszClassName);

    if (bMatch) {
        // it is a match - add window to list of candidates and don't bother to search children
        AddWindow(pWnd, pSwitcher);
    }
    else {
        // search recursively on child windows.
        CWnd* pKid = pWnd->GetWindow(GW_CHILD);
        while (pKid) {
            FindFocusCandidatesRec(pKid, pSwitcher);
            pKid = pKid->GetWindow(GW_HWNDNEXT);
        }
    }
}

// Get the index in the list of candidates of the window that currently has focus
// The window that has focus may be a child of one of the windows in the list
// (e.g. edit control on a formview) so we need to check the focus windows parents too.
int CWindowFocusMgr::GetFocusWndIndex() const
{
    CWnd* pWnd = CWnd::GetFocus();
    int iView = -1;
    while (pWnd && iView == -1) {
        iView = FindInArray(m_aFocusWindows, pWnd);
        pWnd = pWnd->GetParent();
    }
    return iView;
}

// Set focus to the candidate in the list at index i.
// Just defers the call to the switcher associated with the window
void CWindowFocusMgr::SetNewFocusWnd(INT_PTR i)
{
    ASSERT(i >= 0 && i < m_aFocusWindows.GetCount());
    CWnd* pNewFocus = m_aFocusWindows.GetAt(i);
    CWindowFocusSwitcher* pSwitcher = m_aSwitchers.GetAt(m_aWndSwitcherIndices.GetAt(i));
    pSwitcher->SetFocus(pNewFocus);

}



// --------------------------------------------------------------------------
// CViewFocusSwitcher
// --------------------------------------------------------------------------

void CViewFocusSwitcher::FindWindows(CWindowFocusMgr* pMgr, CWnd* pAppMainWnd)
{
    // find all CView derived windows that are descendants of
    // pAppMainWnd and add them to candidate list.
    CFrameWnd* pMainFrame = DYNAMIC_DOWNCAST(CFrameWnd, pAppMainWnd);
    pMgr->AddWindowsRec(pMainFrame->GetActiveFrame(), this);
}


bool CViewFocusSwitcher::MatchWindow(CWnd* pWnd) const
{
    // match any window derived from CView
    return pWnd->IsKindOf(RUNTIME_CLASS(CView));
}


void CViewFocusSwitcher::SetFocus(CWnd* pWnd)
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


bool CViewFocusSwitcher::MustBeVisible() const
{
    return true;
}



// --------------------------------------------------------------------------
// ViewIgnoringTabbedViewsFocusSwitcher
// --------------------------------------------------------------------------

bool ViewIgnoringTabbedViewsFocusSwitcher::MatchWindow(CWnd* pWnd) const
{
    return ( __super::MatchWindow(pWnd) &&
             !pWnd->GetParent()->IsKindOf(RUNTIME_CLASS(CMFCTabCtrl)) );
}
