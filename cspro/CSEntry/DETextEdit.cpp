#include "StdAfx.h"
#include "DETextEdit.h"
#include "leftprop.h"
#include "LeftView.h"
#include "MainFrm.h"
#include "OperatorStatisticsLog.h"
#include "RunView.h"


IMPLEMENT_DYNAMIC(CDETextEdit, CDEBaseEdit)

BEGIN_MESSAGE_MAP(CDETextEdit, CDEBaseEdit)
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    ON_WM_ACTIVATE()
    ON_WM_SETFOCUS()
    ON_WM_CREATE()
    ON_WM_ERASEBKGND()
    ON_WM_KILLFOCUS()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


CDETextEdit::CDETextEdit()
{
}


CDETextEdit::~CDETextEdit()
{
}


// CDETextEdit message handlers

void CDETextEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
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

    CPoint caretPoint = GetCaretPos();
    int nCharPos = CharFromPos(caretPoint);
    if(nChar != VK_TAB){//eat the tab key in unicode edit
        CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
    }

    CMainFrame* pFrame = (CMainFrame*) (pParent)->GetParentFrame();

    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);
    ContentType contentType = pDictItem->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");

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
        //case VK_UP://return if the ctl+up/down to do the default of the edit control( which is changing the lines)
        //  LineScroll(-1);
        //  return;
        //case VK_DOWN:
        //  LineScroll(1);
        //  return;
        case VK_RETURN:
            return;
            break;
        default: // I do not get the '/' from the nChar So I need to check the scan code

            CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,GetField()->GetParent());
            bool bProcessEndGrp = false;
            if(pRoster){
                bProcessEndGrp = pRoster->UsingFreeMovement();
            }
            if(!pRunDoc->GetCurFormFile()->IsPathOn() || bProcessEndGrp) {

                int iScanCode  =  nFlags & 0xFF;// To get the scan code do this
                if(iScanCode == 53 && pRunDoc->GetAppMode() != VERIFY_MODE)
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
    case VK_SPACE:
        {
            CPoint caretPoint = GetCaretPos();
            int nCharPos = CharFromPos(caretPoint);

            int nLineIndex = HIWORD(nCharPos);
            int nCharIndex = LOWORD(nCharPos);

            if(nCharIndex  == m_pField->GetDictItem()->GetLen() - 1) { // SERPRO Add ()
                pParent->SendMessage(UWM::CSEntry::ChangeEdit, VK_RIGHT, (long)this);
            }
        }
        SetModifiedFlag(true);
        return;
    case VK_DELETE:
    case VK_BACK_SPACE:
        //OnKeydown is already called. Just set the modified flag
        SetModifiedFlag(true);
        return;
    case VK_END:
        m_bRemoveText = false;
        break;
    default:
        break;
    }

    if(contentType == ContentType::Alpha) {
        if(nChar == VK_LEFT){
            m_bRemoveText = false;  // BMD 13 Jan 2004

            if(nCharPos ==0 ) {
                pParent->SendMessage(UWM::CSEntry::ChangeEdit, nChar, (long)this);
            }
        }

        else if(nChar == VK_RIGHT) {
            m_bRemoveText = false;  // BMD 13 Jan 2004

            CPoint caretPoint = GetCaretPos();
            int nCharPos = CharFromPos(caretPoint);

            int nLineIndex = HIWORD(nCharPos);
            int nCharIndex = LOWORD(nCharPos);

            if(nCharIndex == m_pField->GetDictItem()->GetLen() ) { // SERPRO Add ()
                pParent->SendMessage(UWM::CSEntry::ChangeEdit, VK_RIGHT, (long)this);
            }
        }

        else  if ((nChar == VK_DOWN || nChar == VK_UP || nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_F2 || nChar == VK_TAB) &&pParent)  {  // BMD 13 Jan 2004
            int nLineIndex = HIWORD(nCharPos);
            int nCharIndex = LOWORD(nCharPos);
            int nLineCount = GetLineCount();
            if((nLineIndex > 0 && nChar == VK_UP) ||(nLineIndex + 1 < nLineCount && nChar == VK_DOWN)){//go the next line
                m_bRemoveText = false;
                return;
            }
            pParent->SendMessage(UWM::CSEntry::ChangeEdit, nChar, (long)this);
        }
    }

    // handles only up and down arrows, otherwise they get grabbed before OnChar
    else if ((nChar == VK_DOWN || nChar == VK_UP || nChar == VK_RIGHT || nChar == VK_LEFT|| nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_F2  ) && pParent)  {// RHF Jan 30, 2000
        pParent->SendMessage(UWM::CSEntry::ChangeEdit, nChar, (long)this);
    }
}


void CDETextEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
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
            CDEBaseEdit::OnChar(nChar, nRepCnt, nFlags);
            GetParent()->SendMessage(UWM::CSEntry::SlashKey, nChar, (LPARAM)this->GetField());
            return;
        }
    }
    int iScanCode  =  nFlags & 0xFF;// To get the scan code do this
    if(iScanCode == 78 && !pRunDoc->GetRunApl()->IsPathOn()){
        CDEBaseEdit::OnChar(nChar, nRepCnt, nFlags);
                if(pRunDoc->GetAppMode() == VERIFY_MODE && pParent->GetCheatKey() ){
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
    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
    CString sString;
    bool bIsValidChar = IsValidChar(nChar);

    GetWindowText(sString);

    //int iCharPos = -1;
    int iFieldLength = pDictItem->GetLen();
    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0) {//if the decimal is not accounted
        iFieldLength++;
    }

    if(bIsValidChar){
        //Left Shift Character positions
        SetModifiedFlag(true);
        ProcessCharKey(nChar);//this changes the nChar if it is uppercase
        if(contentType == ContentType::Alpha) {
            //Do nothing
            /*iCharPos = GetCharFromCaretPos(); //Get Before advancing
            AdvanceCaretPos();*/
        }
        else if(contentType == ContentType::Numeric && pDictItem->GetDecimal() !=0) {
            ASSERT(FALSE); //DETextEdit only for Alpha
            /*
            if(this->IsPosBeforeDecimal()) { //Do not advance to decimal portion if non decimal part is not filled
                CString sString;
                GetWindowText(sString);
                CString sNDecimal = this->GetNonDecimalPart(sString);
                bool bAutoSlide = false; //SAVY&&&
                if((nChar== DECIMAL_CHAR) || (sNDecimal.GetLength() == iFieldLength - pDictItem->GetDecimal() -1 && bAutoSlide)) {
                    AdvanceCaretPos();
                                        SetHasDecimalPoint( true ); // RHF Jul 05, 2007, Bug fix decimals in value set
                }
            }
            else { //caret in the decimal part
                iCharPos = GetCharFromCaretPos(); //Get Before advancing
                AdvanceCaretPos();
            }*/
        }
    }
    else {
        if(m_bRemoveText) {
            m_bRemoveText = false;
        }
    }



    bool bCtrl  = GetKeyState(VK_CONTROL)<0;
    bool bMultiLine = m_pField->UseUnicodeTextBox() && m_pField->GetDictItem()->GetContentType() == ContentType::Alpha && m_pField->AllowMultiLine();
    if(bMultiLine && bCtrl && nChar == 10 ){
        //CDEBaseEdit::OnChar(nChar, nRepCnt, nFlags);
        DefWindowProc(WM_CHAR,nChar,MAKELONG(nRepCnt,nFlags)); // 20120919 this is necessary to get CEdit::OnChar to recognize the new nChar
        return;
    }
    else if(nChar == VK_RETURN){
        if (GetParent())  {
            GetParent()->SendMessage(UWM::CSEntry::ChangeEdit, VK_RETURN, (long)this);
        }
        return;
    }
    else if(nChar == VK_TAB){ //do not process the tab key. It is done in OnKeyDown
        m_bRemoveText = true; //Reset the remove text
        return;
    }

     //CDEBaseEdit::OnChar(nChar, nRepCnt, nFlags);
    DefWindowProc(WM_CHAR,nChar,MAKELONG(nRepCnt, nFlags)); // 20120919 this is necessary to get CEdit::OnChar to recognize the new nChar

    if (bIsValidChar || nChar == VK_BACK) {
        CString sString;
        GetWindowText(sString);

        bool bFldDone = false;
        if(contentType == ContentType::Numeric && pDictItem->GetDecimal() == 0){
            ASSERT(FALSE);
            /*sString.TrimLeft(); // only for numeric
            bFldDone =  sString.GetLength() == pDictItem->GetLen();*/

        }
        if(contentType == ContentType::Numeric && pDictItem->GetDecimal() != 0){
            ASSERT(FALSE);
            /*sString.TrimRight(); // only for decimal
            int iLength = pDictItem->GetLen();
            if(pDictItem->GetDecimal() !=0 && !pDictItem->GetDecChar()){
                iLength ++ ; //Account for decimal character
            }
            if(nChar == VK_BACK){
                sString.TrimLeft();
            }
            bFldDone =  sString.GetLength() == iLength;*/

        }
        else if(contentType == ContentType::Alpha){
            CString sText;
            GetWindowText(sText); //TODO: Unicode complex script ??
            sText = sText.TrimRight();
            bFldDone = sText.GetLength()  == pDictItem->GetLen();
        }


        if(bFldDone  && !m_pField->IsEnterKeyRequired())
            GetParent()->SendMessage(UWM::CSEntry::ChangeEdit, VK_RETURN, (long)this);


    }
    else if (nChar == VK_ESCAPE || nChar == VK_RETURN || nChar == VK_UP ||  nChar == VK_RIGHT|| nChar == VK_LEFT ||
        nChar == VK_DOWN || nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_F2 ) {

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



void CDETextEdit::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CDEBaseEdit::OnActivate(nState, pWndOther, bMinimized);

    // TODO: Add your message handler code here

}


void CDETextEdit::OnSetFocus(CWnd* pOldWnd)
{
    CDEBaseEdit::OnSetFocus(pOldWnd);

    // RHF INIC Jan 17, 2003
    if( GetCheckFocus() > 0 ) {
        SetCheckFocus( GetCheckFocus()-1 );
    }
    // RHF END Jan 17, 2003
}


int CDETextEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDEBaseEdit::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetFont(GetFieldFont());
    // TODO:  Add your specialized creation code here

    return 0;
}


