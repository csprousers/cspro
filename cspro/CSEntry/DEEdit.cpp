// CEdit.cpp : implementation file
//

#include "StdAfx.h"
#include "DEEdit.h"
#include "CSEntry.h"
#include "leftprop.h"
#include "LeftView.h"
#include "MainFrm.h"
#include "OperatorStatisticsLog.h"
#include "RunView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDEEdit

IMPLEMENT_DYNAMIC(CDEEdit, CDEBaseEdit)

// RHF INIT Jul 05, 2007. Bug fix decimals in value set
void CDEEdit::SetHasDecimalPoint( bool bHasDecimalPoint ) {
        m_bHasDecimalPoint = bHasDecimalPoint;
}
// RHF END Jul 05, 2007. Bug fix decimals in value set
CDEEdit::CDEEdit()
{
    m_iArabicCaretChar = 0;
    SetHasDecimalPoint(false); // RHF Jul 05, 2007, Bug fix decimals in value set
}

CDEEdit::~CDEEdit()
{
}

BEGIN_MESSAGE_MAP(CDEEdit, CDEBaseEdit)

        //{{AFX_MSG_MAP(CDEEdit)
        ON_WM_PAINT()
        ON_WM_KILLFOCUS()
        ON_WM_ACTIVATE()
        ON_WM_SETFOCUS()
        ON_WM_LBUTTONDOWN()
        ON_WM_ERASEBKGND()
        ON_WM_CREATE()
        ON_WM_KEYDOWN()
        ON_WM_CHAR()

        //}}AFX_MSG_MAP

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDEEdit message handlers
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::OnPaint()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::OnPaint()
{

    CPaintDC dc(this); // device context for painting
    int iSaveDC= dc.SaveDC();
    //  dc.SelectObject(m_brBkgnd);
 /*   CFont font;
    //  VERIFY(font.CreatePointFont(100, "System", &dc));
    VERIFY(font.CreateFont (20, 0, 0, 0, 600, FALSE, FALSE, 0, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH,
        _T("Courier New")));*/

    dc.SelectObject(GetFieldFont());

    // RHF INIC Nov 22, 2002
    bool    bHidden=m_pField->IsHidden();
    if( bHidden ) {
        dc.SetTextColor( m_clrBkgnd );    // text
    }
    else {
    // RHF END Nov 22, 2002
        dc.SetTextColor( m_clrText );    // text
    }

    dc.SetBkColor( m_clrBkgnd );    // text bkgnd

    CRect rect ;
    this->GetClientRect(&rect);
    CSize   sizeChar(0,0);
    this->GetUnits(sizeChar);

    CString sString ;
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();

    ASSERT(pDictItem);
    this->GetWindowText(sString);

    CONTENT_TYPE_REFACTOR::LOOK_AT();
    if(m_pField->GetDictItem()->GetContentType() == ContentType::Alpha && m_pField->GetFont().IsArabic()) {
        // for alpha fields with arabic fonts don't draw tick marks
        // and use single call to textout with whole string to get ligatures
        dc.SetTextAlign(TA_RIGHT | TA_BOTTOM | TA_RTLREADING);
        CIMSAString sTrim(sString);
        sTrim.TrimRight();
        CSize sizeStr = dc.GetTextExtent(sTrim);
        dc.TextOut(rect.right, rect.bottom, sTrim);
    }
    else {
        //  int iLength = sString.GetLength();
        int iLength = pDictItem->GetLen();
        if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
            iLength++; //The length does not account for the decimal character in this case
        }


        for (int iIndex  =0; iIndex < iLength-1; iIndex ++) {
            CRect rectP(rect);
            this->ClientToScreen(rectP);
            this->GetParent()->ScreenToClient(&rectP);
    //        TRACE(_T("%s left = %d , top = %d , bottom =%d ,right =%d \n"),pDictItem->GetLabel(),rectP.left,rectP.top,rectP.bottom,rectP.right);
            /*m_iTickWidth =1;
            if(m_iTickWidth != 1) {
                BOOL bRet = FALSE;
                CPen penLine(PS_SOLID, m_iTickWidth, rgbCellLines);
                CPen* pOldPen = dc.SelectObject(&penLine);
                dc.MoveTo(rect.left + sizeChar.cx * (iIndex+1) +(iIndex+1)*2 +iIndex*SEP_SIZE,rect.bottom);
                bRet = dc.LineTo(rect.left + sizeChar.cx * (iIndex+1)+(iIndex+1)*2+iIndex*SEP_SIZE ,(rect.bottom*3)/4);
                dc.SelectObject(pOldPen);
                ASSERT(bRet);
            }
            else */
            { //use default pen
                dc.MoveTo(rect.left + sizeChar.cx * (iIndex+1) +(iIndex+1)*2 +iIndex*SEP_SIZE,rect.bottom);
                dc.LineTo(rect.left + sizeChar.cx * (iIndex+1)+(iIndex+1)*2+iIndex*SEP_SIZE,(rect.bottom*3)/4);
            }
        }

        //sString = sString.Trim();
        if(!sString.IsEmpty()) {
            for(int iIndex =0 ; iIndex <sString.GetLength(); iIndex++) {
                //Because of the font rendering in VistaOrLater we need to position the character a pixel
                //away otherwise it will overwrite the tick mark.Without this fix it will work
                //on win7 in only compatibility mode
                int iX = rect.left + sizeChar.cx * (iIndex) +(2*iIndex + 1)+iIndex*SEP_SIZE;
                int iY = rect.bottom - sizeChar.cy;
                dc.TextOut(iX,iY,sString.GetAt(iIndex));
            }
        }
    }
    dc.RestoreDC(iSaveDC);

