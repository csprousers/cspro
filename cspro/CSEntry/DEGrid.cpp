// DEGrid.cpp: implementation of the CDEGrid class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "DEGrid.h"
#include "CSEntry.h"
#include "DETextEdit.h"
#include "runview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

#define DEFAULT_COLWIDTH        70
#define DEFAULT_ROWHEIGHT       40

//const csprochar DECIMAL_CHAR = _T('.') ; //later on get it from locale

BEGIN_MESSAGE_MAP(CDEGrid, CGridWnd)
//{{AFX_MSG_MAP(CTblGrid)
// NOTE - the ClassWizard will add and remove mapping macros here.
//        DO NOT EDIT what you see in these blocks of generated code !
    ON_WM_HSCROLL()
    ON_WM_LBUTTONUP()
    ON_WM_VSCROLL()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDEGrid::CDEGrid(CDERoster* pRoster)
{
    m_pRoster = pRoster;
    m_dwStyle = WS_BORDER|WS_CHILD;
    SetDecimalChar(DECIMAL_CHAR);
    m_bRedraw = true;
    m_iCurrentOccs = 0;
}

CDEGrid::~CDEGrid()
{
}

/***************************************************
Set the data for the grid
****************************************************/
void CDEGrid::OnSetup()
{
}


/***************************************************
OnEditStart
This message is sent whenever the grid is ready
to start editing a cell
A return of TRUE allows the editing a return of
FALSE stops editing
Plus the properties of the CEdit class can be modified
****************************************************/
int CDEGrid::OnEditStart(int col, long row,CWnd **edit)
{
    return FALSE;
}

/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CDEGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag){
    return FALSE;
    //You may need this to update data
}


bool CDEGrid::OnCanSizeCol(int iCol) const
{
        UNREFERENCED_PARAMETER(iCol);
        return false;
}

bool CDEGrid::OnCanSizeRow(int iRow) const
{
        UNREFERENCED_PARAMETER(iRow);
        return false;
}


bool CDEGrid::OnCanSelectRow(int iRow) const
{
        UNREFERENCED_PARAMETER(iRow);
        return false;
}

bool CDEGrid::OnCanSelectCol(int iCol) const
{
    UNREFERENCED_PARAMETER(iCol);
    return false;
}

bool CDEGrid::OnCanSelectCell(int iRow, int iCol) const
{
    UNREFERENCED_PARAMETER(iRow);
    UNREFERENCED_PARAMETER(iCol);
    return false;
}

void CDEGrid::OnLClicked(int iRow, int iCol)
{
    return;
}


