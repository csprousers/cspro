// ==========================================================================
//              Class Implementation : COXGridEdit
// ==========================================================================

// Source file : OXGridEdit.cpp

// Copyright © Dundas Software Ltd. 1997 - 1998, All Rights Reserved

// //////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "oxgedt.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(COXGridEdit, CEdit)

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// Definition of static members
// Data members -------------------------------------------------------------
// protected:
    // CString m_sDeferedWindowText;
    // --- The window text to be set during the next OnSize event
    //     (if m_bDeferedWindowText == TRUE)

    // BOOL m_bDeferedWindowText;
    // --- Whether to change the window text during the next OnSize event

    // CString m_sOldWindowText;
    // --- The window text before it was changed during OnSize

    // CPoint m_ptWindowPos;
    // --- The new top left corner of this control (if m_bWindowPos == TRUE)

    // BOOL m_bWindowPos;
    // --- Whether to keep the control at the fixed position of m_ptWindowPos

    // int m_nWindowHeight;
    // --- The new height of this control (if m_bWindowHeight == TRUE)

    // BOOL m_bWindowHeight;
    // --- Whether to keep the control at the fixed height of m_nWindowHeight

    // int m_nCXOffset;
    // --- An additional ofset that must be added to the normal width

    // BOOL m_bAdjustWindowWidth;
    // --- Whether to calculate the edit control width from the text width
    //     and to add m_nCXOffset

    // UINT m_nEndKeyChar;
    // --- Virtual key code with which the editing was ended (0 if ended in another way)

    // BOOL m_bEndKeyShift;
    // --- The shift status when the key was pressed that ended the editing
    // BOOL m_bEndKeyCtrl;
    // --- The ctrl status when the key was pressed that ended the editing

// private:

// Member functions ---------------------------------------------------------
// public:

COXGridEdit::COXGridEdit()
{
    Initialize();
    ASSERT_VALID(this);
}

void COXGridEdit::Initialize()
{
    m_sDeferedWindowText.Empty();
    m_bDeferedWindowText = FALSE;
    m_ptWindowPos = CPoint(0,0);
    m_bWindowPos = FALSE;
    m_nWindowHeight = 0;
    m_bWindowHeight = FALSE;
    m_bAdjustWindowWidth = FALSE;
    m_nCXOffset = 0;
    m_nEndKeyChar = 0;
    m_bEndKeyShift = FALSE;
    m_bEndKeyCtrl = FALSE;
    m_nRightParentBorder = -1;
    m_bFitToClient=FALSE;
}

void COXGridEdit::SetDeferedWindowText(LPCTSTR lpszString)
{
    m_sDeferedWindowText = lpszString;
    m_bDeferedWindowText = TRUE;
}

void COXGridEdit::SetWindowPos(const CPoint& pt)
{
    m_ptWindowPos = pt;
    m_bWindowPos = TRUE;
}

void COXGridEdit::SetWindowHeight(int nHeight)
{
    m_nWindowHeight = nHeight;
    m_bWindowHeight = TRUE;
}

void COXGridEdit::AdjustWindowWidth(int nWidth)
{
    m_nCXOffset = nWidth;
    m_bAdjustWindowWidth = TRUE;
}

BOOL COXGridEdit::GetEndKey(UINT& nChar, BOOL& bShift, BOOL& bCtrl)
{
    nChar = m_nEndKeyChar;
    bShift = m_bEndKeyShift;
    bCtrl =  m_bEndKeyCtrl;
    return (nChar != 0);
}

#ifdef _DEBUG
void COXGridEdit::AssertValid() const
{
    CEdit::AssertValid();
}

void COXGridEdit::Dump(CDumpContext& dc) const
{
    CEdit::Dump(dc);
    dc << _T("\nm_sDeferedWindowText: ") << m_sDeferedWindowText;
    dc << _T("\nm_bDeferedWindowText: ") << m_bDeferedWindowText;
    dc << _T("\nm_ptWindowPos: ") << m_ptWindowPos;
    dc << _T("\nm_bWindowPos: ") << m_bWindowPos;
    dc << _T("\nm_nWindowHeight: ") << m_nWindowHeight;
    dc << _T("\nm_bWindowHeight: ") << m_bWindowHeight;
    dc << _T("\nm_nCXOffset: ") << m_nCXOffset;
    dc << _T("\nm_bAdjustWindowWidth: ") << m_bAdjustWindowWidth;
    dc << _T("\nm_nEndKeyChar: ") << m_nEndKeyChar;
    dc << _T("\nm_bEndKeyShift: ") << m_bEndKeyShift;
    dc << _T("\nm_bEndKeyCtrl: ") << m_bEndKeyCtrl;
    dc << _T("\n");
}
#endif //_DEBUG

COXGridEdit::~COXGridEdit()
{
}

