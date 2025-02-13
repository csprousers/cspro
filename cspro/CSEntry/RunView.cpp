// RunView.cpp : implementation of the CEntryrunView class
#include "StdAfx.h"
#include "RunView.h"
#include "CaseView.h"
#include "CustMsg.h"
#include "CSEntry.h"
#include "DETextEdit.h"
#include "MainFrm.h"
#include "Rundoc.h"
#include <zCapiO/capi.h>
#include <ZBRIDGEO/npff.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEntryrunView

BOOL bReset = TRUE;
bool bGoTo  = true;

IMPLEMENT_DYNCREATE(CEntryrunView, CScrollView)

BEGIN_MESSAGE_MAP(CEntryrunView, CScrollView)
//{{AFX_MSG_MAP(CEntryrunView)
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
    ON_COMMAND(ID_ENDGRP, OnNextgrp)
    ON_UPDATE_COMMAND_UI(ID_ENDGRP, OnUpdateEndgrp)
    ON_COMMAND(ID_INSERT_GROUPOCC, OnInsertGroupocc)
    ON_UPDATE_COMMAND_UI(ID_INSERT_GROUPOCC, OnUpdateInsertGroupocc)
    ON_COMMAND(ID_DELETE_GRPOCC, OnDeleteGrpocc)
    ON_UPDATE_COMMAND_UI(ID_DELETE_GRPOCC, OnUpdateDeleteGrpocc)
    ON_COMMAND(ID_SORTGRPOCC, OnSortgrpocc)
    ON_UPDATE_COMMAND_UI(ID_SORTGRPOCC, OnUpdateSortgrpocc)
    ON_COMMAND(ID_EDIT_FIELD_NOTE, OnFieldNote)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIELD_NOTE, OnUpdateNote)
    ON_COMMAND(ID_EDIT_CASE_NOTE, OnCaseNote)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CASE_NOTE, OnUpdateNote)
    ON_MESSAGE(UWM::CSEntry::ChangeEdit, OnEditChange)
    ON_MESSAGE(WM_IMSA_CSENTRY_REFRESH_DATA, OnRefreshData)
    ON_MESSAGE(UWM::CSEntry::EndGroup, OnEndgrp)
    ON_MESSAGE(UWM::CSEntry::EndLevel, OnEndLevel)
    ON_MESSAGE(UWM::CSEntry::NextLevelOccurrence, OnNextLevelOcc)
    ON_MESSAGE(UWM::CSEntry::AdvanceToEnd, OnAdvToEnd)
    ON_MESSAGE(UWM::CSEntry::PreviousPage, OnPageUp)
    ON_MESSAGE(UWM::CSEntry::NextPage, OnPageDown)
    ON_MESSAGE(UWM::CSEntry::SlashKey, OnSlashKey)
    ON_MESSAGE(UWM::CSEntry::InsertAfterOccurrence, OnInsertAfter)
    ON_MESSAGE(UWM::CSEntry::PreviousPersistent, OnPreviousPersistent)
    ON_MESSAGE(UWM::CSEntry::CheatKey, OnCheatKey)
    ON_MESSAGE(UWM::CSEntry::MoveToField, OnMoveToField)
    ON_MESSAGE(UWM::CSEntry::PlusKey, OnPlusKey)
    ON_MESSAGE(UWM::CSEntry::ShowCapi, OnShowCapi) // RHF Nov 22, 2002
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEntryrunView construction/destruction
/////////////////////////////////////////////////////////////////////////////////
//
//      CEntryrunView::CEntryrunView()
//
/////////////////////////////////////////////////////////////////////////////////
CEntryrunView::CEntryrunView()
{
    m_iVerify = 0;
    m_iCurrentFormFileNum = 0; // RHF Jan 12, 2000
    m_bCheatKey = FALSE;

    m_pOldField = NULL;

    SetupFieldColors(nullptr);
}

/////////////////////////////////////////////////////////////////////////////////
//
//      CEntryrunView::~CEntryrunView()
//
/////////////////////////////////////////////////////////////////////////////////
CEntryrunView::~CEntryrunView()
{
    RemoveAllEdits();
    RemoveAllGrids2();

    ShowCapi( (CDEBaseEdit*)NULL ); // RHF Jan 30, 2000
}

/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CEntryrunView::PreCreateWindow(CREATESTRUCT& cs)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CEntryrunView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    BOOL bPreCreateOk = CScrollView::PreCreateWindow(cs);
    return bPreCreateOk;
}