void CDEGrid::OnCellField_LClicked(CDEField* pFld, int iOcc)
{
    if (!pFld) {
        return;
    }

    if (GetRoster()->GetRightToLeft() && GetRoster()->GetOrientation() == RosterOrientation::Vertical) {
        // cols are reversed in right to left vertical
        iOcc = GetNumCols() - iOcc;
    }

    //Send a message to the parent for Move to and reset the grid data
    //Move to this field an update grid data
    CEntryrunView* pView = (CEntryrunView*)GetParent();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pView->GetDocument();

    if(pDoc->GetAppMode() == VERIFY_MODE){
        return ; //GSF requested no click stuff in verify mode
    }
    CDERoster* pRoster = GetRoster();
    if( pDoc->IsAutoEndGroup() ) { // RHF Nov 07, 2000
        if((pDoc->GetAppMode() == MODIFY_MODE || pDoc->GetAppMode() == VERIFY_MODE) && iOcc > pRoster->GetDataOccs()) {
            AfxMessageBox(MGF::GetMessageText(MGF::OccurrenceDoesNotExist).c_str());
            return;
        }
    }
    CRunAplEntry* pRunApl = pDoc->GetRunApl();
    bool bProcessSpreadSheetMode = m_pRoster->UsingFreeMovement();
    bool bCanEnterNotAppl = false;
    bool bAskWhenNotAppl = false;
    if(bProcessSpreadSheetMode) {
        bCanEnterNotAppl =  pRunApl->GetSettings()->CanEnterNotappl();
        bAskWhenNotAppl = pRunApl->GetSettings()->AskWhenNotappl();

        pRunApl->GetSettings()->SetEnterNotappl(true);
        pRunApl->GetSettings()->SetDontAskWhenNotappl();
    }

    //SAVY 04/11/00
    BOOL  bPostProc = TRUE;
    CDEItemBase* pCurBase = pDoc->GetCurField();
    CDEBaseEdit* pCurEdit = pView->SearchEdit((CDEField*)pCurBase);

    if(pCurBase){
        if(pCurEdit){
            bPostProc = pView->ChkPProcReq(pCurEdit); //Check the requirement for the postproc before putting the value in buffers
            pView->PutEditValInBuffers(pCurEdit);
            pCurEdit->SetModifiedFlag(false);
            pCurEdit->SetRemoveTxtFlag(true);
        }
    }

    // 20130608 when a roster had protected items, clicking on a field past the added items (after an endgroup in
    // the logic) caused a crash; now i'll just disallow clicking on protected (and mirror) fields
    if( pFld->IsProtected() || pFld->IsMirror() )
        return;

    CDEItemBase* pBase = pDoc->GetRunApl()->MoveToField(pFld->GetDictItem()->GetSymbol(),iOcc,bPostProc);

    if(pBase == pFld && ((CDEField*)pBase)->GetParent()->GetCurOccurrence() == iOcc) {

        if(GetEdit() && pCurEdit != GetEdit()) {
            ((CDEBaseEdit*)GetEdit())->SetModifiedFlag(false);
        }
        pView->UpdateGridEdits((CDEField*)pBase);
        pDoc->SetCurField((CDEField*)pBase);
        pView->GoToFld((CDEField*)pBase);
        SetGridData(pDoc->GetRunApl());
        pView->UpdateFields();

        if(pCurEdit == GetEdit()) {
            CIMSAString sData = pDoc->GetRunApl()->GetVal(pFld);
            pCurEdit->SetWindowText(sData);
            pCurEdit->SetRemoveTxtFlag(true);
        }
    }
    else if (pBase) {
        pFld = (CDEField*) pBase;
        pView->ChkFrmChangeNUpdate(pFld);
        pView->UpdateGridEdits(pFld);
        pDoc->SetCurField(pFld);
        pView->GoToFld(pFld);
        SetGridData(pDoc->GetRunApl());
        pView->UpdateFields();

        if(pCurEdit == GetEdit()){
            CIMSAString sData = pDoc->GetRunApl()->GetVal(pFld);
            pCurEdit->SetWindowText(sData);
            pCurEdit->SetRemoveTxtFlag(true);
        }
    }

    bool    bShowLabels=true;
    if( bShowLabels )
        pView->ShowCapi( (CDEField *) pBase );

    if(bProcessSpreadSheetMode) {
         //Turn the flags of settings back to its original
        pRunApl->GetSettings()->SetEnterNotappl(bCanEnterNotAppl);
        if(bAskWhenNotAppl){
            pRunApl->GetSettings()->SetMustAskWhenNotappl();
        }
        else {
            pRunApl->GetSettings()->SetDontAskWhenNotappl();
        }
    }
}


BOOL CDEGrid::ShowField(CDEField* pField, int iOcc)
{
    CEntryrunView* pView = (CEntryrunView*)GetParent();

    CEntryrunDoc* pDoc = (CEntryrunDoc*)pView->GetDocument();
    if(pDoc->GetAppMode() != VERIFY_MODE)
        return TRUE;
    if(!pField->GetVerifyFlag())
        return TRUE;
    BOOL bShow = pView->CheckToShow(m_pRoster);

    if(bShow) {
        return bShow;
    }

    else {
        CDEItemBase* pCurBase = pDoc->GetCurField();
        if(!pCurBase) {
            return bShow; // We are not processing the current roster so return the default
        }
        CDEItemBase* pParent = pCurBase->GetParent();
        if(pParent != m_pRoster) {
            return bShow; // We are not processing the current roster so return the default
        }
        else {  //Check if the current occurrence has been matched
            if( iOcc <  m_pRoster->GetCurOccurrence() )  {
                return TRUE;
            }
            else if( iOcc >  m_pRoster->GetCurOccurrence() ) {
                return FALSE;
            }
            else { //check if the pField comes before the pCurField
                for(int iIndex = 0; iIndex < m_pRoster->GetNumItems(); iIndex++) {
                    if(m_pRoster->GetItem(iIndex) == pCurBase)
                        return FALSE;           //Found the current field before the test field
                    if(m_pRoster->GetItem(iIndex) == pField) //Found this before the current field
                        return TRUE;
                }
            }


        }


    }

    return FALSE;
}

