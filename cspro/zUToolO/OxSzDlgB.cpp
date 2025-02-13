// ===================================================================================
//                  Class Implementation : COXSizeDialogBar
// ===================================================================================

// Header file : OXSizeDlgBar.cpp

// Copyright © Dundas Software Ltd. 1997 - 1998, All Rights Reserved
// Some portions Copyright (C)1994-5    Micro Focus Inc, 2465 East Bayshore Rd, Palo Alto, CA 94303.

// //////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <afxpriv.h>
/*#include <oledb.h>
#include <olebind.h>
#include <ocdbid.h>
#include <ocdb.h>*/
#include <afxocc.h>
#include "OxSzDlgB.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(COXSizeDialogBar, COXSizeControlBar)

#define new DEBUG_NEW

GADGETRESIZEHANDLE CreateGadgetResizeHandle(CWnd* pWnd);
void DestroyGadgetResizeHandle(GADGETRESIZEHANDLE Handle);
void ResizeGadgetsOnWindow(GADGETRESIZEHANDLE Handle, int cx, int cy);

#ifndef _AFX_NO_OCC_SUPPORT
    // Following header file is supplied as part of MFC4.0, but is NOT in the standard include directory.
    // For convenience, a copy of occimpl.h is included along with these sources. If you upgrade MFc, you should
    // ensure this source file is upgraded also.
#ifndef __OX_OCCIMPL_H__
    #include <..\src\mfc\occimpl.h>
#define __OX_OCCIMPL_H__
#endif
#endif

/*#ifndef __OXMFCIMPL_H__
#include <..\src\afximpl.h>
#define __OXMFCIMPL_H__
#endif*/


typedef struct _Rect_And_Hwnd
    {
    CRect   m_rect;
    HWND    m_hWnd;
    }   RECT_AND_HWND;

struct COXSizeDialogBar_ExtData
    {
    _AFX_OCC_DIALOG_INFO* m_pOccDialogInfo;
    LPCTSTR m_lpszTemplateName;
    };


/////////////////////////////////////////////////////////////////////////////
// Definition of static members

// Data members -------------------------------------------------------------
// protected:

// private:

// Member functions ---------------------------------------------------------
// public:

#ifdef _AFXDLL
const TCHAR _afxWndControlBar[] = _T("AfxControlBar");  // class for control bars
#endif

COXSizeDialogBar::COXSizeDialogBar(int nStyle)
    : COXSizeControlBar(nStyle)
    {
    m_GadgetResizeHandle = NULL;

#ifndef _AFX_NO_OCC_SUPPORT
    m_lpszTemplateName = NULL;
    m_pOccDialogInfo = NULL;
#endif
    }

COXSizeDialogBar::~COXSizeDialogBar()
    {
    DestroyGadgetResizeHandle(m_GadgetResizeHandle);

    DestroyWindow();    // avoid PostNcDestroy problems
    }


BEGIN_MESSAGE_MAP(COXSizeDialogBar, COXSizeControlBar)
    //{{AFX_MSG_MAP(COXSizeDialogBar)
    ON_WM_ERASEBKGND()
    //}}AFX_MSG_MAP
#ifndef _AFX_NO_OCC_SUPPORT
    ON_MESSAGE(WM_INITDIALOG, HandleInitDialog)
#endif
END_MESSAGE_MAP()


#ifdef _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Following is taken from afximpl.h, to check that the dialog template
// for a dialog bars are indeed ok to use. Only present if debugging

// Extended dialog templates (new in Win95)
#pragma pack(push, 1)

#pragma pack(pop)