//    font.DeleteObject();
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::OnKillFocus(CWnd* pNewWnd)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::OnKillFocus(CWnd* pNewWnd)
{

    this->HideCaret();
    CWnd::OnKillFocus(pNewWnd);
    DestroyCaret();
    //Pad the decimal character blanks with '0's
   /* if(m_pField && m_pField->GetDictItem() && m_pField->GetDictItem()->GetDecimal() != 0){
        CIMSAString sString;
        GetWindowText(sString);
        sString.Replace(' ','0');
        SetWindowText(sString);
    }*/

    this->RedrawWindow();

}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CWnd::OnActivate(nState, pWndOther, bMinimized);
    switch(nState) {
    case WA_CLICKACTIVE:
    case WA_ACTIVE:
        SetCaret();
        break;
    case WA_INACTIVE:
        this->HideCaret();
        DestroyCaret();
        break;
    default:
        break;

    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::OnSetFocus(CWnd* pOldWnd)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::OnSetFocus(CWnd* pOldWnd)
{
    CWnd::OnSetFocus(pOldWnd);
    SetCaret();
    this->RedrawWindow();
    m_bFirstDecimal = true;

    if( GetCheckFocus() > 0 ) {
        SetCheckFocus( GetCheckFocus()-1 );
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::OnLButtonDown(UINT nFlags, CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
    CDEBaseEdit::OnLButtonDown( nFlags,  point);
}

//static bool    m_bResetSpecialKeys=false;
/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CDEEdit::PreTranslateMessage(MSG* pMsg)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CDEEdit::PreTranslateMessage(MSG* pMsg)
{
    return CDEBaseEdit::PreTranslateMessage(pMsg);
}


BOOL CDEEdit::OnEraseBkgnd(CDC* pDC)
{
    return CDEBaseEdit::OnEraseBkgnd(pDC);
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::GetUnits(CSize& size)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::GetUnits(CSize& size) const
{
    ASSERT_VALID(this);

    if(m_szUnit == CSize(0,0)) {
        CDEEdit* pEdit = const_cast<CDEEdit*>(this);
        ASSERT(pEdit->GetSafeHwnd());

        CClientDC dc(pEdit);

        int iSaveDC = dc.SaveDC();

        dc.SelectObject(m_pField->GetFont().GetCFont());
        // Get the horizontal base units
        size = dc.GetTextExtent(_T("0"),1);
        m_szUnit = size;

        dc.RestoreDC(iSaveDC);
    }
    else {
        size = m_szUnit;
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::CalcCaretPos(CPoint& point)const
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::CalcCaretPos(CPoint& point)const
{
    CSize   sizeChar(0,0);
    GetUnits(sizeChar);

    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);

    ContentType contentType = pDictItem->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT();
    CRect rectClient;
    this->GetClientRect(&rectClient);
    switch(contentType) {
        case ContentType::Numeric:
            if(pDictItem->GetDecimal() == 0) {//if not decimal number
                point.x = rectClient.right-sizeChar.cx ;
                point.y = rectClient.bottom-CARET_HEIGHT;
            }
            else {
                int iLength = pDictItem->GetLen();
                if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
                    iLength++;
                }
                int iLengthBeforeDecimal = iLength - pDictItem->GetDecimal() -1 ; //subtract one for the decimal csprochar
                point.x = rectClient.left + sizeChar.cx * (iLengthBeforeDecimal-1)+ (iLengthBeforeDecimal-1)*SEP_SIZE+ 2*(iLengthBeforeDecimal-1);
                point.y = rectClient.bottom-CARET_HEIGHT;
            }
            break;
        case ContentType::Alpha:
            {
                if(!m_pField->GetFont().IsArabic()) {
                    point.x = rectClient.left;
                    point.y = rectClient.bottom-CARET_HEIGHT;
                }
                else {
                    point.x = rectClient.right - sizeChar.cx;
                    point.y = rectClient.bottom-CARET_HEIGHT;
                }

            }
        default:
            break;
    }
    return;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::SetCaret()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::SetCaret()
{
        CSize   sizeChar(0,0);
        GetUnits(sizeChar);

    sizeChar.cx += 2; //To account for Pixels on either side of the character
        this->CreateSolidCaret(sizeChar.cx,CARET_HEIGHT);

// RHF INIT Jul 05, 2007, Bug fix decimals in value set
        const CDictItem* pDictItem = m_pField ? m_pField->GetDictItem() : NULL;
        int iCursorMovement = 0;// the cursor remains before the point

        if( pDictItem && pDictItem->GetDecimal() > 0 && m_bHasDecimalPoint) {
            CString sString;
            GetWindowText(sString);
            sString.TrimRight();
            int iLen=sString.GetLength();
            int iPos=sString.Find(DECIMAL_CHAR);//'.');
            if( iPos >= 0 ) {
                iCursorMovement = iLen - iPos + 1;
                iCursorMovement--;
            }
        }
// RHF END Jul 05, 2007, Bug fix decimals in value set

        CPoint point(0,0);
        CalcCaretPos(point);
    this->SetCaretPos(point);
        m_iArabicCaretChar = 0;


// RHF INIT Jul 05, 2007, Bug fix decimals in value set
        for( int i=0; i < iCursorMovement; i++ ) {
                AdvanceCaretPos();
        }
// RHF END Jul 05, 2007, Bug fix decimals in value set

        this->ShowCaret();
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::SetCaretSize(int sizeX)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::SetCaretSize(int sizeX)
{
    this->CreateSolidCaret(sizeX,CARET_HEIGHT);
    this->ShowCaret();
}

/////////////////////////////////////////////////////////////////////////////////
//
//      int CDEEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
//
/////////////////////////////////////////////////////////////////////////////////
int CDEEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
        if (CWnd::OnCreate(lpCreateStruct) == -1)
                return -1;

        CRect cRect(lpCreateStruct->x,lpCreateStruct->y,lpCreateStruct->x+lpCreateStruct->cx,lpCreateStruct->y+lpCreateStruct->cy);
        //this->GetWindowRect(cRect);
        if( ComputeRect(cRect) ) {
                this->MoveWindow(&cRect);
                this->GetClientRect(&cRect);
        }
        return 0;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      bool CDEEdit::ComputeRect(CRect& clientRect)
//
/////////////////////////////////////////////////////////////////////////////////
bool CDEEdit::ComputeRect(CRect& clientRect)
{
    bool bRet  = false;
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    int iLength = pDictItem->GetLen();
    if(iLength ==0) {
            return bRet;
    }
    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0) {
        iLength++; //If Decimal character does not go in data file the length does not include the decimal character
    }
    int iX  = clientRect.left;
    int iY =  clientRect.top;

    CSize size(0,0);
    GetUnits(size);

    int iXB = GetSystemMetrics(SM_CXBORDER);
    int iYB = GetSystemMetrics(SM_CYBORDER);

    int iRight = iX + size.cx*iLength + 2*iXB;

    // add space for tick marks for all but alpha fields with arabic font
    CONTENT_TYPE_REFACTOR::LOOK_AT();
    if(pDictItem->GetContentType() != ContentType::Alpha || !m_pField->GetFont().IsArabic()) {
        iRight += (iLength -1)*SEP_SIZE + 2*iLength;
    }
    if(iRight != clientRect.right) {
            bRet  = true;
            clientRect.right = iRight ;
    }
    if(clientRect.bottom - iY  != size.cy){
            clientRect.bottom = clientRect.top + size.cy+ 2*iYB;
            bRet = true;
    }
    return bRet;
}






/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // RHF END  Dec 18, 2003 BUCEN_DEC2003 Changes
    if( m_iRemappedKeyDown == 0 ) {
        m_iRemappedKeyDown = -1;
        return;
    }
    else if( m_iRemappedKeyDown > 0 ) {
        nChar = m_iRemappedKeyDown;
        m_iRemappedKeyDown = -1;
        ResetSpecialKeys(m_bShift,m_bControl,m_bAlt);  // BMD 30 Jan 2004
    }
    // RHF END  Dec 18, 2003 BUCEN_DEC2003 Changes

    CEntryrunView* pParent = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());
    ASSERT(pParent);
    CEntryrunDoc* pRunDoc = pParent->GetDocument();
    CRunAplEntry* pRunApl  = pRunDoc ? pRunDoc->GetRunApl() : NULL;

    if(pRunDoc && pRunDoc->GetOperatorStatisticsLog() && !pRunDoc->GetOperatorStatisticsLog()->GetCurrentStatsObj())
    {
        CString csMode =    ( pRunDoc->GetAppMode() == VERIFY_MODE ) ?  _T("VER") :
                            ( pRunDoc->GetAppMode() == ADD_MODE ) ?     _T("ADD") :
                                                                        _T("MOD");

        pRunDoc->GetOperatorStatisticsLog()->NewStatsObj(csMode,pRunApl->GetOperatorId());
    }

    int iScanCode  =  nFlags & 0xFF;// To get the scan code do this
    if(iScanCode == 78){
        CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
        return;
    }

    bool bArabic = m_pField->GetFont().IsArabic();

    APP_MODE appMode = pRunDoc->GetAppMode();
    if( appMode == VERIFY_MODE && pParent->GetCheatKey() ){
        if( GetKeyState(VK_CONTROL) < 0 ) {
            switch(nChar) {
            case VK_F2:         // BMD 05 Mar 2002
                break;
            case _T('G'):
            {
                CWnd* pWnd = GetFocus();
                if(pWnd == this) {
                    CMainFrame* pFrame = (CMainFrame*) (pParent)->GetParentFrame();
                    //set focus to tree view
                    if(pFrame->GetLeftView()) {
                        int iIndex = pFrame->GetLeftView()->GetPropSheet()->GetActiveIndex();
                        if(iIndex == 0 ) {
                            pFrame->GetCaseView()->SetFocus();
                        }
                        else {
                            pFrame->GetCaseTree()->SetFocus();

                        }
                        return;
                    }
                }
                else {
                    SetFocus();
                    return;
                }
            }

            break;

            default:
                return;
            }
        }
        else {
            return;
        }
    }
    // TODO: Add your message handler code here and/or call default
    if(!IsWindowVisible()){
        CString sString;
        GetWindowText(sString);
        ShowWindow(SW_SHOW);
        CDEGrid* pGrid = pParent->FindGrid(m_pField->GetParent());
        if(pGrid){
            int iOcc = m_pField->GetParent()->GetCurOccurrence();
            int iRow ,iCol;
        //fixed a bug in find field .it expects 1 based and we were passing zero based so iOcc+1
            if(pRunDoc->GetCurField() != m_pField){
                SetWindowText(_T(""));
                this->ShowWindow(SW_HIDE);
                return;
            }
            if(iOcc==0){
                SetWindowText(_T(""));
                this->ShowWindow(SW_HIDE);
                return;
            }
            pGrid->FindField(m_pField, iOcc, &iRow, &iCol);
            pGrid->EnsureVisible(iRow,iCol);
            SetWindowText(sString);
        }
    }


    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);

    CMainFrame* pFrame = (CMainFrame*) (pParent)->GetParentFrame();

    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    ContentType contentType = pDictItem->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT();

    if( GetKeyState(VK_CONTROL) < 0 ) {

        switch(nChar) {
        case  VK_F3:
            if(!pRunDoc->GetCurFormFile()->IsPathOn() && appMode != VERIFY_MODE) {
                pParent->SendMessage(UWM::CSEntry::InsertAfterOccurrence, 0, (LPARAM)this->GetField());
            }
            return;

        case  VK_F12:
            if(!pRunDoc->GetCurFormFile()->IsPathOn()) {
               pFrame->SendMessage(WM_COMMAND,ID_NEXT_LEVEL);
            }
            return;

        case VK_PRIOR :
            if(!pRunDoc->GetCurFormFile()->IsPathOn()) {
                CCaseView* pView = pFrame->GetCaseView();
                CTreeCtrl &treeCtrl = pView->GetTreeCtrl();
                HTREEITEM hItem = treeCtrl.GetSelectedItem();
                hItem = treeCtrl.GetPrevSiblingItem(hItem);
                if(hItem) {
                pFrame->SendMessage(WM_COMMAND,ID_PREV_CASE);
                }
                return;
            }

        case VK_NEXT:
            if(!pRunDoc->GetCurFormFile()->IsPathOn()) {
                CCaseView* pView = pFrame->GetCaseView();
                CTreeCtrl &treeCtrl = pView->GetTreeCtrl();
                HTREEITEM hItem = treeCtrl.GetSelectedItem();
                hItem = treeCtrl.GetNextSiblingItem(hItem);
                if(hItem) {
                pFrame->SendMessage(WM_COMMAND,ID_NEXT_CASE);
                }
                return;
            }

        case VK_HOME:
            if(!pRunDoc->GetCurFormFile()->IsPathOn()) {
                pFrame->SendMessage(WM_COMMAND,ID_FIRST_CASE);
                return;
            }

        case VK_END:
            if(!pRunDoc->GetCurFormFile()->IsPathOn()) {
                pFrame->SendMessage(WM_COMMAND,ID_LAST_CASE);
                return;
            }

        case _T('G'):
            {
                CWnd* pWnd = GetFocus();
                if(pWnd == this) {


                    CMainFrame* pParentFrame = (CMainFrame*) (pParent)->GetParentFrame();
                    //set focus to tree view
                    if(pParentFrame->GetLeftView()) {
                        int iIndex = pParentFrame->GetLeftView()->GetPropSheet()->GetActiveIndex();
                        if(iIndex == 0 ) {
                            pParentFrame->GetCaseView()->SetFocus();
                        }
                        else {
                            pParentFrame->GetCaseTree()->SetFocus();

                        }
                        return;
                    }

                }
                else {
                    SetFocus();
                    return;
                }
            }
            break;

        default: // I do not get the '/' from the nChar So I need to check the scan code

            CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,GetField()->GetParent());
            bool bProcessEndGrp = false;
            if(pRoster){
                bProcessEndGrp = pRoster->UsingFreeMovement();
            }
            if(!pRunDoc->GetCurFormFile()->IsPathOn() || bProcessEndGrp) {

                UINT nScanCode =  nFlags & 0xFF;// To get the scan code do this
                if(nScanCode == 53 && pRunDoc->GetAppMode() != VERIFY_MODE)
                    pParent->SendMessage(UWM::CSEntry::EndGroup, 0, (LPARAM)this->GetField());
            }

            return;
        }
    }


    switch(nChar){

    case VK_F12:
        ((CMainFrame*)AfxGetMainWnd())->SendMessage(WM_COMMAND,ID_NEXT_LEVEL_OCC);
        return;
    case VK_F11:
        ((CMainFrame*)AfxGetMainWnd())->SendMessage(WM_COMMAND,ID_INTEDIT);
        return;
    case VK_F10:
        ((CMainFrame*)AfxGetMainWnd())->SendMessage(WM_COMMAND,ID_ADVTOEND);
        return;
    case  VK_F3:
        pParent->SendMessage(WM_COMMAND,ID_INSERT_GROUPOCC);
        return;
    case  VK_F4:
        pParent->SendMessage(WM_COMMAND,ID_DELETE_GRPOCC);
        return;
    case  VK_F5:
        pParent->SendMessage(WM_COMMAND,ID_SORTGRPOCC);
        return;
    case  VK_F6:
        pFrame->SendMessage(WM_COMMAND,ID_GOTO);
        return;
    case VK_F7:
        pParent->SendMessage(UWM::CSEntry::PreviousPersistent);
        return;
    case VK_DELETE:
        if(contentType == ContentType::Numeric){
            CIMSAString sBlank = _T("");
            sBlank = _T("");
            SetWindowText(sBlank); //remove the text

            //  RedrawWindow();
            if(pDictItem->GetDecimal()!=0){//Move the cursor back to the decimal part
                BackupCaretPos();
            }
        }
        else if (contentType == ContentType::Alpha){
            CString sString;
            GetWindowText(sString);
            CString sTemp(_T(' '),m_pField->GetDictItem()->GetLen()-sString.GetLength());
            sString += sTemp;
            int iCharPos = GetCharFromCaretPos();
            if(bArabic)  {
                //iCharPos = m_pField->GetDictItem()->GetLen() - iCharPos -1;
            }
            if(iCharPos >= 0){
                for(int iIndex = iCharPos ; iIndex < sString.GetLength()-1;iIndex++){
                    sString.SetAt(iIndex,sString.GetAt(iIndex +1));
                }
                sString.SetAt(sString.GetLength()-1,' ');
                SetWindowText(sString);
                // BackupCaretPos();Do not do this acc to Glenn's spec
                //      RedrawWindow();
            }
        }
        SetModifiedFlag(true);


        return;
    case VK_SPACE:
        if(contentType == ContentType::Numeric){
            SetWindowText(_T("")); //remove the text

            //  RedrawWindow();
            if(pDictItem->GetDecimal()!=0){//Move the cursor back to the decimal part
                BackupCaretPos();
            }
        }
        else if (contentType == ContentType::Alpha){
            nChar = _TCHAR(' ');
        }
        SetModifiedFlag(true);
        return;
    case VK_BACK_SPACE:
        {
            CString sString;
            GetWindowText(sString);
            if(contentType == ContentType::Numeric) {
                if(sString.GetLength() && pDictItem->GetDecimal()==0) {
                    CString sTemp(sString,sString.GetLength()-1);
                    SetWindowText(sTemp);
                    //  RedrawWindow();
                }
                else if(sString.GetLength() &&  pDictItem->GetDecimal()!=0) {
                    int iLength = pDictItem->GetLen();
                    if(pDictItem->GetDecimal() != 0 && !pDictItem->GetDecChar()){
                        iLength++;
                    }
                    ASSERT(sString.GetLength() == iLength);

                                        // RHF INIT Jul 05, 2007, Bug fix decimals in value set
                                        CString sStringAux;

                                        sStringAux = sString;
                                        sStringAux.TrimRight();

                                        if( sStringAux.GetLength() > 0 && sStringAux.Right(1) == DECIMAL_CHAR ) // '.' )
                                                SetHasDecimalPoint(false);
                                        // RHF END Jul 05, 2007, Bug fix decimals in value set


                    int iCharPos = GetCharFromCaretPos();
                    if(IsPosBeforeDecimal()){
                        //Is before decimal
                        for(int iIndex=iCharPos ;iIndex >0 ;iIndex--){
                            sString.SetAt(iIndex,sString.GetAt(iIndex-1));
                        }
                        sString.SetAt(0,' ');
                        SetWindowText(sString);
                    }
                    else { //is after decimal
                        int iFind = sString.Find(DECIMAL_CHAR);
                        if (iCharPos == iFind +1 && sString.GetAt(iCharPos) == ' ') {
                            BackupCaretPos();
                        }
                        else if(sString.GetAt(iCharPos) == ' '){
                            sString.SetAt(iCharPos-1,' ');
                            CPoint point = GetCaretPos();
                            CSize sizeChar(0,0);
                            GetUnits(sizeChar);
                            point.x -= sizeChar.cx + SEP_SIZE +2 ; //offset it by the next csprochar size
                            SetCaretPos(point);
                        }
                        else if(sString.GetAt(iCharPos) != ' '){
                            sString.SetAt(iCharPos,' ');
                        }
                        SetWindowText(sString);
                    }
                    //SetWindowText(sTemp);
                    //  RedrawWindow();
                }
            }
            else if(contentType == ContentType::Alpha) {
                int iCharPos = GetCharFromCaretPos();
                if (static_cast<UINT>(iCharPos + 1) == pDictItem->GetCompleteLen() &&
                    static_cast<UINT>(sString.TrimRight().GetLength()) == pDictItem->GetCompleteLen()) {
                    // because the caret remains at the last character (instead of moving on to a
                    // nonexistent tickmark position), delete it instead of processing the backspace
                    OnKeyDown(VK_DELETE, nRepCnt, nFlags);
                }
                else if(iCharPos > 0){
                   // sString.TrimRight();
                    for(int iIndex = iCharPos-1 ; iIndex < sString.GetLength()-1;iIndex++){
                        sString.SetAt(iIndex,sString.GetAt(iIndex +1));
                    }
                    if(sString.GetLength()-1 >=0 ){
                        sString.SetAt(sString.GetLength()-1,' ');
                    }
                    CString sTemp(_T(' '), m_pField->GetDictItem()->GetLen()- sString.GetLength());
                    sString += sTemp;

                    SetWindowText(sString);
                    BackupCaretPos();
                    //  RedrawWindow();
                }
            }
        }
        SetModifiedFlag(true);
        return;
    default:
        break;
    }

    if(contentType == ContentType::Alpha) {
        if((nChar == VK_LEFT && !bArabic) ||
            (nChar == VK_RIGHT && bArabic)){
            m_bRemoveText = false;  // BMD 13 Jan 2004
            int nCharPos = GetCharFromCaretPos();
            if(nCharPos ==0 ) {
                pParent->SendMessage(UWM::CSEntry::ChangeEdit, nChar, (long)this);

            }
            else {
                BackupCaretPos();
            }

        }
        else if((nChar == VK_RIGHT && !bArabic) || (nChar == VK_LEFT && bArabic)) {
            m_bRemoveText = false;  // BMD 13 Jan 2004
            int nCharFromCaretPos = GetCharFromCaretPos();
            if(static_cast<UINT>(nCharFromCaretPos) == m_pField->GetDictItem()->GetLen() -1 ) { // SERPRO Add ()
                pParent->SendMessage(UWM::CSEntry::ChangeEdit, VK_RIGHT, (long)this);
            }
            else {
                AdvanceCaretPos();
            }
        }
        else  if ((nChar == VK_DOWN || nChar == VK_UP || nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_F2 ) &&pParent)  {  // BMD 13 Jan 2004
            pParent->SendMessage(UWM::CSEntry::ChangeEdit, nChar, (long)this);
        }
    }
    // handles only up and down arrows, otherwise they get grabbed before OnChar
    else if ((nChar == VK_DOWN || nChar == VK_UP || nChar == VK_RIGHT || nChar == VK_LEFT|| nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_F2 ) && pParent)  {// RHF Jan 30, 2000
        pParent->SendMessage(UWM::CSEntry::ChangeEdit, nChar, (long)this);
    }
}


void CDEEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
// RHF END  Dec 14, 2003 BUCEN_DEC2003 Changes   Modified BMD 30 Jan 2004
    if( m_iRemappedKeyChar == 0 ) {
        m_iRemappedKeyChar = -1;
        return;
    }
    m_iRemappedKeyChar = -1;

// RHF END  Dec 14, 2003 BUCEN_DEC2003 Changes

    CRunAplEntry* pRunApl = GetRunAplEntry();

    if( pRunApl && pRunApl->HasSpecialFunction(SpecialFunction::OnChar) ) // 20120207
    {
        // don't run this if there is a move pending (this could happen with, for example, chinese, because the user will have entered
        // several characters and then they'll be fed rapid fire to OnChar without the previous move having executed)
        if( pRunApl->HasSomeRequest() )
            return;

        double dNewCharCtl = pRunApl->ExecSpecialFunction(SpecialFunction::OnChar, nChar);

        if( pRunApl->HasSomeRequest() )
        {
            pRunApl->SetProgressForPreEntrySkip(); // 20130415
            CEntryrunView * pParent = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());
            pParent->PostMessage(UWM::CSEntry::MoveToField, 0, 0);
        }

        pRunApl->StopIfNecessary(); // 20121023 a stop executed in OnChar was being (temporarily) ignored

        if( dNewCharCtl == 0 || dNewCharCtl == DEFAULT )
            return;

        nChar = (UINT)dNewCharCtl;
    }


    // TODO: Add your message handler code here and/or call default
    CEntryrunView* pParent = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());
    ASSERT(pParent);

    CEntryrunDoc*     pRunDoc = ((CEntryrunView*)pParent)->GetDocument();
    pRunDoc->AddKeyStroke();

    if(nChar == _T('/')) {
        // int iScanCode  =  nFlags & 0xFF; To get the scan code do this
        int iExt = nFlags & 0x0100;  //To know if it is from and extended key do this
        if (GetParent() && iExt)  {
            CWnd::OnChar(nChar, nRepCnt, nFlags);
            GetParent()->SendMessage(UWM::CSEntry::SlashKey, nChar, (LPARAM)this->GetField());
            return;
        }
    }
    int iScanCode  =  nFlags & 0xFF;// To get the scan code do this
    if(iScanCode == 78 && !pRunDoc->GetRunApl()->IsPathOn()){
        CWnd::OnChar(nChar, nRepCnt, nFlags);
        if(pRunDoc->GetAppMode() == VERIFY_MODE && pParent->GetCheatKey() ) {
            return;
        }
        else {
            GetParent()->SendMessage(UWM::CSEntry::PlusKey, (WPARAM)this, (LPARAM)this->GetField());
            return;
        }
    }

    if(pRunDoc->GetAppMode() == VERIFY_MODE && pParent->GetCheatKey() ){
        switch(nChar) {
        case VK_F2:
            break;
        default:
            return;
        }
    }
    CMainFrame* pFrame = (CMainFrame*)pParent->GetParentFrame();

    pFrame->Start();

    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    ContentType contentType = pDictItem->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT();
    CString sString;
    bool bIsValidChar = IsValidChar(nChar);

    GetWindowText(sString);

    int iCharPos = -1;
    int iFieldLength = pDictItem->GetLen();
    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0) {//if the decimal is not accounted
        iFieldLength++;
    }

    if(bIsValidChar){
        //Left Shift Character positions
        SetModifiedFlag(true);
        ProcessCharKey(nChar);
        if(contentType == ContentType::Alpha) {
            iCharPos = GetCharFromCaretPos(); //Get Before advancing
            AdvanceCaretPos();
        }
        else if(contentType == ContentType::Numeric && pDictItem->GetDecimal() !=0) {

            if(this->IsPosBeforeDecimal()) { //Do not advance to decimal portion if non decimal part is not filled
                CString csString;
                GetWindowText(csString);
                CString sNDecimal = this->GetNonDecimalPart(csString);
                bool bAutoSlide = false; //SAVY&&&
                if((nChar== DECIMAL_CHAR) || (static_cast<UINT>(sNDecimal.GetLength()) == iFieldLength - pDictItem->GetDecimal() -1 && bAutoSlide)) {
                    AdvanceCaretPos();
                                        SetHasDecimalPoint( true ); // RHF Jul 05, 2007, Bug fix decimals in value set
                }
            }
            else { //caret in the decimal part
                iCharPos = GetCharFromCaretPos(); //Get Before advancing
                AdvanceCaretPos();
            }
        }
    }
    else {
        if(m_bRemoveText) {
            m_bRemoveText = false;
        }
    }

    CWnd::OnChar(nChar, nRepCnt, nFlags);




    if (bIsValidChar || nChar == VK_BACK) {
        CString csString;
        GetWindowText(csString);
        bool bFldDone = false;
        if(contentType == ContentType::Numeric && pDictItem->GetDecimal() == 0){
            csString.TrimLeft(); // only for numeric
            bFldDone = (static_cast<UINT>(csString.GetLength()) == pDictItem->GetLen());

        }
        if(contentType == ContentType::Numeric && pDictItem->GetDecimal() != 0){
            csString.TrimRight(); // only for decimal
            int iLength = pDictItem->GetLen();
            if(pDictItem->GetDecimal() !=0 && !pDictItem->GetDecChar()){
                iLength ++ ; //Account for decimal character
            }
            if(nChar == VK_BACK){
                csString.TrimLeft();
            }
            bFldDone =  csString.GetLength() == iLength;

        }
        else if(contentType == ContentType::Alpha){
            bFldDone = (static_cast<UINT>(iCharPos) == pDictItem->GetLen()-1);
        }


        if(bFldDone  && !m_pField->IsEnterKeyRequired())
            GetParent()->SendMessage(UWM::CSEntry::ChangeEdit, VK_RETURN, (long)this);


    }
    else if (nChar == VK_ESCAPE || nChar == VK_RETURN || nChar == VK_TAB || nChar == VK_UP || nChar == VK_RIGHT ||
        nChar == VK_LEFT || nChar == VK_DOWN || nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_F2  ) {

        if (GetParent())  {
            GetParent()->SendMessage(UWM::CSEntry::ChangeEdit, nChar, (long)this);
        }
        else  {
            //            CEdit::OnChar(nChar, uRepCnt, nFlags);
        }
    }
    else {

        if(pRunDoc) {
            pRunDoc->AddEntryError();
        }
        ::MessageBeep(0);

    }
}


