//***************************************************************************
//  File name: DDGView.cpp
//
//  Description:
//       Data Dictionary grid view implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   bmd     Created for CSPro 2.0
//
//***************************************************************************

#include "StdAfx.h"
#include "QSFCndVw.h"

/////////////////////////////////////////////////////////////////////////////
// CQSFCndView

IMPLEMENT_DYNCREATE(CQSFCndView, CView)

CQSFCndView::CQSFCndView()
{
    EnableAutomation();
}

CQSFCndView::~CQSFCndView()
{
}

void CQSFCndView::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CView::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CQSFCndView, CView)
    //{{AFX_MSG_MAP(CQSFCndView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_UPDATE_COMMAND_UI(ID_QSFCOND_DELETE, OnUpdateEditDelete)
    ON_COMMAND(ID_QSFCOND_DELETE, OnEditDelete)
    ON_COMMAND(ID_QSFCOND_ADD, OnEditAdd)
    ON_UPDATE_COMMAND_UI(ID_QSFCOND_ADD, OnUpdateEditAdd)
    ON_COMMAND(ID_QSFCOND_INSERT, OnEditInsert)
    ON_UPDATE_COMMAND_UI(ID_QSFCOND_INSERT, OnUpdateEditInsert)
    ON_WM_SETFOCUS()
    ON_COMMAND(ID_QSFCOND_MODIFY, OnEditModify)
    ON_UPDATE_COMMAND_UI(ID_QSFCOND_MODIFY, OnUpdateEditModify)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CQSFCndView drawing

void CQSFCndView::OnDraw(CDC* /*pDC*/)
{
//  CDocument* pDoc = GetDocument();
    // TODO: add draw code here
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnCreate
//
/////////////////////////////////////////////////////////////////////////////

int CQSFCndView::OnCreate(LPCREATESTRUCT lpCreateStruct) {

    if (CView::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }

    m_lf.lfHeight           = 16;
    m_lf.lfWidth            = 0;
    m_lf.lfEscapement       = 0;
    m_lf.lfOrientation      = 0;
    m_lf.lfWeight           = FW_NORMAL;
    m_lf.lfItalic           = FALSE;
    m_lf.lfUnderline        = FALSE;
    m_lf.lfStrikeOut        = FALSE;
    m_lf.lfCharSet          = ANSI_CHARSET;
    m_lf.lfOutPrecision     = OUT_DEFAULT_PRECIS;
    m_lf.lfClipPrecision    = CLIP_DEFAULT_PRECIS;
    m_lf.lfQuality          = DEFAULT_QUALITY;
    m_lf.lfPitchAndFamily   = FF_SWISS;
    lstrcpy(m_lf.lfFaceName,_T("Arial"));

    RECT rect = {0,0,50,50};
    CUGCell cell;


    // Create Item Grid
    m_gridCond.SetGridFont(&m_lf);
    m_gridCond.CreateGrid(WS_CHILD, rect, this, 1234);
    m_gridCond.SetDefFont(m_gridCond.AddFont(m_lf.lfFaceName, m_lf.lfHeight, m_lf.lfWeight));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnUpdate
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    if (lHint == Hint::CapiEditorUpdateQuestion || lHint == Hint::CapiEditorUpdateQuestionStyles) {
        m_gridCond.OnUpdate();
        if (GetViewModel().CanHaveText()) {
            EnableWindow(TRUE);
            //  m_gridCond.ShowWindow(TRUE);
            m_gridCond.EnableWindow(TRUE);
        }
        else {
            EnableWindow(FALSE);
            //  m_gridCond.ShowWindow(FALSE);
            m_gridCond.EnableWindow(FALSE);
        }
    }
    else if (lHint == Hint::CapiEditorUpdateCondition) {
        m_gridCond.OnUpdate();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/) {

//  CView::OnSize(nType, cx, cy);
    ResizeGrid();
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::ResizeGrid
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::ResizeGrid(void)
{
    CRect rect;
    GetClientRect(&rect);
    m_gridCond.ShowWindow(SW_SHOW);
    m_gridCond.Resize(rect);
    m_gridCond.SetFocus();

    if (GetFocus() != this) {
        m_gridCond.SetFocus();         // BMD remove to reduce flicker 19 Apr 2000
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnEditMakeSubitems
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnEditModify() {
   m_gridCond.SendMessage(WM_COMMAND, ID_QSFCOND_MODIFY);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnEditAdd() {

    m_gridCond.SendMessage(WM_COMMAND, ID_QSFCOND_ADD);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnEditInsert
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnEditInsert()
{
    m_gridCond.SendMessage(WM_COMMAND, ID_QSFCOND_INSERT);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnEditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnEditDelete()
{
    m_gridCond.SendMessage(WM_COMMAND, ID_QSFCOND_DELETE);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnUpdateEditModify
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnUpdateEditModify(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnUpdateEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnUpdateEditAdd(CCmdUI* pCmdUI) {
    pCmdUI->Enable(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnUpdateEditInsert
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnUpdateEditInsert(CCmdUI* pCmdUI) {
    pCmdUI->Enable(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnUpdateEditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnUpdateEditDelete(CCmdUI* pCmdUI) {
    pCmdUI->Enable(TRUE);
}
/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////
void CQSFCndView::OnSetFocus(CWnd* /*pOldWnd*/)
{
    m_gridCond.SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::PreTranslateMessage
//
/////////////////////////////////////////////////////////////////////////////

BOOL CQSFCndView::PreTranslateMessage(MSG* pMsg)
{
    // handle RTF-specific accelerators ...
    if (m_hAccel != NULL && TranslateAccelerator(m_hWnd, m_hAccel, pMsg)) {
       return TRUE;
    }

    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE) {
        if (m_gridCond.IsEditing()) {
            ::SendMessage(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
            return TRUE;
        }
    }
    return CView::PreTranslateMessage(pMsg);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CQSFCndView::OnInitialUpdate
//
/////////////////////////////////////////////////////////////////////////////

void CQSFCndView::OnInitialUpdate() {

    CView::OnInitialUpdate();

    // load RTF accelerator (ctrl+A, ctrl+M, Ctrl+V etc.)
    HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(IDR_COND_ACCELERATOR), RT_ACCELERATOR);
    m_hAccel = ::LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_COND_ACCELERATOR));

    CRect rect;
    GetClientRect(&rect);
    m_gridCond.Size(rect);
}

void CQSFCndView::OnEditCut()
{
    m_gridCond.Cut();
}

void CQSFCndView::OnEditPaste()
{
    m_gridCond.Paste();
}

void CQSFCndView::OnEditUndo()
{
    m_gridCond.Undo();
}

void CQSFCndView::OnEditCopy()
{
    m_gridCond.Copy();
}

CapiEditorViewModel& CQSFCndView::GetViewModel()
{
    return GetFormDoc()->GetCapiEditorViewModel();
}

CFormDoc* CQSFCndView::GetFormDoc()
{
    CFormDoc* doc = DYNAMIC_DOWNCAST(CFormDoc, GetDocument());
    ASSERT(doc);
    return doc;
}