// diagnostic routine to check for and decode dialog templates
// return FALSE if a program error occurs (i.e. bad resource ID or
//   bad dialog styles).
BOOL AFXAPI CheckDialogTemplate(LPCTSTR lpszResource, BOOL bInvisibleChild)
    {
    ASSERT(lpszResource != NULL);
    HINSTANCE hInst = AfxFindResourceHandle(lpszResource, RT_DIALOG);
    HRSRC hResource = ::FindResource(hInst, lpszResource, RT_DIALOG);
    if (hResource == NULL)
        {
        if (HIWORD(lpszResource) != 0)
            TRACE1("ERROR: Cannot find dialog template named '%s'.\n",
                lpszResource);
        else
            TRACE1("ERROR: Cannot find dialog template with IDD 0x%04X.\n",
                LOWORD((DWORD)lpszResource));
        return FALSE;
        }

    if (!bInvisibleChild)
        return TRUE;        // that's all we need to check

    // we must check that the dialog template is for an invisible child
    //  window that can be used for a form-view or dialog-bar
    HGLOBAL hTemplate = LoadResource(hInst, hResource);
    if (hTemplate == NULL)
        {
        TRACE0("Warning: LoadResource failed for dialog template.\n");
        // this is only a warning, the real call to CreateDialog will fail
        return TRUE;        // not a program error - just out of memory
        }
    DLGTEMPLATEEX* pTemplate = (DLGTEMPLATEEX*)LockResource(hTemplate);
    DWORD dwStyle;
    if (pTemplate->signature == 0xFFFF)
        dwStyle = pTemplate->style;
    else
        dwStyle = ((DLGTEMPLATE*)pTemplate)->style;
    UnlockResource(hTemplate);
    FreeResource(hTemplate);

    if (dwStyle & WS_VISIBLE)
        {
        if (HIWORD(lpszResource) != 0)
            TRACE1("ERROR: Dialog named '%s' must be invisible.\n",
                lpszResource);
        else
            TRACE1("ERROR: Dialog with IDD 0x%04X must be invisible.\n",
                LOWORD((DWORD)lpszResource));
        return FALSE;
        }

    if (!(dwStyle & WS_CHILD))
        {
        if (HIWORD(lpszResource) != 0)
            TRACE1("ERROR: Dialog named '%s' must have the child style.\n",
                lpszResource);
        else
            TRACE1("ERROR: Dialog with IDD 0x%04X must have the child style.\n",
                LOWORD((DWORD)lpszResource));
        return FALSE;
        }

    return TRUE;
    }

#endif //_DEBUG