/////////////////////////////////////////////////////////////////////////////
// CEntryrunView drawing
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnDraw(CDC* pDC)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnDraw(CDC* pDC)
{
    CEntryrunDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);

    CDEFormFile* pFF = pDoc->GetCurFormFile();

    //Get the current form Number
    CEntryrunDoc* pRunDoc = GetDocument();
    int iFormNum = pRunDoc->GetCurFormNum();
    const CDEForm* pForm = pFF->GetForm(iFormNum);

    COLORREF background_color = ( pForm != nullptr ) ? pForm->GetBackgroundColor().ToCOLORREF() :
                                                       GetSysColor(COLOR_BTNFACE);
    pDC->SetMapMode(MM_TEXT);
    pDC->SetBkColor(background_color);

    //Draw all the static items
    APP_MODE mode = pDoc->GetAppMode();
    if( mode == NO_MODE ) {
        DrawScreenStats(pDC);
    }
    else {
        DrawStaticItems();
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnInitialUpdate()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnInitialUpdate()
{
    CScrollView::OnInitialUpdate();

    CSize sizeTotal;
    // TODO: calculate the total size of this view
    sizeTotal.cx = sizeTotal.cy = 100;
    SetScrollSizes(MM_TEXT, sizeTotal);
    ResetForm();
}


void CEntryrunView::OnSize(UINT nType,int cx,int cy) // 20100423 for centering forms
{
    CScrollView::OnSize(nType,cx,cy);

    // 20100622 this will resize the responses window if the user resizes the main window
    CMainFrame * pMainFrm = (CMainFrame *) AfxGetMainWnd();
    if( pMainFrm )
        pMainFrm->PostMessage(UWM::CSEntry::ShowCapi, 1);

    CEntryrunDoc* pDoc;
    CDEFormFile* pFormfile;
    CDEForm* pForm;
    CNPifFile* pPIF;
    Application* pApp;

    if( ( pDoc = GetDocument() ) != NULL && ( pPIF = pDoc->GetPifFile() ) != NULL && ( pApp = pPIF->GetApplication() ) != NULL &&
        pApp->GetCenterForms() && ( pFormfile = pDoc->GetCurFormFile() ) != NULL && ( pForm = pFormfile->GetForm(pDoc->GetCurFormNum()) ) != NULL )
    {
        if( pDoc->GetAppMode() == NO_MODE ) // this code only matters is a form is on the screen
            return;

        CDEField * pField = (CDEField *)pDoc->GetCurField();
        CDEBaseEdit * pEdit = SearchEdit(pField);
        CString fieldText;
        CPoint caret;
        CPoint scrollPos;
        int newSpacing;

        pForm->UpdateDims(cx,&newSpacing);

        if( !newSpacing )
            return; // no adjustment needed

        scrollPos = GetScrollPosition();

        SetScrollSizes(MM_TEXT,CSize(pForm->GetDims().BottomRight()));

        if( pEdit )
        {
            pEdit->GetWindowText(fieldText);    // get what's currently been entered in the field
            caret = pEdit->GetCaretPos();       // get where the cursor is in the field

            RECT rect;
            pEdit->GetWindowRect(&rect);    // this block of code is needed to move the edit box when it is in a roster
            rect.left += newSpacing;        // as it was getting drawn in the new spot but the old edit box lingered
            rect.right += newSpacing;
            ScreenToClient(&rect);
            pEdit->MoveWindow(&rect);
        }


        CClientDC dc(this);             // redraw the background
        ClearScreen((CDC *)&dc);
        OnEraseBkgnd(&dc);
        DrawStaticItems();


        for( int i = 0; i < pForm->GetNumItems(); i++ ) // redraw the items on the forms
        {
            CDEItemBase* pItem = pForm->GetItem(i);

            if( pItem->GetItemType() == CDEFormBase::Roster )
            {
                CDEGrid* pGrid = FindGrid((CDERoster*)pItem);

                if( pGrid )
                {
                    CRect cRect;
                    cRect = pItem->GetDims();
                    RECT rect = cRect;
                    pGrid->MoveWindow(&rect);
                }
            }

            if( pItem->GetItemType() == CDEFormBase::Field )
            {
                CDEBaseEdit * pThisEdit = SearchEdit((CDEField *)pItem);

                if( pThisEdit )
                {
                    CRect cRect;
                    cRect = pItem->GetDims();
                    RECT rect = cRect;
                    pThisEdit->MoveWindow(&rect); // this is necessary because otherwise the grid wasn't getting moved
                }
            }
        }

        if( pEdit && !fieldText.IsEmpty() )
        {
            pEdit = SearchEdit(pField);  // we need to search for the window again because the old pointer is no longer valid
            pEdit->SetWindowText(fieldText);
            pEdit->SetRemoveTxtFlag(false);
            pEdit->SetCaretPos(caret);
        }

        ScrollToPosition(scrollPos);
    }
}


/////////////////////////////////////////////////////////////////////////////
// CEntryrunView message handlers
/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CEntryrunView::OnEraseBkgnd(CDC* pDC)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CEntryrunView::OnEraseBkgnd(CDC* pDC)
{
    CRect rect;
    GetClientRect(rect);

    CEntryrunDoc* pRunDoc = GetDocument();
    CDEFormFile* pFF = pRunDoc->GetCurFormFile();

    //Get the current form Number
    int iFormNum = pRunDoc->GetCurFormNum();
    const CDEForm* pForm = pFF->GetForm(iFormNum);

    COLORREF background_color = ( pForm != nullptr ) ? pForm->GetBackgroundColor().ToCOLORREF() :
                                                       GetSysColor(COLOR_BTNFACE);
    pDC->FillSolidRect(rect, background_color);

    return TRUE;
}


///////////////////////////////////////////////////////////////
//Edit Controls management routines
/////////////////////////////////////////////////////////////////////////////////
//
//      int CEntryrunView::AddEdit(CDEBaseEdit* pEdit)
//
/////////////////////////////////////////////////////////////////////////////////
int CEntryrunView::AddEdit(CDEBaseEdit* pEdit)
{

    ASSERT_KINDOF(CDEBaseEdit, pEdit);
    return m_aEdit.Add(pEdit);
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::InsertEditAt(CDEBaseEdit* pEdit, int iIndex)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::InsertEditAt(CDEBaseEdit* pEdit, int iIndex)
{
    ASSERT_KINDOF(CDEBaseEdit, pEdit);
    m_aEdit.InsertAt (iIndex, pEdit);
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::RemoveEdit(int iIndex)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::RemoveEdit(int iIndex)
{
    delete  m_aEdit[iIndex];
    m_aEdit.RemoveAt(iIndex);
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::RemoveAllEdits(void)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::RemoveAllEdits(void)
{
    int iSize = m_aEdit.GetSize();
    for (int i = 0; i<iSize; i++) {
        CDEBaseEdit* pEdit = m_aEdit[i];
        m_aEdit[i] = NULL;
        ASSERT( pEdit );
        if( pEdit ){
            //AfxMessageBox( pEdit->GetField()->GetName() );

            delete( pEdit );
        }
        //delete  m_aEdit[i];
    }
    m_aEdit.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::RemoveAllGrids2(void)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::RemoveAllGrids2(bool bDelete /*=true*/)
{
    int iSize = m_aGrid.GetSize();
    for (int i = 0; i<iSize; i++) {
        if(bDelete){
            CDEBaseEdit* pEdit = DYNAMIC_DOWNCAST(CDEBaseEdit,m_aGrid[i]->GetEdit());
            if(pEdit){
                pEdit->DestroyWindow();
                m_aGrid[i]->SetEdit(NULL);
                delete pEdit;
            }
            m_aGrid[i]->DestroyWindow();
            delete  m_aGrid[i];
        }
        else {
            CDEBaseEdit* pEdit = DYNAMIC_DOWNCAST(CDEBaseEdit,m_aGrid[i]->GetEdit());
            if(pEdit){
                pEdit->ShowWindow(SW_HIDE);
                pEdit->SetWindowText(_T(""));
                pEdit->SetModifiedFlag(false);
            }
            m_aGrid[i]->ShowWindow(SW_HIDE);
            int iScrollCol = 1;
            if (m_aGrid[i]->GetRoster()->GetRightToLeft()) {
                iScrollCol = m_aGrid[i]->GetNumCols();
            }
            m_aGrid[i]->ScrollTo(1, iScrollCol);
            m_aGrid[i]->m_bRedraw = false;
        }
    }
    if(bDelete){
        m_aGrid.RemoveAll();
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      bool CEntryrunView::ShowGrid(CDERoster* pRoster ,bool bShow /*=TRUE*/)
//
/////////////////////////////////////////////////////////////////////////////////
bool CEntryrunView::ShowGrid(CDERoster* pRoster ,bool bShow /*=true*/)
{
    int iSize = m_aGrid.GetSize();
    bool bRet = false;
    for (int i = 0; i<iSize; i++) {
        {
            CDEGrid* pGrid = m_aGrid[i];
            if(pGrid->GetRoster() == pRoster) {
                if(bShow){
                    bRet =true;

                    m_aGrid[i]->ShowWindow(SW_SHOW);
                    break;
                }
                else {
                    bRet = true;
                    m_aGrid[i]->ShowWindow(SW_HIDE);
                    break;
                }
            }
        }
    }
    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ResetGrids()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ResetGrids(bool bForceReset /*= false*/)
{
    int iSize = m_aGrid.GetSize();
    for (int i = 0; i<iSize; i++) {
        CDEGrid* pGrid = m_aGrid[i];
        pGrid->ResetGridData(bForceReset);
    }
}
/***************************************************************************************
Form Draw
*****************************************************************************************/
/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CEntryrunView::ResetForm()
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CEntryrunView::ResetForm()
{
    CEntryrunDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);

    ScrollToPosition(CPoint(0,0));

    //TO DO Get Current FORM            // RHF Jan 12, 2000   pls see above

    //For Now get it from the pifobject
    CNPifFile* pPIF = pDoc->GetPifFile();
    if(!pPIF)
        return FALSE;
    Application* pApp = pPIF->GetApplication();
    if(!pApp)
        return FALSE;

    // RHF COM Jan 12, 2000 //for Now we are assuming only one form
    CDEFormFile* pFormfile = pDoc->GetCurFormFile();
    CDEBaseEdit::SetFieldFont(&pFormfile->GetFieldFont().GetCFont());

    CEntryrunDoc* pRunDoc = GetDocument();

    //Get the current form Number
    int iFormNum = pRunDoc->GetCurFormNum();

    if(iFormNum == -1)
        return FALSE;

    CDEForm* pForm = pFormfile->GetForm(iFormNum);

    //No form to work with
    if (!pForm)
        return FALSE;

    CClientDC dc(this);
    ClearScreen ((CDC*)&dc);
    OnEraseBkgnd(&dc);

    dc.SetBkColor(pForm->GetBackgroundColor().ToCOLORREF());

    if( pApp->GetCenterForms() ) // 20100421
    {
        RECT rect;
        GetWindowRect(&rect);

        if( pForm->UpdateDims(rect.right - rect.left) ) // UpdateDims returning true means grids need to be moved
        {
            for( int i = 0; i < pForm->GetNumItems(); i++ )
            {
                CDEItemBase* pItem = pForm->GetItem(i);

                if( pItem->GetItemType() == CDEFormBase::Roster )
                {
                    CDEGrid* pGrid = FindGrid((CDERoster*)pItem);

                    if( pGrid != nullptr )
                    {
                        RECT grid_rect = static_cast<RECT>(pItem->GetDims());
                        pGrid->MoveWindow(&grid_rect); // this is necessary because otherwise the grid wasn't getting moved
                    }
                }
            }
        }
    }

    else
    {
        pForm->UpdateDims(); //added this to fix the scroll sizez correctly //if perormance is affected do it one time later on
    }


    SetScrollSizes(MM_TEXT, CSize (pForm->GetDims().BottomRight()));        // each pg has unique scroll size

    //Remove all edits (later on do this only if the current form has changed)
    RemoveAllEdits();
    RemoveAllGrids2(false);

    // clear any extended controls
    DeleteLabels();

    if(pRunDoc->GetAppMode() == NO_MODE)
        return TRUE ;
    //Draw all the static items first
    DrawStaticItems();


    //recurse thru the items
    int iItems = pForm->GetNumItems();

    for (int iIndex = 0; iIndex < iItems; iIndex++) {

        CDEItemBase* pItem = (CDEItemBase*) pForm->GetItem(iIndex);

        switch (pItem->GetItemType()){

        case CDEFormBase::Roster:
            {
                CDERoster* pRoster = (CDERoster*) pItem;
                //Find the grid from the array and display if it exists
                if(ShowGrid(pRoster)){
                    CDEGrid* pGrid = FindGrid(pRoster);
                    if(pGrid){
                        pGrid->ResetGridData();
                    }
                    break;
                }

                //Set the dict item for the roster
                for (int iIndex2 = 0; iIndex2 < pRoster->GetNumItems() ; iIndex2++){
                    CDEItemBase* pBase = pRoster->GetItem(iIndex2);
                    SetDictItem((CDEField*)pBase);
                }
                pRoster->RefreshStubsFromOccurrenceLabels(*pFormfile);
                CDEGrid* pGrid = new CDEGrid(pRoster);
                pGrid->SetDecimalChar(DECIMAL_CHAR);
                pGrid->SetGridBkColor(pForm->GetBackgroundColor().ToCOLORREF());

                // following code changed to account for grids that were never rendered on the screen before
                // saving; their dims will be (0,0,0,0).  This code is pasted from CFormScrollView::CreateGrid
                // CSC 10/19/00
                //                pGrid->Create(pRoster->GetDims(),this,pRoster);
#define MIN_NUM_ROSTER_COLS  6  // min view is 5 cols + 1 for stub
#define MIN_NUM_ROSTER_ROWS 11  // 10 fields + 1 for stub
                if (pRoster->GetDims().BottomRight()==CPoint(0,0))  {
                    // user is creating a new roster, no viewport size available ... show based on num rows/cols
                    int iRows = std::min (MIN_NUM_ROSTER_ROWS, pRoster->GetMaxLoopOccs()+1);   // +1 csc 11/16/00
                    int iCols = std::min (MIN_NUM_ROSTER_COLS, pRoster->GetNumCols());
                    if (pRoster->GetOrientation() == RosterOrientation::Vertical)  {
                        int iTmp = iRows;
                        iRows = iCols;
                        iCols = iTmp;
                    }
                    pGrid->Create(pRoster->GetDims().TopLeft(), iRows, iCols, this, pRoster);
                }
                else  {
                    // this roster already existed at some point, use previous dimensions...
                    pGrid->Create(pRoster->GetDims(), this, pRoster);
                }

                m_aGrid.Add(pGrid);

                CDEBaseEdit* pEdit = new CDEEdit();
                //Create the edit field
                pEdit->SetField((CDEField*)pRoster->GetItem(0));
                pEdit->Create(ES_AUTOHSCROLL, CRect(0,0,0,0), this, (UINT)-1);
                pEdit->ModifyStyleEx(WS_EX_CLIENTEDGE,0);

                pGrid->SetEdit(pEdit);
                pEdit->ShowWindow(SW_HIDE);
            }
            break;
        case CDEFormBase::Field :
            SetDictItem(assert_cast<CDEField*>(pItem));
            AddEditItem(assert_cast<CDEField*>(pItem));
            break;

        case CDEFormBase::Text  :
            break;
        default:
            break;
        }
    }

    //Set focus to the current Field
    GoToFld((CDEField*)pDoc->GetCurField());
    if((CDEField*)pDoc->GetCurField()){
        this->SetCurrentFormFileNum((CDEField*)pDoc->GetCurField());
    }

    // JH 12/7/05 start scrolled to right for languages like Arabic
    CPoint scrollToPt(0,0);

    if(pFormfile->GetRTLRostersFlag()){
        scrollToPt = CPoint(pForm->GetDims().right, 0);
    }

    ScrollToPosition(scrollToPt);   // order of scroll/update changed to accomodate intelligent update  CSC 6/7/00
    UpdateFields();

    if(pForm && pDoc->GetPifFile()->GetApplication()->GetUseQuestionText()) {
        AfxGetMainWnd()->SendMessage(UWM::Capi::SetWindowHeight, pForm->GetQuestionTextHeight());
    }

    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ClearScreen(CDC* pDC)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ClearScreen(CDC* pDC)
{
    CRect rcClient;
    GetClientRect(rcClient);

    pDC->FillSolidRect(rcClient,::GetSysColor(COLOR_BTNFACE));
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::DrawStaticItems()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::DrawStaticItems()
{
    CEntryrunDoc*   pDoc = GetDocument();

    //TO DO Get Current FORM            // RHF Jan 12, 2000   pls see above
    ASSERT_VALID(pDoc);

    //For Now get it from the pifobject
    CNPifFile* pPIF = pDoc->GetPifFile();
    if(pPIF == nullptr)
        return;

    //If the application is not loaded return
    Application* pApp = pPIF->GetApplication();

    CEntryrunDoc* pRunDoc = GetDocument();
    const CRunAplEntry* pRunApl = pRunDoc->GetRunApl();
    if (pRunApl == nullptr || !pRunApl->HasAppLoaded())
        return;

    //for Now we are assuming only one form later on get current form file
    CDEFormFile* pFormfile = NULL;
    if(!pApp->GetRuntimeFormFiles().empty()) {
        pFormfile = pRunDoc->GetCurFormFile();
    }
    else {
        return;
    }

    int iFormNum = pRunDoc->GetCurFormNum();
    const CDEForm* pForm = pFormfile->GetForm(iFormNum);

    //No form to work with
    if (pForm == nullptr)
        return;

    CClientDC dc(this);
    //Problem in windows 98 for Grids > 3 in the new scheme . Getting Mainwindow DC instead of runview DC
    //So explicitly get the views DC and use it

    int iSaveDC = dc.SaveDC();
    OnPrepareDC((CDC*)&dc);
    dc.SetBkColor(pForm->GetBackgroundColor().ToCOLORREF());//

    //Draw the boxes as they are
    pForm->GetBoxSet().Draw(&dc);

    //recurse thru the items
    for( int iIndex = 0; iIndex < pForm->GetNumItems(); iIndex++ )
    {
        // Display the associated text of the field, or standalone text
        const CDEItemBase* pItem = pForm->GetItem(iIndex);
        const CDEText* pText = ( pItem->GetItemType() == CDEFormBase::Field ) ? &assert_cast<const CDEField*>(pItem)->GetCDEText() :
                               ( pItem->GetItemType() == CDEFormBase::Text )  ? assert_cast<const CDEText*>(pItem) :
                                                                                nullptr;

        if( pText != nullptr )
            pText->DrawMultiline(&dc);
    }

    dc.RestoreDC(iSaveDC);
}

/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CEntryrunView::AddEditItem(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CEntryrunView::AddEditItem(CDEField* pField)
{
    //default style for edit control
    DWORD dwStyle = WS_CHILD | WS_VISIBLE;

    //Get the rect for the field
    CRect fldRect = pField->GetDims();


    CClientDC dc(this);
    CSize sizeCh = dc.GetTextExtent(_T("9"));
    /*if(fldRect.Height() < sizeCh.cy + 6) {
        //SAVY&& if SMG changes her logics you have to change this
        //This should be called only if the height is insufficient to enter data
        int iDiff = sizeCh.cy + 6 -fldRect.Height();
        int iInc = 0;
        if(iDiff % 2) {
            iInc = (iDiff+1) / 2;
        }
        else {
            iInc = iDiff / 2;
        }
        fldRect.top -= iInc;
        fldRect.bottom += iInc;
    }*///redundant code


    const CDictItem* pItem = pField->GetDictItem();
    //Get the data type
    ContentType contentType;
    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");

    if(pItem) {
        contentType = pItem->GetContentType();
    }
    else {
        //The Field has to be either from a dict item or from a VSet
        contentType = ContentType::Alpha;
    }

    //Set the edit style
    if(contentType == ContentType::Numeric) {
        //I cannot Set this 'Cos ES_AUTOHSCROLL Will not work with ES_RIGHT
        dwStyle |= (ES_MULTILINE | ES_RIGHT);
        //dwStyle |=  ES_RIGHT;
    }
    else {
        dwStyle |= ES_LEFT |ES_AUTOHSCROLL ;
        bool bMultiLine = pField->AllowMultiLine();
        if(bMultiLine && pField->UseUnicodeTextBox()){
            dwStyle = ES_LEFT | ES_MULTILINE |ES_AUTOVSCROLL ; //No AutoHScroll to enable wrap
        }
        else if(pField->UseUnicodeTextBox()){//kludge - without this the border on the top does not get drawn right in the unicode text box
             dwStyle |= ES_MULTILINE;
        }
    }

    //You can set here the password style for the field if it is required
    if (pField->IsProtected()|| pField->IsMirror())
        dwStyle |= ES_READONLY;

    CDEBaseEdit* pEdit = NULL;
    //For now all alphaa are created wih the new text field.
    if (pField->UseUnicodeTextBox() && pField->GetDictItem()->GetContentType() == ContentType::Alpha ){
            dwStyle |=  WS_BORDER;
            pEdit =  (CDEBaseEdit*)(new CDETextEdit());
    }
    else {
        pEdit =  new CDEEdit();
    }

    //Create the edit field
    pEdit->SetField(pField);
    pEdit->Create(dwStyle, &fldRect, this, (UINT)-1);

    if (pField->IsProtected()|| pField->IsMirror()){
        if(pField->GetVerifyFlag() && pField->IsMirror()){
            pField->SetVerifyFlag(false);
        }
    }

    //Get the data for the field
    CIMSAString sData= pField->GetData();
    if(contentType == ContentType::Numeric) {
        sData.Trim();
    }

    //Set the edit field text
    pEdit->SetWindowText(sData);

    if(pField->GetDictItem())
        pEdit->LimitText(pField->GetDictItem()->GetLen());
    //pEdit->SetFont(pDoc->m_pFontData);

    AddEdit(pEdit);

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
    CScrollView::OnPrepareDC(pDC, pInfo);
    if(pInfo) {     //if your are printing
        pDC->SetMapMode(MM_LOENGLISH);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CEntryrunView::SetDictItem(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
bool CEntryrunView::SetDictItem(CDEField* pField)
{
    if(pField->GetDictItem() != nullptr)
        return true;

    //Get the current formFile
    CEntryrunDoc* pDoc = GetDocument();
    //For Now assume one form file (It is better if the engine keeps track of this current
    //form file in case of multiple form file scenarios)
    CDEFormFile* pFormFile = pDoc->GetCurFormFile();

    //Search thru the dicts for the dict item
    CString sDictName = pField->GetItemDict();

    if( pFormFile->GetDictionary()->GetName().CompareNoCase(sDictName) == 0 ) {
        const CDictItem* pItem = pFormFile->GetDictionary()->LookupName<CDictItem>(pField->GetItemName());

        if(pItem != nullptr) {
            pField->SetDictItem(pItem);
            return true;
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CEntryrunView::OnEditChange (UINT wParam, LONG lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CEntryrunView::OnEditChange (UINT wParam, LONG lParam)
{
    CDEBaseEdit* pEdit = (CDEBaseEdit*)lParam;

    bool bProcessSpreadSheetMode = false;
    if(pEdit && pEdit->GetField()){
     CDEField* pField = pEdit->GetField();
      if(pField->GetParent()->IsKindOf(RUNTIME_CLASS(CDERoster))){
          CDERoster* pRoster = (CDERoster*)pField->GetParent();
          if(pRoster->UsingFreeMovement()){
              bProcessSpreadSheetMode = true;
          }
      }
    }

    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();
    switch(wParam)  {

    case VK_ESCAPE:
        {
            if(!pEdit->GetModifiedFlag()){
                ((CMainFrame*)AfxGetMainWnd())->OnStop();
                return 0L;
            }
            else{
                CString csValue = pRunApl->GetVal(pEdit->GetField());
                pEdit->SetModifiedFlag(false);
                if(pRunDoc->GetAppMode() != VERIFY_MODE){
                    pEdit->SetWindowText(csValue);
                }
                pEdit->SendMessage(WM_SETFOCUS);
            }

        }
        break;
    case VK_F2:
        {
            if (!pRunApl || !pRunApl->HasAppLoaded() )
                return 0L;

            CCapi* pCapi=pRunApl->GetCapi();
            const CDictItem* pItem = pEdit->GetField()->GetDictItem();

            if( pItem != NULL ) {
                DEFLD DeFld;

                pRunApl->GetDeFld( pEdit->GetField(), &DeFld );

                pCapi->SetFrameWindow( this );
                pCapi->SetAroundField( pEdit );
                pCapi->ToggleHelp( &DeFld );
            }
        }
        break;

    case VK_RETURN:
        {
            CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,pEdit->GetField()->GetParent());
            if(pRoster && pRoster->UsingFreeMovement()) {
                RosterEdit(pEdit,VK_RETURN);
            }
            else {
                OnEditEnter(pEdit);
            }

        }
        break;

        // RHF INIC Jan 28, 2000
    case -VK_TAB:
        OnEditPrev(pEdit);
        break;
        // RHF INIC Jan 28, 2000

    case VK_TAB:
        if (GetKeyState(VK_SHIFT) < 0)  {
            // shift-tab
            OnEditPrev(pEdit);

        }
        else  {
            // regular tab key
            OnEditEnter(pEdit);
        }
        break;
    case VK_NEXT:
       // if(pRunDoc->GetAppMode() != VERIFY_MODE){
            OnPageDown((WPARAM)0,(LPARAM)pEdit->GetField());
        //}
        break;
    case VK_PRIOR:
        OnPageUp((WPARAM)0,(LPARAM)pEdit->GetField());
        break;
    case VK_UP:

    case VK_LEFT:
        if(!bProcessSpreadSheetMode)
        {
            BOOL bMode = pRunDoc->GetAppMode() == MODIFY_MODE || pRunDoc->GetAppMode() == ADD_MODE || pRunDoc->GetAppMode() == VERIFY_MODE;
            if(pRunDoc && bMode )
                OnEditPrev(pEdit);
        }
        else {
            BOOL bMode = pRunDoc->GetAppMode() == MODIFY_MODE || pRunDoc->GetAppMode() == ADD_MODE || pRunDoc->GetAppMode() == VERIFY_MODE;
            if(pRunDoc && bMode )
                RosterEdit(pEdit,wParam);
        }
        break;

    case VK_DOWN:

    case VK_RIGHT:
        if(!bProcessSpreadSheetMode){
            BOOL bMode = pRunDoc->GetAppMode() == MODIFY_MODE || pRunDoc->GetAppMode() == ADD_MODE || pRunDoc->GetAppMode() == VERIFY_MODE;
            if(pRunDoc && bMode)
                OnEditEnter(pEdit);
        }
        else {
            BOOL bMode = pRunDoc->GetAppMode() == MODIFY_MODE || pRunDoc->GetAppMode() == ADD_MODE || pRunDoc->GetAppMode() == VERIFY_MODE;
            if(pRunDoc && bMode)
                RosterEdit(pEdit,wParam);
        }
        break;
    default:
        ASSERT(FALSE);
        break;
    }


    return (0L);
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::GoToFld(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::GoToFld(CDEField* pField) {
    DoGoToFld( pField );
}

void CEntryrunView::DoGoToFld(CDEField* pField)
{
    //Get the parent of the field
    //if it is a roster then get the corresponding grid and set the field
    CEntryrunDoc* pRunDoc = GetDocument();
    if(!pField)
        return;

    CDEGroup* pGroup = pField->GetParent();

    if(pGroup && pGroup->IsKindOf(RUNTIME_CLASS(CDERoster))) {
        CDEGrid* pGrid = FindGrid(pGroup);

        if(pGrid) { //UNComment all these line once the implemenatation is in place
            int iOcc =pGroup->GetCurOccurrence();
            //when in path off prevent going beyond the dynamix max occs. This case happens when
            //you have data occurrence in the file more than the max dynamic occs and you enter the case
            //in modify mode
            if (!pRunDoc->GetCurFormFile()->IsPathOn() && pGroup->GetMaxDEField() && iOcc > GetDynamicMaxOccs(pGroup)) return;
            if( iOcc <= 0 ) return; // RHF Jun 13, 2001 SERPRO
            if(!pGrid->m_bRedraw){
                pGrid->SetRedraw(FALSE);
            }
            pGrid->GoToField(pField,iOcc);
            if(!pGrid->m_bRedraw){
                pGrid->SetRedraw(TRUE);
                pGrid->RedrawWindow();
                pGrid->m_bRedraw = true;
            }
            if(pGrid->GetEdit()){
                pGrid->StartEdit(pField,iOcc);
                //pGrid->GetEdit()->SetSel(0,-1);
                pRunDoc->SetCurField( pField );
            }
            return;

        }
    }
    //Find the CDEBaseEdit which has this as its associated field
    int iCount = m_aEdit.GetSize();
    for(int iIndex =0; iIndex < iCount ; iIndex ++) {
        if(m_aEdit[iIndex]->GetField() == pField) {
            m_aEdit[iIndex]->SetFocus();
            ContentType contentType;
            CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
            const CDictItem* pItem = pField->GetDictItem();

            if(pItem) {
                contentType = pItem->GetContentType();
            }
            else {
                //The Field has to be either from a dict item or from a VSet
                contentType = ContentType::Alpha;
            }

            //Set the edit style
            CIMSAString sString ;
            //      CDEBaseEdit* pEdit  = m_aEdit[iIndex];
            if(m_aEdit[iIndex]->GetModifiedFlag()) {
                m_aEdit[iIndex]->GetWindowText(sString);
            }
            else {
                sString = pField->GetData();
            }
            if(contentType == ContentType::Numeric) {
                sString.Trim();
            }
            if(pRunDoc->GetAppMode() != VERIFY_MODE)
                m_aEdit[iIndex]->SetWindowText(sString);
            else
                m_aEdit[iIndex]->SetWindowText(m_sVerify);
            m_aEdit[iIndex]->SetSel(0,-1);
            //                      m_aEdit[iIndex]->SetSel(-1,-1);

            break;
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnEditEnter(CDEBaseEdit* pEdit)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnEditEnter(CDEBaseEdit* pEdit)
{
    m_pOldField = NULL;
    //Validation happens before the control comes this place at
    //the subclassed Edit control so the data is Valid for the
    //questionnaire
    CEntryrunDoc* pRunDoc = GetDocument();

    if(!OutOfSequence(pEdit))  {
        pEdit->SetFocus();
        return;
    }

    if(pRunDoc->GetAppMode() == VERIFY_MODE)
    {
        CIMSAString sWindowText;
        pEdit->GetWindowText(sWindowText) ;
        CIMSAString sSafeWindowText  = sWindowText;
        CDEField* pField = pEdit->GetField();
        CIMSAString sData = pRunDoc->GetRunApl()->GetVal(pField->GetSymbol(), 0);

        if(pEdit->GetField()->GetDictItem()->GetContentType() == ContentType::Numeric) {
            //if the item has zero fill  and is not a decimal zero fill the sWindowText
            const CDictItem* pDictItem = pEdit->GetField()->GetDictItem();
            if(pDictItem->GetDecimal() ==0 && pDictItem->GetZeroFill()){
                sWindowText.Trim();
                if(!sWindowText.IsEmpty()) {
                    int iLength = pDictItem->GetLen() - sWindowText.GetLength();
                    CString sZero(_T('0'),iLength);
                    sWindowText = sZero+sWindowText;
                }

            }
            else {//it is decimal
                //now decimal csprochar could be yes or could be no
                const CDictItem* pDictItem = pEdit->GetField()->GetDictItem();
                if(pDictItem->GetDecChar()){//Dec Char yes
                    if(pDictItem->GetZeroFill()){// zero fill yes
                        sWindowText.Replace(_T(' '), '0');
                        sData.Replace(_T(' '),'0');
                    }
                    else {
                        sWindowText.Trim();
                        if( !sWindowText.Compare(_T(".")) || !sWindowText.Compare(_T(",")) ){
                            sWindowText =_T("");
                        }

                        else // 20140528 to ensure that verification of decimals works properly
                        {
                            int iDotPos = sWindowText.FindOneOf(_T(".,"));
                            int iCurrentNumDecimals = sWindowText.GetLength() - iDotPos - 1;
                            int iDecimalsToAdd = pDictItem->GetDecimal() - iCurrentNumDecimals;

                            if( iDecimalsToAdd > 0 )
                                sWindowText.Append(CString(_T('0'),iDecimalsToAdd));
                        }
                    }

                    if( sWindowText.Find(_T(',')) >= 0 ) // convert periods to commas, otherwise the window text won't match the data in the buffer
                        sData.Replace(_T('.'),_T(','));
                }
                else {//Dec Char no
                    int iDotPos = sWindowText.FindOneOf(_T(".,")); // 20140528 the decimal portion of the number wasn't being zero-filled in non-zerofill situations
                    sWindowText.Remove('.');
                    sWindowText.Remove(',');

                    if(pDictItem->GetZeroFill()) // zero fill yes
                    {
                        sWindowText.Replace(_T(' '), '0');
                        sData.Replace(_T(' '),'0');
                    }

                    else
                    {
                        //Savy to fix the decimal blank field causing mismatch in verify mode due to zero fill when entering blank 2/19/2015
                        CIMSAString sTemp = sWindowText;
                        sTemp.Trim();//In case of just the presence of only the dec char, do not zero fill the decimal part just return the empty string

                        if( iDotPos >= 0 && sTemp.GetLength() > 0)
                        {
                            for( int i = iDotPos; i < sWindowText.GetLength(); i++ ) // start at iDotPos because the decimal point has been removed
                            {
                                if( sWindowText[i] == _T(' ') )
                                    sWindowText.SetAt(i,_T('0'));
                            }
                        }

                        sWindowText.Trim();
                    }

                }
            }
           sData.Trim();
        }
        if(sWindowText.CompareNoCase(sData) != 0) { // if the user typed other than the original text

            if(m_sVerify.CompareNoCase(sSafeWindowText) == 0 ) { // if it is same as the previous text
                m_iVerify++;
                if(m_iVerify < 2) {
                    CCustMsg custMsg;
                    custMsg.m_eMsgType = VerifyMsg;
                    custMsg.m_pEdit = pEdit;
                    ::MessageBeep(0);
                    custMsg.ShowMessage();
                    pEdit->SetRemoveTxtFlag(TRUE);

                    m_sVerify = sSafeWindowText;
                    GoToFld(pEdit->GetField());
                    return;
                }
                else {
                    pRunDoc->AddVerifyError(TRUE);  //The keyer made an error and the verified modified it
                    m_sVerify.Empty();
                    m_iVerify = 0;
                }
            }
            else {
                CCustMsg custMsg;
                custMsg.m_eMsgType = VerifyMsg;
                custMsg.m_pEdit = pEdit;
                ::MessageBeep(0);
                custMsg.ShowMessage();
                pEdit->SetRemoveTxtFlag(TRUE);

                m_iVerify = 1;
                m_sVerify = sSafeWindowText;
                GoToFld(pEdit->GetField());
                return ;
            }
        }
        else {
            if(m_iVerify > 0) {
                pRunDoc->AddVerifyError();  //The verifier made an error and then corrected to the original
            }

            m_sVerify.Empty();
            m_iVerify = 0;

        }
    }
    //Get the Data from the Edit Control
    CIMSAString  sData;
    pEdit->GetWindowText(sData);

    //SetGridEdit(pEdit->GetField()); commented this and moved this down for accept message display problems

    BOOL bCheckModified = FALSE;
    APP_MODE app_mode = pRunDoc->GetAppMode();

    if(app_mode == MODIFY_MODE || app_mode == VERIFY_MODE || app_mode == ADD_MODE)
        bCheckModified = TRUE;
    if(bCheckModified && !pRunDoc->GetQModified()) {
        CIMSAString sOData;
        CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,pEdit->GetField()->GetParent());
        if(pRoster) {
            int iOcc = pRoster->GetCurOccurrence();
            CDEGrid* pGrid = FindGrid(pRoster);
            sOData = pGrid->GetFieldData(pEdit->GetField(),iOcc); //original value
        }
        else {
            sOData = pEdit->GetField()->GetData(); //original value
        }
        CIMSAString sLData = sData;
        if(pEdit->GetField()->GetDictItem()->GetContentType() == ContentType::Numeric) {
            sOData.Trim();
            sLData.Trim();
        }
        if(sLData.CompareNoCase(sOData) != 0 ) {
            pRunDoc->SetQModified(TRUE);
        }
    }

    //Set the data in the field
    const CDictItem* pDictItem = pEdit->GetField()->GetDictItem();

    if( pDictItem->GetContentType() == ContentType::Numeric && pDictItem->GetDecimal() ) // 20120312
        sData.Replace(',','.');

    if( pEdit->GetField()->AllowMultiLine() ) // 20120816
        sData = WS2CS(SO::ToNewlineLF(CS2WS(sData)));

    pEdit->GetField()->SetData(sData);

    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetDocument();
    CRunAplEntry* pApl = pDoc->GetRunApl();

    //Go to the next field  here the data is saved
    CDEItemBase* pItem = NULL;
    pItem = pApl->NextField(TRUE);

    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
    if(pEdit->GetField()->GetDictItem()->GetContentType() == ContentType::Alpha){
        CPoint point(0,0);
        CDEEdit * pDEEdit = DYNAMIC_DOWNCAST(CDEEdit,pEdit);
        if(pDEEdit){
            pDEEdit->CalcCaretPos(point);
            pDEEdit->SetCaretPos(point); //BMD request for reenter on Alpha fields
        }
    }

    SetGridEdit(pEdit->GetField());

    //if(pItem != pDoc->GetCurField() || iCurrentOcc != pItem->GetParent()->GetCurOccurrence()) {
    if(pItem) {
        pEdit->SetModifiedFlag(false);
        pEdit->SetRemoveTxtFlag(true);
    }
    //}

    if(app_mode == VERIFY_MODE)
    {
        pDoc->IncVerifiedField();
    }

    if(!pItem) {

        //SAVY &&&  if it is other an add mode just assume this is end of the case and if it is modified
        //Go to the next case
        //How do I save a questionnaire ???
        int iMode=pDoc->GetAppMode();
        if(iMode != ADD_MODE)  {
            pDoc->SetQModified(FALSE);
            ProcessModifyMode();
            return;
        }
        else  {
            // RHF INIC Nov 19, 2002
            if(pDoc->IsPartialAdd())  {
                CNPifFile* pPIF = pRunDoc->GetPifFile();
                bool bAutoAdd =pPIF->GetAutoAddFlag();

                bool bLast = ((CMainFrame*)AfxGetMainWnd())->GetCaseView()->IsLastCase();
                pDoc->SetQModified(FALSE);
                pDoc->SetCurField(NULL); // RHF Feb 23, 2004
                ((CMainFrame*)AfxGetMainWnd())->OnStop();
                if(bLast && bAutoAdd) {
                   // ((CMainFrame*)AfxGetMainWnd())->OnAdd(); to fix stop(-1) ghost process when ids are all protected
                    ((CMainFrame*)AfxGetMainWnd())->PostMessage(WM_COMMAND,ID_ADD);
                }
                return ;

            }
            else {
                pDoc->SetQModified(FALSE);
                pDoc->SetCurField(NULL); // RHF Feb 23, 2004
                ((CMainFrame*)AfxGetMainWnd())->OnStop();
            }
            // RHF END Nov 19, 2002
            return;
        }
    }


    ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));


    if(pDoc->GetAppMode() == ADD_MODE)
        ProcessFldAttrib(assert_cast<CDEField*>(pItem));

    ShowCapi( (CDEField *) pItem ); // RHF Jan 20, 2000
}


void CEntryrunView::OnEditPrev(CDEBaseEdit* pEdit)
{
    m_pOldField = NULL;
    //Validation happens before the control comes this place at
    //the subclassed Edit control so the data is Valid for the
    //questionnaire

    //Get the Data from the Edit Control
    CIMSAString  sData;
    pEdit->GetWindowText(sData);

    const CDictItem* pDictItem = pEdit->GetField()->GetDictItem();

    if( pDictItem->GetContentType() == ContentType::Numeric && pDictItem->GetDecimal() ) // 20120312
        sData.Replace(',','.');

    if( pEdit->GetField()->AllowMultiLine() )
        sData = WS2CS(SO::ToNewlineLF(CS2WS(sData)));

    int iPrevOcc = pEdit->GetField()->GetParent()->GetCurOccurrence();


    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetDocument();
    CRunAplEntry* pApl = pDoc->GetRunApl();

    //Set the data in the field
    if(pDoc->GetAppMode() != VERIFY_MODE){
        pEdit->GetField()->SetData(sData);
    }

    SetGridEdit(pEdit->GetField());


    //Go to the next field  here the data is saved
    CDEItemBase* pItem = NULL;
    if(pDoc->GetAppMode() != VERIFY_MODE){
        pItem = pApl->PreviousField(TRUE);
    }
    else {
        pItem = pApl->PreviousField(FALSE);
    }

    if(pItem && pItem != pDoc->GetCurField()) {
        pEdit->SetModifiedFlag(false);
        pEdit->SetRemoveTxtFlag(true);
        //SAVY for Bug#2102 Uparrow in verify retains text from the previous field
        if(pDoc->GetAppMode() == VERIFY_MODE){//when the item changes and it is verify mode
            m_sVerify.Empty();
        }
    }
    else if(pItem) {
        int iCurOcc = pItem->GetParent()->GetCurOccurrence();
        if(iCurOcc != iPrevOcc) {//when the occurrence changes
            pEdit->SetModifiedFlag(false);
            pEdit->SetRemoveTxtFlag(true);
            //SAVY for Bug#2102 Uparrow in verify retains text from the previous field
            if(pDoc->GetAppMode() == VERIFY_MODE){
                m_sVerify.Empty();
            }
        }
    }
    else if(!pItem){//Process the case where onfocus can end the questionnaire by using onstop

        //SAVY &&&  if it is other an add mode just assume this is end of the case and if it is modified
        //Go to the next case
        //How do I save a questionnaire ???
        int iMode=pDoc->GetAppMode();
        if(iMode != ADD_MODE)  {
            pDoc->SetQModified(FALSE);
            ProcessModifyMode();
        }
        else  {
            // RHF INIC Nov 19, 2002
            if(pDoc->IsPartialAdd())  {
                CNPifFile* pPIF = pDoc->GetPifFile();
                bool bAutoAdd =pPIF->GetAutoAddFlag();

                bool bLast = ((CMainFrame*)AfxGetMainWnd())->GetCaseView()->IsLastCase();
                pDoc->SetQModified(FALSE);
                pDoc->SetCurField(NULL); // RHF Feb 23, 2004
                ((CMainFrame*)AfxGetMainWnd())->OnStop();
                if(bLast && bAutoAdd) {
                   // ((CMainFrame*)AfxGetMainWnd())->OnAdd(); to fix stop(-1) ghost process when ids are all protected
                    ((CMainFrame*)AfxGetMainWnd())->PostMessage(WM_COMMAND,ID_ADD);
                }

            }
            else {
                pDoc->SetQModified(FALSE);
                pDoc->SetCurField(NULL); // RHF Feb 23, 2004
                ((CMainFrame*)AfxGetMainWnd())->OnStop();
                return;
            }
            // RHF END Nov 19, 2002
        }
    }

    if( pItem == NULL )
        return; // RHF Feb 17, 2000

    // SAVY 06/14 this->ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));
    //Do not call update fields

    {
        CDEField* pNewField = assert_cast<CDEField*>(pItem);
        ASSERT(pNewField);
        CEntryrunDoc * pRunDoc = GetDocument();
        //For Now

        CDEField*    pField=  (CDEField*)pRunDoc->GetCurField();
        int iCurFormNum = -1;
        if(pField)
            iCurFormNum = pField->GetFormNum();
        pRunDoc->SetCurField(pNewField);

        BOOL bFormFileChange = FALSE;
        int iOldFormFileNum = this->GetCurrentFormFileNum();

        SetCurrentFormFileNum( pNewField );

        if(iOldFormFileNum != GetCurrentFormFileNum()) {
            bFormFileChange = TRUE;
        }



        if(pNewField->GetFormNum() != iCurFormNum || bFormFileChange) { // RHF Jan 12, 2000
            ResetForm();
            DrawStaticItems();
            // set focus to the field
            GoToFld( pNewField );
            ScrollToField(pNewField);   // order of scroll/update changed to accomodate intelligent update  CSC 6/7/00
            UpdateFields(); //We have a paint problem on delete record Aug 21 ,2000

        }
        else {
            // set focus to the field
            GoToFld( pNewField );
            ScrollToField(pNewField);   // order of scroll/update changed to accomodate intelligent update  CSC 6/7/00
            UpdateFields(); //We have a paint problem on delete record Aug 21 ,2000
        }
    }

    ShowCapi( (CDEField *) pItem ); // RHF Jan 20, 2000
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::UpdateFields()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::UpdateFields()
{
    //Update the data in the edit controls
    int iCount = m_aEdit.GetSize();
    BOOL bVisible;
    CRect rcClient, rcEdit, rcIntersect;
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry*  pRunApl = pRunDoc->GetRunApl();
    CDEField*       pField;

    for( int i = 0; i < iCount; i++) {
        // see if this field is visible (CSC 6/7/00 for efficiency)
        bVisible = TRUE;
        if (bVisible)  {
            pField = m_aEdit[i]->GetField();
            ASSERT( pField );
            const CDictItem* pItem = pField->GetDictItem();

            if( pItem != NULL )
            {
                CString csData = pRunApl->GetVal(pItem->GetSymbol(), 0);
                pField->SetData(csData);
            }

            ContentType contentType;
            CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
            BOOL bShow  = CheckToShow(pField);
            if(pItem) {
                contentType = pItem->GetContentType();
            }
            else {
                //The Field has to be either from a dict item or from a VSet
                contentType = ContentType::Alpha;
            }
            //Set the edit style
            CIMSAString sString ;
            if(m_aEdit[i]->GetModifiedFlag() && m_aEdit[i]->GetField() == pField) {
                m_aEdit[i]->GetWindowText(sString);
            }
            else {
                sString = pField->GetData();
            }
            if(!bShow) {
                if(pField == pRunDoc->GetCurField()) {
                    sString = m_sVerify;
                }
                else {
                    sString = _T("");
                }
            }

            m_aEdit[i]->SetWindowText(sString);
            /*  if (pRunDoc->GetCurField()==pField)  {    // CSC 6/13/00 -- only do a SetSel for the active CEdit control
            m_aEdit[i]->SetSel(0,-1);
        }*/
        }
    }
    UpdateWindow();  // CSC 6/7/00

    for(int i = 0; i < m_aGrid.GetSize() ; i++) {
        m_aGrid[i]->SetGridData(pRunApl);
    }

    CDEBaseEdit* pEdit = SearchEdit((CDEField*)pRunDoc->GetCurField());
    if(pEdit) {
        pEdit->SetSel(-1,-1);
        pEdit->SetSel(0,-1);
    }
}


// RHF INIC 20/8/99
// GSF changed this to a member of the view and fixed memory leak
//static CEntryColor* pEntryColor=NULL;
/////////////////////////////////////////////////////////////////////////////////
//
//      HBRUSH CEntryrunView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
//
/////////////////////////////////////////////////////////////////////////////////
HBRUSH CEntryrunView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if( nCtlColor == CTLCOLOR_EDIT ) {
        CDEBaseEdit*    pEdit=(CDEBaseEdit*) pWnd;
        CEntryrunDoc* pRunDoc = GetDocument();
        CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
        CDEField*   pField;

        pField = pEdit->GetField();

        const CDictItem* pDictItem = pField->GetDictItem(); // RHF Jan 12, 2000

        int iIntensity = 3; // 3 = current field
        if( pRunApl->GetCurItemBase() != pField )
            iIntensity = (pDictItem == NULL) ? 0 : pRunApl->GetStatus( pDictItem->GetSymbol(),0 ); // RHF Jan 12, 2000

        COLORREF cColorFg = RGB(0,0,0);
        COLORREF cColorBg = GetFieldBackgroundColor(iIntensity);
        pEdit->SetColor( pDC, cColorFg, cColorBg );

        return GetFieldBrush(iIntensity);
    }
    else {
        HBRUSH hbr = CScrollView::OnCtlColor(pDC, pWnd, nCtlColor);
        return hbr;
    }
}
// RHF END 20/8/99
/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CEntryrunView::OnRefreshData(WPARAM wParam, LONG lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CEntryrunView::OnRefreshData(WPARAM wParam, LONG lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UpdateFields();
    return( 0 );
}
// RHF END 24/08/99
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ScrollToField(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ScrollToField(CDEField* pField)
{
    // where reading order is right to left, when we scroll to field
    // we want to show the field and everything to the right as opposed to the usual behavior
    // where we scroll so that we show the field and everything to the left of the field.
    bool bScrollRightToLeft = GetDocument()->GetCurFormFile()->GetRTLRostersFlag();

    //See if the edit control is with in the client rect other wise scroll by the difference in the coords
    CRect textRect = pField->GetCDEText().GetDims();
    CRect ItemRect = pField->GetDims();
    CDEGrid* pGrid = NULL;
    if(ItemRect == CRect(0,0,0,0)) {
        //this could be a field in roster
        return;
    }
    if(pField->GetParent()->GetItemType() == CDEFormBase::Roster){
        //if the parent is a roster
        pGrid = FindGrid(pField->GetParent());
        if(pGrid) {
            //                  CRect cellrect = gridCell.GetRect();
            int iRow,iCol;
            pGrid->FindField(pField,pField->GetParent()->GetCurOccurrence(), &iRow, &iCol);
            CGridCell gridCell = pGrid->GetCell(iRow,iCol);
            CRect rect = gridCell.GetRect();
            CPoint ptOffset(pGrid->GetScrollPos());
            rect.OffsetRect(-ptOffset.x,-ptOffset.y);
            pGrid->ClientToScreen(&rect);
            this->ScreenToClient(&rect);
            rect.OffsetRect(this->GetDeviceScrollPosition());
            ItemRect = rect;
            textRect = rect;
        }
    }

    textRect.NormalizeRect();
    ItemRect.NormalizeRect();

    CRect URect;
    URect.UnionRect(&textRect,&ItemRect);


    CPoint TLPoint = URect.TopLeft();
    CPoint BRPoint = URect.BottomRight();


    CClientDC dc(this);
    CRect rect;
    GetClientRect(&rect);
    int iSaveDC = dc.SaveDC();
    OnPrepareDC((CDC*)&dc);
    dc.DPtoLP(&rect);
    bool bUpdate  = false;
    if(!rect.PtInRect(TLPoint) || !rect.PtInRect(BRPoint) ) {
        int cx = BRPoint.x - rect.right;
        if (bScrollRightToLeft) {
            cx = TLPoint.x - rect.left; // JH 12/7/05 reverse scroll for languages like Arabic
        }
        int cy = BRPoint.y - rect.bottom;
        CSize size(cx,cy);
        dc.LPtoDP(&size);
        OnScrollBy(size);
        if(pGrid){
            bUpdate = true;
        }
    }

    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
    if(pField->GetDictItem()->GetContentType() == ContentType::Alpha){
        //Now the bottom righ point of the field is in the rect
        //Now check if the top left of the item rect is in the bounding rect
        //if it is not then get it in
        CClientDC dc(this);
        CRect rect;
        GetClientRect(&rect);
        OnPrepareDC((CDC*)&dc);
        dc.DPtoLP(&rect);
        bool bUpdate  = false;
        CPoint TLPoint = ItemRect.TopLeft();
        if(!rect.PtInRect(TLPoint)) {
            int cx = TLPoint.x - rect.right;
            if (bScrollRightToLeft) {
                cx = TLPoint.x - rect.left; // JH 12/7/05 reverse scroll for languages like Arabic
            }
            int cy = 0;
            CSize size(cx,cy);
            dc.LPtoDP(&size);
            OnScrollBy(size);
            if(pGrid){
                bUpdate = true;
            }
        }
        if(!pGrid) {
            return;
        }
    }

    //If the Bottom right of the edit control is not in rect yet then scroll to its position
    dc.RestoreDC(iSaveDC);
    OnPrepareDC((CDC*)&dc);
    GetClientRect(&rect);
    dc.DPtoLP(&rect);

    CPoint testPoint   = pField->GetDims().BottomRight();
    if(pGrid) {
        testPoint = URect.BottomRight();
    }
    if(!rect.PtInRect(testPoint)) {
        int cx = testPoint.x - rect.right;
        if (bScrollRightToLeft) {
            cx = testPoint.x - rect.left; // JH 12/7/05 reverse scroll for languages like Arabic
        }
        int cy = testPoint.y - rect.bottom;
        CSize size(cx,cy);
        dc.LPtoDP(&size);
        OnScrollBy(size);
        if(pGrid){
            bUpdate = true;
        }
    }
    if(pGrid && bUpdate){
        pGrid->Invalidate();
        pGrid->UpdateWindow();
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CEntryrunView::OnEndgrp(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CEntryrunView::OnEndgrp(WPARAM wParam, LPARAM lParam)
{
    CDEField* pField = (CDEField*)lParam;

    // don't endgroup if the field is an ID item
    if( pField->GetDictItem()->GetRecord()->GetSonNumber() == COMMON )
        return 1L;

    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    bGoTo = false; //To fix the CTrl+/ bug in the grid when the grid is on the same page as that of the other fields

    //Set the grid edit
    SetGridEdit(pField);

    CDEBaseEdit* pEdit = this->SearchEdit(pField);

    //Check the requirement for the postproc before putting the value in buffers
    BOOL bPostProc = this->ChkPProcReq(pEdit);

    //Put the value in the buffers
    this->PutEditValInBuffers(pEdit);

    // in add mode on rosters with sequential fields, check that a non-sequential value
    // is filled in on the current occurrence; if not, delete the occurrence
    CDEGroup* pParentGroup = pField->GetParent();

    if( pRunApl->InAddMode() && pParentGroup->GetMaxLoopOccs() > 1 )
    {
        int iCurOccurrence = pParentGroup->GetCurOccurrence();
        bool bOccurrenceHasData = false;

        for( int i = 0; !bOccurrenceHasData && i < pParentGroup->GetNumItems(); i++ )
        {
            CDEItemBase* pItemBase = pParentGroup->GetItem(i);

            if( !pItemBase->isA(CDEFormBase::eItemType::Field) )
                continue;

            CDEField* pRosterField = assert_cast<CDEField*>(pItemBase);

            if( !pRosterField->IsSequential() )
            {
                const CDictItem* pRosterItem = pRosterField->GetDictItem();

                CString csValue = pRunApl->GetVal(pRosterItem->GetSymbol(), iCurOccurrence);

                for( int j = 0; !bOccurrenceHasData && j < csValue.GetLength(); j++ )
                    bOccurrenceHasData = ( csValue[j] != _T(' ') );
            }
        }

        if( !bOccurrenceHasData )
        {
            OnDeleteGrpocc();

            // 20130318 the grid was being redrawn not showing the field as protected, so redraw it
            ResetGrids();

            bPostProc = FALSE;
        }
    }

    // endgroup
    CDEItemBase* pItem = pRunApl->EndGroup(bPostProc);


    if( assert_nullable_cast<CDEField*>(pItem) != pRunDoc->GetCurField() )
        ResetVerifyString();

    //RHF/VC INIC 22/11/99
    if(!pItem) {
        bGoTo = true;
        //SAVY &&&  if it is other an add mode just assume this is end of the case and if it is modified
        //Go to the next case
        //How do I save a questionnaire ???
        CNPifFile* pPIF = pRunDoc->GetPifFile();
        bool bAutoAdd =pPIF->GetAutoAddFlag();

        if(pRunDoc->IsPartialAdd())  {
            bool bLast = ((CMainFrame*)AfxGetMainWnd())->GetCaseView()->IsLastCase();
            pRunDoc->SetQModified(FALSE);
            pRunDoc->SetCurField(NULL); // RHF Feb 23, 2004
            ((CMainFrame*)AfxGetMainWnd())->OnStop();
            if(bAutoAdd){
                if(bLast) {
                    // ((CMainFrame*)AfxGetMainWnd())->OnAdd(); to fix stop(-1) ghost process when ids are all protected
                    ((CMainFrame*)AfxGetMainWnd())->PostMessage(WM_COMMAND,ID_ADD);
                }
            }
            return 0l;

        }
        else if(pRunDoc->GetAppMode() != ADD_MODE)  {
            pRunDoc->SetQModified(FALSE);
            ProcessModifyMode();
            return 0l;
        }
        else if(!bAutoAdd) {
            pRunDoc->SetQModified(FALSE);
            pRunDoc->SetCurField(NULL); // RHF Feb 23, 2004
            ((CMainFrame*)AfxGetMainWnd())->OnStop();
            return 0l;
        }
        else {
            // 20130318, added the three following lines of code because stops in the postproc of a group wouldn't actually stop
            pRunDoc->SetQModified(FALSE);
            pRunDoc->SetCurField(NULL);
            ((CMainFrame*)AfxGetMainWnd())->OnStop();
            // 20130318 end of addition
            return 0l;
        }
    }
    //RHF/VC END 22/11/99

    if( pItem ) {
        bGoTo = true;
        this->ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));
        if(pRunDoc->GetAppMode() == ADD_MODE)
            ProcessFldAttrib(assert_cast<CDEField*>(pItem));

        ShowCapi( (CDEField *) pItem ); // RHF Jan 20, 2000
    }
    bGoTo = true;
    return 0L;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnUpdateEndgrp(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnUpdateEndgrp(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    pCmdUI->Enable(FALSE);
    if(!pRunApl) {
        return;
    }

    APP_MODE mode =  pRunDoc->GetAppMode();
    if(mode != ADD_MODE && mode != MODIFY_MODE) {
        return;
    }
    else {
        pCmdUI->Enable(TRUE);
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ScrollToCell(CRect* pRect)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ScrollToCell(CRect* pRect)
{
    CClientDC dc(this);
    OnPrepareDC((CDC*)&dc);
    CRect ClientRect;
    GetClientRect(&ClientRect);
    dc.DPtoLP(&ClientRect);
    CPoint testPoint =  pRect->BottomRight();

    if(!ClientRect.PtInRect(testPoint)) {
        int cx = testPoint.x - ClientRect.right;
        int cy = testPoint.y - ClientRect.bottom;
        CSize size(cx,cy);
        dc.LPtoDP(&size);

        int nMapMode ;
        CSize sizeTotal,sizePage,sizeLine;

        GetDeviceScrollSizes(nMapMode,sizeTotal,sizePage,sizeLine);
        sizeLine.cx = sizeLine.cx *3;
        sizeLine.cy = sizeLine.cy *3;
        OnScrollBy(size+sizeLine);

    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CEntryrunView::OnMoveToField(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CEntryrunView::OnMoveToField(WPARAM wParam, LPARAM /*lParam*/)
{
    CDEBaseEdit*    pEdit = (CDEBaseEdit*) wParam;
    CEntryrunDoc*   pRunDoc = GetDocument();
    CRunAplEntry*   pRunApl = pRunDoc->GetRunApl();

    // main action: call MoveToField

    //SAVY 04/11
    BOOL bPostProc = TRUE;
    if(pRunDoc->GetCurField()){
        CDEBaseEdit* pCurEdit = this->SearchEdit((CDEField*)(pRunDoc->GetCurField()));
        if(pCurEdit){
            bPostProc = this->ChkPProcReq(pCurEdit);
            this->PutEditValInBuffers(pCurEdit);
            pCurEdit->SetModifiedFlag(false);
            pCurEdit->SetRemoveTxtFlag(true);
        }
    }
    // BUCEN_2003 changes
    // RHF INIC Nov 06, 2003
    CDEItemBase*     pItem;
    if( pEdit == NULL ) {
        BOOL    bPathOn = pRunDoc->GetCurFormFile() && pRunDoc->GetCurFormFile()->IsPathOn();
        bool    bCheckRange=bPostProc || bPathOn;
        pItem = pRunApl->RunCsDriver(bCheckRange);
    }
    else {
        pItem = pRunApl->MoveToField( pEdit->GetField(),bPostProc );
    }
    // RHF END Nov 06, 2003
    // BUCEN_2003 changes

//  --- next 14 lines are copied/adapted from OnEditEnter
    if(!pItem) {
        CEntryrunDoc* pDoc = (CEntryrunDoc*) GetDocument();

         // RHF INIC Feb 23, 2004
        if(pRunDoc->IsPartialAdd())  {
            bool bLast = ((CMainFrame*)AfxGetMainWnd())->GetCaseView()->IsLastCase();
            pRunDoc->SetQModified(FALSE);
            pDoc->SetCurField(NULL); // RHF Feb 23, 2004
            ((CMainFrame*)AfxGetMainWnd())->OnStop();
            if(bLast) {
                // ((CMainFrame*)AfxGetMainWnd())->OnAdd(); to fix stop(-1) ghost process when ids are all protected
                ((CMainFrame*)AfxGetMainWnd())->PostMessage(WM_COMMAND,ID_ADD);
            }
            return 0l;
        }
        // RHF END Feb 23, 2004

        //SAVY &&&  if it is other an add mode just assume this is end of the case and if it is modified
        //Go to the next case
        //How do I save a questionnaire ???
        if(pDoc->GetAppMode() != ADD_MODE)  {
            pDoc->SetQModified(FALSE);
            ProcessModifyMode();
            return 0;
        }

        else  {
            // 20130314, added the three following lines of code because moves triggering stops (triggered from a userbar click) wouldn't actually stop
            pRunDoc->SetQModified(FALSE);
            pRunDoc->SetCurField(NULL);
            ((CMainFrame*)AfxGetMainWnd())->OnStop();
            // 20130314 end of addition
            return 0;
        }
    }
//  --- equalizing code with OnEditEnter        <end>   // victor Dec 10, 01

    //For Now
    UpdateGridEdits(assert_cast<CDEField*>(pItem));
    this->ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));
    if(pRunDoc->GetAppMode() == ADD_MODE)
        ProcessFldAttrib(assert_cast<CDEField*>(pItem));

    ShowCapi( (CDEField *) pItem ); // RHF Jan 20, 2000

    return 0;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::SetGridEdit(CDEField* pField )
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::SetGridEdit(CDEField* pField )
{
    if(!pField)
        return;

    //If the field does not belong to roster then we dont need to do set the
    //Grid edit
    CDERoster* pParentRoster = DYNAMIC_DOWNCAST(CDERoster,pField->GetParent());

    if(!pParentRoster){
        //if it is not a roster
        return;
    }
    if(pParentRoster){
        CDEGrid* pGrid = FindGrid(pParentRoster);
        if(pGrid){
            pGrid->GetEdit()->ShowWindow(SW_HIDE);
            pGrid->UpdateWindow();

        }

    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnNextgrp()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnNextgrp()
{
    SendMessage(UWM::CSEntry::EndGroup, 0, 0);
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::UpdateGridEdits(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::UpdateGridEdits(CDEField* pField)
{
    //Get the current field and if it doesn belong to roster
    //then hide the grids edit
    if(pField) {
        CDERoster* pParentRoster = DYNAMIC_DOWNCAST(CDERoster, pField->GetParent());
        for(int iIndex =0; iIndex < m_aGrid.GetSize() ; iIndex ++) {
            if(pParentRoster != m_aGrid[iIndex]->GetRoster()){
                m_aGrid[iIndex]->GetEdit()->ShowWindow(SW_HIDE);
                ((CDEBaseEdit*)m_aGrid[iIndex]->GetEdit())->SetModifiedFlag(false);
            }
        }

    }
    else {
        for(int iIndex =0; iIndex < m_aGrid.GetSize() ; iIndex ++) {
                m_aGrid[iIndex]->GetEdit()->ShowWindow(SW_HIDE);
                ((CDEBaseEdit*)m_aGrid[iIndex]->GetEdit())->SetModifiedFlag(false);
        }

    }
}
// Function name        : CEntryrunView::CheckToShow
// Description      : Checks if we should hide the text in the fields/rosters
// Return type          : BOOL
// Argument         : CDEItemBase* pBase (Fields/Rosters)
/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL  CEntryrunView::CheckToShow(const CDEItemBase* pTestBase)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL  CEntryrunView::CheckToShow(const CDEItemBase* pTestBase)
{
    BOOL bShow = FALSE;

    CEntryrunDoc* pRunDoc = GetDocument();
    ASSERT(pRunDoc);

    if(pRunDoc->GetAppMode() != VERIFY_MODE){
        return TRUE;
    }
    if(pRunDoc->GetAppMode() == VERIFY_MODE && m_bCheatKey){
        return TRUE;

    }
    //For Now get it from the pifobject
    CNPifFile* pPIF =        pRunDoc->GetPifFile();
    if(!pPIF)
        return bShow;
    Application* pApp = pPIF->GetApplication();
    if(!pApp)
        return bShow;

    CDEField* pTestField = DYNAMIC_DOWNCAST(CDEField,pTestBase);
    if(pTestField && !pTestField->GetVerifyFlag()){//if the test field is set to need not verify display the field
        return TRUE;
    }
    //Assuming current field is always a CDEField;
    CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pRunDoc->GetCurField());

    if(!pField) //No Current field Hide all the stuff
        return bShow;

    //SAVY &&&
    //for Now we are assuming only one form
    auto pFormfile = pApp->GetRuntimeFormFiles()[m_iCurrentFormFileNum]; // RHF Jan 12, 2000
    int iFormNum = pRunDoc->GetCurFormNum();

    if(iFormNum == -1)
        return bShow;

    //Get the form
    CDEForm* pForm = pFormfile->GetForm(iFormNum);
    ASSERT(pForm);

    int iItems = pForm->GetGroup()->GetNumItems();

    CDEItemBase* pCurrentBase = pField;
    //if the current fields parent is a roster then we need to compare against this
    if(pField->GetParent()->IsKindOf(RUNTIME_CLASS(CDERoster))){
        pCurrentBase =  pField->GetParent();
    }

    for (int iIndex =0; iIndex < iItems; iIndex++) {

        CDEItemBase* pItem = (CDEItemBase*) pForm->GetGroup()->GetItem(iIndex);

        if(pItem == pCurrentBase) { //we have found the current field before the test field so no need to check any more
            bShow = FALSE;
            return bShow;
        }//So Hide the text
        if(pItem == pTestBase) { //We have found the pTestBase before the current field so we need to display the text
            bShow  = TRUE;
            return bShow;
        }
    }

    return bShow;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    // TODO: Add your specialized code here and/or call the base class

    CScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);
    if(bActivate && pActivateView == this) {
        CEntryrunDoc* pDoc = GetDocument();
        CDEField* pField = (CDEField*)pDoc->GetCurField();
        if(pDoc && pDoc->GetRunApl() && pField) {
            CDEGroup* pGroup = pField->GetParent();
            if(pGroup->IsKindOf(RUNTIME_CLASS(CDERoster))){
                CDEGrid* pGrid = FindGrid(pGroup);
                if(pGrid) {
                    pGrid->m_bRedraw = false;
                }

            }
            GoToFld(pField);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnSetFocus(CWnd* pOldWnd)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnSetFocus(CWnd* pOldWnd)
{
    CScrollView::OnSetFocus(pOldWnd);
    CEntryrunDoc* pDoc = GetDocument();
    HWND hwnd = pOldWnd->GetSafeHwnd();
    hwnd = ::GetParent(hwnd);
    if(pDoc && pDoc->GetAppMode() == NO_MODE){
       ((CMainFrame*)AfxGetMainWnd())->GetCaseView()->SetFocus();
       return;
    }

    if(pDoc && pDoc->GetRunApl() &&  pDoc->GetRunApl()->HasStarted() && pDoc->GetCurField() &&bGoTo) {
        if(hwnd && hwnd != this->GetSafeHwnd()){
            GoToFld((CDEField*)pDoc->GetCurField());
            //SAVY&&& fix this after Nepal app .GOTo is an overkill .all you need to do is
            //set focus to the edit. I do not know what implications itmay have 'cos of this.So do it later
            //comment the above line and remove the comments of the following lines
            /*CDEBaseEdit* pEdit = SearchEdit((CDEField*)pDoc->GetCurField());
            if(pEdit){
            pEdit->SetFocus();
        }*/
        }
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ProcessFldAttrib(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ProcessFldAttrib(CDEField* pField)
{
    CEntryrunDoc* pDoc = GetDocument();
    ASSERT(pDoc->GetAppMode() == ADD_MODE);

    if(pField->IsSequential() && !pField->GetParent()->IsKindOf(RUNTIME_CLASS(CDERoster))) {
        ASSERT(pField->GetDictItem()->GetContentType() == ContentType::Numeric);
        CRunAplEntry* pRunApl = pDoc->GetRunApl();

        CIMSAString sData = pRunApl->GetVal(pField->GetDictItem()->GetSymbol(), 0);
        sData.Trim();

        if(sData.IsEmpty()) {

            CDEGroup* pGroup = (CDEGroup*)pField->GetParent();
            while(pGroup) {
                if(pGroup->GetMaxLoopOccs() != 1) // See if it is multiple
                    break;
                else  {
                    pGroup = pGroup->GetParent();
                }
            }
            if(!pGroup)
                return;
            int iIndex = pGroup->GetCurOccurrence();
            if(iIndex > 1) {
                CString csValue = pRunApl->GetVal(pField->GetDictItem()->GetSymbol(), iIndex - 1);
                int iVal = _ttoi(csValue);
                if(iVal)
                    iIndex = iVal+1;
            }

            sData = IntToString(iIndex);
            pField->SetData( sData );

            CDEBaseEdit* pEdit = this->SearchEdit(pField);
            if(pEdit) {
                pEdit->SetWindowText(sData);
                pEdit->SetSel(0,-1);
            }
        }
    }

    else if(pField->IsSequential() && !pField->GetParent()->IsKindOf (RUNTIME_CLASS (CDERoster))) {
        CIMSAString sData = pField->GetData();
        CDEBaseEdit* pEdit = this->SearchEdit(pField);
        if(pEdit) {
            pEdit->SetWindowText(sData);
            pEdit->SetSel(0,-1);
        }
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CEntryrunView::OutOfSequence(CDEBaseEdit* pEdit)
//
/////////////////////////////////////////////////////////////////////////////////

BOOL CEntryrunView::OutOfSequence(CDEBaseEdit* pEdit)
{
    CDEField* pField = pEdit->GetField();
    if(!pField)
        return TRUE;

    CEntryrunDoc* pDoc = GetDocument();

    if(pField->IsSequential()) {
        ASSERT(pField->GetDictItem()->GetContentType() == ContentType::Numeric);
        CRunAplEntry* pRunApl = pDoc->GetRunApl();

        CString sData;
        CDEGroup* pGroup = (CDEGroup*)pField->GetParent();
        while(pGroup) {
            if(pGroup->GetMaxLoopOccs() != 1) // See if it is multiple
                break;
            else  {
                pGroup = pGroup->GetParent();
            }
        }
        if(!pGroup)
            return TRUE;
        int iIndex = pGroup->GetCurOccurrence();
        if(iIndex > 1) {
            CString csValue = pRunApl->GetVal(pField->GetDictItem()->GetSymbol(), iIndex - 1);
            int iVal = _ttoi(csValue);
            if(iVal)
                iIndex = iVal+1;
        }

        sData = IntToString(iIndex);

        CIMSAString sInput;
        pEdit->GetWindowText(sInput);
        sInput.Trim();
        sInput.TrimLeft(_T("0")); //remove the left zeros

        if(sData.CompareNoCase(sInput) !=0){
            CCustMsg custMsg;
            custMsg.m_eMsgType = OutOfSeq;
            custMsg.m_pEdit = pEdit;
            ::MessageBeep(0);
            custMsg.ShowMessage();
            UINT iRet = custMsg.GetRetVal();
            if(iRet == 1) {
                return TRUE;
            }
            else {
                pEdit->SetRemoveTxtFlag(TRUE);
                return FALSE;
            }

        }
    }
    return TRUE;
}
// RHF INIC Jan 16, 2000
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::SetCurrentFormFileNum( CDEField* pField )
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::SetCurrentFormFileNum( CDEField* pField )
{
    if( pField != NULL ) {
        CEntryrunDoc*   pRunDoc = GetDocument();
        CRunAplEntry*        pRunApl = pRunDoc->GetRunApl();

        if( pRunApl ) {
            m_iCurrentFormFileNum =  pField->GetFormFileNumber();
        }
    }
}
// RHF END Jan 16, 2000

// RHF INIC Jan 30, 2000
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ShowCapi( const CDEField* pField ) const
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ShowCapi( const CDEField* pField ) const
{
    CDEBaseEdit*        pEdit=SearchEdit( pField );
    ShowCapi( pEdit );
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ShowCapi(const CDEBaseEdit* pEdit) const
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ShowCapi(const CDEBaseEdit* pEdit) const
{
    if( pEdit != NULL ) {
        if (pEdit->IsKindOf(RUNTIME_CLASS(CDETextEdit))) {
            ((CDETextEdit*)pEdit)->RefreshEditStyles();
        }
        ShowQuestion( pEdit );
        ShowLabels( pEdit );
        // RHF INIC Dec 04, 2002
        CEntryrunDoc* pRunDoc = ((CEntryrunView *)this)->GetDocument();
        CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

        if (!pRunApl || !pRunApl->HasAppLoaded())
            return;

        CCapi* pCapi=pRunApl->GetCapi();
        pCapi->CheckInZone(false);
    }
    else {
        DeleteLabels();
    }
}
// RHF END Jan 30, 2000
// RHF INIC Jan 14, 2000
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ShowLabels( const CDEField* pField ) const
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ShowLabels( const CDEField* pField ) const
{
    CDEBaseEdit*        pEdit=SearchEdit( pField );
    if( pEdit != NULL )
        ShowLabels( (const CDEBaseEdit*)pEdit );
    else
        DeleteLabels();
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ShowQuestion( const CDEField* pField ) const
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ShowQuestion( const CDEField* pField ) const
{
    CDEBaseEdit*        pEdit=SearchEdit( pField );
    if( pEdit != NULL )
        ShowQuestion( (const CDEBaseEdit*)pEdit );
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ShowQuestion(const CDEBaseEdit* pEdit) const
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ShowQuestion(const CDEBaseEdit* pEdit) const
{
    CEntryrunDoc* pRunDoc = ((CEntryrunView *)this)->GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    if (!pRunApl || !pRunApl->HasAppLoaded())
        return;

    CCapi* pCapi=pRunApl->GetCapi();
    const CDictItem* pItem = ((CDEBaseEdit*)pEdit)->GetField()->GetDictItem();

    if( pItem != NULL ) {
        DEFLD DeFld;
        //Defld={0,0};
        // DeFld.Init(); // called by DEFLD constructor // rcl Jun 28, 2004

        pRunApl->GetDeFld( ((CDEBaseEdit*)pEdit)->GetField(), &DeFld );

        pCapi->SetFrameWindow( (CWnd*)this );
        pCapi->SetAroundField( (CDEBaseEdit*)pEdit );
        pCapi->DoQuestion( &DeFld );
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ShowLabels(const CDEBaseEdit* pEdit) const
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ShowLabels(const CDEBaseEdit* pEdit) const
{
    CEntryrunDoc* pRunDoc = ((CEntryrunView *)this)->GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    if (!pRunApl || !pRunApl->HasAppLoaded())
        return;

    CCapi* pCapi=pRunApl->GetCapi();
    const CDictItem* pItem = ((CDEBaseEdit*)pEdit)->GetField()->GetDictItem();


    if( pItem != NULL ) {
        DEFLD DeFld;
        //Defld={0,0};
        // DeFld.Init(); // called by DEFLD constructor // rcl Jun 28, 2004

        pRunApl->GetDeFld( ((CDEBaseEdit*)pEdit)->GetField(), &DeFld );

        pCapi->SetFrameWindow( (CWnd*)this );
        pCapi->SetAroundField( (CDEBaseEdit*)pEdit );
        pCapi->DeleteLabels();
        pCapi->DoLabelsModeless( &DeFld );
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      CCapi* CEntryrunView::GetCapi() const
//
/////////////////////////////////////////////////////////////////////////////////
CCapi* CEntryrunView::GetCapi() const
{
    CEntryrunDoc* pRunDoc = ((CEntryrunView *)this)->GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    if (!pRunApl || !pRunApl->HasAppLoaded())
        return( NULL );

    return( pRunApl->GetCapi() );
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::DeleteLabels() const
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::DeleteLabels() const
{

    CCapi* pCapi=GetCapi();
    if( pCapi )
        pCapi->DeleteLabels();
}

// Return CDEEDIt of pField. If pField belongs to a roster, pField must be the current
// field
/////////////////////////////////////////////////////////////////////////////////
//
//      CDEBaseEdit*CEntryrunView::SearchEdit( const CDEField* pField ) const
//
/////////////////////////////////////////////////////////////////////////////////
CDEBaseEdit*CEntryrunView::SearchEdit( const CDEField* pField ) const
{
    int         iCount = m_aEdit.GetSize();
    CDEBaseEdit*    pEdit=NULL;

    if( pField == NULL )
        return( NULL );

    CDEGroup* pGroup = ((CDEField*)pField)->GetParent();
    if(pGroup && pGroup->IsKindOf(RUNTIME_CLASS(CDERoster))) {
        CDEGrid* pGrid = this->FindGrid(pGroup);
        if(pGrid ) {
            pEdit = (CDEBaseEdit*)pGrid->GetEdit();
            if(pEdit &&  pEdit->GetField() != pField ){
                pEdit = NULL;
            }
        }
    }
    else {
        for(int iIndex =0; iIndex < iCount ; iIndex ++)
            if(m_aEdit[iIndex]->GetField() == pField) {
                pEdit = m_aEdit[iIndex];
                break;
            }
    }
    return( pEdit );
}
// RHF END Jan 14, 2000

// in verify mode, go to the next case if one exists; otherwise end the modify mode
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ProcessModifyMode()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::ProcessModifyMode()
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CCaseView* pView = pFrame->GetCaseView();
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    bool bVerifyMode = ( pRunDoc->GetAppMode() == VERIFY_MODE );

    if( ( pRunDoc->GetAppMode() != MODIFY_MODE ) && !bVerifyMode )
        return;

    pRunApl->SetEndingModifyMode();

    pFrame->BuildKeyArray();
    pView->BuildTree();

    bool close_csentry_after_stopping = false;

    pFrame->OnStop(&close_csentry_after_stopping);

    // if the case listing is locked, close CSEntry after the first interaction with a case
    if( close_csentry_after_stopping )
        return;

    pRunApl->SetEndingModifyMode(false);

    if( bVerifyMode ) // select the next case for verification
        pFrame->OnVerifyCase();

    else
        pView->RestoreSelectPos(); // restore the selection of the modified case
}


LONG CEntryrunView::OnEndLevel(WPARAM /*wParam*/, LPARAM lParam)
{
    CEntryrunDoc*   pRunDoc = GetDocument();
    CRunAplEntry*   pRunApl = pRunDoc->GetRunApl();

    if(pRunDoc->GetRunApl()->GetCurrentLevel() == 1) //Do not do end level
        return 1L;

    CDEField* pField = (CDEField*)lParam;

    /*   if(pField->GetDictItem()->GetRecord()->GetSonNumber() == COMMON)
    return 1L; */

    //Set the grid edit
    SetGridEdit(pField);


    CDEBaseEdit* pEdit = this->SearchEdit(pField);
    BOOL  bPostProc = this->ChkPProcReq(pEdit); //Check the requirement for the postproc before putting the value in buffers

    this->PutEditValInBuffers(pEdit);
    /*pEdit->SetModifiedFlag(false);
    pEdit->SetRemoveTxtFlag(true);*/

    BOOL bWriteNode = TRUE;
    if(pRunApl->IsNewNode() && pRunDoc->GetCurFormFile()->GetNumLevels() > 1){
        bWriteNode = FALSE;
    }
    CDEItemBase*    pItem   = pRunApl->EndLevel(bPostProc,false,pRunApl->GetCurrentLevel()-1,bWriteNode);

    //RHF/VC INIC 22/11/99
    if(!pItem) {
        bGoTo = true;
        CNPifFile* pPIF = pRunDoc->GetPifFile();
        bool bAutoAdd =pPIF->GetAutoAddFlag();
        //SAVY &&&  if it is other an add mode just assume this is end of the case and if it is modified
        //Go to the next case
        //How do I save a questionnaire ???

        if(pRunDoc->IsPartialAdd())  {
            bool bLast = ((CMainFrame*)AfxGetMainWnd())->GetCaseView()->IsLastCase();
            pRunDoc->SetQModified(FALSE);
            ((CMainFrame*)AfxGetMainWnd())->OnStop();
            if(bAutoAdd){
                if(bLast) {
                    // ((CMainFrame*)AfxGetMainWnd())->OnAdd(); to fix stop(-1) ghost process when ids are all protected
                    ((CMainFrame*)AfxGetMainWnd())->PostMessage(WM_COMMAND,ID_ADD);
                }
            }
            return 0l;

        }
        else if(pRunDoc->GetAppMode() != ADD_MODE)  {

            pRunDoc->SetQModified(FALSE);
            ProcessModifyMode();
            return 0l;

        }
        else if(!bAutoAdd) {
            pRunDoc->SetQModified(FALSE);
            ((CMainFrame*)AfxGetMainWnd())->OnStop();
            return 0l;
        }
        else {
            return 0l;
        }
    }
    //RHF/VC END 22/11/99

    if( pItem ) {
        this->ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));
        if(pRunDoc->GetAppMode() == ADD_MODE)
            ProcessFldAttrib(assert_cast<CDEField*>(pItem));

        ShowCapi( (CDEField *) pItem ); // RHF Jan 20, 2000
    }

    return 0L;
}



LONG CEntryrunView::OnNextLevelOcc(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    CEntryrunDoc*   pRunDoc = GetDocument();
    CRunAplEntry*        pRunApl = pRunDoc->GetRunApl();

    CDEField* pField = (CDEField*)lParam;

    //Set the grid edit
    SetGridEdit(pField);

    CDEBaseEdit* pEdit = this->SearchEdit(pField);
    BOOL  bPostProc = this->ChkPProcReq(pEdit); //Check the requirement for the postproc before putting the value in buffers
    this->PutEditValInBuffers(pEdit);

    BOOL bWriteNode = TRUE;

    // RHF INIC Jan 25, 2001
    CDEItemBase*    pItem;
    /*pEdit->SetModifiedFlag(false);
    pEdit->SetRemoveTxtFlag(true);*/
    if( wParam == 1 ) {
        CIMSAString sData;
        pEdit->GetWindowText(sData);
        pField->SetData(sData);
        pItem = pRunApl->AdvanceToEnd( true,TRUE );
    }
    else
        pItem  = pRunApl->EndLevel(bPostProc,false,pRunApl->GetCurrentLevel(),bWriteNode);
    // RHF END Jan 25, 2001

    // RHF COM Jan 25, 2001CDEItemBase*    pItem   = pRunApl->EndLevel(bPostProc,false,pRunApl->GetCurrentLevel(),bWriteNode);

    //RHF/VC INIC 22/11/99
    if(!pItem) {

        //SAVY &&&  if it is other an add mode just assume this is end of the case and if it is modified
        //Go to the next case
        //How do I save a questionnaire ???
        if(pRunDoc->IsPartialAdd())  {
            CNPifFile* pPIF = pRunDoc->GetPifFile();
            bool bAutoAdd =pPIF->GetAutoAddFlag();
            bool bLast = ((CMainFrame*)AfxGetMainWnd())->GetCaseView()->IsLastCase();
            pRunDoc->SetQModified(FALSE);
            pRunDoc->SetCurField(NULL); // RHF Feb 23, 2004
            ((CMainFrame*)AfxGetMainWnd())->OnStop();
            if(bLast && bAutoAdd) {
                // ((CMainFrame*)AfxGetMainWnd())->OnAdd(); to fix stop(-1) ghost process when ids are all protected
                ((CMainFrame*)AfxGetMainWnd())->PostMessage(WM_COMMAND,ID_ADD);
            }
            return 0l;

        }
        else if(pRunDoc->GetAppMode() != ADD_MODE)  {

            pRunDoc->SetQModified(FALSE);
            ProcessModifyMode();
            return 0l;

        }
        else  {

            return 0l;
        }
    }
    //RHF/VC END 22/11/99

    if( pItem ) {
        this->ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));
        if(pRunDoc->GetAppMode() == ADD_MODE)
            ProcessFldAttrib(assert_cast<CDEField*>(pItem));

        ShowCapi( (CDEField *) pItem ); // RHF Jan 20, 2000
    }

    return 0L;
}


LONG CEntryrunView::OnPageUp(WPARAM /*wParam*/, LPARAM lParam)
{
    CEntryrunDoc*   pRunDoc = GetDocument();
    if(pRunDoc->GetAppMode() == VERIFY_MODE && !GetCheatKey() ) {
        return 0l;
    }

    if(pRunDoc->GetCurFormFile()->IsPathOn())
        return 0l ;

    CIMSAString csCurDict = ((CDEField*)pRunDoc->GetCurField())->GetItemDict(); // RHF Jan 12, 2000

    CDEField* pField = (CDEField*)lParam;

    //Set the grid edit
    SetGridEdit(pField);

    CDEBaseEdit* pEdit = this->SearchEdit(pField);
    this->PutEditValInBuffers(pEdit);

    //Get the PageUpField
    //The proc will execute 'cos of movetofield
    if(!DoPageUpField(pField)) {
        //Stay in this field
        ShowCapi( pField ); // RHF Aug 25, 2003
    }

    return 0L;
}

BOOL CEntryrunView::DoPageUpField(CDEField* pField)
{
    ASSERT(pField);

    int iCurFormNum = pField->GetFormNum();
    CEntryrunDoc* pRunDoc = this->GetDocument();
    ASSERT(pRunDoc);
    ASSERT(pRunDoc->GetPifFile());

    Application* pApp = pRunDoc->GetPifFile()->GetApplication();
    ASSERT(pApp);

    auto pFormfile = pApp->GetRuntimeFormFiles()[m_iCurrentFormFileNum];
    CDEForm* pCurForm = pFormfile->GetForm(iCurFormNum);

    ASSERT(pCurForm->GetGroup());
    CDEForm* pForm = this->FindPageUpForm(pField);

    CDEField* pGoToField = NULL;

    if(!pForm)
        return FALSE;
    BOOL bFormChange =TRUE;
    if(pForm == pCurForm) {
        //Go to the previous occurrrence
        //SAVY 04/11
        BOOL bPostProc = TRUE;
        if(pRunDoc->GetCurField()){
            CDEBaseEdit* pCurEdit = this->SearchEdit((CDEField*)(pRunDoc->GetCurField()));
            if(pCurEdit){
                bPostProc = this->ChkPProcReq(pCurEdit);
                this->PutEditValInBuffers(pCurEdit);
            }
        }
        pGoToField =  this->FindFirstEdit(pForm->GetGroup());
        int iCurOcc = pCurForm->GetGroup()->GetCurOccurrence();
        pGoToField = (CDEField*)pRunDoc->GetRunApl()->MoveToField(pGoToField->GetDictItem()->GetSymbol(),iCurOcc-1,bPostProc);
        bFormChange = FALSE;

    }
    else if (pForm->GetGroup()->GetDataOccs() == 1)  {
        pGoToField =  this->FindFirstEdit(pForm->GetGroup());
        //SAVY 04/11
        BOOL bPostProc = TRUE;
        if(pRunDoc->GetCurField()){
            CDEBaseEdit* pCurEdit = this->SearchEdit((CDEField*)(pRunDoc->GetCurField()));
            if(pCurEdit){
                bPostProc = this->ChkPProcReq(pCurEdit);
                this->PutEditValInBuffers(pCurEdit);
            }
        }
        int iGoToField =0 ;
        if(pGoToField->GetParent() && pGoToField->GetParent()->IsKindOf(RUNTIME_CLASS(CDERoster))){
            iGoToField = 1 ;
        }
        pGoToField = (CDEField*)pRunDoc->GetRunApl()->MoveToField(pGoToField->GetDictItem()->GetSymbol(),iGoToField,bPostProc);

        ASSERT(pGoToField);
    }
    else if (pForm->GetGroup()->GetDataOccs() > 1)  {
        pGoToField =  this->FindFirstEdit(pForm->GetGroup());
        ASSERT(pGoToField);
        //SAVY 04/11
        BOOL bPostProc = TRUE;
        if(pRunDoc->GetCurField()){
            CDEBaseEdit* pCurEdit = this->SearchEdit((CDEField*)(pRunDoc->GetCurField()));
            if(pCurEdit){
                bPostProc = this->ChkPProcReq(pCurEdit);
                this->PutEditValInBuffers(pCurEdit);
            }
        }
        int iGoToOcc = pForm->GetGroup()->GetDataOccs();
        pGoToField = (CDEField*)pRunDoc->GetRunApl()->MoveToField(pGoToField->GetDictItem()->GetSymbol(),iGoToOcc,bPostProc);

        //Set the occurrence number to the last occurrence of this group
    }
    else {
        // 20101106 allow the user to page up in operator controlled mode even if the previous screen was one in which
        // no data was entered
        if( pRunDoc->GetCurFormFile()->IsPathOn() )
            return FALSE;

        // the code below is fairly ridiculous but it works; the problem with just going to the desired field is that, because
        // nothing has has been entered in it, the GoToField function won't actually go to it (because we're going back in flow)
        // but instead will go to the previous field with entered data; then calling the function again will skip to the desired
        // field, which works because it's forward in the flow and such a move is allowed

        GoToField(FindFirstEdit(pForm->GetGroup()),1);
        GoToField(FindFirstEdit(pForm->GetGroup()),1);

        return TRUE;
    }

    ASSERT(pGoToField);


    if( pGoToField ) {

        pRunDoc->SetCurField( (CDEField*) pGoToField );
        if(bFormChange) { // RHF Jan 12, 2000
            ResetForm();
            DrawStaticItems();
        }

        // set focus to the field

        GoToFld( pGoToField);

        ScrollToField(pGoToField);
        UpdateFields();   // order of scroll/update changed to accomodate intelligent update  CSC 6/7/00

        ShowCapi( pGoToField ); // RHF Aug 21, 2003
        return TRUE;
    }

    return FALSE;
}

CDEForm* CEntryrunView::FindPageUpForm(CDEField* pField)
{
    ASSERT(pField);
    //Get the current form if it has previous occurrence return the same one
    int iCurFormNum = pField->GetFormNum();
    CEntryrunDoc* pDoc = this->GetDocument();
    ASSERT(pDoc);
    ASSERT(pDoc->GetPifFile());
    Application* pApp = pDoc->GetPifFile()->GetApplication();
    ASSERT(pApp);

    auto pFormfile = pApp->GetRuntimeFormFiles()[m_iCurrentFormFileNum];
    CDEForm* pCurForm = pFormfile->GetForm(iCurFormNum);
    CDEForm* pForm = NULL;

    BOOL bRoster = pField->GetParent()->IsKindOf(RUNTIME_CLASS(CDERoster));

    int iCurrentOcc = pField->GetParent()->GetCurOccurrence();

    if(/*pField->GetParent()->GetTotalOccs() > 1  && */iCurrentOcc-1 > 0 && !bRoster) {
        pForm = pCurForm;
        return pForm;
    }
    //Get the current form level
    //    int iLevel = pCurForm->GetLevel();

    //check the previous forms
    for(int iIndex =iCurFormNum-1; iIndex >= 0; iIndex -- ) {
        pForm  =pFormfile->GetForm(iIndex);
        /*if(pForm->GetLevel() != iLevel) {//if level is different from the current one dont consider it
        pForm  = NULL;
        continue;
        //SAVY 03/21 Glenn wants to move across the level boundaries
    }*/

        if(FindFirstEdit(pForm->GetGroup()))  {//if the form has no editable  do not consider it
            break;
        }
        else {
            pForm  = NULL;
        }

    }

    return pForm;
}

CDEField* CEntryrunView::FindFirstEdit(CDEGroup* pGroup, bool bSkipMirrorProtectedPersistent/* = true*/)
{
    int iTotalItems = pGroup->GetNumItems();
    CDEField* pField = NULL;
    for(int iItem =0; iItem < iTotalItems ; iItem++) {
        CDEItemBase* pItem  = pGroup->GetItem(iItem);
        if(pItem->IsKindOf(RUNTIME_CLASS(CDEField))) {
            pField = assert_cast<CDEField*>(pItem);
            //if it is a mirror or protected field you cant go in
            if( bSkipMirrorProtectedPersistent && ( pField->IsMirror() || pField->IsProtected() || pField->IsPersistent() ) )
                continue;
            else
                break;

        }
        if(pItem->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
            pField = FindFirstEdit((CDEGroup*)pItem);
            if(pField)
                break;
        }
    }

    return pField;
}





/*****************************************************************************
Page down implementation

*****************************************************************************/
// Function name        : CEntryrunView::OnPageDown

// Description      :
// Return type          : LONG
// Argument         : WPARAM wParam
// Argument         : LPARAM lParam
LONG CEntryrunView::OnPageDown(WPARAM /*wParam*/, LPARAM lParam)
{
    CEntryrunDoc*   pRunDoc = GetDocument();

    if(pRunDoc->GetAppMode() == VERIFY_MODE && !GetCheatKey() ) {
        return 0l;
    }

    if(pRunDoc->GetCurFormFile()->IsPathOn())
        return 0l ;

    CIMSAString csCurDict = ((CDEField*)pRunDoc->GetCurField())->GetItemDict(); // RHF Jan 12, 2000

    CDEField* pField = (CDEField*)lParam;

    //Set the grid edit
    SetGridEdit(pField);
    CDEBaseEdit* pEdit = this->SearchEdit(pField);
    this->PutEditValInBuffers(pEdit);

    //Get the PageUpField
    //The proc will execute in page up cos it if being done by moveto
    if(!DoPageDownField(pField)) {
        //Stay in this field
        GoToFld( pField);
    }

    return 0L;
}



BOOL CEntryrunView::DoPageDownField(CDEField* pField)
{
    ASSERT(pField);

    int iCurFormNum = pField->GetFormNum();
    CEntryrunDoc* pRunDoc = this->GetDocument();
    ASSERT(pRunDoc);
    ASSERT(pRunDoc->GetPifFile());

    Application* pApp = pRunDoc->GetPifFile()->GetApplication();
    ASSERT(pApp);

    auto pFormfile = pApp->GetRuntimeFormFiles()[m_iCurrentFormFileNum];
    CDEForm* pCurForm = pFormfile->GetForm(iCurFormNum);

    ASSERT(pCurForm->GetGroup());
    CDEForm* pForm = this->FindPageDownForm(pField);


    CDEField* pGoToField = NULL;

    if(!pForm)
        return FALSE;
    BOOL bFormChange =TRUE;
    if(pForm == pCurForm) {
        //Go to the next occurrrence
        pGoToField =  this->FindFirstEdit(pForm->GetGroup());
        int iCurOcc = pCurForm->GetGroup()->GetCurOccurrence();
        //SAVY 04/11
        BOOL bPostProc = TRUE;
        if(pRunDoc->GetCurField()){
            CDEBaseEdit* pCurEdit = this->SearchEdit((CDEField*)(pRunDoc->GetCurField()));
            if(pCurEdit){
                bPostProc = this->ChkPProcReq(pCurEdit);
                this->PutEditValInBuffers(pCurEdit);
            }
        }
        pGoToField = (CDEField*)pRunDoc->GetRunApl()->MoveToField(pGoToField->GetDictItem()->GetSymbol(),iCurOcc +1 ,bPostProc);
        bFormChange = FALSE;

    }
    else if (pForm)  {
        pGoToField =  this->FindFirstEdit(pForm->GetGroup());
        //SAVY 04/11
        BOOL bPostProc = TRUE;
        if(pRunDoc->GetCurField()){
            CDEBaseEdit* pCurEdit = this->SearchEdit((CDEField*)(pRunDoc->GetCurField()));
            if(pCurEdit){
                bPostProc = this->ChkPProcReq(pCurEdit);
                this->PutEditValInBuffers(pCurEdit);
            }
        }
        pGoToField = (CDEField*)pRunDoc->GetRunApl()->MoveToField(pGoToField->GetDictItem()->GetSymbol(),0,bPostProc);

        ASSERT(pGoToField);
    }
    else {
        return FALSE;
    }

    ASSERT(pGoToField);


    if( pGoToField ) {

        pRunDoc->SetCurField( (CDEField*) pGoToField );
        if(bFormChange) { // RHF Jan 12, 2000
            ResetForm();
            DrawStaticItems();
        }


        // set focus to the field

        GoToFld( pGoToField);

        ScrollToField(pGoToField);
        UpdateFields();   // order of scroll/update changed to accomodate intelligent update  CSC 6/7/00

        ShowCapi( pGoToField ); // RHF Aug 21, 2003

        return TRUE;
    }

    return FALSE;
}



CDEForm* CEntryrunView::FindPageDownForm(CDEField* pField)
{
    ASSERT(pField);
    //Get the current form if it has previous occurrence return the same one
    int iCurFormNum = pField->GetFormNum();
    CEntryrunDoc* pDoc = this->GetDocument();
    ASSERT(pDoc);
    ASSERT(pDoc->GetPifFile());
    Application* pApp = pDoc->GetPifFile()->GetApplication();
    ASSERT(pApp);
    APP_MODE appMode = pDoc->GetAppMode();

    auto pFormfile = pApp->GetRuntimeFormFiles()[m_iCurrentFormFileNum];
    int iNumForms = pFormfile->GetNumForms();
    CDEForm* pCurForm  =pFormfile->GetForm(iCurFormNum);
    CDEForm* pForm = NULL;
    int iCurrentOcc = pField->GetParent()->GetCurOccurrence();
    int iDataOccs = pField->GetParent()->GetDataOccs();
    BOOL bRoster = pField->GetParent()->IsKindOf(RUNTIME_CLASS(CDERoster));

    BOOL bCheckNext =FALSE;
    if(iCurrentOcc >= iDataOccs && appMode == ADD_MODE  ){
        bCheckNext=TRUE;
    }
    ASSERT( appMode != VERIFY_MODE ); // RHF Aug 07, 2002
    if( !pDoc->IsAutoEndGroup() && iCurrentOcc >= iDataOccs && appMode == MODIFY_MODE  ){ // RHF Nov 07, 2000
        bCheckNext=TRUE;
    }

    if( iDataOccs > 1  && iCurrentOcc + 1 <=  iDataOccs && !bRoster ) {
        pForm = pCurForm;
        return pForm;
    }
    //Get the current form level
    //    int iLevel = pCurForm->GetLevel();

    //check the previous forms
    for(int iIndex =iCurFormNum+1; iIndex < iNumForms; iIndex++ ) {
        pForm  =pFormfile->GetForm(iIndex);
        /*if(pForm->GetLevel() != iLevel) {//if level is different from the current one dont consider it
        pForm  = NULL;
        continue;
        //SAVY 03/21 Glenn wants to move across the level boundaries
    }*/
        CDEField* pFirstField =  FindFirstEdit(pForm->GetGroup());
        if(pFirstField)  {//if the form has no editable do not consider it
            if(bCheckNext) {
                if(pFirstField->GetParent()->GetDataOccs() ==0 )
                {
                    // 20101106 in the past you couldn't page down if the next form had no data on it, but now
                    // i'll let the user page down to the empty form if they have some data on any form beyond
                    // the empty form; for instance, if the user puts info on form 1, skips form 2, and puts info on form 3
                    // in operator controlled mode the user can now page down from form 1 to form 2, instead of the previous
                    // behavior, in which nothing happened
                    if( pFormfile->IsPathOn() )
                        return NULL;

                    // check to see if there are any data occurrences on any form beyond this one
                    for( int j = iIndex + 1; j < iNumForms; j++ )
                    {
                        CDEForm* pFutureForm = pFormfile->GetForm(j);

                        if( pFutureForm->GetGroup()->GetDataOccs() != 0 )
                            return pForm;
                    }

                    return FALSE;
                }
            }
            break;
        }
        else {
            pForm  = NULL;
        }

    }

    return pForm;
}



//Works now only in ADD / MODIFY MODE
LONG CEntryrunView::OnSlashKey(WPARAM /*wParam*/, LPARAM lParam)
{
    CEntryrunDoc*   pRunDoc = GetDocument();

    /*if(pRunDoc->GetAppMode() == VERIFY_MODE) {
        return 0l;
    }*/
    BOOL bPathOff = !pRunDoc->GetCurFormFile()->IsPathOn();
    if(!bPathOff)
        return 0l;

    if(pRunDoc->GetAppMode() == ADD_MODE && !pRunDoc->GetQModified() && !pRunDoc->IsPartialAdd()) //if it is a new case and Q is not modified
        return 0l;

    //  CIMSAString csCurDict = ((CDEField*)pRunDoc->GetCurField())->GetItemDict(); // RHF Jan 12, 2000

    CDEField* pField = (CDEField*)lParam;

    if(pField->GetDictItem()->GetRecord()->GetSonNumber() == COMMON)
        return 1L;

    APP_MODE appMode = pRunDoc->GetAppMode() ;

    //Savy &&& later on check for dynamic maxloops
    if(appMode != VERIFY_MODE && pField->GetParent()->GetMaxLoopOccs() == pField->GetParent()->GetCurOccurrence()) {
        this->OnEndgrp(0,(LPARAM)pField);
        return 1L;
    }

    //Set the grid edit
    SetGridEdit(pField);


    BOOL bValidMode  = (appMode == ADD_MODE || appMode == MODIFY_MODE || appMode == VERIFY_MODE);
    if(!bValidMode) {
        return 0l;
    }

    BOOL bEndGroup = FALSE;
    ASSERT(pField->GetParent());
    if(pRunDoc->GetAppMode() == ADD_MODE) {
        //SAVY && when the max loop occs is not fixed then we need to get the
        //dependent item and  get it as of now this is not supported in the interface
        int iMaxOccs = pField->GetParent()->GetMaxLoopOccs();
        if(pField->GetParent()->GetCurOccurrence() <= iMaxOccs)
            bEndGroup =FALSE;
        else
            bEndGroup = TRUE;
    }
    else if(pRunDoc->GetAppMode() == MODIFY_MODE) {
        int iDataOccs = pField->GetParent()->GetDataOccs();
        if(iDataOccs == 1 || iDataOccs == pField->GetParent()->GetCurOccurrence()) {
            bEndGroup = TRUE;
        }
        else {
            //Move to the next occurrence and the first field of this group
            bEndGroup = FALSE;

        }
    }
    else if(pRunDoc->GetAppMode() == VERIFY_MODE) {
        int iDataOccs = pField->GetParent()->GetDataOccs();
        if(iDataOccs == 1 || iDataOccs == pField->GetParent()->GetCurOccurrence()) {
            bEndGroup = TRUE;
        }
        else {
            //Move to the next occurrence and the first field of this group
            bEndGroup = FALSE;

        }
        CDEGroup* pParent = pField->GetParent();
        ASSERT(pParent);
        BOOL bCheck = FALSE;

        for(int iIndex =0; iIndex < pParent->GetNumItems(); iIndex++) {
            CRunAplEntry* pRunApl = pRunDoc->GetRunApl();
            if(pParent->GetItem(iIndex) != pField && !bCheck){
                continue;
            }
            else if(pParent->GetItem(iIndex) == pField && !bCheck) {
                bCheck = TRUE;

                //check if the window text is same as the buffer value
                // if true proceed with the process
                //else return by showing the "does not match message"

                CIMSAString sString = pRunApl->GetVal(pField->GetDictItem()->GetSymbol(), 0);
                sString.Trim();
                CDEBaseEdit* pEdit = NULL;
                pEdit =  SearchEdit(pField);
                CIMSAString sWindowText;
                pEdit->GetWindowText(sWindowText);
                sWindowText.Trim();
                if(sString.CompareNoCase(sWindowText) != 0){
                    CCustMsg custMsg;
                    custMsg.m_eMsgType = VerifyMsg;
                    custMsg.m_pEdit = pEdit;
                    ::MessageBeep(0);
                    custMsg.ShowMessage();
                    if(pEdit) {
                        pEdit->SetRemoveTxtFlag(TRUE);
                        pEdit->SetFocus();
                        GoToFld(pEdit->GetField());
                    }
                    return 0L;
                }

                continue;
            }
            CDEField* pField = DYNAMIC_DOWNCAST(CDEField,pParent->GetItem(iIndex));
            if(!pField){
                continue;
            }
            else {

                CIMSAString sString = pRunApl->GetVal(pField->GetDictItem()->GetSymbol(), 0);
                sString.Trim();
                CDEBaseEdit* pEdit = NULL;
                pEdit =  SearchEdit(pField);
                if(!sString.IsEmpty())
                {
                    if(pEdit)
                        pEdit->SetRemoveTxtFlag(TRUE);

                    return 0L;
                }
            }
        }
    }

    if(bEndGroup) {
        this->OnEndgrp(0,(LPARAM)pField);
        return 0L;
    }

    CDEBaseEdit* pEdit = this->SearchEdit(pField);
    BOOL  bPostProc = this->ChkPProcReq(pEdit); //Check the requirement for the postproc before putting the value in buffers
    PutEditValInBuffers(pEdit) ;
    pEdit->SetModifiedFlag(false);

    CDEItemBase* pItem = pRunDoc->GetRunApl()->EndGroupOcc(bPostProc);
    /*SAVY 04/11 8:00
    int iOcc =  pField->GetParent()->GetCurOccurrence() +1;
    CDEField* pMoveToField = this->FindFirstEdit(pField->GetParent());
    CDEItemBase*    pItem   =  pRunDoc->GetRunApl()->MoveToField(pMoveToField->GetDictItem()->GetSymbol(),iOcc);
    */
    //THIS IS DANGEROUS && I am assuming that the move to field succeeded and am incrementing the current occurrence
    //of the grou
    //    if(pItem == pMoveToField) {
    //        pMoveToField->GetParent()->SetCurOccurrence(iOcc);
    //    }
    if( assert_cast<CDEField*>(pItem) != pRunDoc->GetCurField() ) {
        ResetVerifyString();
    }

    //RHF/VC INIC 22/11/99
    if(!pItem) {

        //SAVY &&&  if it is other an add mode just assume this is end of the case and if it is modified
        //Go to the next case
        //How do I save a questionnaire ???
        if(pRunDoc->GetAppMode() != ADD_MODE)  {

            pRunDoc->SetQModified(FALSE);
            ProcessModifyMode();
            return 0l;

        }
        else  {

            return 0l;
        }
    }
    //RHF/VC END 22/11/99

    if( pItem ) {
        ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));
        if(pRunDoc->GetAppMode() == ADD_MODE)
            ProcessFldAttrib(assert_cast<CDEField*>(pItem));

        ShowCapi( (CDEField *) pItem ); // RHF Jan 20, 2000
    }

    return 0L;
}



//Call this function before setting the pNewField
//Check for form change , Update the form fields and go to the new Field
void CEntryrunView::ChkFrmChangeNUpdate(CDEField* pNewField)
{
    ASSERT(pNewField);
    CEntryrunDoc * pRunDoc = GetDocument();
    //For Now

    CDEField*    pField=  (CDEField*)pRunDoc->GetCurField();
    int iCurFormNum = -1;
    if(pField)
        iCurFormNum = pField->GetFormNum();
    pRunDoc->SetCurField(pNewField);

    BOOL bFormFileChange = FALSE;
    int iOldFormFileNum = this->GetCurrentFormFileNum();

    SetCurrentFormFileNum( pNewField );

    if(iOldFormFileNum != GetCurrentFormFileNum()) {
        bFormFileChange = TRUE;
    }



    if(pNewField->GetFormNum() != iCurFormNum || bFormFileChange) { // RHF Jan 12, 2000
        ResetForm();
        DrawStaticItems();
    }


    // set focus to the field
    //SAVY, HERE WE HAVE SCOPE FOR ENHANCNG PERFORMANCE .AGAIN THE NEED FOR THE LIST OF FEILDS
    //REQUIRED FOR UPDATE IS FELT HERE.
    UpdateFields(); //This first one is for the Preproc value changes Bug reported by RHF
    GoToFld( pNewField );
    ScrollToField(pNewField);   // order of scroll/update changed to accomodate intelligent update  CSC 6/7/00
    UpdateFields();
    CDEBaseEdit* pEdit = SearchEdit(pNewField);
    if(pEdit){
        pEdit->PostMessage(WM_SETFOCUS);
    }

}

void CEntryrunView::PutEditValInBuffers(CDEBaseEdit* pEdit)
{
    ASSERT(pEdit);
    CDEField* pField = pEdit->GetField();
    ASSERT(pField);
    CEntryrunDoc* pRunDoc  = GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    CIMSAString sString;
    pEdit->GetWindowText(sString);

    const CDictItem* pDictItem = pField->GetDictItem();

    if( pDictItem->GetContentType() == ContentType::Numeric && pDictItem->GetDecimal() ) // 20120312
        sString.Replace(',','.');

    CString sData = pRunApl->GetVal(pField->GetDictItem()->GetSymbol(), 0);

    if(sString.CompareNoCase(sData) != 0 )
    {
        if( pField->AllowMultiLine() ) // 20120816
            sString = WS2CS(SO::ToNewlineLF(CS2WS(sString)));

        pField->SetData(sString);
        sString = pField->GetData(); //this gets the processed SetData text which could have replaced  \r\n with \n
        ASSERT(pField->GetDictItem());
        pRunApl->PutVal(pField,sString);
    }

    CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
    if(pEdit->GetField()->GetDictItem()->GetContentType() == ContentType::Alpha){
        CPoint point(0,0);
        CDEEdit* pDEEdit = DYNAMIC_DOWNCAST(CDEEdit,pEdit);
        if(pDEEdit){
            pDEEdit->CalcCaretPos(point);
            pDEEdit->SetCaretPos(point); //BMD request for reenter on Alpha fields
        }
    }
    pEdit->SetModifiedFlag(false);
    pEdit->SetRemoveTxtFlag(true);
}

//Check for postproc requirement
//if the value in the edit control is different from the one in buffers then
//it returns TRUE else it will return FALSE
BOOL CEntryrunView::ChkPProcReq(CDEBaseEdit* pEdit)
{
    BOOL bRet = FALSE;
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();
    ASSERT(pRunApl);

    if(pEdit) {
        CIMSAString sString;
        pEdit->GetWindowText(sString);
        CDEField* pField = pEdit->GetField();

        sString.Trim();

        CIMSAString sData = pRunApl->GetVal(pField->GetDictItem()->GetSymbol(), 0);
        sData.Trim();

        if( ( !sString.CompareNoCase(_T(".")) || !sString.CompareNoCase(_T(",")) ) && sData.IsEmpty() ) {
            // 20101106 users couldn't Ctrl-/ to exit out of rosters from decimal fields because
            // CSEntry thought that there was marked data (the empty decimal point) in the field
        }
        else if(sString.CompareNoCase(sData) != 0 ) {

            if( pField->AllowMultiLine() ) // 20120816
                sString = WS2CS(SO::ToNewlineLF(CS2WS(sString)));

            pField->SetData(sString);
            bRet = TRUE;
        }
    }

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//              CDEGrid* CEntryrunView::FindGrid(CDEGroup* pGroup)
//
/////////////////////////////////////////////////////////////////////////////////////////////////

//find the grid which has this group
CDEGrid* CEntryrunView::FindGrid(CDEGroup* pGroup) const
{
    CDEGrid* pRet = NULL;
    for (int iIndex =0; iIndex < this->m_aGrid.GetSize(); iIndex++) {
        CDEGrid* pGrid = m_aGrid[iIndex];
        if(pGrid->GetRoster() == pGroup) {
            pRet = pGrid;
            break;
        }
    }
    return pRet;
}
void CEntryrunView::OnInsertGroupocc()
{
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    ASSERT(pRunApl);

    bool bRet = false;
    SetGridEdit((CDEField*)pRunDoc->GetCurField());

    CDEItemBase* pBase = pRunApl->InsertOcc(bRet);
    ASSERT(pBase);
    if(pBase && bRet) {
        pRunDoc->SetQModified(TRUE);
        this->UpdateFields();
        CDEBaseEdit* pEdit = this->SearchEdit((CDEField*)pBase);
        if(pEdit) {
            pEdit->SetModifiedFlag(false);
            pEdit->SetRemoveTxtFlag(true);
        }

        ChkFrmChangeNUpdate((CDEField*)pBase);
        if(pRunDoc->GetAppMode() == ADD_MODE){
            ProcessFldAttrib((CDEField*)pBase);
        }

    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnUpdateNote()
//      void CEntryrunView::OnFieldNote()
//      void CEntryrunView::OnCaseNote()
//
/////////////////////////////////////////////////////////////////////////////////

void CEntryrunView::OnUpdateNote(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = GetDocument();
    BOOL bEnable = ( pDoc->GetAppMode() != NO_MODE );
    pCmdUI->Enable(bEnable);
}

void CEntryrunView::OnFieldNote()
{
    ((CMainFrame*)AfxGetMainWnd())->DoNote(false);
}

void CEntryrunView::OnCaseNote()
{
    ((CMainFrame*)AfxGetMainWnd())->DoNote(true);
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnInsertGroupoccAfter() {           // victor Mar 26, 02
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnInsertGroupoccAfter()
{           // victor Mar 26, 02
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    ASSERT(pRunApl);

    bool bRet = false;
    SetGridEdit((CDEField*)pRunDoc->GetCurField());

    CDEItemBase* pBase = pRunApl->InsertOccAfter(bRet);
    ASSERT(pBase);
    if(pBase && bRet) {
        pRunDoc->SetQModified(TRUE);
        this->UpdateFields();
        CDEBaseEdit* pEdit = this->SearchEdit((CDEField*)pBase);
        if(pEdit) {
            pEdit->SetModifiedFlag(false);
            pEdit->SetRemoveTxtFlag(true);
        }

        ChkFrmChangeNUpdate((CDEField*)pBase);
        if(pRunDoc->GetAppMode() == ADD_MODE){
            ProcessFldAttrib((CDEField*)pBase);
        }
    }
}
void CEntryrunView::OnUpdateInsertGroupocc(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    pCmdUI->Enable(FALSE);
    if(!pRunApl) {
        return;
    }

    APP_MODE mode =  pRunDoc->GetAppMode();
    if(mode != ADD_MODE && mode != MODIFY_MODE) {
        return;
    }
    else {
        CDEField* pCurField = (CDEField*)pRunDoc->GetCurField();
        if(pCurField) {
            int iMaxOccs = pCurField->GetParent()->GetMaxLoopOccs();
            int iCompare = pCurField->GetParent()->GetTotalOccs();
            if( pRunDoc->IsAutoEndGroup() ) { // RHF Nov 07, 2000
                if(mode == MODIFY_MODE ) {
                    iCompare = pCurField->GetParent()->GetDataOccs();
                }
            }
            if(iMaxOccs > 1  && iCompare <= iMaxOccs){
                pCmdUI->Enable(TRUE);
            }
        }
    }

}

void CEntryrunView::OnDeleteGrpocc()
{
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    ASSERT(pRunApl);

    bool bRet = false;
    SetGridEdit((CDEField*)pRunDoc->GetCurField());

    CDEItemBase* pBase = pRunApl->DeleteOcc(bRet);
    ASSERT(pBase);
    if(pBase && bRet) {
        pRunDoc->SetQModified(TRUE);
        this->UpdateFields();
        ChkFrmChangeNUpdate((CDEField*)pBase);
        if(pRunDoc->GetAppMode() == ADD_MODE){
            ProcessFldAttrib((CDEField*)pBase);
        }

    }

}

void CEntryrunView::OnUpdateDeleteGrpocc(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    pCmdUI->Enable(FALSE);
    if(!pRunApl) {
        return;
    }

    APP_MODE mode =  pRunDoc->GetAppMode();
    if(mode != ADD_MODE && mode != MODIFY_MODE) {
        return;
    }
    else {
        CDEField* pCurField = (CDEField*)pRunDoc->GetCurField();
        if(pCurField) {
            int iMaxOccs = pCurField->GetParent()->GetMaxLoopOccs();
            int iCompare = pCurField->GetParent()->GetTotalOccs();
            if( pRunDoc->IsAutoEndGroup() ){ // RHF Nov 07, 2000
                if(mode == MODIFY_MODE ) {
                    iCompare = pCurField->GetParent()->GetDataOccs();
                }
            }
            if(iMaxOccs > 1  && iCompare <= iMaxOccs){
                pCmdUI->Enable(TRUE);
            }
        }
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnSortgrpocc()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnSortgrpocc()
{
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    ASSERT(pRunApl);
    bool bRet = false;

    SetGridEdit((CDEField*)pRunDoc->GetCurField());
    CDEItemBase* pBase = pRunApl->SortOcc(true,bRet);
    ASSERT(pBase);
    if(pBase && bRet) {
        this->UpdateFields();
        ChkFrmChangeNUpdate((CDEField*)pBase);
        if(pRunDoc->GetAppMode() == ADD_MODE){
            ProcessFldAttrib((CDEField*)pBase);
        }

    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnUpdateSortgrpocc(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::OnUpdateSortgrpocc(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    pCmdUI->Enable(FALSE);
    if(!pRunApl) {
        return;
    }

    APP_MODE mode =  pRunDoc->GetAppMode();
    if(mode != ADD_MODE && mode != MODIFY_MODE) {
        return;
    }
    else {
        CDEField* pCurField = (CDEField*)pRunDoc->GetCurField();
        if(pCurField) {
            int iMaxOccs = pCurField->GetParent()->GetMaxLoopOccs();
            int iCompare = pCurField->GetParent()->GetTotalOccs();
            if( pRunDoc->IsAutoEndGroup() ){ // RHF Nov 07, 2000
                if(mode == MODIFY_MODE) {
                    iCompare = pCurField->GetParent()->GetDataOccs();
                }
            }
            if(iMaxOccs > 1  && iCompare <= iMaxOccs){
                pCmdUI->Enable(TRUE);
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CEntryrunView::OnInsertAfter(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CEntryrunView::OnInsertAfter(WPARAM /*wParam*/, LPARAM lParam)
{
    CEntryrunDoc*   pRunDoc = GetDocument();

    BOOL bPathOff = !pRunDoc->GetCurFormFile()->IsPathOn();
    if(!bPathOff)
        return 0l;
    if(pRunDoc->GetAppMode() == ADD_MODE && !pRunDoc->GetQModified()) //if it is a new case and Q is not modified
        return 0l;

    CDEField* pField = (CDEField*)lParam;

    if(pField->GetDictItem()->GetRecord()->GetSonNumber() == COMMON)
        return 1L;

    //Savy &&& later on check for dynamic maxloops
    if(pField->GetParent()->GetMaxLoopOccs() == pField->GetParent()->GetCurOccurrence()) {
        //if max group occs reached we cannot add any more
        return 0L;
    }

    //Set the grid edit
    SetGridEdit(pField);

    APP_MODE appMode = pRunDoc->GetAppMode() ;
    BOOL bValidMode  = (appMode == ADD_MODE || appMode == MODIFY_MODE);
    if(!bValidMode) {
        return 0l;
    }

    BOOL bInsertAfter = FALSE;
    ASSERT(pField->GetParent());
    if(pRunDoc->GetAppMode() == ADD_MODE || pRunDoc->GetAppMode() == MODIFY_MODE) {
        //SAVY && when the max loop occs is not fixed then we need to get the
        //dependent item and  get it as of now this is not supported in the interface
        int iMaxOccs = pField->GetParent()->GetMaxLoopOccs();
        int iDataOccs = pField->GetParent()->GetDataOccs();
        if(pField->GetParent()->GetCurOccurrence() == iMaxOccs || iDataOccs == iMaxOccs)
            bInsertAfter=FALSE;
        else
            bInsertAfter = TRUE;
    }
    else {
        //Move to the next occurrence and the first field of this group
        bInsertAfter = FALSE;
    }

    if(!bInsertAfter) {
        return 1L;
    }

    CDEBaseEdit* pEdit = this->SearchEdit(pField);
    BOOL  bPostProc = this->ChkPProcReq(pEdit); //Check the requirement for the postproc before putting the value in buffers
    PutEditValInBuffers(pEdit) ;

    OnInsertGroupoccAfter();

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::OnPreviousPersistent()
//
/////////////////////////////////////////////////////////////////////////////////
LONG CEntryrunView::OnPreviousPersistent(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();
    ASSERT(pRunApl);

    SetGridEdit((CDEField*)pRunDoc->GetCurField());
    CDEItemBase* pBase = pRunApl->PreviousPersistentField();

    ASSERT(pBase);
    if(pBase) {
        this->UpdateFields();
        ChkFrmChangeNUpdate((CDEField*)pBase);
        if(pRunDoc->GetAppMode() == ADD_MODE){
            ProcessFldAttrib((CDEField*)pBase);
        }

        ShowCapi((CDEField*)pBase); // SAVY Feb 27, 2003
    }

    return 0L;
}


template<typename GFC>
void CEntryrunView::GoToField(GFC get_field_callback)
{
    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    // main action: call MoveToField

    //SAVY 04/11
    BOOL bPostProc = TRUE;
    if(pRunDoc->GetCurField()){
        CDEBaseEdit* pCurEdit = this->SearchEdit((CDEField*)(pRunDoc->GetCurField()));
        if( pCurEdit ) {
            pCurEdit->GetParent()->PostMessage(WM_IMSA_CSENTRY_REFRESH_DATA);
            bPostProc = this->ChkPProcReq(pCurEdit);
            this->PutEditValInBuffers(pCurEdit);
        }
    }

    CDEItemBase* pItem  = get_field_callback(pRunApl);

    if( !pItem ) {
        //Reset the focus to the current  field
        CDEItemBase* pBase = pRunDoc->GetCurField();
        if(pBase)  {
            GoToFld( (CDEField*) pBase );
            ShowCapi( (CDEField *) pBase ); // RHF Nov 14, 2002
        }
        return;
    }

    //For Now
    UpdateGridEdits(assert_cast<CDEField*>(pItem));
    this->ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));
    if(pRunDoc->GetAppMode() == ADD_MODE)
        ProcessFldAttrib(assert_cast<CDEField*>(pItem));

    ShowCapi( assert_cast<CDEField*>(pItem) ); // RHF Nov 14, 2002
}

void CEntryrunView::GoToField(CDEField* pField, int iOcc /*=-1*/)
{
    GoToField([&](CRunAplEntry* pRunApl) { return pRunApl->MoveToField(pField->GetDictItem()->GetSymbol(), iOcc, true); });
}

void CEntryrunView::GoToField(const CaseItemReference& case_item_reference)
{
    GoToField([&](CRunAplEntry* pRunApl) { return pRunApl->MoveToField(case_item_reference, true); });
}


LONG CEntryrunView::OnAdvToEnd(WPARAM /*wParam*/, LPARAM lParam)
{
    CEntryrunDoc*   pRunDoc = GetDocument();
    CRunAplEntry*   pRunApl = pRunDoc->GetRunApl();

    CDEField* pField = (CDEField*)lParam;

    //Set the grid edit
    SetGridEdit(pField);

    CDEBaseEdit* pEdit = this->SearchEdit(pField);
    //    BOOL  bPostProc = this->ChkPProcReq(pEdit); //Check the requirement for the postproc before putting the value in buffers
    this->PutEditValInBuffers(pEdit);

    CIMSAString sData;
    pEdit->GetWindowText(sData);

    if( pField->AllowMultiLine() ) {
        sData = WS2CS(SO::ToNewlineLF(CS2WS(sData)));
    }

    pField->SetData(sData);


    BOOL bWriteNode = TRUE;
    if(pRunApl->IsNewNode() && pRunDoc->GetCurFormFile()->GetNumLevels() > 1){
        bWriteNode = FALSE;
    }

    // RHF INIC Jan 25, 2001
    CDEItemBase*    pItem = NULL;

    APP_MODE mode = pRunDoc->GetAppMode();
    switch (mode) {
    case ADD_MODE:
        {
            bool bAutoAdd = pRunDoc->GetPifFile()->GetAutoAddFlag();
            pRunDoc->GetPifFile()->SetAutoAddFlag(false);
            pItem = pRunApl->AdvanceToEnd( false,TRUE );
            pRunDoc->GetPifFile()->SetAutoAddFlag(bAutoAdd);
        }
        break;
    case MODIFY_MODE:
        pItem = pRunApl->AdvanceToEnd( false,TRUE );
        break;
    case VERIFY_MODE:
        // ????
        break;
    }

    if(!pItem) {

        //SAVY &&&  if it is other an add mode just assume this is end of the case and if it is modified
        //Go to the next case
        //How do I save a questionnaire ???
        if(pRunDoc->GetAppMode() != ADD_MODE)  {

            pRunDoc->SetQModified(FALSE);
            ProcessModifyMode();
            return 0l;

        }
        else  {
            ((CMainFrame*)AfxGetMainWnd())->OnStop();
             return 0L;
        }
    }
    //RHF/VC END 22/11/99

    if( pItem ) {
        this->ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));
        if(pRunDoc->GetAppMode() == ADD_MODE)
            ProcessFldAttrib(assert_cast<CDEField*>(pItem));

        ShowCapi( (CDEField *) pItem ); // RHF Jan 20, 2000
    }

    return 0L;
}

LONG CEntryrunView::OnCheatKey(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CEntryrunDoc* pRunDoc = GetDocument();
    if(GetDocument()->GetAppMode() != VERIFY_MODE){
        return 0l;
    }
    m_bCheatKey = !m_bCheatKey;

    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();
    CDEField* pField =(CDEField*)pRunDoc->GetCurField();
    CDEBaseEdit* pEdit = SearchEdit(pField);
    CIMSAString sSafeString;
    if(m_bCheatKey) {
        pEdit->GetWindowText(sSafeString);
    }
    this->UpdateFields();

    if(m_bCheatKey){
        if(pEdit)
        {
            CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster, pField->GetParent());
            CString csValue = pRunApl->GetVal(pField->GetDictItem()->GetSymbol(),
                ( pRoster != nullptr ) ? pRoster->GetCurOccurrence() : 0);
            pEdit->SetWindowText(csValue);
        }
    }
    else {
        CDEField* pField =(CDEField*)pRunDoc->GetCurField();
        CDEBaseEdit* pEdit = SearchEdit(pField);
        if(pEdit){
            pEdit->SetWindowText(sSafeString);
            sSafeString = _T("");
        }
    }
    return 0l;
}

// RHF INIC Nov 22, 2002
LONG CEntryrunView::OnShowCapi(WPARAM wParam, LPARAM lParam) {
    CDEBaseEdit* pEdit = (CDEBaseEdit*)lParam;
    if(!pEdit){
        return 0L;
    }

    CEntryrunDoc* pRunDoc = GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    if (!pRunApl || !pRunApl->HasAppLoaded() )
        return 0L;

    int     iVar=pEdit->GetField()->GetSymbol();

    if( wParam == 1 ) { // Only refresh the current capi windows
        CCapi* pCapi=pRunApl->GetCapi();
        pCapi->RefreshPosition();
        pCapi->CheckInZone(false); // RHF Jan 13, 2003
        pCapi->CheckOverlap(true);
    }
    else{
        pRunApl->RunGlobalOnFocus( iVar );
        ShowCapi( pEdit->GetField() ); // RHF Nov 02, 2001
    }

    return 0L;
}
// RHF END Nov 22, 2002

/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CEntryrunView::OnPlusKey(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CEntryrunView::OnPlusKey(WPARAM wParam, LPARAM lParam)
{
    CEntryrunDoc* pRunDoc = GetDocument();

    BOOL bPathOff = !pRunDoc->GetCurFormFile()->IsPathOn();
    if(!bPathOff)
        return 0l;

    CDEItemBase* pSkipToEntity = NULL;

    CDEBaseEdit* pEdit = (CDEBaseEdit*)(wParam);
    ASSERT(pEdit);

    CDEField* pField = (CDEField*)lParam;
    SetGridEdit(pField);

    APP_MODE appMode = pRunDoc->GetAppMode() ;
    BOOL bValidMode  = (appMode == ADD_MODE || appMode == MODIFY_MODE || appMode == VERIFY_MODE);
    if(!bValidMode) {
        return 0l;
    }

    //Get the form and find the pluskey field
    CDEFormFile* pFormFile = pRunDoc->GetCurFormFile();
    CIMSAString sPlusTarget = pField->GetPlusTarget();
    if(sPlusTarget.IsEmpty()){
        OnEditChange ((long)VK_RETURN,(long)pEdit);
        return 0l;
    }

    else if(sPlusTarget.CompareNoCase(_T("<END>")) ==0){
        OnSlashKey(0,(long)pField);
        return 0l;
    }

    else {
        //Find the field from the form file
        CDEGroup* pGroup  = pField->GetParent();
        ASSERT(pGroup);
        for(int iIndex =0; iIndex < pGroup->GetNumItems(); iIndex++) {
            CDEItemBase* pBase = pGroup->GetItem(iIndex);
            if(pBase->GetName().CompareNoCase(sPlusTarget) == 0) {
                if(pBase->IsKindOf(RUNTIME_CLASS(CDEField)) || pBase->IsKindOf(RUNTIME_CLASS(CDEBlock))){
                    pSkipToEntity = pBase;
                    break;
                }
                else if(pBase->IsKindOf(RUNTIME_CLASS(CDEGroup))){
                    CDEGroup* pGroup = (CDEGroup*)pBase;
                    for(int iItem =0; iItem < pGroup->GetNumItems(); iIndex ++) {
                        CDEItemBase* pBase2 = pGroup->GetItem(iItem);
                        if(pBase2->IsKindOf(RUNTIME_CLASS(CDEField)) || pBase2->IsKindOf(RUNTIME_CLASS(CDEBlock))){
                            pSkipToEntity = pBase2;
                            break;
                        }
                    }
                }
            }
        }

        //pFormFile->FindField(sPlusTarget,&pForm,(CDEItemBase**)&pSkipToFld);
        if( pSkipToEntity == NULL ) {
            OnEditChange ((long)VK_RETURN,(long)pEdit);
            return 0l;
        }
        else {
            //Check if the form is same as the current form
            int iFormNum = pRunDoc->GetCurFormNum();
            CDEForm* pCurForm =pFormFile->GetForm(iFormNum);

            if(pFormFile->GetForm(pSkipToEntity->GetFormNum()) != pCurForm) {
                OnEditChange ((long)VK_RETURN,(long)pEdit);
                return 0l;
            }
        }
    }
    // CDEBaseEdit* pEdit = this->SearchEdit(pField);
    BOOL  bPostProc = this->ChkPProcReq(pEdit); //Check the requirement for the postproc before putting the value in buffers
    PutEditValInBuffers(pEdit);
    pEdit->SetModifiedFlag(false);

    //find the field
    CRunAplEntry* pRunApl=pRunDoc->GetRunApl();
    CDEItemBase* pItem = pRunApl->MoveToFieldOrBlock(pSkipToEntity, bPostProc);
    if( assert_cast<CDEField*>(pItem) != pRunDoc->GetCurField() ) {
        ResetVerifyString();
    }

    //RHF/VC INIC 22/11/99
    if(!pItem) {
        //SAVY &&&  if it is other an add mode just assume this is end of the case and if it is modified
        //Go to the next case
        //How do I save a questionnaire ???
        if(pRunDoc->GetAppMode() != ADD_MODE)  {
            pRunDoc->SetQModified(FALSE);
            ProcessModifyMode();
            return 0l;

        }
        else  {
            return 0l;
        }
    }
    //RHF/VC END 22/11/99

    if( pItem ) {
        ChkFrmChangeNUpdate(assert_cast<CDEField*>(pItem));
        if(pRunDoc->GetAppMode() == ADD_MODE)
            ProcessFldAttrib(assert_cast<CDEField*>(pItem));

        ShowCapi( (CDEField *) pItem ); // RHF Jan 20, 2000
    }

    return 0L;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::BuildGrids(void)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::BuildGrids(void)
{
    CEntryrunDoc*   pDoc = GetDocument();
    CNPifFile* pPIF = pDoc->GetPifFile();
    ASSERT(pPIF);
    Application* pApp = pPIF->GetApplication();
    ASSERT(pApp);

    for (const auto& pFormfile : pApp->GetRuntimeFormFiles()) {
        CDEItemBase* pItem = NULL;
        for (int iForm = 0; iForm < pFormfile->GetNumForms(); iForm++){ //for each  form
            CDEForm* pForm = pFormfile->GetForm(iForm);
            for(int iItem=0; iItem < pForm->GetNumItems(); iItem++){//for each item
                //Find the grid from the array and display if it exists
                pItem = (CDEItemBase*) pForm->GetItem(iItem);
                if (pItem->GetItemType() != CDEFormBase::Roster){
                    continue;
                }
                CDERoster* pRoster = (CDERoster*) pItem;
                if(ShowGrid(pRoster,false)){
                    continue;
                }
                //Set the dict item for the roster
                for (int iIndex = 0; iIndex < pRoster->GetNumItems() ; iIndex++){
                    CDEItemBase* pBase = pRoster->GetItem(iIndex);
                    if( pBase->isA(CDEFormBase::Field) )
                        SetDictItem((CDEField*)pBase);
                }
                pRoster->RefreshStubsFromOccurrenceLabels(*pFormfile);
                CDEGrid* pGrid = new CDEGrid(pRoster);
                pGrid->SetDecimalChar(DECIMAL_CHAR);
                pGrid->SetGridBkColor(pForm->GetBackgroundColor().ToCOLORREF());

                // following code changed to account for grids that were never rendered on the screen before
                // saving; their dims will be (0,0,0,0).  This code is pasted from CFormScrollView::CreateGrid
                // CSC 10/19/00
                //                pGrid->Create(pRoster->GetDims(),this,pRoster);
#define MIN_NUM_ROSTER_COLS  6  // min view is 5 cols + 1 for stub
#define MIN_NUM_ROSTER_ROWS 11  // 10 fields + 1 for stub
                if (pRoster->GetDims().BottomRight()==CPoint(0,0))  {
                    // user is creating a new roster, no viewport size available ... show based on num rows/cols
                    int iRows = std::min (MIN_NUM_ROSTER_ROWS, pRoster->GetMaxLoopOccs()+1);   // +1 csc 11/16/00
                    int iCols = std::min (MIN_NUM_ROSTER_COLS, pRoster->GetNumCols());
                    if (pRoster->GetOrientation() == RosterOrientation::Vertical)  {
                        int iTmp = iRows;
                        iRows = iCols;
                        iCols = iTmp;
                    }
                    pGrid->Create(pRoster->GetDims().TopLeft(), iRows, iCols, this, pRoster);
                    pGrid->ShowWindow(SW_HIDE);
                }
                else  {
                    // this roster already existed at some point, use previous dimensions...
                    pGrid->Create(pRoster->GetDims(), this, pRoster);
                    pGrid->ShowWindow(SW_HIDE);
                }

                pGrid->SetGridBkColor(pForm->GetBackgroundColor().ToCOLORREF());

                m_aGrid.Add(pGrid);

                CDEField* pFirstField = FindFirstEdit(pRoster, false);

                CDEBaseEdit* pEdit = new CDEEdit();
                //Create the edit field
                pEdit->SetField(pFirstField);
                pEdit->Create(ES_AUTOHSCROLL, CRect(0,0,0,0), this, (UINT)-1);
                pEdit->ModifyStyleEx(WS_EX_CLIENTEDGE,0);

                pGrid->SetEdit(pEdit);
                pEdit->ShowWindow(SW_HIDE);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CEntryrunView::DrawScreenStats()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::DrawScreenStats(CDC* pDC)
{
    RemoveAllEdits();
    RemoveAllGrids2(false);

    ClearScreen(pDC);
    OnEraseBkgnd(pDC);

    CEntryrunDoc* pDoc = GetDocument();
    if( pDoc == NULL || pDoc->GetAppMode() != NO_MODE || pDoc->GetRunApl() == NULL )
        return;

    DataRepository* pInputRepo = pDoc->GetRunApl()->GetInputRepository();

    if( pInputRepo == NULL )
        return;

    CString csDataFilename = pInputRepo->GetName(DataRepositoryNameType::ForListing);

    CRect rect;
    GetClientRect(&rect);

    int X = 0;
    int Y = 0;
    int iMargin = 10;

    CFont font;
    font.CreateFont(16, 0, 0, 0, 400, FALSE, FALSE, 0, ANSI_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY, FF_DONTCARE,
                         _T("Courier New"));
    CFont* pOldFont = pDC->SelectObject(&font);

    pDC->SetBkMode(TRANSPARENT);
    CSize sz = pDC->GetTextExtent(csDataFilename);
    CSize szMin = pDC->GetTextExtent(_T("Verified 999999999     100 %"));
    CRect rectBox;
    if (sz.cx > szMin.cx) {
        X = (rect.Width()  - sz.cx - 2*iMargin) / 2;
        Y = (rect.Height() - 10*sz.cy - 2*iMargin) / 2;
        rectBox = CRect(X, Y, X+sz.cx+2*iMargin - 1, Y+(14*sz.cy)+(2*iMargin) - 1);
    }
    else {
        X = (rect.Width()  - szMin.cx - 2*iMargin) / 2;
        Y = (rect.Height() - 10*sz.cy - 2*iMargin) / 2;
        rectBox = CRect(X, Y, X+szMin.cx+2*iMargin - 1, Y+(14*sz.cy)+(2*iMargin) - 1);
    }
    pDC->DrawFrameControl(&rectBox, DFC_BUTTON, DFCS_BUTTONPUSH);
    CRect rectFill = CRect(rectBox.left+2,rectBox.top+2,rectBox.right-2,rectBox.bottom-2);
    pDC->FillSolidRect(&rectFill,RGB(255,255,255));

    pDC->TextOut(rectBox.left + iMargin,rectBox.top + iMargin,csDataFilename);
    pDC->MoveTo(rectBox.left + iMargin,rectBox.top + iMargin + sz.cy);
    pDC->LineTo(rectBox.right - iMargin,rectBox.top + iMargin + sz.cy);


    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CString csMsg;

    if( pFrame->m_iNumCases != 0 )
    {
        csMsg.Format(_T("Cases    %9d"),pFrame->m_iNumCases);
        Y += sz.cy * 2;
        pDC->TextOut(X + iMargin, Y + iMargin, csMsg);
    }

    if( pFrame->m_iTotalPartial != 0 )
    {
        float fPercent = ((float)pFrame->m_iTotalPartial / pFrame->m_iNumCases) * 100;
        csMsg.Format(_T("Partial  %9d     %3.0f%%"),pFrame->m_iTotalPartial,fPercent);
        Y += sz.cy * 2;
        pDC->TextOut(X + iMargin, Y + iMargin, csMsg);

        csMsg.Format(_T("  Add      %9d"),pFrame->m_iNumPartialAdd);
        Y += sz.cy;
        pDC->TextOut(X + iMargin, Y + iMargin, csMsg);

        csMsg.Format(_T("  Modify   %9d"),pFrame->m_iNumPartialModify);
        Y += sz.cy;
        pDC->TextOut(X + iMargin, Y + iMargin, csMsg);

        csMsg.Format(_T("  Verify   %9d"),pFrame->m_iPartialVerify);
        Y += sz.cy;
        pDC->TextOut(X + iMargin, Y + iMargin, csMsg);
    }

    if( pFrame->m_iVerified != 0 )
    {
        float fPercent = ((float)pFrame->m_iVerified / pFrame->m_iNumCases) * 100;
        csMsg.Format(_T("Verified %9d     %3.0f%%"),pFrame->m_iVerified,fPercent);
        Y += sz.cy * 2;
        pDC->TextOut(X + iMargin, Y + iMargin, csMsg);
    }

    if (pFrame->m_iDeleted != 0)
    {
        csMsg.Format(_T("Deleted    %9d"), pFrame->m_iDeleted);
        Y += sz.cy * 2;
        pDC->TextOut(X + iMargin, Y + iMargin, csMsg);
    }

    if (pFrame->m_iDuplicates != 0)
    {
        csMsg.Format(_T("Duplicates    %9d"), pFrame->m_iDuplicates);
        Y += sz.cy * 2;
        pDC->TextOut(X + iMargin, Y + iMargin, csMsg);
    }

    pDC->SelectObject(pOldFont);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CEntryrunView::RosterEdit(CDEBaseEdit* pEdit,UINT uKey)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::RosterEdit(CDEBaseEdit* pEdit,UINT uKey)
{
    //Find the field and the occ given the current field and occ .
    CDEField* pField = NULL;
    int iOcc =-1;
    int iCurOcc =  pEdit->GetField()->GetParent()->GetCurOccurrence();
    FindNextFieldXLMode(pEdit->GetField(), uKey,pField,iOcc);
    CDEGroup* pGroup = pEdit->GetField()->GetParent();
    bool bLastOcc = (iCurOcc == GetDynamicMaxOccs(pGroup));
    if(pField &&(pEdit->GetField() == pField) && IsLastField(pField) && bLastOcc && uKey == VK_RETURN){
        SendMessage(UWM::CSEntry::EndGroup, 0, (LPARAM)GetDocument()->GetCurField());
        return;
    }
    if(pField && iOcc != -1)
    {
        CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,pEdit->GetField()->GetParent());
        ASSERT(pRoster);
        CDEGrid* pGrid = FindGrid(pRoster);
        pGrid->OnCellField_LClicked(pField,iOcc);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CEntryrunView::FindNextFieldXLMode(CDEField* pCurField,UINT uKey,CDEField* pNextField,int& iOcc)//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunView::FindNextFieldXLMode(CDEField* pCurField,UINT uKey,CDEField*& pNextField,int& iOcc)
{
    CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,pCurField->GetParent());
    ASSERT(pRoster);

    CDEGrid* pGrid = FindGrid(pRoster);
    ASSERT(pGrid);

    int iCurOcc =  pCurField->GetParent()->GetCurOccurrence();
    int iRow = -1;
    int iCol = -1;
    pGrid->FindField(pCurField,iCurOcc,&iRow, &iCol);

    int iMaxOccs = pRoster->GetMaxLoopOccs();
    iMaxOccs = GetDynamicMaxOccs(pRoster);
    APP_MODE mode = GetDocument()->GetAppMode();
    if(!GetDocument()->GetRunApl()->IsPathOn() && (mode == MODIFY_MODE || mode == VERIFY_MODE)) {
        iMaxOccs = pRoster->GetTotalOccs();
    }

    if(pRoster->GetFreeMovement() == FreeMovement::Horizontal) { //movement is horizontal
        //orientation of grid is horizontal
        if(pRoster->GetOrientation() == RosterOrientation::Horizontal) {//orientation is horizontal
            switch(uKey) {
            case VK_UP:
                //Get the next occ if you can
                if(iCurOcc == 1) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pPrevField = GetPrevField(pCurField);
                    int iPrevCol =-1;
                    int iPrevRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pPrevField == pCurField) {
                        return;
                    }
                    else if(pPrevField){
                        pGrid->FindField(pPrevField,iCurOcc,&iPrevRow, &iPrevCol);
                        //check if it is from the same column if yes go to the field
                        if(iPrevCol == iCol){
                            iOcc = iCurOcc;
                            pNextField = pPrevField; // do not change the occ
                        }
                    }

                }
                else {
                    CDEField* pPrevField = GetPrevField(pCurField);
                    int iPrevCol =-1;
                    int iPrevRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pPrevField) {
                        pGrid->FindField(pPrevField,iCurOcc,&iPrevRow, &iPrevCol);
                        if(iPrevCol == iCol && iPrevRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            if(pPrevField != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc -1;
                            }

                            pNextField = pPrevField;
                        }
                        else if(iPrevCol == iCol && iPrevRow != iRow ){
                            iOcc = iCurOcc -1;
                            //Get last field  from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(pCol->GetNumFields()-1);
                        }
                        else {
                            iOcc = iCurOcc -1;
                            //Get last field  from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(pCol->GetNumFields()-1);
                        }
                    }
                    else {
                        iOcc = iCurOcc -1;
                        //Get last field  from the column
                        CDECol* pCol = pRoster->GetCol(iCol);
                        pNextField = pCol->GetField(pCol->GetNumFields()-1);
                    }

                }
                break;
            case VK_DOWN:
                //Get the next occ if you can
                if(iCurOcc == iMaxOccs) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0 == pCurField) {
                        return;
                    }
                    else if(pNextField0){
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        //check if it is from the same column if yes go to the field
                        if(iNextCol == iCol){
                            iOcc = iCurOcc;
                            pNextField = pNextField0; // do not change the occ
                        }
                    }
                    return;
                }
                else {
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0) {
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        if(iNextCol == iCol && iNextRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc;
                            if(pNextField0 != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc +1;
                            }
                            pNextField = pNextField0;
                        }
                        else if(iNextCol == iCol && iNextRow != iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(0);
                        }
                        else {
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(0);
                        }
                    }
                    else {
                        iOcc = iCurOcc +1;
                        //Get first field from the column
                        CDECol* pCol = pRoster->GetCol(iCol);
                        pNextField = pCol->GetField(0);
                    }

                }
                break;
            case VK_LEFT:
                //Get the Previous field if you can  . //leave the occ as is
                pNextField = GetPrevField(pCurField);
                if(pNextField && pNextField != pCurField) {
                    iOcc = iCurOcc;
                }
                break;
            case VK_RIGHT:
                //Get the Next field if you can  . //leave the occ as is
                pNextField = GetNextField(pCurField);
                if(pNextField && pNextField != pCurField) {
                    iOcc = iCurOcc;
                }
                break;
            case VK_RETURN:
                //Get the Next field if you can  . //leave the occ as is
                pNextField = GetNextField(pCurField);
                if(pNextField && pNextField != pCurField ){
                    iOcc = iCurOcc;
                }
                else if(pNextField && pNextField == pCurField && IsLastField(pCurField) && iCurOcc == iMaxOccs) {
                    iOcc = iCurOcc;
                }
                else {
                    CDEField* pLocalField = GetFirstNonProtectedField(pCurField->GetParent());
                    if(pLocalField){
                        if(iCurOcc < iMaxOccs) { //boundary
                            iOcc = iCurOcc+1;
                        }
                        pNextField = pLocalField;
                    }
                }
                break;
            default:
                break;
            }
        }
        else {//orientation of grid is vertical
            switch(uKey) {
            case VK_DOWN:
                //Get the Next field if you can  . //leave the occ as is
                pNextField = GetNextField(pCurField);
                if(pNextField && pNextField != pCurField) {
                    iOcc = iCurOcc;
                }
                break;
            case VK_UP:
                //Get the Previous field if you can  . //leave the occ as is
                pNextField = GetPrevField(pCurField);
                if(pNextField && pNextField != pCurField) {
                    iOcc = iCurOcc;
                }
                break;
            case VK_LEFT:
                //Get the next occ if you can
                if(iCurOcc == 1) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pPrevField = GetPrevField(pCurField);
                    int iPrevCol =-1;
                    int iPrevRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pPrevField == pCurField) {
                        return;
                    }
                    else if(pPrevField){
                        pGrid->FindField(pPrevField,iCurOcc,&iPrevRow, &iPrevCol);
                        //check if it is from the same row if yes go to the field
                        if(iPrevRow == iRow){
                            iOcc = iCurOcc;
                            pNextField = pPrevField; // do not change the occ
                        }
                    }

                }
                else {
                    CDEField* pPrevField = GetPrevField(pCurField);
                    int iPrevCol =-1;
                    int iPrevRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pPrevField) {
                        pGrid->FindField(pPrevField,iCurOcc,&iPrevRow, &iPrevCol);
                        if(iPrevCol == iCol && iPrevRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            if(pPrevField != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc -1;
                            }

                            pNextField = pPrevField;
                        }
                        else if(iPrevCol != iCol && iPrevRow == iRow ){
                            iOcc = iCurOcc -1;
                            //Get last field  from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(pCol->GetNumFields()-1);
                        }
                        else {
                            iOcc = iCurOcc -1;
                            //Get last field  from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(pCol->GetNumFields()-1);
                        }
                    }
                    else {
                        iOcc = iCurOcc -1;
                        //Get last field  from the column
                        CDECol* pCol = pRoster->GetCol(iRow);
                        pNextField = pCol->GetField(pCol->GetNumFields()-1);
                    }
                }
                break;

            case VK_RIGHT:
                //Get the next occ if you can
                if(iCurOcc == iMaxOccs) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0 == pCurField) {
                        return;
                    }
                    else if(pNextField0){
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        //check if it is from the same column if yes go to the field
                        if(iNextRow == iRow){
                            iOcc = iCurOcc;
                            pNextField = pNextField0; // do not change the occ
                        }
                    }
                    return;
                }
                else {
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0) {
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        if(iNextCol == iCol && iNextRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc;
                            if(pNextField0 != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc +1;
                            }
                            pNextField = pNextField0;
                        }
                        else if(iNextCol == iCol && iNextRow != iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(0);
                        }
                        else {
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(0);
                        }
                    }
                    else {
                        iOcc = iCurOcc +1;
                        //Get first field from the column
                        CDECol* pCol = pRoster->GetCol(iRow);
                        pNextField = pCol->GetField(0);
                    }

                }
                break;
            case VK_RETURN:
                //Get the next occ if you can
                if(iCurOcc == iMaxOccs) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0 == pCurField) {
                        pNextField = pCurField;
                        return;
                    }
                    else if(pNextField0){
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        //check if it is from the same column if yes go to the field
                        if(iRow != iNextRow){ //From a different row
                            iOcc = 1;
                            pNextField = pNextField0;
                        }
                        else { //from a different row
                            iOcc = iCurOcc;
                            pNextField = pNextField0; // do not change the occ
                        }
                    }
                    return;
                }
                else {
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0) {
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        if(iNextCol == iCol && iNextRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc;
                            if(pNextField0 != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc +1;
                            }
                            pNextField = pNextField0;
                        }
                        else if(iNextCol != iCol && iNextRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(0);
                        }
                        else {
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(0);
                        }
                    }
                    else {
                        iOcc = iCurOcc +1;
                        //Get first field from the column
                        CDECol* pCol = pRoster->GetCol(iRow);
                        pNextField = pCol->GetField(0);
                    }

                }
                break;

            default:
                break;
            }
        }
    }
    else { //movement is vertical
        //orientation of grid is horizontal
        if(pRoster->GetOrientation() == RosterOrientation::Horizontal) {
            switch(uKey) {
            case VK_UP:
                 //Get the next occ if you can
                if(iCurOcc == 1) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pPrevField = GetPrevField(pCurField);
                    int iPrevCol =-1;
                    int iPrevRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pPrevField == pCurField) {
                        return;
                    }
                    else if(pPrevField){
                        pGrid->FindField(pPrevField,iCurOcc,&iPrevRow, &iPrevCol);
                        //check if it is from the same row if yes go to the field
                        if(iPrevCol == iCol){
                            iOcc = iCurOcc;
                            pNextField = pPrevField; // do not change the occ
                        }
                    }

                }
                else {
                    CDEField* pPrevField = GetPrevField(pCurField);
                    int iPrevCol =-1;
                    int iPrevRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pPrevField) {
                        pGrid->FindField(pPrevField,iCurOcc,&iPrevRow, &iPrevCol);
                        if(iPrevCol == iCol && iPrevRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            if(pPrevField != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc -1;
                            }

                            pNextField = pPrevField;
                        }
                        else if(iPrevCol != iCol && iPrevRow == iRow ){
                            iOcc = iCurOcc -1;
                            //Get last field  from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(pCol->GetNumFields()-1);
                        }
                        else {
                            iOcc = iCurOcc -1;
                            //Get last field  from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(pCol->GetNumFields()-1);
                        }
                    }
                    else {
                        iOcc = iCurOcc -1;
                        //Get last field  from the column
                        CDECol* pCol = pRoster->GetCol(iCol);
                        pNextField = pCol->GetField(pCol->GetNumFields()-1);
                    }
                }
                break;
            case VK_DOWN:
                //Get the next occ if you can
                if(iCurOcc == iMaxOccs) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0 == pCurField) {
                        return;
                    }
                    else if(pNextField0){
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        //check if it is from the same column if yes go to the field
                        if(iNextCol == iCol){
                            iOcc = iCurOcc;
                            pNextField = pNextField0; // do not change the occ
                        }
                    }
                    return;
                }
                else {
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0) {
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        if(iNextCol == iCol && iNextRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc;
                            if(pNextField0 != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc +1;
                            }
                            pNextField = pNextField0;
                        }
                        else if(iNextCol == iCol && iNextRow != iRow ){
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(0);
                        }
                        else {
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(0);
                        }
                    }
                    else {
                        iOcc = iCurOcc +1;
                        //Get first field from the column
                        CDECol* pCol = pRoster->GetCol(iCol);
                        pNextField = pCol->GetField(0);
                    }

                }
                break;
            case VK_LEFT:
                //Get the Previous field if you can  . //leave the occ as is
                pNextField = GetPrevField(pCurField);
                if(pNextField && pNextField != pCurField) {
                    iOcc = iCurOcc;
                }
                break;
            case VK_RIGHT:
                //Get the Next field if you can  . //leave the occ as is
                pNextField = GetNextField(pCurField);
                if(pNextField && pNextField != pCurField) {
                    iOcc = iCurOcc;
                }
                break;
            case VK_RETURN:
               //Get the next occ if you can
                if(iCurOcc == iMaxOccs) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0 == pCurField) {
                        pNextField = pCurField;
                        return;
                    }
                    else if(pNextField0){
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        //check if it is from the same column if yes go to the field
                        if(iCol != iNextCol){ //From a different row
                            iOcc = 1;
                            pNextField = pNextField0;
                        }
                        else { //from a different row
                            iOcc = iCurOcc;
                            pNextField = pNextField0; // do not change the occ
                        }
                    }
                    return;
                }
                else {
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0) {
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        if(iNextCol == iCol && iNextRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc;
                            if(pNextField0 != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc +1;
                            }
                            pNextField = pNextField0;
                        }
                        else if(iNextCol != iCol && iNextRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(0);
                        }
                        else {
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iCol);
                            pNextField = pCol->GetField(0);
                        }
                    }
                    else {
                        iOcc = iCurOcc +1;
                        //Get first field from the column
                        CDECol* pCol = pRoster->GetCol(iCol);
                        pNextField = pCol->GetField(0);
                    }

                }
                break;

            default:
                break;
            }
        }
        else {//orientation of grid is vertical
            switch(uKey) {
            case VK_DOWN:
                 //Get the Next field if you can  . //leave the occ as is
                pNextField = GetNextField(pCurField);
                if(pNextField && pNextField != pCurField) {
                    iOcc = iCurOcc;
                }
                break;
            case VK_UP:
                 //Get the Previous field if you can  . //leave the occ as is
                pNextField = GetPrevField(pCurField);
                if(pNextField && pNextField != pCurField) {
                    iOcc = iCurOcc;
                }
                break;
            case VK_LEFT:
                 //Get the next occ if you can
                if(iCurOcc == 1) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pPrevField = GetPrevField(pCurField);
                    int iPrevCol =-1;
                    int iPrevRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pPrevField == pCurField) {
                        return;
                    }
                    else if(pPrevField){
                        pGrid->FindField(pPrevField,iCurOcc,&iPrevRow, &iPrevCol);
                        //check if it is from the same column if yes go to the field
                        if(iPrevRow == iRow){
                            iOcc = iCurOcc;
                            pNextField = pPrevField; // do not change the occ
                        }
                    }

                }
                else {
                    CDEField* pPrevField = GetPrevField(pCurField);
                    int iPrevCol =-1;
                    int iPrevRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pPrevField) {
                        pGrid->FindField(pPrevField,iCurOcc,&iPrevRow, &iPrevCol);
                        if(iPrevCol == iCol && iPrevRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            if(pPrevField != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc -1;
                            }

                            pNextField = pPrevField;
                        }
                        else if(iPrevCol != iCol && iPrevRow == iRow ){
                            iOcc = iCurOcc -1;
                            //Get last field  from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(pCol->GetNumFields()-1);
                        }
                        else {
                            iOcc = iCurOcc -1;
                            //Get last field  from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(pCol->GetNumFields()-1);
                        }
                    }
                    else {
                        iOcc = iCurOcc -1;
                        //Get last field  from the column
                        CDECol* pCol = pRoster->GetCol(iRow);
                        pNextField = pCol->GetField(pCol->GetNumFields()-1);
                    }

                }
                break;

            case VK_RIGHT:
              //Get the next occ if you can
                if(iCurOcc == iMaxOccs) { //boundary
                    //check for other fields in the joined cell .SAVY&&&
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0 == pCurField) {
                        return;
                    }
                    else if(pNextField0){
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        //check if it is from the same column if yes go to the field
                        if(iNextRow == iRow){
                            iOcc = iCurOcc;
                            pNextField = pNextField0; // do not change the occ
                        }
                    }
                    return;
                }
                else {
                    CDEField* pNextField0 = GetNextField(pCurField);
                    int iNextCol =-1;
                    int iNextRow =-1;
                    //if  it comes from same row and column as  the curfield then that is the new field ;
                    if(pNextField0) {
                        pGrid->FindField(pNextField0,iCurOcc,&iNextRow, &iNextCol);
                        if(iNextCol == iCol && iNextRow == iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc;
                            if(pNextField0 != pCurField) {
                                iOcc = iCurOcc;
                            }
                            else {
                                iOcc = iCurOcc +1;
                            }
                            pNextField = pNextField0;
                        }
                        else if(iNextCol == iCol && iNextRow != iRow ){//do not  change the occurrence for traversal with in the cell
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(0);
                        }
                        else {
                            iOcc = iCurOcc +1;
                            //Get first field from the column
                            CDECol* pCol = pRoster->GetCol(iRow);
                            pNextField = pCol->GetField(0);
                        }
                    }
                    else {
                        iOcc = iCurOcc +1;
                        //Get first field from the column
                        CDECol* pCol = pRoster->GetCol(iRow);
                        pNextField = pCol->GetField(0);
                    }

                }
                break;
            case VK_RETURN:
                //Get the Next field if you can  . //leave the occ as is
                pNextField = GetNextField(pCurField);
                if(pNextField && pNextField != pCurField ){
                    iOcc = iCurOcc;
                }
                else if(pNextField && pNextField == pCurField && IsLastField(pCurField) && iCurOcc == iMaxOccs) {
                    iOcc = iCurOcc;
                }
                else {
                    CDEField* pLocalField = GetFirstNonProtectedField(pCurField->GetParent());
                    if(pLocalField){
                        if(iCurOcc < iMaxOccs) { //boundary
                            iOcc = iCurOcc+1;
                        }
                        pNextField = pLocalField;
                    }
                }
                break;
            default:
                break;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CDEField* CEntryrunView::GetNextField(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
CDEField* CEntryrunView::GetNextField(CDEField* pField)
{
    CDEField* pRet = pField;
    CDEGroup* pGroup = pField->GetParent();
    int iNumItems = pGroup->GetNumItems();
    bool bFound = false;
    for(int iIndex = 0; iIndex < iNumItems ; iIndex++) {
        if(pGroup->GetItem(iIndex) == pField)  {
            bFound = true;
            continue;
        }
        if(!bFound){
            continue;
        }
        else {
            if(pGroup->GetItem(iIndex)->IsKindOf(RUNTIME_CLASS(CDEField))){
                if(!((CDEField*)pGroup->GetItem(iIndex))->IsProtected()){
                    pRet = (CDEField*)pGroup->GetItem(iIndex);
                    break;
                }
            }
        }
    }
    return pRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  CDEField* CEntryrunView::GetPrevField(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
CDEField* CEntryrunView::GetPrevField(CDEField* pField)
{
    CDEField* pRet = NULL;
    CDEGroup* pGroup = pField->GetParent();
    int iNumItems = pGroup->GetNumItems();
    for(int iIndex = 0; iIndex < iNumItems ; iIndex++) {
        if(pGroup->GetItem(iIndex) == pField)  {
            if(!pRet) {
                pRet = pField;
            }
            break;
        }
        if(pGroup->GetItem(iIndex)->IsKindOf(RUNTIME_CLASS(CDEField))){
            if(!((CDEField*)pGroup->GetItem(iIndex))->IsProtected()){
                pRet = (CDEField*)pGroup->GetItem(iIndex);
            }
        }
    }
    return pRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CEntryrunView::GetDynamicMaxOccs(CDEGroup* pGroup)
//
/////////////////////////////////////////////////////////////////////////////////
int CEntryrunView::GetDynamicMaxOccs(CDEGroup* pGroup)
{
    int iMaxOccs = pGroup->GetMaxLoopOccs();
    if(!pGroup->GetMaxDEField()){
        return iMaxOccs;
    }
    else {
        CIMSAString sVal;
        CEntryrunDoc* pRunDoc = GetDocument();
        CRunAplEntry* pRunApl = pRunDoc->GetRunApl();
        CDEField* pDynField = NULL;
        CDEForm* pForm  = NULL;
        pRunDoc->GetCurFormFile()->FindField(pGroup->GetMaxDEField()->GetName(),&pForm,(CDEItemBase**)&pDynField);

        if( pDynField )
            sVal = pRunApl->GetVal(pGroup->GetMaxDEField()->GetSymbol(), pDynField->GetParent()->GetCurOccurrence());

        else
            sVal = pRunApl->GetVal(pGroup->GetMaxDEField()->GetSymbol(), 1);

        sVal.ReleaseBuffer();
        sVal.Trim();
        if(sVal.IsNumeric()){
            int iVal = (int)sVal.Val();
            if(iVal >=0 && iVal <= iMaxOccs) {
                iMaxOccs = iVal;
            }
        }
    }
    return iMaxOccs;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CDEField* CEntryrunView::GetFirstNonProtectedField(CDEGroup* pGroup )
//
/////////////////////////////////////////////////////////////////////////////////
CDEField* CEntryrunView::GetFirstNonProtectedField(CDEGroup* pGroup )
{
    ASSERT(pGroup);
    CDEField* pRetFld = NULL;
    for(int iIndex =0 ;iIndex < pGroup->GetNumItems(); iIndex++){
        pRetFld = DYNAMIC_DOWNCAST(CDEField,pGroup->GetItem(iIndex));
        if(pRetFld && !pRetFld->IsProtected()){ //Get the first non protected field
            break;
        }
    }
    return pRetFld ;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CDEField* CEntryrunView::GetLastNonProtectedField(CDEGroup* pGroup)
//
/////////////////////////////////////////////////////////////////////////////////
CDEField* CEntryrunView::GetLastNonProtectedField(CDEGroup* pGroup)
{

    ASSERT(pGroup);
    CDEField* pRetFld = NULL;
    for(int iIndex =pGroup->GetNumItems()-1 ;iIndex >=0 ; iIndex--){
        pRetFld = DYNAMIC_DOWNCAST(CDEField,pGroup->GetItem(iIndex));
        if(pRetFld && !pRetFld->IsProtected()){ //Get the first non protected field
            break;
        }
    }
    return pRetFld ;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CEntryrunView::IsLastField(CDEGroup* pGroup , CDEField* pCurField)
//
/////////////////////////////////////////////////////////////////////////////////
bool CEntryrunView::IsLastField(CDEField* pCurField)
{
    ASSERT(pCurField);
    return (GetLastNonProtectedField(pCurField->GetParent()) == pCurField);
}


void CEntryrunView::SetVerifyStringFromControls(CString csString,bool bIsNumeric) // 20140312
{
    CString csTrimmedString = m_sVerify;

    if( bIsNumeric )
        csTrimmedString.Trim();

    if( csString.CompareNoCase(csTrimmedString) != 0 )
    {
        m_iVerify = 0;
        m_sVerify = csString;
    }
}


void CEntryrunView::SetupFieldColors(const CDEFormFile* form_file)
{
    if( form_file != nullptr )
        m_fieldColors = form_file->GetEvaluatedFieldColors();

    m_fieldBrushes.resize(m_fieldColors.GetColors().size());

    auto field_bursh_itr = m_fieldBrushes.begin();

    // the intensity order is different from the FieldStatus order
    for( FieldStatus field_status : { FieldStatus::Unvisited,
                                      FieldStatus::Skipped,
                                      FieldStatus::Visited,
                                      FieldStatus::Current } )
    {
        COLORREF color = m_fieldColors.GetColor(field_status).ToCOLORREF();

        if( std::get<1>(*field_bursh_itr) == nullptr || std::get<0>(*field_bursh_itr) != color )
            *field_bursh_itr = std::make_tuple(color, std::make_unique<CBrush>(color));

        ++field_bursh_itr;
    }
}