BOOL CDETextEdit::OnEraseBkgnd(CDC* pDC)
{
    // TODO: Add your message handler code here and/or call default

    return CDEBaseEdit::OnEraseBkgnd(pDC);
}


void CDETextEdit::OnKillFocus(CWnd* pNewWnd)
{
    CDEBaseEdit::OnKillFocus(pNewWnd);

    // TODO: Add your message handler code here
}


void CDETextEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
    CEntryrunView* pParent = DYNAMIC_DOWNCAST(CEntryrunView,GetParent());
    ASSERT(pParent);
    CEntryrunDoc* pRunDoc = pParent->GetDocument();

    if(pRunDoc->GetAppMode() == VERIFY_MODE){
        return ;
    }

    //Have to set focus first. The logic may end the entry and the edit window could be null.
    CEdit::OnLButtonDown(nFlags, point);
    SetFocus();

    pParent->SendMessage(UWM::CSEntry::MoveToField, (WPARAM)this, 0);
    pParent->SendMessage(WM_IMSA_CSENTRY_REFRESH_DATA);


    return;
}


BOOL CDETextEdit::Create(DWORD dwStyle, CRect rect, CWnd* pParent ,UINT  nID)
{
    UNREFERENCED_PARAMETER(dwStyle);

    //Savy refactored the code to add multi line . dwStyle was not being  considered
    dwStyle |= WS_VISIBLE|WS_CHILD ; //WS_BORDER|
    return      CEdit::Create(dwStyle,rect,pParent,nID);
}



