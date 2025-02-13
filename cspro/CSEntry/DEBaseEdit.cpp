// DEBaseEdit.cpp : implementation file
//

#include "StdAfx.h"
#include "DEBaseEdit.h"
#include "leftprop.h"
#include "LeftView.h"
#include "MainFrm.h"
#include "RunView.h"
#include <zCapiO/SelectCtrl.h>
#include <zEngineO/OnKeyMapping.h>


// RHF INIC Dec 18, 2003 BUCEN_DEC2003 Changes
int  CDEBaseEdit::m_iRemappedKeyDown=-1;
int  CDEBaseEdit::m_iRemappedKeyChar=-1;
bool CDEBaseEdit::m_bShift=false;
bool CDEBaseEdit::m_bControl=false;
bool CDEBaseEdit::m_bAlt=false;
//int  CDEBaseEdit::m_iTickWidth = 1;
// RHF END Dec 18, 2003 BUCEN_DEC2003 Changes




CFont* CDEBaseEdit::m_pFont = NULL;
CSize  CDEBaseEdit::m_szUnit = CSize(0,0);

bool CDEBaseEdit::m_bTrapNextHelp = false;

CDEBaseEdit* CDEBaseEdit::m_pLastEdit=NULL;


// CDEBaseEdit

IMPLEMENT_DYNAMIC(CDEBaseEdit, CEdit)

CDEBaseEdit::CDEBaseEdit()
{
    m_clrText = RGB( 0, 0, 0 );
    m_clrBkgnd = RGB( 255, 255, 255 );
    m_brBkgnd.CreateSolidBrush( m_clrBkgnd );
    m_bModified=false;
    m_bRemoveText = true;
    m_bFirstDecimal = true;
    m_pField = NULL;
    m_iCheckFocus = 0;
    m_bUseSequential = false;
}

CDEBaseEdit::~CDEBaseEdit()
{
    if( m_pLastEdit == this ) {
        SetLastEdit( NULL );
    }
}


BEGIN_MESSAGE_MAP(CDEBaseEdit, CEdit)
    ON_MESSAGE(UWM::CSEntry::RefreshSelected, OnRefreshSelected)
    ON_MESSAGE(UWM::CSEntry::ControlsSetWindowText, OnControlsSetWindowText)
    ON_MESSAGE(UWM::CSEntry::SimulatedKeyDown, OnSimulatedKeyDown) //FABN Jan 16, 2003
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_KEYDOWN()
    ON_WM_HELPINFO()
END_MESSAGE_MAP()



// CDEBaseEdit message handlers


CDEBaseEdit* CDEBaseEdit::GetLastEdit() {
    return m_pLastEdit;
}