BEGIN_MESSAGE_MAP(COXGridEdit, CEdit)
    //{{AFX_MSG_MAP(COXGridEdit)
    ON_WM_SIZE()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void COXGridEdit::OnSize(UINT nType, int cx, int cy)
{
    CEdit::OnSize(nType, cx, cy);

    // If the defered window text has not yet been set, do it now
    if (m_bDeferedWindowText)
    {
        m_bDeferedWindowText = FALSE;
        // ... Make sure the window exists
        ASSERT(::IsWindow(m_hWnd));
        SetWindowText(m_sDeferedWindowText);
        // ... Make sure the window still exists after the SetWindowText !
        ASSERT(::IsWindow(m_hWnd));
    }
}

void COXGridEdit::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos)
{
    // Adjust window position if necessary
    if (m_bWindowPos)
    {
        lpwndpos->x = m_ptWindowPos.x;
        lpwndpos->y = m_ptWindowPos.y;
    }

    // Adjust window width if necessary
    if (m_bWindowHeight)
        lpwndpos->cy = m_nWindowHeight;

    // Adjust window width if necessary
    if (m_bAdjustWindowWidth)
    {
        // Appearently there is no good way to calculate the exact width of the
        // text in this edit control.
        // In Win95 we use the function PosFromChar which gives the best results
        // When this function is not available we use GetTextExtentPoint which appears
        //  to have a quit large deviation
        int nTextSizeX = 0;
        int nTextLength = GetWindowTextLength();
        if (nTextLength != 0)
        {
            CPoint ptFirst(0, 0);
            CPoint ptLast(0, 0);
            ptFirst = PosFromChar(0);
            // PosFromChar(nTextLength) should return the coordinates of the character position
            //  just past the last character.
            // But this does not work, so we use PosFromChar(nTextLength - 1) and
            //  add the avarage character size to it
            ptLast=PosFromChar(nTextLength - 1);
            if(ptLast.x!=0)
            {
                nTextSizeX = ptLast.x - ptFirst.x;
                if (nTextLength != 1)
                    nTextSizeX = (nTextSizeX * nTextLength) / (nTextLength - 1);
                else
                    // If only one character in control, use fixed value (for speed)
                    nTextSizeX = 10;
            }
            else
            {
                // PosFromChar falied, probably using Win NT 3.51 and not Window 95
                // (will use GetTextExtentPoint instead)
                CString sText;
                GetWindowText(sText);
                CSize textSize(0, 0);
                CDC* pDC=GetDC();
                ASSERT_VALID(pDC);
                VERIFY(GetTextExtentPoint(pDC->GetSafeHdc(),
                    (LPCTSTR)sText, sText.GetLength(), &textSize));
                nTextSizeX = textSize.cx;
                ReleaseDC(pDC);
            }
        }

        lpwndpos->cx = nTextSizeX + m_nCXOffset;
    }


    // Get the right border position of the parent window
    // (if we haven't done so before)
    if (m_nRightParentBorder < 0)
    {
        CWnd* pParent;
        CWnd* pParentsParent;
        pParent = GetParent();
        pParentsParent = pParent->GetParent();

        CRect parentRect;
        pParent->GetClientRect(parentRect);
        // pParent->ClientToScreen(parentRect);
        m_nRightParentBorder = parentRect.right;
    }

    // Make sure this edit control is completely within its parent's
    // client area
    if(m_bFitToClient || m_nRightParentBorder<lpwndpos->x+lpwndpos->cx)
        lpwndpos->cx=m_nRightParentBorder-lpwndpos->x;

    CEdit::OnWindowPosChanging(lpwndpos);
}


void COXGridEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // Check whether the Tab key key was pressed
    if (nChar == VK_TAB)
    {
        // Store the key that ended the editing
        m_nEndKeyChar = nChar;
        if (::GetKeyState(VK_SHIFT) & 0x8000)
            m_bEndKeyShift = TRUE;
        if (::GetKeyState(VK_CONTROL) & 0x8000)
            m_bEndKeyCtrl = TRUE;

        // End the editing in the edit control by setting the focus
        // back to the list control (parent)
        GetParent()->SetFocus();

        // Do not call the base class, because VT_TAB is an illegal key
        // and this will produce a message beep
        return;
    }

    CEdit::OnChar(nChar, nRepCnt, nFlags);
}

void COXGridEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // Check whether the Insert key was pressed
    if ((nChar == VK_INSERT) || (nChar == VK_UP) || (nChar == VK_DOWN))
    {
        // Store the key that ended the editing
        m_nEndKeyChar = nChar;
        if (::GetKeyState(VK_SHIFT) & 0x8000)
            m_bEndKeyShift = TRUE;
        if (::GetKeyState(VK_CONTROL) & 0x8000)
            m_bEndKeyCtrl = TRUE;

        // End the editing in the edit control by setting the focus
        // back to the list control (parent)
        GetParent()->SetFocus();
    }

    CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

// protected:
// private:

// ==========================================================================