void CDEGrid::StartEdit(CDEField* pFld, int iOcc)
{
    ASSERT(pFld);
    CDEBaseEdit* pEdit = (CDEBaseEdit*)GetEdit();
    if(pEdit) {
        CDEField* pOldField= pEdit->GetField();
        bool bFldChange = (pOldField != pFld);
        pEdit->SetField(pFld);

        //       pEdit->ShowWindow(SW_HIDE);
        DWORD dwStyle = pEdit->GetEditStyle(pFld);
        SetEdit(NULL);

        if(pFld->UseUnicodeTextBox()){
            CDETextEdit* pTextEdit = DYNAMIC_DOWNCAST(CDETextEdit,pEdit);
            //when accept box is shown, the caret for the text edit is destroyed and cannot be recreated.
            //destroying the window and recreating it when the caretpos is invalid.
            if(pTextEdit == NULL || (pTextEdit && pTextEdit->CharFromPos(pTextEdit->GetCaretPos()) == -1 )){
                if(pEdit->GetSafeHwnd() != NULL){
                    pEdit->DestroyWindow();
                }

                ASSERT(pFld->GetDictItem()->GetContentType() == ContentType::Alpha);
                if (pFld->GetDictItem()->GetContentType() == ContentType::Alpha){
                    dwStyle |= ES_LEFT |ES_AUTOHSCROLL;
                    bool bMultiLine = pFld->AllowMultiLine();
                    if(bMultiLine && pFld->UseUnicodeTextBox()){
                        dwStyle = ES_LEFT | ES_MULTILINE |ES_AUTOVSCROLL ; //No AutoHScroll to enable wrap
                    }
                }

                //You can set here the password style for the field if it is required
                if (pFld->IsProtected()|| pFld->IsMirror())
                    dwStyle |= ES_READONLY;


                pTextEdit = new CDETextEdit();
                pTextEdit->SetField(pEdit->GetField());

                pTextEdit->Create(dwStyle, CRect(0,0,0,0), this->GetParent(), 100);
                pTextEdit->ModifyStyleEx(WS_EX_CLIENTEDGE,0);

                delete pEdit; //delete the old edit

                pEdit = pTextEdit;
                pEdit->ShowWindow(SW_HIDE);

            }
        }
        else {
                CDEEdit* pTextEdit = DYNAMIC_DOWNCAST(CDEEdit,pEdit);
                if(pTextEdit == NULL){
                if(pEdit->GetSafeHwnd() != NULL){
                    pEdit->DestroyWindow();
                }

                pTextEdit = new CDEEdit();
                pTextEdit->SetField(pEdit->GetField());

                pTextEdit->Create(dwStyle, CRect(0,0,0,0), this->GetParent(), 100);
                pTextEdit->ModifyStyleEx(WS_EX_CLIENTEDGE,0);

                delete pEdit; //delete the old edit

                pEdit = pTextEdit;
                pEdit->ShowWindow(SW_HIDE);

            }
        }
        //pEdit->DestroyWindow();
        if(pEdit->GetSafeHwnd() == NULL) {
            pEdit->Create(dwStyle, CRect(0,0,0,0), this->GetParent(), 100);
        }
        pEdit-> ModifyStyle(WS_BORDER,0);

        SetEdit(pEdit);

        pEdit->PostMessage(WM_SETFOCUS);
        int iLength = pFld->GetDictItem()->GetLen();
        if(iLength)
            pEdit->LimitText(iLength);

        //call the base class to position it
        CEntryrunView* pView = (CEntryrunView*)GetParent();
        CEntryrunDoc* pDoc = (CEntryrunDoc*)pView->GetDocument();

        CIMSAString sData;
        if(pView->CheckToShow(pFld)){
            int iTemp = pFld->GetRuntimeOccurrence();
            // pFld->SetOcc(iOcc+1);
            pFld->SetRuntimeOccurrence(iOcc);
            sData = pDoc->GetRunApl()->GetVal(pFld);
            pFld->SetRuntimeOccurrence(iTemp);
        }
        if(pEdit->GetUseSequential()){
            sData = pFld->GetData();
        }
        if(pOldField->IsSequential() && bFldChange){
           pEdit->SetUseSequential(false);
        }

        if(!pEdit->GetModifiedFlag() || bFldChange){
            pEdit->SetWindowText(sData);
        }
        CGridWnd::StartEdit(pFld,iOcc);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CDEGrid::OnCanSwapCol()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CDEGrid::OnCanSwapCol(void) const
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CDEGrid::OnCanSwapRow()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CDEGrid::OnCanSwapRow(void) const
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CDEGrid::OnCanDeleteCol()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CDEGrid::OnCanDeleteCol(int iCol) const
{
    UNREFERENCED_PARAMETER(iCol);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CDEGrid::OnCanDeleteRow()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CDEGrid::OnCanDeleteRow(int iRow) const
{
    UNREFERENCED_PARAMETER(iRow);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CDEGrid::OnCanSelectField()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CDEGrid::OnCanSelectField(int iRow, int iCol, int iFld) const
{
    UNREFERENCED_PARAMETER(iRow);
    UNREFERENCED_PARAMETER(iCol);
    UNREFERENCED_PARAMETER(iFld);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnCanSelectText()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CDEGrid::OnCanSelectText(int iRow, int iCol, int iTxt) const
{
    UNREFERENCED_PARAMETER(iRow);
    UNREFERENCED_PARAMETER(iCol);
    UNREFERENCED_PARAMETER(iTxt);
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnCanSelectBox()
//
// base-class version prevents selection if the box is partially clipped by the cell boundaries
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CDEGrid::OnCanSelectBox(int iRow, int iCol, int iBox) const
{
    UNREFERENCED_PARAMETER(iRow);
    UNREFERENCED_PARAMETER(iCol);
    UNREFERENCED_PARAMETER(iBox);
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CDEGrid::OnCanDrawBox()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CDEGrid::OnCanDrawBox(int iRow, int iCol)
{
    UNREFERENCED_PARAMETER(iRow);
    UNREFERENCED_PARAMETER(iCol);
    return false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CDEGrid::OnCanDrawText()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CDEGrid::OnCanDrawText(int iRow, int iCol)
{
    UNREFERENCED_PARAMETER(iRow);
    UNREFERENCED_PARAMETER(iCol);
    return false;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEBaseEdit::Pad(CString& sString)
// Valid for decimal numbers
/////////////////////////////////////////////////////////////////////////////////
void CDEGrid::Pad(CString& sString ,CDEField* pField)
{
    ASSERT(pField);
    const CDictItem* pDictItem = pField->GetDictItem();
    ASSERT(pDictItem);
    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
    if(pDictItem->GetContentType() == ContentType::Alpha){
        return;
    }

    int iDigitsAfterDecimal = pDictItem->GetDecimal();
    if(iDigitsAfterDecimal==0 ){ //This is not a decimal csprochar
        //Pad the numeric val with blanks on to the left side
        CString sTemp(_T(' '),pDictItem->GetLen()- sString.GetLength());
        sString =sTemp +sString;
        return;
    }

    BOOL bDecChar = pDictItem->GetDecChar();
    int iLength = pDictItem->GetLen();
    if(!bDecChar  && pDictItem->GetDecimal() != 0){
        //Decimal csprochar is not accounted for in the length
        iLength++;
    }

    if(!sString.IsEmpty() && !bDecChar){
        //Get Decimal Char from the system metrics
        int iPos = sString.Find(DECIMAL_CHAR); // '.');
        if(iPos == -1) {
            CString sNDecimal(sString.Left(iLength - pDictItem->GetDecimal()-1));
            CString sDecimal(sString.Right(pDictItem->GetDecimal()));
            sString = sNDecimal + DECIMAL_CHAR + sDecimal; //_T(".") + sDecimal;
            return;
        }
    }
    CString sBeforeDecimal = GetNonDecimalPart(sString,pField);
    CString sAfterDecimal = GetDecimalPart(sString,pField);
    sString = MakeDecNumString(sBeforeDecimal,sAfterDecimal,pField);
}
/////////////////////////////////////////////////////////////////////////////////
//
//      CString CDEGrid::GetDecimalPart(const CString& sString , CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
CString CDEGrid::GetDecimalPart(const CString& sString , CDEField* pField)
{
    CString sAfterDecimal;
    ASSERT(pField);
    const CDictItem* pDictItem = pField->GetDictItem();
    ASSERT(pDictItem);
    int iDigitsAfterDecimal = pDictItem->GetDecimal();
    if(iDigitsAfterDecimal==0 ) //This is not a decimal csprochar
        return sAfterDecimal;
    int iPos = sString.Find('.'); // DECIMAL_CHAR);
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
//      CString CDEBaseEdit::GetNonDecimalPart(const CString& sString,CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
CString CDEGrid::GetNonDecimalPart(const CString& sString,CDEField* pField)
{
    CString sBeforeDecimal;
    ASSERT(pField);
    const CDictItem* pDictItem = pField->GetDictItem();
    ASSERT(pDictItem);
    int iDigitsAfterDecimal = pDictItem->GetDecimal();
    if(iDigitsAfterDecimal==0 ) //This is not a decimal csprochar
        return sBeforeDecimal;
    int iStringLength = sString.GetLength();
    int iPos = sString.Find('.'); // DECIMAL_CHAR);
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
//CString CDEGrid::MakeDecNumString(const CString& sNonDecimal, const CString & sDecimal,CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
CString CDEGrid::MakeDecNumString(const CString& sNonDecimal, const CString & sDecimal,CDEField* pField)
{
    CString sRet;
    ASSERT(pField);
    const CDictItem* pDictItem = pField->GetDictItem();
    ASSERT(pDictItem);

    CString sLNDecimal(sNonDecimal);
    CString sLDecimal(sDecimal);

    int iDigitsAfterDecimal = pDictItem->GetDecimal();

    int iLength = pDictItem->GetLen();
    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
        //Decimal csprochar is not accounted for in the length
        iLength++;
    }

     //Pad the characters before decimal
    int iSpaces = iLength-iDigitsAfterDecimal-1 - sLNDecimal.GetLength(); //subtract one for the decimal character
    CString sTemp2(_T(' '),iSpaces);
    sLNDecimal = sTemp2 +sLNDecimal;
    //Pad the characters after decimal
    CString sTemp1(_T(' '),iDigitsAfterDecimal-sLDecimal.GetLength());
    sLDecimal += sTemp1;
    //patch the number
    sRet = sLNDecimal+ CString(DECIMAL_CHAR) +sLDecimal;
    int iRetLen = sRet.GetLength();
    ASSERT( iRetLen == iLength);
    return sRet;
}


void CDEGrid::SetGridData(CRunAplEntry* pApl)
{
    ASSERT(pApl);
    CDERoster* pRoster = this->GetRoster();

    CEntryrunView* pView = (CEntryrunView*)GetParent();
    int iMaxLoopOccs = pView->GetDynamicMaxOccs(pRoster);
    if(iMaxLoopOccs != 0 && iMaxLoopOccs != m_iCurrentOccs){
        m_iCurrentOccs = iMaxLoopOccs;
        int iTempOccs = pRoster->GetMaxLoopOccs();
        pRoster->SetMaxLoopOccs(iMaxLoopOccs);
        this->BuildGrid();
        this->RecalcLayout();
        pRoster->SetMaxLoopOccs(iTempOccs);
    }

    StartQueue();


    //int iDataOccs = pRoster->GetDataOccs();
    int iMinCol = 0;
    int iMaxCol = 0;
    int iMinRow = 0;
    int iMaxRow = 0;
    int iCol = -1;
    int iRow = -1;
    CPoint ptOffset(GetScrollPos() - CPoint(GetRightJustifyOffset(),0));
    CRect rcDraw;

    GetClientRect(&rcDraw);

    if (pRoster->GetOrientation() == RosterOrientation::Vertical)  {
        iMinRow = 0;
        iMaxRow = GetNumRows();
        iMinCol = 0;
        //iMaxCol = iDataOccs;
        iMaxCol = GetNumCols();
    }
    else {
        iMinRow = 0;
        //iMaxRow = iDataOccs;
        iMaxRow = GetNumRows();
        iMinCol = 0;
        iMaxCol = GetNumCols();
    }
    // calc min/max row and col (csc moved 4/30/01)
    for (iCol=1 ; iCol<iMaxCol ; iCol++)  {
        CGridCell& cell = GetHeaderCell(iCol);
        CRect rc = cell.GetRect();
        rc.OffsetRect(CPoint(-ptOffset.x,0));
        if (rc.left > rcDraw.right)  {
            iMaxCol = iCol;
            break;
        }
        if (rc.right <= rcDraw.left)  {
            continue;
        }
        if (iMinCol==0)  {
            iMinCol=iCol;
        }
    }

    // swap col order for right to left
    if (GetRoster()->GetRightToLeft() && pRoster->GetOrientation() == RosterOrientation::Vertical) {
        int tmp = iMaxCol;
        iMaxCol = GetNumCols()- iMinCol;
        iMinCol = GetNumCols() - tmp;
    }

    for (iRow=1 ; iRow<iMaxRow ; iRow++)  {
        CGridCell& cell = GetStubCell(iRow);
        CRect rc = cell.GetRect();
        rc.OffsetRect(CPoint(0,-ptOffset.y));

        if (rc.top > rcDraw.bottom)  {
            iMaxRow = iRow;
            break;
        }
        if (rc.bottom <= rcDraw.top)  {
            continue;
        }
        if (iMinRow==0)  {
            iMinRow=iRow;
        }
    }
    // end special 4/30/01 csc section
    int iStartOcc = 0;
    int iStartItem = 0;
    int iMaxOccs =0;
    int iMaxItems  = 0;
    if (pRoster->GetOrientation() == RosterOrientation::Vertical)  {
        iStartItem = iMinRow  ;
        iStartItem--;
        if(iStartItem <0)
          iStartItem =0;
        iMaxItems = iMaxRow ;
        iStartOcc = iMinCol ;
        iStartOcc--;
        if(iStartOcc <0 )
          iStartOcc=0;
        iMaxOccs = iMaxCol;

    }
    else {
        iStartOcc = iMinRow ;
        iStartOcc--;
        if(iStartOcc <0 )
          iStartOcc=0;
        iMaxOccs = iMaxRow ;
        iStartItem = iMinCol;
        iStartItem--;
        if(iStartItem <0)
          iStartItem =0;
        iMaxItems = iMaxCol ;
    }
    /*if(iMaxOccs >= pRoster->GetTotalOccs()){
        iMaxOccs = pRoster->GetTotalOccs();
    }*/ //GSF request for Trevor's bug


    if(iMaxOccs >= iMaxLoopOccs){
        iMaxOccs = iMaxLoopOccs;
    }


    CArray<CDEField*,CDEField*> arrFields;
    for(int iIndex = iStartItem ; iIndex < iMaxItems ; iIndex ++) {
        int iColIndex = iIndex;
        if (pRoster->GetRightToLeft() && pRoster->GetOrientation() == RosterOrientation::Horizontal) {
            // swap col index for right to left
            iColIndex = pRoster->GetNumCols() - iIndex - 1;
        }
        CDECol* pCol = pRoster->GetCol(iColIndex);
        for(int iItem =0; iItem < pCol->GetNumFields(); iItem++){
            arrFields.Add(pCol->GetField(iItem));
        }
    }
    iStartItem = 0;
    iMaxItems = arrFields.GetSize();
    for(int iOcc =iStartOcc ; iOcc < iMaxOccs; iOcc++) { //for each occurrence
        for(int iField = iStartItem; iField < iMaxItems ;iField++) { //for each field

         //   CDEItemBase* pBase = pRoster->GetItem(iField);
            CDEField* pField = arrFields.GetAt(iField);
            //CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pBase);
            if(!pField){
                continue;
            }
            //Here use the GetVal With Occurence
            const CDictItem* pItem = pField->GetDictItem();
            ASSERT(pItem);
            int iTemp = pField->GetRuntimeOccurrence();
            pField->SetRuntimeOccurrence(iOcc+1);

            BOOL bShow = ShowField(pField,iOcc+1);
            CIMSAString sData;
            if(!bShow){
                sData = _T("");
            }
            else{
                sData = pApl->GetVal(pField);
            }
            Pad(sData,pField);
            SetFieldData(pField,pField->GetRuntimeOccurrence(),sData);
            int iIntensity = pApl->GetStatus(pField);

            COLORREF cColorBg = pView->GetFieldBackgroundColor( iIntensity );

            // GSF INIC Jan 07, 2003
            if(!pField->IsProtected() && !pField->IsMirror() && !pRoster->UsingFreeMovement()){
                this->SetFieldBColor(pField,pField->GetRuntimeOccurrence(),cColorBg);
            }
            // GSF END Jan 07, 2003

            //Reset the Occ
            pField->SetRuntimeOccurrence(iTemp);
        }
    }
    StopQueue();
}

void CDEGrid::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CGridWnd::OnHScroll(nSBCode,nPos,pScrollBar);


    CEntryrunView* pView = (CEntryrunView*)GetParent();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pView->GetDocument();
    if(pDoc->GetCurField()->GetParent() == this->GetRoster() && this->GetEdit() && this->GetEdit()->GetSafeHwnd()){
        this->GetEdit()->SetFocus();   // csc and savy 1/23/01
    }
    if(GetUpdateFlag()){
        SetGridData(pDoc->GetRunApl());
    }
    else {
        SetUpdateFlag(TRUE);
    }
}

void CDEGrid::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CGridWnd::OnVScroll(nSBCode,nPos,pScrollBar);

    CEntryrunView* pView = (CEntryrunView*)GetParent();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pView->GetDocument();
    if(pDoc->GetCurField()->GetParent() == this->GetRoster() && this->GetEdit() && this->GetEdit()->GetSafeHwnd()){
        this->GetEdit()->SetFocus();   // csc and savy 1/23/01
    }
    if(GetUpdateFlag()){
        SetGridData(pDoc->GetRunApl());
    }
    else {
        SetUpdateFlag(TRUE);
    }
}

void CDEGrid::OnLButtonUp(UINT nFlags, CPoint point)
{
    CGridWnd::OnLButtonUp(nFlags,point);
    CEntryrunView* pView = (CEntryrunView*)GetParent();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pView->GetDocument();
    SetGridData(pDoc->GetRunApl());
  //  this->RedrawWindow();
}

void CDEGrid::ResetGridData(bool bForceReset /*= false*/)
{
    CDERoster* pRoster = this->GetRoster();
    StartQueue();

    // end special 4/30/01 csc section
    int iStartOcc = 0;
    int iStartItem = 0;
    int iMaxOccs =0;
    int iMaxItems  = 0;

    iStartOcc = 0;

    CEntryrunView* pView = (CEntryrunView*)GetParent();
    iMaxOccs = pView->GetDynamicMaxOccs(pRoster);
    if(bForceReset || (iMaxOccs != this->m_iCurrentOccs)) {
        this->BuildGrid();
        this->RecalcLayout();
    }

  //  iMaxOccs = pRoster->GetMaxLoopOccs();
    iStartItem=0;
    iMaxItems = pRoster->GetNumItems();

    CIMSAString sData ;
    sData = _T("");
    for(int iOcc =iStartOcc ; iOcc < iMaxOccs; iOcc++) { //for each occurrence
        for(int iField = iStartItem; iField < iMaxItems ;iField++) { //for each field

            CDEItemBase* pBase = pRoster->GetItem(iField);
            CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pBase);
            if(!pField){
                continue;
            }
            //Here use the GetVal With Occurence
            const CDictItem* pItem =  pField->GetDictItem();
            ASSERT(pItem);
            int iTemp = pField->GetRuntimeOccurrence();
            pField->SetRuntimeOccurrence(iOcc+1);

            //Pad(sData,pField);
            SetFieldData(pField,pField->GetRuntimeOccurrence(),sData);
            this->SetFieldBColor(pField,pField->GetRuntimeOccurrence(),RGB(255,255,255));
            pField->SetRuntimeOccurrence(iTemp);
        }
    }
    StopQueue();
}