void CDEBaseEdit::SetLastEdit( CDEBaseEdit*pEdit ) {
    m_pLastEdit=pEdit;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEBaseEdit::SetColor( CDC* pDC, COLORREF cColorFg, COLORREF cColorBg )
//
/////////////////////////////////////////////////////////////////////////////////
void CDEBaseEdit::SetColor( CDC* pDC, COLORREF cColorFg, COLORREF cColorBg )
{
    pDC->SetTextColor( cColorFg );
    pDC->SetBkColor( cColorBg );
}

// RHF END Dec 15, 2003 BUCEN_DEC2003 Changes  Modifed BMD 30 Jan 2004
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEBaseEdit::RemapChar( CRunAplEntry* pRunApl, UINT nChar, int* iCtrlFlags)
//
/////////////////////////////////////////////////////////////////////////////////
UINT CDEBaseEdit::RemapChar( CRunAplEntry* pRunApl, UINT nChar, int* iCtrlFlags)
{
    *iCtrlFlags = -1;

    if( pRunApl != NULL && pRunApl->HasSpecialFunction(SpecialFunction::OnKey) ) {
        /*m_bShift  = ( GetKeyState(VK_SHIFT) < 0 );
        m_bControl= ( GetKeyState(VK_CONTROL) < 0 );
        m_bAlt    = ( GetKeyState(VK_MENU) < 0 );*/
        m_bShift  = ( GetAsyncKeyState(VK_SHIFT) < 0 ); // 20120326 changed due to behavior observed using Ctrl+Alt+F in Nepal
        m_bControl= ( GetAsyncKeyState(VK_CONTROL) < 0 );
        m_bAlt    = ( GetAsyncKeyState(VK_MENU) < 0 );

        int     iChar=nChar;
        int     iCtrl=0;
        int     iCharCtrl;

        if( m_bShift )
            iCtrl += OnKeyMapping::Shift;
        if( m_bControl )
            iCtrl += OnKeyMapping::Ctrl;
        if( m_bAlt )
            iCtrl += OnKeyMapping::Alt;

        iCharCtrl = iChar + iCtrl;

        bool bPressedHelpKey = iCharCtrl == 112; // allow the user to disable the help menu

        if( bPressedHelpKey )
            m_bTrapNextHelp = true;

        double dNewCharCtl = pRunApl->ExecSpecialFunction(SpecialFunction::OnKey, iCharCtrl);

        if( bPressedHelpKey && dNewCharCtl == 112 ) // the user isn't overriding help
            m_bTrapNextHelp = false;

        // If OnKey function doesn't return a value from OnKey (i.e. DEFAULT is returned)
        // then treat it as if they returned zero.  With older compilers converting to int turned
        // it to zero anyway but w. VS2005 this behavior changed.  JH 6/29/07
        if (dNewCharCtl == DEFAULT) {
            dNewCharCtl = 0;
        }

        int iNewChar = (int)dNewCharCtl % 1000;
        int iNewCtl = (int)dNewCharCtl - iNewChar;
        if (iNewChar < 0 || iNewChar > 255) {
            AfxMessageBox(_T("Bad on key character value"));
            iNewChar = iChar;
        }
        if (iNewCtl > 7000) {
            AfxMessageBox(_T("Bad control key value"));
            iNewCtl = iCtrl;
        }
        if (iNewCtl != iCtrl) {
            bool bShift = false;
            bool bControl = false;
            bool bAlt = false;
            if (iNewCtl >=OnKeyMapping::Alt) {
                bAlt = true;
                iNewCtl -= OnKeyMapping::Alt;
            }
            if (iNewCtl >=OnKeyMapping::Ctrl) {
                bControl = true;
                iNewCtl -= OnKeyMapping::Ctrl;
            }
            if (iNewCtl >=OnKeyMapping::Shift) {
                bShift = true;
                iNewCtl -= OnKeyMapping::Shift;
            }
            ResetSpecialKeys(bShift,bControl,bAlt);
        }

        if( iNewChar > 0 ) {
            if( iNewCtl >= 0 && iCtrl != iNewCtl ) // Flags changed. Ignore invalid values
                *iCtrlFlags = 0;
            nChar = iNewChar;
        }
        else
            nChar = 0;
    }

    return nChar;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEBaseEdit::ResetSpecialKeys( bool bShift, bool bCtrl, bool bAlt)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEBaseEdit::ResetSpecialKeys( bool bShift, bool bCtrl, bool bAlt)
{
    BYTE     KeyState[256];

    if( GetKeyboardState( KeyState ) ) {
        KeyState[VK_SHIFT]  = bShift ? 0xFF : 0; // Turn on/off Control Key
        KeyState[VK_CONTROL]= bCtrl  ? 0xFF : 0; // Turn on/off Control Key
        KeyState[VK_MENU]   = bAlt   ? 0xFF : 0; // Turn on/off Control Key
        SetKeyboardState( KeyState );
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEBaseEdit::GetRunAplEntry()
//
/////////////////////////////////////////////////////////////////////////////////
CRunAplEntry* CDEBaseEdit::GetRunAplEntry()
{
    // RHF COM Apr 06, 2006        CEntryrunView* pParent = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());
    CEntryrunView* pParent = IsWindow(GetSafeHwnd()) ? DYNAMIC_DOWNCAST(CEntryrunView,GetParent()) : NULL;

    CEntryrunDoc* pRunDoc  = pParent ? pParent->GetDocument() : NULL;
    CRunAplEntry* pRunApl  = pRunDoc ? pRunDoc->GetRunApl() : NULL;

    return pRunApl;
}


//FABN Jan 16, 2003
LRESULT CDEBaseEdit::OnSimulatedKeyDown( WPARAM wParam, LPARAM lParam )
{
    CArray<DWORD,DWORD>*  pArray = (CArray<DWORD,DWORD>*) wParam;

    ASSERT(pArray->GetSize()==6);

    //params from the original OnKeyDown call
    UINT nChar  = pArray->GetAt(0);
    UINT nRepCnt= pArray->GetAt(1);
    UINT nFlags = pArray->GetAt(2);

    OnKeyDown( nChar, nRepCnt, nFlags );

    return TRUE;
}


// 20100616
// extended controls will call this function to set the text of the field
// if the user has selected an aspect of the control that affects the field
LRESULT CDEBaseEdit::OnControlsSetWindowText(WPARAM wParam,LPARAM lParam)
{
    CaptureType capture_type = (CaptureType)wParam;
    CString* pString = (CString*)lParam;
    SetWindowText(*pString);

    CEntryrunView* pView = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());

    if( pView->GetDocument()->GetAppMode() == VERIFY_MODE ) // 20140312
    {
        CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
        pView->SetVerifyStringFromControls(*pString,GetField()->GetDictItem()->GetContentType() == ContentType::Numeric);
    }

    else
    {
        m_pField->SetData(*pString);
    }

    CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,m_pField->GetParent());

    if( pRoster != nullptr ) // 20100625 values in rosters weren't getting set
    {
        int oldOcc = m_pField->GetRuntimeOccurrence();

        m_pField->SetRuntimeOccurrence(m_pField->GetParent()->GetCurOccurrence());

        CEntryrunView* pView  = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());

        pView->GetDocument()->GetRunApl()->PutVal(m_pField,pString->GetBuffer());

        m_pField->SetRuntimeOccurrence(oldOcc);
    }

    pView->GetDocument()->SetQModified(true);

    if( !m_pField->IsEnterKeyRequired() && capture_type != CaptureType::CheckBox && capture_type != CaptureType::ToggleButton )
        pView->PostMessage(UWM::CSEntry::ChangeEdit, VK_RETURN, (LPARAM)this);

    return 0;
}


LONG CDEBaseEdit::OnRefreshSelected(WPARAM wParam, LPARAM /*lParam*/)
{
    AfxGetMainWnd()->SendMessage(UWM::CSEntry::RefreshSelected, (WPARAM)this);
    return 0;
}


BOOL CDEBaseEdit::OnEraseBkgnd(CDC* pDC)
{
    if(!pDC){
        return TRUE;
    }
    ASSERT(m_pField);

        CRect rect;
    GetClientRect(rect);
        if(m_pField->IsProtected() || m_pField->IsMirror()){
            m_clrBkgnd = GetSysColor(COLOR_3DFACE);     // BMD 17 Apr 2001
            pDC->FillSolidRect(rect,m_clrBkgnd);
            return TRUE;
        }
    if(m_pField->IsPersistent() || m_pField->IsAutoIncrement()){
        m_clrBkgnd = GetSysColor(COLOR_BTNFACE);  // BMD 17 Apr 2001
        int red = GetRValue(m_clrBkgnd);
        int green = GetGValue(m_clrBkgnd);
        int blue = GetBValue(m_clrBkgnd);
        red = red + (255 - red)/2;
        green = green + (255 - green)/2;
        blue = blue + (255-blue)/2;
        m_clrBkgnd = RGB(red,green,blue);

        pDC->FillSolidRect(rect,m_clrBkgnd);
        return TRUE;
    }
    pDC->FillSolidRect(rect,RGB(255,255,255));

    CEntryrunView* pView  = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());
    ASSERT(pView);
    CEntryrunDoc* pDoc = pView->GetDocument();
    CRunAplEntry* pRunApl  = pDoc->GetRunApl();
    int iSaveDC = pDC->SaveDC();

    int iIntensity = 3; // 3 = current field
    if( pRunApl->GetCurItemBase() != m_pField )
    {
        const CDictItem* pDictItem = m_pField->GetDictItem();
        iIntensity = (pDictItem == NULL) ? 0 : pRunApl->GetStatus( pDictItem->GetSymbol(),0 );
    }

    COLORREF cColorBg = pView->GetFieldBackgroundColor(iIntensity);
    m_clrBkgnd = cColorBg;
    pDC->FillSolidRect(rect,cColorBg);

    pDC->RestoreDC(iSaveDC);
    return TRUE ;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CDEBaseEdit::PreTranslateMessage(MSG* pMsg)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CDEBaseEdit::PreTranslateMessage(MSG* pMsg)
{

    // RHF INIC Dec 17, 2003 BUCEN_DEC2003 Changes
    bool    bProcess=true;

    if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) {

        CEntryrunView* pParent = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());

        ASSERT(pParent);
        CEntryrunDoc* pRunDoc = pParent->GetDocument();
        CRunAplEntry* pRunApl = pRunDoc ? pRunDoc->GetRunApl() : NULL;

        MSG     localMsg;
        memcpy( &localMsg, pMsg, sizeof(MSG));

        int     iCtrlFlags;
        UINT    nChar = RemapChar( pRunApl, localMsg.wParam, &iCtrlFlags ); // pMsg can be changed because (for example) errmsg send a new message to the application!
        SetFocus();  // BMD 26 Mar 2004
        if( pRunApl->HasSomeRequest() )
        {
            pRunApl->SetProgressForPreEntrySkip(); // 20130415
            pParent->PostMessage(UWM::CSEntry::MoveToField, 0, 0);
        }

        pRunApl->StopIfNecessary(); // 20121023 a stop executed in OnKey was being (temporarily) ignored

        if (pRunApl->HasSpecialFunction(SpecialFunction::OnKey)) {  // BMD 21 Jan 2004
            m_iRemappedKeyChar = nChar;
            m_iRemappedKeyDown = nChar;
        }
        else {
            m_iRemappedKeyChar = -1;
            m_iRemappedKeyDown = -1;
        }

        memcpy( pMsg, &localMsg, sizeof(MSG));
        if( nChar == 0 )
            bProcess = false;
        else if( pMsg->wParam != nChar || iCtrlFlags >= 0 ) {
            pMsg->wParam = nChar;

        }
    }

    if( !bProcess )
        return TRUE;
    // RHF END  Dec 17, 2003 BUCEN_DEC2003 Changes

    return CEdit::PreTranslateMessage(pMsg);

}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEBaseEdit::OnLButtonDown(UINT nFlags, CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////

void CDEBaseEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
    CEntryrunView* pParent = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());
    ASSERT(pParent);
    CEntryrunDoc* pRunDoc = pParent->GetDocument();

    if(pRunDoc->GetAppMode() == VERIFY_MODE){
        return ;
    }

    CEdit::OnLButtonDown(nFlags, point);

    SetFocus();

    GetParent()->PostMessage(UWM::CSEntry::MoveToField, (WPARAM)this, 0);
    GetParent()->PostMessage(WM_IMSA_CSENTRY_REFRESH_DATA);

    return;
}

void CDEBaseEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // TODO: Add your message handler code here and/or call default
    CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}



BOOL CDEBaseEdit::OnHelpInfo(HELPINFO *lpHelpInfo) // 20120405
{
    if( m_bTrapNextHelp )
    {
        m_bTrapNextHelp = false;
        return FALSE;
    }

    else
    {
        return CEdit::OnHelpInfo(lpHelpInfo);
    }
}