bool CDETextEdit::IsValidChar(UINT nChar) const
{
    bool bRet = false;
    if(m_bRemoveText && nChar > 32 && (csprochar)nChar != _T('/')) {
        CDETextEdit* pEdit = const_cast<CDETextEdit*>(this);
        pEdit->SetWindowText(_T(""));
        pEdit->m_bRemoveText =false;
    }
    ASSERT(m_pField);
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);

    ContentType contentType = pDictItem->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");

    CString sString;
    GetWindowText(sString);

    int iLength = pDictItem->GetLen();
    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
        iLength++; //Account for the decimal csprochar
    }

    if(contentType == ContentType::Numeric) {
        ASSERT(FALSE); //CDETextEdit is for only alpha fields
       /* bool bSigned = true; //always true for numeric
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
                bool bNDFilled = sNDecimal.GetLength() == iLength - pDictItem->GetDecimal() -1;
                bool bDFilled = sDecimal.GetLength() == pDictItem->GetDecimal();

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
        */
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
//      void CDETextEdit::ProcessCharKey(UINT& nChar)
//
/////////////////////////////////////////////////////////////////////////////////
void CDETextEdit::ProcessCharKey(UINT& nChar)
{
    //if the current text is all filled remove everything and place this csprochar
    //right justified for numeric nonw decimal fields
    ASSERT(m_pField); //always has to have a dict item
    const CDictItem* pDictItem = m_pField->GetDictItem();
    ASSERT(pDictItem);

    /*int iLength = pDictItem->GetLen();
    if(pDictItem->GetDecimal() !=0 && !pDictItem->GetDecChar()){
        iLength++;
    }*/

    CString sString;
    GetWindowText(sString);

    ContentType contentType = m_pField->GetDictItem()->GetContentType();
    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");

    if(contentType == ContentType::Numeric) {
        ASSERT(FALSE); // CDETextEdit is only for alphas
        /*int iDecimal = pDictItem->GetDecimal();
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
        */
    }
    else if(contentType == ContentType::Alpha) {
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
       /* CString sTemp(_T(' '),iLength - sString.GetLength());
        sString += sTemp;

        int iChar = GetCharFromCaretPos();
        ASSERT(iChar >= 0);
//        sString.SetAt(iChar,_TCHAR(nChar));       // gsf 8-mar-01
        sString.SetAt(iChar,_TCHAR(nGlennChar));    // gsf 8-mar-01
        SetWindowText(sString);
        //      this->RedrawWindow();*/
        return;
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void  CDETextEdit::SetWindowText(const CString& sString)
//
/////////////////////////////////////////////////////////////////////////////////
void CDETextEdit::SetWindowText(const CString& sString)
{
    // RHF INIC Jan 09, 2003
    CString rString;
    GetWindowText(rString);
    // RHF END Jan 09, 2003

    const wstring_view rString_trimmed_sv = SO::TrimRightSpace(rString);
    
    std::wstring sTempString = SO::TrimRightSpace(sString);

    // if a multiline field, turn newlines into \r\n
    if( m_pField->AllowMultiLine() ) {
        SO::MakeNewlineCRLF(sTempString);
    }
    // otherwise, turn newlines into spaces
    else {
        NewlineSubstitutor::MakeNewlineToSpace(sTempString);
    }

    CEdit::SetWindowText(sTempString.c_str());

    // RHF INIC Jan 10, 2003
    if( rString_trimmed_sv != sTempString )
        PostMessage(UWM::CSEntry::RefreshSelected);// RHF Jan 09, 2003
    // RHF END Jan 10, 2003

    Invalidate(); //SAVY&&& 10/03/00 for now
}

void CDETextEdit::GetWindowText(CString& rString) const
{
    CDEBaseEdit::GetWindowText(rString);

    //pad the empty spaces with blanks
    SO::MakeExactLength(rString, m_pField->GetDictItem()->GetLen());
}

void CDETextEdit::RefreshEditStyles()
{
    if (m_pField && GetSafeHwnd()) {
        m_pField->IsProtected() ? SetReadOnly(TRUE) : SetReadOnly(FALSE);
    }
}