BOOL COXSizeDialogBar::Create(CWnd* pParentWnd, LPCTSTR lpszTemplateName,
                              UINT nStyle, UINT nID)
{
    ASSERT(pParentWnd != NULL);
    ASSERT(lpszTemplateName != NULL);
    ASSERT((nStyle & WS_THICKFRAME)==NULL);

// Alas, MFC doesn't export many of the debugg-only functions, but I've added
// the code for this safety net as it helps track down errors faster, and
// only has an overhead in debug code
#ifdef _DEBUG
// dialog template must exist and be invisible with WS_CHILD set
    if (!CheckDialogTemplate(lpszTemplateName, TRUE))
    {
        ASSERT(FALSE);          // invalid dialog template name
        PostNcDestroy();        // cleanup if Create fails too soon
        return FALSE;
    }
#endif //_DEBUG

    // allow chance to modify styles
#if _MFC_VER <= 0x0421
    m_dwStyle = nStyle;
#else
    m_dwStyle = nStyle&CBRS_ALL;
#endif
    CREATESTRUCT cs;
    memset(&cs, 0, sizeof(cs));
    cs.lpszClass = _afxWndControlBar;
    cs.style = (DWORD)nStyle | WS_CHILD;
    cs.hMenu = (HMENU)nID;
    cs.hInstance = AfxGetInstanceHandle();
    cs.hwndParent = pParentWnd->GetSafeHwnd();
    if (!PreCreateWindow(cs))
        return FALSE;

    // create a modeless dialog
#ifndef _AFX_NO_OCC_SUPPORT
    m_lpszTemplateName = lpszTemplateName;
#endif

    // initialize common controls
    //VERIFY(AfxDeferRegisterClass(AFX_WNDCOMMCTLS_REG));

    BOOL bSuccess = CreateDlg(lpszTemplateName, pParentWnd);

#ifndef _AFX_NO_OCC_SUPPORT
    m_lpszTemplateName = NULL;
#endif

    if (!bSuccess)
        return FALSE;

    // dialog template MUST specify that the dialog
    //  is an invisible child window
    SetDlgCtrlID(nID);
    CRect rect;


    // force WS_CLIPSIBLINGS
    // I also remove the titlebar. This means the resource can include a caption, which
    // will be used when the frame is floating.
    ModifyStyle(WS_CAPTION, WS_CLIPSIBLINGS);
    if (!ExecuteDlgInit(lpszTemplateName))
        return FALSE;

    // m_sizeDefault isn't actually used by the COXSizeDialogBar, but since it's public, it
    // seemed sensible to keep it available. Otherwise it might be difficult to get hold
    // of the original size of the dialog (as specified in the resource file).

    //GetWindowRect(&rect);
    GetClientRect(&rect);
    m_sizeDefault = rect.Size();    // set fixed size

    // Move to position 0,0
    // NB: size not forced to zero, as this can affect resizing if bar is
    // immediately docked
    SetWindowPos(NULL, 0, 0, 0, 0,
        SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOSIZE|SWP_FRAMECHANGED);

    // set all the sizes to be the default after the positioning done above
    m_VertDockSize  = m_sizeDefault;
    m_HorzDockSize  = m_sizeDefault;
    m_FloatSize     = m_sizeDefault;


    // if auto-sizing, store the rectangles of all the child windows.
    if(m_Style & SZBARF_DLGAUTOSIZE)
    {
        m_GadgetResizeHandle = CreateGadgetResizeHandle(this);
    }

    return TRUE;
}


// need to paint the background...
BOOL COXSizeDialogBar::OnEraseBkgnd(CDC* pDC)
{
    return COXSizeControlBar::OnEraseBkgnd(pDC);
}


void COXSizeDialogBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
    COXSizeControlBar::OnUpdateCmdUI(pTarget,bDisableIfNoHndler);
    UpdateDialogControls(pTarget, bDisableIfNoHndler);
}


void COXSizeDialogBar::OnSizedOrDocked(int cx, int cy, BOOL bFloating, int flags)
{
    COXSizeControlBar::OnSizedOrDocked(cx, cy, bFloating, flags);
    if (cx == 0 || cy == 0)
        return;

    // if auto-sizing, go thru and reposition all the controls.
    if (m_Style & SZBARF_DLGAUTOSIZE)
    {
        CRect rectClient;
        GetClientRect(&rectClient);
        ResizeGadgetsOnWindow(m_GadgetResizeHandle, rectClient.Width(),
            rectClient.Height());
    }
}

/////////////////////////////////////////////////////////////////////////////
// Gadget resizing functions - used to resize all the child windows of a dialog
// bar above. Routines extracted as they may be more used in general

// For a given window, creates an array of the child windows, that can be used
// for resizing later on.
GADGETRESIZEHANDLE CreateGadgetResizeHandle(CWnd* pWnd)
    {
    CPtrArray* pArray = new CPtrArray;

    // 1st element of the array stores the client rectangle of the given window
    RECT_AND_HWND* pRectWnd = new RECT_AND_HWND;
    pWnd->GetClientRect(&(pRectWnd->m_rect));
    pRectWnd->m_hWnd = NULL;
    pArray->Add(pRectWnd);
    TCHAR szClassName[64];

    for (HWND hWndChild = ::GetTopWindow(pWnd->m_hWnd); hWndChild != NULL;
    hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT))
        {
        RECT_AND_HWND* pRectWnd = new RECT_AND_HWND;
        pRectWnd->m_hWnd = hWndChild;
        GetClassName(pRectWnd->m_hWnd, szClassName, 64);
        ::GetWindowRect(hWndChild, &(pRectWnd->m_rect));
        pWnd->ScreenToClient(&(pRectWnd->m_rect));

        // if it's a drop down combo box, then we set bottom -ve to indicate
        // we should keep the height fixed. Otherwise we might find that the drop
        // down size gets stupidly small.
        if (_tcscmp(szClassName, _T("ComboBox")) == 0 &&
            (GetWindowLong(hWndChild, GWL_STYLE) & (CBS_DROPDOWN | CBS_DROPDOWNLIST)) )
            {
            pRectWnd->m_rect.bottom = - (pRectWnd->m_rect.bottom - pRectWnd->m_rect.top);
            }

        pArray->Add(pRectWnd);
        }

    return (GADGETRESIZEHANDLE) pArray;
    }