bool CDEEdit::IsValidChar(UINT nChar) const
{
    bool bRet = false;
    if(m_bRemoveText && nChar > 32 && (csprochar)nChar != _T('/')) {
        CDEEdit* pEdit = const_cast<CDEEdit*>(this);
        pEdit->SetWindowText(_T(""));
        pEdit->m_bRemoveText =false;
    }
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);

    ContentType contentType = pDictItem->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT();

    CString sString;
    GetWindowText(sString);

    int iLength = pDictItem->GetLen();
    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
        iLength++; //Account for the decimal csprochar
    }

    if(contentType == ContentType::Numeric) {
        bool bSigned = true; //always true for numeric
        bool bDecimal = pDictItem->GetDecimal() != 0;

        sString.TrimLeft();
        sString.TrimRight();

        if(_TCHAR(nChar) >= _T('0') && _TCHAR(nChar) <= '9') {
            bRet = true;
            if(pDictItem->GetDecimal() != 0 ){
                CString sWindowText;
                GetWindowText(sWindowText);
                CString sNDecimal = GetNonDecimalPart(sWindowText);
                CString sDecimal = GetDecimalPart(sWindowText);
                bool bNDFilled = (static_cast<UINT>(sNDecimal.GetLength()) == iLength - pDictItem->GetDecimal() -1);
                bool bDFilled = (static_cast<UINT>(sDecimal.GetLength()) == pDictItem->GetDecimal());

                bool bAutoslide = false; //SAVY&&& get from field
                if(IsPosBeforeDecimal() && !bAutoslide && bNDFilled ){
                    bRet = false;
                }
                else if(!IsPosBeforeDecimal() && m_pField->IsEnterKeyRequired() && bDFilled ) {
                    bRet = false;
                }

            }
            else if(sString.GetLength() == iLength && m_pField->IsEnterKeyRequired()){
                bRet = false;
            }
        }
        else if(bDecimal && _TCHAR(nChar) == DECIMAL_CHAR ) {
            if(m_bFirstDecimal){
                CDEEdit* pEdit = const_cast<CDEEdit*>(this);
                pEdit->m_bFirstDecimal = false;
                bRet = true;
            }
        }
        else if(bSigned && _TCHAR(nChar) == '-' ){
            sString.Remove(DECIMAL_CHAR);
            if(sString.IsEmpty() || (sString.GetLength()==iLength)){
                bRet = true;
            }
        }

    }
    else if (contentType == ContentType::Alpha){
        if(nChar >=32){
          bRet = true;
        }
        if(m_pField->IsEnterKeyRequired() ){
            sString.TrimRight();
            if(sString.GetLength() == iLength){
                bRet =false;
            }
        }
    }
    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::ProcessCharKey(UINT& nChar)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::ProcessCharKey(UINT& nChar)
{
    //if the current text is all filled remove everything and place this csprochar
    //right justified for numeric nonw decimal fields
    ASSERT(m_pField); //always has to have a dict item
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);

    int iLength = pDictItem->GetLen();
    if(pDictItem->GetDecimal() !=0 && !pDictItem->GetDecChar()){
        iLength++;
    }

    CString sString;
    GetWindowText(sString);
    ContentType contentType = m_pField->GetDictItem()->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT();
    if(contentType == ContentType::Numeric) {
        int iDecimal = pDictItem->GetDecimal();
        if (iDecimal ==0 ) {
            sString.TrimLeft();
            sString.TrimRight();
            if(sString.GetLength() == iLength) {
                sString = _T(""); //remove the text to start with a fresh one
            }
            sString += _TCHAR(nChar);
            CString sTemp(_T(' '),iLength - sString.GetLength());
            sTemp += sString;
            SetWindowText(sTemp);
            //  this->RedrawWindow();
            return;
        }
        else {
            if(nChar == DECIMAL_CHAR){
                return;
            }
            sString.TrimLeft();
            sString.TrimRight();
            if(sString.GetLength() == iLength) {
                sString = _T(""); //remove the text to start with a fresh one
                BackupCaretPos();
            }
            //Get the CharPos from caret
            if(IsPosBeforeDecimal()){
                CString sNonDecimal = GetNonDecimalPart(sString);
                CString sDecimal =  GetDecimalPart(sString);
                sNonDecimal.TrimLeft(); sNonDecimal.TrimRight();
                sNonDecimal += _TCHAR(nChar);
                sString = MakeDecNumString(sNonDecimal,sDecimal);
                sString.GetLength();
                SetWindowText(sString);
            }
            else {
                CString sNonDecimal = GetNonDecimalPart(sString);
                CString sDecimal =  GetDecimalPart(sString);
                sDecimal.TrimLeft(); sDecimal.TrimRight();
                sDecimal += _TCHAR(nChar);
                sString = MakeDecNumString(sNonDecimal,sDecimal);
                SetWindowText(sString);
            }
            //if it is before the decimal and the characters are not filled push left the integral portion
            //the decimal portion
        }
    }
    else  if(contentType == ContentType::Alpha) {
        // gsf 8-mar-01:  support upper case attribute
        if (m_pField->IsUpperCase()) {
            nChar = std::toupper(nChar);
        }

      //WE NEED A FLAG   sString.TrimRight(); //Check this SAVY&&& 10/03/00
       /* if(sString.GetLength() == iLength) {
            sString = ""; //remove the text to start with a fresh one
        }*/ //Check this SAVY&&& 10/03/00
        if(m_bRemoveText) {
            sString = _T("");
            m_bRemoveText = false;
        }
        CString sTemp(_T(' '),iLength - sString.GetLength());
        sString += sTemp;

        int iChar = GetCharFromCaretPos();
        ASSERT(iChar >= 0);
        sString.SetAt(iChar, (TCHAR)nChar);    // gsf 8-mar-01
        SetWindowText(sString);
        return;
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CDEEdit::Create(DWORD dwStyle, CRect rect, CWnd* pParent ,UINT  nID)
//
/////////////////////////////////////////////////////////////////////////////////

BOOL CDEEdit::Create(DWORD dwStyle, CRect rect, CWnd* pParent ,UINT  nID)
{
    UNREFERENCED_PARAMETER(dwStyle);
    return      CWnd::Create(NULL,_T(""),WS_VISIBLE|WS_BORDER|WS_CHILD,rect,pParent,nID);
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void  CDEEdit::SetWindowText(const CString& sString)
//
/////////////////////////////////////////////////////////////////////////////////
void  CDEEdit::SetWindowText(const CString& sString)
{
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    int iLength = pDictItem->GetLen();
    ContentType contentType = pDictItem->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT();

    if(pDictItem->GetDecimal() !=0 && !pDictItem->GetDecChar()){
        iLength++;
    }

    CString sText(sString);
    int iStringLength = sText.GetLength() ;
    if(iStringLength != iLength) {
        if(contentType == ContentType::Numeric) {
            //Pad the string with left blanks if the length is not equal to iLength
            if(pDictItem->GetDecimal() == 0 ) { //if the number is not decimal
                CString sTemp;
                if( iLength > iStringLength )
                {
                    CString sTemp2(_T(' '),iLength-iStringLength);
                    sTemp = sTemp2;
                }

                sTemp += sText;
                sText = sTemp;
            }
            else { //it is decimal

                // 20120312 in the case that this field is being initially populated, swap the period with a comma
                if( DECIMAL_CHAR == _T(',') )
                    sText.Replace(_T('.'),_T(','));

                PadBlanksToDecimal(sText);
            }
        }

        else if (contentType == ContentType::Alpha){
            //Pad the string with right blanks if the length is not equal to iLength
            CString sTemp(_T(' '), iLength-sString.GetLength());
            if(!m_pField->GetFont().IsArabic()) {
                sText += sTemp;
            }
            else {
                sTemp += sText;
                sText = sTemp;
            }
        }
    }

    if(pDictItem->GetDecimal()!=0){
        int iPos = iLength - pDictItem->GetDecimal() -1;
        sText.SetAt(iPos,DECIMAL_CHAR);
    }

    // RHF INIC Jan 09, 2003
    CString rString;
    GetWindowText(rString);

    // RHF END Jan 09, 2003

    CWnd::SetWindowText(sText);




    // RHF INIC Jan 10, 2003
    if( rString != sText )
        PostMessage(UWM::CSEntry::RefreshSelected);// RHF Jan 09, 2003
    // RHF END Jan 10, 2003

    Invalidate(); //SAVY&&& 10/03/00 for now

}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::AdvanceCaretPos()
//
//  Works for Alpha later on will extend to decimal . Should not use for numeric
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::AdvanceCaretPos()
{
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    ContentType contentType = pDictItem->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT();

    bool bDecimal = pDictItem->GetDecimal() != 0 && contentType == ContentType::Numeric;
    CSize   sizeChar(0,0);
    GetUnits(sizeChar);

    CRect rectClient;
    this->GetClientRect(&rectClient);
    CPoint point = GetCaretPos();

    if(contentType == ContentType::Alpha) {
        if(!m_pField->GetFont().IsArabic()) {
            point.x += sizeChar.cx + SEP_SIZE +2 ; //offset it by the next csprochar size and two pixels on either side of the csprochar
            if(rectClient.PtInRect(point)){
                this->SetCaretPos(point);
            }
            else {
                //ASSERT(FALSE);
            }
        }
        else {
            m_iArabicCaretChar = std::min(m_iArabicCaretChar+1, (int) pDictItem->GetLen()-1);
            int iCaretSize;
            CPoint newPos = GetCaretPosFromChar(m_iArabicCaretChar, iCaretSize);
            SetCaretSize(iCaretSize);
            this->SetCaretPos(newPos);
        }
    }
    else if(bDecimal) {
        //Check if we are still in the non decimal portion
        //if so go to the portion after the decimal
        if(!IsPosBeforeDecimal()) {
            point.x += sizeChar.cx + SEP_SIZE +2 ; //offset it by the next csprochar size and two pixels on either side of the csprochar
            if(rectClient.PtInRect(point)){
                this->SetCaretPos(point);
            }
        }
        else {
            point.x += (sizeChar.cx + SEP_SIZE +2)*2  ; //offset it by the next csprochar size and two pixels on either side of the csprochar
            //offset to skip the decimal
            if(rectClient.PtInRect(point)){
                this->SetCaretPos(point);
            }
            int iPos = GetCharFromCaretPos();
            int iLength = pDictItem->GetLen();
            if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
                //take into account of the decimal csprochar in the length
                iLength++;
            }
            if (static_cast<UINT>(iPos) > iLength - pDictItem->GetDecimal()){
                m_bFirstDecimal = false;
            }
        }

    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::BackupCaretPos()
//
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::BackupCaretPos()
{
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    ContentType contentType = pDictItem->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT();

    if(contentType == ContentType::Alpha) {
        CSize   sizeChar(0,0);
        GetUnits(sizeChar);

        CRect rectClient;
        this->GetClientRect(&rectClient);
        CPoint point = GetCaretPos();

        if(!m_pField->GetFont().IsArabic()) {
            point.x -= sizeChar.cx + SEP_SIZE +2 ; //offset it by the next csprochar size
            if(rectClient.PtInRect(point)){
                this->SetCaretPos(point);
            }
            else {
                //ASSERT(FALSE);
            }
        }
        else {
            m_iArabicCaretChar = std::max(m_iArabicCaretChar - 1, 0);
            int iCaretSize;
            CPoint newPos = GetCaretPosFromChar(m_iArabicCaretChar, iCaretSize);
            SetCaretSize(iCaretSize);
            this->SetCaretPos(newPos);
        }
    }
    else if (pDictItem->GetDecimal() !=0){
                SetHasDecimalPoint( false ); // RHF Jul 05, 2007, Bug fix decimals in value set
        m_bFirstDecimal = true; //reset the flag
        CPoint point(0,0);
        CalcCaretPos(point);
        this->SetCaretPos(point);
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      int CDEEdit::GetCharFromCaretPos(void)
//
// used only  for alpha fields
/////////////////////////////////////////////////////////////////////////////////
int CDEEdit::GetCharFromCaretPos(void) const
{
    CRect rectClient;
    this->GetClientRect(&rectClient);

    CSize sizeChar(0,0);
    GetUnits(sizeChar);
    CPoint caretPos = GetCaretPos();

    int iChar = 0;
    CONTENT_TYPE_REFACTOR::LOOK_AT();
    if(m_pField->GetDictItem()->GetContentType() == ContentType::Numeric || !m_pField->GetFont().IsArabic()) {
        iChar = (caretPos.x - rectClient.left)/(sizeChar.cx +SEP_SIZE+2);
    }
    else {
        iChar = m_iArabicCaretChar;
    }

#ifndef _DEBUG_     //check in the debug version
    const CDictItem* pDictItem = m_pField->GetDictItem();
    int iLength = m_pField->GetDictItem()->GetLen();
     if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
        //Decimal csprochar is not accounted for in the length
        iLength++;
    }
    ASSERT(iChar < iLength);
#endif
    return iChar ;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::GetCaretPosForChar(int iChar, int& iCaretSizeX) const
// Get caret pos for a character in the field given its index in the string.
// With Arabic this is not simply based on the index since some characters
// have different widths e.g. lamb-alof (hit "b" on keyboard)
// Also return appropriate caret size - half width cursor if over
// one of these half width characters.
/////////////////////////////////////////////////////////////////////////////////
CPoint CDEEdit::GetCaretPosFromChar(int iChar, int& iCaretSizeX) const
{
    CPoint point = GetCaretPos();
    point.x = GetCaretPosXForChar(iChar, iCaretSizeX);
    return point;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::GetCaretPosXForChar(int iChar, int& iCaretSizeX) const
// Get caret pos for a character in the field given its index in the string.
// With Arabic this is not simply based on the index since some characters
// have different widths e.g. lamb-alof (hit "b" on keyboard)
// Also return appropriate caret size - half width cursor if over
// one of these half width characters.
/////////////////////////////////////////////////////////////////////////////////
int CDEEdit::GetCaretPosXForChar(int iChar, int& iCaretSizeX) const
{
    // get current text
    CIMSAString sString;
    GetWindowText(sString);

    // trim off whitespace at end of string
    sString.TrimRight();
    const int iStrLen = sString.GetLength();

    CRect r;
    GetClientRect(&r);
    CSize   sizeChar(0,0);
    GetUnits(sizeChar);

    // default caret size - will change below if over half-width csprochar
    iCaretSizeX = sizeChar.cx + 2;

    if (iChar == 0) {
        // start of string is just right edge of edit control
        // offset one caret width
        return r.right - sizeChar.cx;
    }

    // setup device context for drawing arabic text
    CClientDC dc(const_cast<CDEEdit*>(this));
    dc.SetTextAlign(TA_RTLREADING | TA_BOTTOM | TA_RTLREADING);
    dc.SelectObject(m_pFont);

    // past edge of string - find left edge of string (subtract
    // string extent from right edge of edt ctrl) and offset
    // by num characters past edge of string (ok to use fixed width
    // here since all characters past edge of string are spaces.
    if (iChar > iStrLen) {
        CSize sizeStr = dc.GetTextExtent(sString);
        return r.right - sizeStr.cx - (iChar - iStrLen + 1) * sizeChar.cx;
    }

    // use GetCharacterPlacement to classify characters as arabic or latin (gcpRes.lpClass)
    // and get caret positions for each character (gcpRes.lpCaretPos)
    GCP_RESULTS gcpRes;
    memset(&gcpRes, 0, sizeof(gcpRes));
    gcpRes.lStructSize = sizeof(gcpRes);
    gcpRes.lpClass = new char[iStrLen];
    for (int j = 0; j < iStrLen; ++j) {
        gcpRes.lpClass[j] = 0;
    }
    gcpRes.lpCaretPos = new int[iStrLen];
    gcpRes.nGlyphs = iStrLen;

    DWORD iFontInfo = GetFontLanguageInfo(dc);

    GetCharacterPlacement(dc, sString, iStrLen, 0, &gcpRes, (iFontInfo & FLI_MASK));

    // unfortunately GetCharacterPlacement is buggy - results for lpCaretPos
    // are often incorrect.  Can however rely on it to give same caret pos
    // for two arabic characters in a row when those two characters make up
    // a single glyph on the screen (the lamb-alof case, b on the keyboard)
    // Use fixed width character spacing for everything but the above case
    // to calculate correct caret pos.  Do computing offset of caret from
    // right edge of string as we walk along string.
    int i = 0;
    float fDistFromRightEdge = 0;

    // if we are over the edge of the string by 1, look for last character
    // in string
    int iSearchChar = (iChar == iStrLen) ? iStrLen - 1 : iChar;
    ASSERT(iSearchChar < iStrLen);


    while (i <= iSearchChar) {
        if (gcpRes.lpClass[i] == GCPCLASS_ARABIC) {

            if ((i < iStrLen-1) &&
                (gcpRes.lpClass[i+1] == GCPCLASS_ARABIC) &&
                (gcpRes.lpCaretPos[i] == gcpRes.lpCaretPos[i+1])) {
                // two characters mapped to single glyph case
                fDistFromRightEdge += static_cast<float>(sizeChar.cx/2.0);
            }
            else if ((i > 0) &&
                (gcpRes.lpClass[i-1] == GCPCLASS_ARABIC) &&
                (gcpRes.lpCaretPos[i] == gcpRes.lpCaretPos[i-1])) {
                fDistFromRightEdge += static_cast<float>(sizeChar.cx/2.0);
                // two characters mapped to single glyph case
            }
            else {
                fDistFromRightEdge += sizeChar.cx;
                // regular fixed width case
            }
            ++i;
        }
        else {
            // have to reverse latin characters since they go left to
            // right instead of right to left.

            // Find last latin character in sequence.
            int j = i;
            for (j = i; j < iStrLen && gcpRes.lpClass[j] != GCPCLASS_ARABIC; ++j) {
            }

            if (iSearchChar < j) {
                // search is in this sequnce of latin chars, compute offset
                // from end of sequence
                fDistFromRightEdge += (j - iSearchChar) * sizeChar.cx;
            }
            else {
                // search csprochar not in the sequence, update offset and continue search
                fDistFromRightEdge += (j - i) * sizeChar.cx;
            }
            i = j;
        }
    }

    if (iChar == iStrLen && gcpRes.lpClass[iStrLen-1] == GCPCLASS_ARABIC) {
        // if csprochar we are looking for is one past edge of string then
        // offset by one csprochar length for arabic and leave alone for latin.
        // This places caret at left of new text when typing arabic and at
        // right of new text when typing latin
        fDistFromRightEdge += sizeChar.cx;
    }

    // check for half width characters and update caret size
    if ((iChar < iStrLen-1) &&
        (gcpRes.lpClass[iChar+1] == GCPCLASS_ARABIC) &&
        (gcpRes.lpCaretPos[iChar] == gcpRes.lpCaretPos[iChar+1])) {
        iCaretSizeX = iCaretSizeX/2;
    }
    else if ((iChar > 0) &&
        (gcpRes.lpClass[iChar-1] == GCPCLASS_ARABIC) &&
        (gcpRes.lpCaretPos[iChar] == gcpRes.lpCaretPos[iChar-1])) {
        iCaretSizeX = iCaretSizeX/2;
    }

    delete [] gcpRes.lpClass;
    delete [] gcpRes.lpCaretPos;

    return r.right - int(0.5 + fDistFromRightEdge);
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEEdit::PadBlanksToDecimal(CString& sString)
// Valid for decimal numbers
/////////////////////////////////////////////////////////////////////////////////
void CDEEdit::PadBlanksToDecimal(CString& sString)
{
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    int iDigitsAfterDecimal = pDictItem->GetDecimal();
    if(iDigitsAfterDecimal==0 ) //This is not a decimal csprochar
        return;

   // sString.TrimLeft();
   // sString.TrimRight();

    int iLength = pDictItem->GetLen();
    BOOL bDecChar = pDictItem->GetDecChar();
    if(!bDecChar  && pDictItem->GetDecimal() != 0){
        //Decimal csprochar is not accounted for in the length
        iLength++;
    }
    if(!sString.IsEmpty() && !bDecChar){
        //Get Decimal Char from the system metrics
        int iPos = sString.Find(DECIMAL_CHAR); // '.');
        if(iPos == -1) {
                        if(sString.GetLength() != iLength-1){
                                int iSpaces = iLength-sString.GetLength()-1;
                                CString sTemp2(_T(' '),iSpaces);
                                sString = sTemp2 +sString;    //add blanks to the left
                        }
            CString sNDecimal(sString.Left(iLength - pDictItem->GetDecimal()-1));
            CString sDecimal(sString.Right(pDictItem->GetDecimal()));
            sString = sNDecimal + DECIMAL_CHAR + sDecimal; //_T(".") + sDecimal;
            return;
        }
    }
    CString sBeforeDecimal = GetNonDecimalPart(sString);
    CString sAfterDecimal = GetDecimalPart(sString);
    sString = MakeDecNumString(sBeforeDecimal,sAfterDecimal);
}
/////////////////////////////////////////////////////////////////////////////////
//
//      CString CDEEdit::GetDecimalPart(const CString& sString)
//
/////////////////////////////////////////////////////////////////////////////////
CString CDEEdit::GetDecimalPart(const CString& sString)const
{
    CString sAfterDecimal;
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    int iDigitsAfterDecimal = pDictItem->GetDecimal();
    if(iDigitsAfterDecimal==0 ) //This is not a decimal csprochar
        return sAfterDecimal;
    int iPos = sString.Find(DECIMAL_CHAR);
    if(iPos !=-1)  {//Decimal found
        sAfterDecimal = sString;
        sAfterDecimal.Delete(0,iPos+1);
    }
    sAfterDecimal.TrimLeft();
    sAfterDecimal.TrimRight();

    return sAfterDecimal;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      CString CDEEdit::GetNonDecimalPart(const CString& sString)
//
/////////////////////////////////////////////////////////////////////////////////
CString CDEEdit::GetNonDecimalPart(const CString& sString)const
{
    CString sBeforeDecimal;
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    int iDigitsAfterDecimal = pDictItem->GetDecimal();
    if(iDigitsAfterDecimal==0 ) //This is not a decimal csprochar
        return sBeforeDecimal;
    int iStringLength = sString.GetLength();
    int iPos = sString.Find(DECIMAL_CHAR);
    if(iPos !=-1)  {//Decimal found
        sBeforeDecimal = sString;
        sBeforeDecimal.Delete(iPos,iStringLength -iPos);
    }
    else {
        sBeforeDecimal = sString;
    }
    sBeforeDecimal.TrimLeft();
    sBeforeDecimal.TrimRight();

    return sBeforeDecimal;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      bool CDEEdit::IsPosBeforeDecimal()
//
/////////////////////////////////////////////////////////////////////////////////
bool CDEEdit::IsPosBeforeDecimal()const
{
    bool bRet = false;
    int iPos = this->GetCharFromCaretPos();
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);

    int iLength = pDictItem->GetLen();
    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
        //Decimal csprochar is not accounted for in the length
        iLength++;
    }
    if(static_cast<UINT>(iPos) == iLength - pDictItem->GetDecimal() -2) {
        bRet = true;
    }
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      CString CDEEdit::MakeDecNumString(const CString& sNonDecimal, const CString & sDecimal)
//
/////////////////////////////////////////////////////////////////////////////////
CString CDEEdit::MakeDecNumString(const CString& sNonDecimal, const CString & sDecimal)const
{
    CString sRet;
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);

    CString sLNDecimal(sNonDecimal);
    CString sLDecimal(sDecimal);

    int iLength = pDictItem->GetLen();
    int iDigitsAfterDecimal = pDictItem->GetDecimal();

    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
        //Decimal csprochar is not accounted for in the length
        iLength++;
    }

     //Pad the characters before decimal
    int iSpaces = iLength-iDigitsAfterDecimal-1 - sLNDecimal.GetLength(); //subtract one for the decimal character
    CString sTemp2(_T(' '),iSpaces);
    sLNDecimal = sTemp2 +sLNDecimal;
    //Pad the characters after decimal


                int iLen= std::max(iDigitsAfterDecimal-sLDecimal.GetLength(),0);
                CString sTemp1(_T(' '),iLen);

                //CString sTemp1(' ',iDigitsAfterDecimal-sLDecimal.GetLength());

    sLDecimal += sTemp1;
    //patch the number
    sRet = sLNDecimal+ CString(DECIMAL_CHAR) +sLDecimal;
    int iRetLen = sRet.GetLength();
    ASSERT( iRetLen == iLength);
    return sRet;
}


// RHF END Dec 15, 2003 BUCEN_DEC2003 Changes