// deletes the underlying array and it's elements
void DestroyGadgetResizeHandle(GADGETRESIZEHANDLE Handle)
    {
    CPtrArray* pArray = (CPtrArray*)Handle;
    if (pArray != NULL)
        {
        for (int i = pArray->GetUpperBound(); i >= 0; i--)
            delete ((RECT_AND_HWND*)pArray->GetAt(i));
        delete pArray;
        }
    }


// Resizes all the gadgets in an array to match the new size.
// Generally called from an OnSize routine (or the like)
void ResizeGadgetsOnWindow(GADGETRESIZEHANDLE Handle, int cx, int cy)
    {
    CPtrArray* pArray = (CPtrArray*)Handle;

    if (pArray == NULL || cx == 0 || cy == 0)
        return;

    ASSERT(pArray->GetSize() > 0);  // must be at least one element in the array

    // if auto-sizing, go thru and reposition all the controls.
    CSize sizeOrig = ((RECT_AND_HWND*)pArray->GetAt(0))->m_rect.Size();
    if(sizeOrig.cx==0)
        sizeOrig.cx=1;
    if(sizeOrig.cy==0)
        sizeOrig.cy=1;
    for (int i = 1; i < pArray->GetSize(); i++)
        {
        RECT_AND_HWND* pRectWnd = (RECT_AND_HWND*)pArray->GetAt(i);
        CRect newRect = pRectWnd->m_rect;
        newRect.top    = newRect.top    * cy / sizeOrig.cy;
        newRect.left   = newRect.left   * cx / sizeOrig.cx;
        newRect.right  = newRect.right  * cx / sizeOrig.cx;
        // if bottom is < 0, then keep it fixed height - used for combo boxes
        // where we don't really want to resize most of them...
        if (newRect.bottom < 0)
            newRect.bottom = newRect.top - newRect.bottom;
        else
            newRect.bottom = newRect.bottom * cy / sizeOrig.cy;

        ::SetWindowPos(pRectWnd->m_hWnd, NULL,
            newRect.left, newRect.top, newRect.Width(), newRect.Height(),
            SWP_NOZORDER);
        }
    }

#ifndef _AFX_NO_OCC_SUPPORT

LRESULT COXSizeDialogBar::HandleInitDialog(WPARAM, LPARAM)
    {
    Default();  // allow default to initialize first (common dialogs/etc)


    // get extension data - for backward compatibility with existing MRCEXT,
    // this can't be in the base class itself.
    // create OLE controls
    COccManager* pOccManager = afxOccManager;
    if ((pOccManager != NULL) && (m_pOccDialogInfo != NULL))
        {
        if (!pOccManager->CreateDlgControls(this, m_lpszTemplateName, m_pOccDialogInfo))
            {
            TRACE0("Warning: CreateDlgControls failed during dialog bar init.\n");
            return FALSE;
            }
        }

    BOOL bResult=OnInitDialog();
    return bResult;
    }

BOOL COXSizeDialogBar::SetOccDialogInfo(_AFX_OCC_DIALOG_INFO* pOccDialogInfo)
    {
    m_pOccDialogInfo = pOccDialogInfo;
    return TRUE;
    }

#endif