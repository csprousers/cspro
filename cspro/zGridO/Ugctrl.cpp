/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGCtrl
    Purpose
        This is the main grid class. It contains
        and/or controls all of the other grid clases.
        CUGCtrl - stands for C++ Ultimate Grid Control.
        This is the class that other grids are usually
        derived from, since all of the grid's notifications
        are contained in this class.
        For simple and many complex grid applications this
        is the only class that needs to be used and worked
        with directly.
    Instructions
        -Generaly a derived class is created from this
         class and used in other applications. A derived
         class is neccessary if any of the grid's
         notifications are to be used
        -Being derived from CWnd allows the grid to be
         used anywhere a CWnd can be used, plus all CWnd
         functions are available as well.
        -This class automatically creates the grids
          child components (grid, top heading, side heading,
          scrollbars, tabs, and corner button)
        -This class also creates a default memory manager
         datasource and registers the grids standard
         cell types
        -When an instance of this class (or a class
         derived from this) is created, a windows still
         needs to be created (this is the same for any
         CWnd derived class).
         Call the classes CreateGrid to create the window.
         Or call AttachGrid to attach the grid to a control
         in a dialog box.
        -When this class creates or attaches to a window
         it's OnSetup member is automatically called.
         This is a good spot for initial setup of the grid.
************************************************/

#include "StdAfx.h"
#include "Ugctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif



/***************************************************
Constructor
    Initalizes all variables
****************************************************/
CUGCtrl::CUGCtrl()
{

    m_contructorResults = UG_SUCCESS;

    /********************************************
    set up the internal classes
    *********************************************/
    try{
        //data source
        m_dataSrcList =         NULL;
        m_dataSrcListLength =   0;
        m_dataSrcListMaxLength= 0;

        m_fontList =            new CUGPtrList;
        m_bitmapList =          new CUGPtrList;
        m_cellTypeList =        new CUGPtrList;
        m_cellStylesList =      new CUGPtrList;
        m_validateList =        new CUGPtrList;
        m_displayFormatList =   new CUGPtrList;
        m_menu =                new CMenu;
        m_CUGGrid =             new CUGGrid;
        m_CUGTopHdg =           new CUGTopHdg;
        m_CUGSideHdg =          new CUGSideHdg;
        m_CUGVScroll =          new CUGVScroll;
        m_CUGHScroll =          new CUGHScroll;
        m_CUGCnrBtn =           new CUGCnrBtn;
        m_CUGTab =              new CUGTab;

        //scroll hint window
        #ifdef UG_ENABLE_SCROLLHINTS
        m_CUGHint =             new CUGHint;
        #endif

        #ifdef UG_ENABLE_PRINTING
            m_CUGPrint =        new CUGPrint;
        #endif
    }
    catch(...){
        m_contructorResults = 1;
        return;
    }


    //tacking window
    m_trackingWnd           = NULL;

    //set up the 3D pens
    m_threeDLightPen.CreatePen(PS_SOLID,1,RGB(255,255,255));
    m_threeDDarkPen.CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNSHADOW));

    //popup menu
    m_menu->CreatePopupMenu();


    //Set up the 1 sheet and get the GridInfo class pointer
    //to that sheet
    m_currentSheet = 0;
    m_numberSheets = 0;
    m_GIList = NULL;
    m_GI = NULL;
    SetNumberSheets(1);
    m_GI = m_GIList[0];


    //setup inter-class pointers
    m_CUGGrid->m_ctrl       = this;
    m_CUGGrid->m_GI         = m_GI;
    m_CUGTopHdg->m_ctrl     = this;
    m_CUGTopHdg->m_GI       = m_GI;
    m_CUGSideHdg->m_ctrl    = this;
    m_CUGSideHdg->m_GI      = m_GI;
    m_CUGCnrBtn->m_ctrl     = this;
    m_CUGCnrBtn->m_GI       = m_GI;
    m_CUGVScroll->m_ctrl    = this;
    m_CUGVScroll->m_GI      = m_GI;
    m_CUGHScroll->m_ctrl    = this;
    m_CUGHScroll->m_GI      = m_GI;
    m_CUGTab->m_ctrl        = this;
    m_CUGTab->m_GI          = m_GI;

    #ifdef UG_ENABLE_SCROLLHINTS
    m_CUGHint->m_ctrl       = this;
    #endif

    #ifdef UG_ENABLE_PRINTING
        m_CUGPrint->m_ctrl  = this;
        m_CUGPrint->m_GI    = m_GI;
    #endif

    #ifdef UG_ENABLE_DRAGDROP
    m_dropTarget.m_ctrl = this;
    #endif
    m_defEditCtrl.m_ctrl    = this;

    m_editInProgress = FALSE;

    //add the standard cell types
    AddCellType(&m_normalCellType);
    AddCellType(&m_dropListType);
    AddCellType(&m_checkBoxType);
    AddCellType(&m_arrowType);

    //add a NULL datasource to the beginning of the
    //data source list, since index 0 is not used
    AddDataSource(NULL);

    m_enableUpdate = TRUE;
#ifdef UG_ENABLE_FINDDIALOG
    m_findReplaceDialog = NULL;
#endif
    m_findDialogRunning = FALSE;
    m_findDialogStarted = FALSE;
    m_findInAllCols = FALSE;

    m_tabSizing = FALSE;

    m_bVScrollAlwaysPresent = FALSE;                    // IMSA change
    m_bHScrollAlwaysPresent = FALSE;                    // IMSA change
}

/***************************************************
Destructor
    clean up all memory that was allocated
****************************************************/
CUGCtrl::~CUGCtrl()
{
    if(m_dataSrcList != NULL)
        delete[] m_dataSrcList;

    //delete the font list
    for(int loop = 0;loop < m_fontList->GetMaxCount();loop++){
        CFont* font = (CFont*)m_fontList->GetPointer(loop);
        if(font != NULL)
            delete font;
    }
    if(m_fontList !=  NULL)
        delete m_fontList;

    //delete the bitmap list
    for(int loop = 0;loop < m_bitmapList->GetMaxCount();loop++){
        CBitmap* bitmap = (CBitmap*)m_bitmapList->GetPointer(loop);
        if(bitmap != NULL)
            delete bitmap;
    }
    if(m_bitmapList != NULL)
        delete m_bitmapList;

    if(m_cellTypeList != NULL)
        delete m_cellTypeList;

    if(m_cellStylesList != NULL)
        delete m_cellStylesList;

    if(m_validateList != NULL)
        delete m_validateList;

    if(m_displayFormatList != NULL)
        delete m_displayFormatList;

    if(m_menu != NULL)
        delete m_menu;

    if(m_CUGGrid != NULL)
        delete m_CUGGrid;

    if(m_CUGTopHdg != NULL)
        delete m_CUGTopHdg;

    if(m_CUGSideHdg != NULL)
        delete m_CUGSideHdg;

    if(m_CUGVScroll != NULL)
        delete m_CUGVScroll;

    if(m_CUGHScroll != NULL)
        delete m_CUGHScroll;

    if(m_CUGCnrBtn != NULL)
        delete m_CUGCnrBtn;

    //scroll hint window
    #ifdef UG_ENABLE_SCROLLHINTS
        if(m_CUGHint != NULL) {
            m_CUGHint->DestroyWindow();
            delete m_CUGHint;
        }
    #endif

    #ifdef UG_ENABLE_PRINTING
        if(m_CUGPrint != NULL)
            delete m_CUGPrint;
    #endif

    if(m_CUGTab != NULL)
        delete m_CUGTab;

    if(m_GIList != NULL){
        for(int loop = 0; loop < m_numberSheets; loop++){
            if(m_GIList[loop] != NULL)
                delete m_GIList[loop];
        }
        delete[] m_GIList;
    }


}

/***************************************************
Message Map
    Processes default windows messages
****************************************************/
const UINT ugmsg_FindDialog = RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CUGCtrl, CWnd)
    //{{AFX_MSG_MAP(CUGCtrl)
    ON_WM_SIZE()
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_SETFOCUS()
    ON_WM_SYSCOLORCHANGE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_GETDLGCODE()
    ON_REGISTERED_MESSAGE(ugmsg_FindDialog ,ProcessFindDialog)
    //}}AFX_MSG_MAP

END_MESSAGE_MAP()

/***************************************************
OnSysColorChange
This function is called when system colors are changed
It then notifies all of the cell type classes, so
they can update themselves
****************************************************/
void CUGCtrl::OnSysColorChange()
{
    CWnd::OnSysColorChange();

    //call all the child windows and cell types
    // OnSystemChange members
    //check to see if there is room in the current list

    int loop;
    int count = m_cellTypeList->GetCount();

    for(loop=0;loop <count;loop++){
        if(m_cellTypeList->GetPointer(loop) != NULL){
            ((CUGCellType *)m_cellTypeList->GetPointer(loop))->OnSystemChange();
        }
    }
}

/***************************************************
OnHScroll
    pass all scroll messages over to the scroll child
    window
****************************************************/
void CUGCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    m_CUGHScroll->HScroll(nSBCode, nPos);
}

/***************************************************
OnVScroll
    pass all scroll messages over to the scroll child
    window
****************************************************/
void CUGCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    m_CUGVScroll->VScroll(nSBCode, nPos);
}


/***************************************************
OnSize
    Sizes all the child windows whenever the main
    window sizes. This includes the Grid, Headings
    and Scroll bars, tabs, etc.
****************************************************/
void CUGCtrl::OnSize(UINT nType, int cx, int cy)
{

    InvalidateRect(NULL);

    EnableUpdate(FALSE);
    AdjustComponentSizes();
    EnableUpdate(TRUE);
}

/***************************************************
EditCancel
    Purpose
        Cancels any editing if it is in progress
    Params
        none
    Return
        UG_SUCCESS  success
        UG_ERROR    error
****************************************************/
int CUGCtrl::EditCancel(){

    if(m_editInProgress == FALSE)
        return UG_SUCCESS;

    m_editInProgress = FALSE;

    if(GetFocus() == m_GI->m_editCtrl)
        m_CUGGrid->SetFocus();

    //hide the edit control
    m_GI->m_editCtrl->ShowWindow(SW_HIDE);

    return UG_SUCCESS;
}

/***************************************************
    Purpose
        This function is to be called by all edit
        controls that are used within the grid when
        editing is finished or canceled.
    Params
        string      - the string that is in the edit control
        cancelFlag  - edit cancel state
        continueFlag- edit continue state
        continueCol - continue col, if being continued
        continueRow - continue row, if being continued
    Return
        TRUE - if successful
        FALSE - if the current editing must continue
****************************************************/
int CUGCtrl::EditCtrlFinished(LPCTSTR string,BOOL cancelFlag,
                              BOOL continueFlag,int continueCol,
                              long continueRow){

    if(m_editInProgress == FALSE)
        cancelFlag = TRUE;

    //call the OnEditFinished notify in the datasource...
    if( m_GI->m_defDataSource->OnEditFinish(m_editCol,m_editRow,m_editCtrl,string,cancelFlag) == TRUE){
     //call the OnEditFinished notify
     if( OnEditFinish(m_editCol,m_editRow,m_editCtrl,string,cancelFlag) != FALSE){

        //hide the edit control
        m_editCtrl->ShowWindow(SW_HIDE);

        //save the info
        if(!cancelFlag)
            QuickSetText(m_editCol,m_editRow,string);

        //check to see if editing continues in a new cell
        if(continueFlag && m_editParent == m_CUGGrid){

            // adjust target continue cell if cell part of a join
            int startCol, endCol;
            long startRow, endRow;

            startCol = continueCol;
            startRow = continueRow;

            // is continue cell part of a join?
            if(GetJoinRange(&startCol, &startRow, &endCol, &endRow)==UG_SUCCESS) {
                // yes... adjust column
                if(m_editCol == startCol) {
                    // go past end of join we're in
                    if(continueCol > startCol && continueCol <= endCol)
                        continueCol += (endCol-startCol);
                }
                else {
                    // go to start of join we're entering
                    if(continueCol != startCol)
                        continueCol = startCol;
                }

                // adjust row ..
                if(m_editRow == startRow) {
                    if(continueRow > startRow && continueRow <= endRow)
                        continueRow += (endRow - startRow);
                }
                else {
                    if(continueRow != startRow)
                        continueRow = startRow;
                }
            }


            if(continueCol < 0)
                continueCol = m_GI->m_numberCols -1;
            else if(continueCol >= m_GI->m_numberCols)
                continueCol = 0;
            if(continueRow < 0)
                continueRow = m_GI->m_numberRows -1;
            else if(continueRow >= m_GI->m_numberRows)
                continueRow = 0;

            if(m_GI->m_defDataSource->OnEditContinue(m_editCol,m_editRow,&continueCol,&continueRow) == TRUE){
             if(OnEditContinue(m_editCol,m_editRow,&continueCol,&continueRow) != FALSE){
                StartEdit(continueCol,continueRow,0);
                return TRUE;
             }
            }
        }

        m_editInProgress = FALSE;

        return TRUE;
     }
    }

    if(m_editInProgress == FALSE)
        return TRUE;

    return FALSE;
}

/***************************************************
AdjustComponentSizes
    Purpose
        Sizes all of the child windows to match the
        parent window size. A OnAdjustComponentSizes
        notification is called so that the calculated
        sizes can be modified
    Params
        none
    Return
        none
****************************************************/
void CUGCtrl::AdjustComponentSizes(){

    if(GetFocus() != m_CUGGrid)
        m_CUGGrid->SetFocus();

    int loop;
    RECT rect;

    int vScrollWidth = m_GI->m_vScrollWidth;
    int hScrollHeight = m_GI->m_hScrollHeight;
    int origTabWidth = m_GI->m_tabWidth;

    GetClientRect(&rect);

    //check to see if the hScrollbar is to be shown
    int maxWidth = rect.right - m_GI->m_sideHdgWidth;
    if(m_GI->m_tabWidth > 0){
        if(m_GI->m_tabWidth > maxWidth-30)
            m_GI->m_tabWidth = (maxWidth-30);
        if(m_GI->m_tabWidth <4)
            m_GI->m_tabWidth = 4;
    }

    int width = 0;
    for(loop = 0;loop < m_GI->m_numberCols;loop++){
        width += GetColWidth(loop);
        if(width > maxWidth){
            break;
        }
    }

    //check to see if the vScrollbar is to be shown
    int maxHeight = rect.bottom - m_GI->m_topHdgHeight;
    int height = 0;
    for(loop = 0;loop < m_GI->m_numberRows;loop++){
        height += GetRowHeight(loop);
        if(height > maxHeight){
            break;
        }
    }

    if(height < maxHeight && !m_bVScrollAlwaysPresent) {                            // IMSA change
        if(width < maxWidth)
            m_GI->m_vScrollWidth = 0;
        else if(height < maxHeight-hScrollHeight)
            m_GI->m_vScrollWidth = 0;
    }

    if(width < maxWidth && m_GI->m_tabWidth <= 0 && !m_bHScrollAlwaysPresent) {     // IMSA change
        if(height < maxHeight)
            m_GI->m_hScrollHeight = 0;
        else if(width < maxWidth-vScrollWidth)
            m_GI->m_hScrollHeight = 0;
    }

    //setup the vScroll Rect
    m_GI->m_vScrollRect.left    = rect.right - m_GI->m_vScrollWidth;
    m_GI->m_vScrollRect.top     = 0;
    m_GI->m_vScrollRect.right   = rect.right;
    m_GI->m_vScrollRect.bottom  = rect.bottom - m_GI->m_hScrollHeight;

    //setup the hScroll Rect
    m_GI->m_hScrollRect.left    = m_GI->m_tabWidth;
    m_GI->m_hScrollRect.top     = rect.bottom - m_GI->m_hScrollHeight;
    m_GI->m_hScrollRect.right   = rect.right - m_GI->m_vScrollWidth;
    m_GI->m_hScrollRect.bottom  = rect.bottom;

    //setup the tabRect
    m_GI->m_tabRect.left        = 0;
    m_GI->m_tabRect.top         = rect.bottom - m_GI->m_hScrollHeight;
    m_GI->m_tabRect.right       = m_GI->m_tabWidth;
    m_GI->m_tabRect.bottom      = rect.bottom;

    //setup the topHeadingRect
    m_GI->m_topHdgRect.left     = m_GI->m_sideHdgWidth;
    m_GI->m_topHdgRect.top      = 0;
    m_GI->m_topHdgRect.right    = rect.right - m_GI->m_vScrollWidth;
    m_GI->m_topHdgRect.bottom   = m_GI->m_topHdgHeight;

    //setup the side headingRect
    m_GI->m_sideHdgRect.left    = 0;
    m_GI->m_sideHdgRect.top     = m_GI->m_topHdgHeight;
    m_GI->m_sideHdgRect.right   = m_GI->m_sideHdgWidth;
    m_GI->m_sideHdgRect.bottom  = rect.bottom - m_GI->m_hScrollHeight;

    //setup the corner buttonRect
    m_GI->m_cnrBtnRect.left     = 0;
    m_GI->m_cnrBtnRect.top      = 0;
    m_GI->m_cnrBtnRect.right    = m_GI->m_sideHdgWidth;
    m_GI->m_cnrBtnRect.bottom   = m_GI->m_topHdgHeight;

    //setup the GridRect
    m_GI->m_gridRect.left       = m_GI->m_sideHdgWidth;
    m_GI->m_gridRect.top        = m_GI->m_topHdgHeight;
    m_GI->m_gridRect.right      = m_GI->m_topHdgRect.right;
    m_GI->m_gridRect.bottom     = m_GI->m_sideHdgRect.bottom;
    m_GI->m_gridWidth = m_GI->m_gridRect.right - m_GI->m_gridRect.left;
    m_GI->m_gridHeight = m_GI->m_gridRect.bottom - m_GI->m_gridRect.top;

    //call the nofity function
    OnAdjustComponentSizes(&m_GI->m_gridRect,&m_GI->m_topHdgRect,
        &m_GI->m_sideHdgRect,&m_GI->m_cnrBtnRect,&m_GI->m_vScrollRect,
        &m_GI->m_hScrollRect,&m_GI->m_tabRect);

    if(m_tabSizing){
        m_CUGHScroll->MoveWindow(&m_GI->m_hScrollRect);
        m_CUGTab->MoveWindow(&m_GI->m_tabRect);
        m_CUGTab->Update();
        m_CUGHScroll->Update();
        ValidateRect(NULL);
    }
    else{
        m_CUGGrid->MoveWindow(&m_GI->m_gridRect);
        m_CUGTopHdg->MoveWindow(&m_GI->m_topHdgRect);
        m_CUGSideHdg->MoveWindow(&m_GI->m_sideHdgRect);
        m_CUGCnrBtn->MoveWindow(&m_GI->m_cnrBtnRect);
        m_CUGVScroll->MoveWindow(&m_GI->m_vScrollRect);
        m_CUGHScroll->MoveWindow(&m_GI->m_hScrollRect);
        m_CUGTab->MoveWindow(&m_GI->m_tabRect);

        CalcTopRow();
        CalcLeftCol();

        Update();
    }

    //make sure that the scroll dimensions are put back to their
    //original values
    m_GI->m_vScrollWidth = vScrollWidth;
    m_GI->m_hScrollHeight = hScrollHeight;

    m_GI->m_tabWidth = origTabWidth;

    m_GI->m_lastTopRow = m_GI->m_topRow;
    m_GI->m_lastLeftCol = m_GI->m_leftCol;
}
/***************************************************
OnCreate
    Purpose
        This function retrieves the control's parent's
        font, then creates the child windows
****************************************************/
int CUGCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    CreateChildWindows();


    return 0;
}
/***************************************************
CreateChildWindows
    Puropse
        This function creates all the child windows
        and passes them a pointer to this class as well
        as a pointer to the internal data structure
        (child windows include the grid, headings and
        scroll bars).
****************************************************/
BOOL CUGCtrl::CreateChildWindows(){

    RECT rect ={0,0,0,0};

    //set up the child windows
    m_CUGGrid->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,1,NULL);
    m_CUGTopHdg->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,2,NULL);
    m_CUGSideHdg->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,3,NULL);
    m_CUGCnrBtn->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,4,NULL);
    m_CUGTab->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,5,NULL);

    m_CUGVScroll->Create(SBS_VERT |WS_CHILD|WS_VISIBLE,rect,this,6);
    m_CUGHScroll->Create(SBS_HORZ |WS_CHILD|WS_VISIBLE,rect,this,7);

    //set up the hint window
    #ifdef UG_ENABLE_SCROLLHINTS
    m_CUGHint->Create(this);
    #endif

    //setup the default edit control
    if(m_defEditCtrl.Create(WS_CHILD|ES_MULTILINE|ES_AUTOVSCROLL,rect,this,123) == FALSE){
        //fail
    }

    //Size all the child windows
    AdjustComponentSizes();

    return UG_SUCCESS;
}
/***************************************************
OnPaint
    Purpose
        This routine is responsible for painting any area
        that is not covered by one of the child windows
        (such as the bottom right corner)
****************************************************/
void CUGCtrl::OnPaint()
{
    CDC * dc = GetDC();

    CBrush brush;
    RECT rect;
    GetClientRect(&rect);

    brush.CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    dc->FillRect(&rect,&brush);

    ReleaseDC(dc);
    ValidateRect(NULL);

    RedrawAll();
    m_CUGVScroll->Update();
    m_CUGHScroll->Update();
}

/***************************************************
PreCreateWindow
    Purpose
        This routine makes sure that certian window
        styles are used
****************************************************/
BOOL CUGCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= WS_CLIPCHILDREN;
    return CWnd::PreCreateWindow(cs);
}
/************************************************
*************************************************/
UINT CUGCtrl::OnGetDlgCode()
{
    // TODO: Add your message handler code here and/or call default
    return DLGC_WANTALLKEYS|DLGC_WANTARROWS;
    //return CWnd::OnGetDlgCode();
}

/***************************************************
OnSetFocus
    Purpose
        Sets the focus to the grid child window
****************************************************/
void CUGCtrl::OnSetFocus(CWnd* pOldWnd)
{
    m_CUGGrid->SetFocus();
}
/***************************************************
****************************************************/
BOOL CUGCtrl::CreateGrid(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID){

    //BOOL rt = Create((LPCTSTR)NULL,(LPCTSTR)_T(""),(DWORD) dwStyle, (const RECT&) rect,
    BOOL rt = Create(NULL,_T(""),(DWORD) dwStyle, (const RECT&) rect,
        (CWnd*) pParentWnd,(UINT) nID, (CCreateContext*)NULL);

    if(rt){
        OnSetup();
        OnSheetSetup(0);
    }

    return rt;
 }
/***************************************************
AttachGrid
    Purpose
        Attaches a 'CUG30' window from a dialog to the class
        Make sure the InitCUGCtrl(); function has been called
        before the dialog is opened, otherwise it will fail
****************************************************/
BOOL CUGCtrl::AttachGrid(CWnd * wnd,UINT ID){
#ifdef WIN32

    if( SubclassDlgItem(ID,wnd) ){
        long style = GetWindowLong(m_hWnd,GWL_STYLE);
        style = style|WS_CLIPCHILDREN|WS_TABSTOP|WS_BORDER;
        SetWindowLong(m_hWnd,GWL_STYLE,style);

        //check to see if there is a parent window
        //CWnd *w = GetParent();
        //if(w!= NULL)
        //  m_GI->m_defFont = (HFONT)w->SendMessage(WM_GETFONT,0,0);

        CreateChildWindows();

        OnSetup();
        OnSheetSetup(0);

        return UG_SUCCESS;
    }
    else
        return FALSE;

#else

    RECT rect;
    CWnd *ctrl = wnd->GetDlgItem(ID);
    ctrl->GetWindowRect(&rect);
    wnd->ScreenToClient(&rect);
    ctrl->DestroyWindow();

    return CreateGrid(WS_VISIBLE|WS_BORDER|WS_CHILD,rect,wnd,ID);

#endif
}
/***************************************************
CalcTopRow
    Purpose
        Calculates the Max top row allowed then adjusts
        the TopRow if needed (m_maxTopRow). This function
        is called internally
****************************************************/
void CUGCtrl::CalcTopRow(){

    long oldMaxRow = m_GI->m_maxTopRow;

    //calc the max top row
    long row;
    int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
    m_GI->m_maxTopRow = m_GI->m_numLockRows;
    for(row = m_GI->m_numberRows; row >= 0; row--){
        height -= GetRowHeight(row);
        if(height < 0){
            m_GI->m_maxTopRow = row +1;
            if(m_GI->m_maxTopRow >= m_GI->m_numberRows)
                m_GI->m_maxTopRow = m_GI->m_numberRows -1;
            break;
        }
    }

    //make sure that the max row is not below 0
    if(m_GI->m_maxTopRow < m_GI->m_numLockRows)
        m_GI->m_maxTopRow = m_GI->m_numLockRows;
    if(m_GI->m_topRow < m_GI->m_numLockRows)
        m_GI->m_topRow = m_GI->m_numLockRows;

    //adjust the top row if needed
    if(m_GI->m_topRow > m_GI->m_maxTopRow)
        m_GI->m_topRow = m_GI->m_maxTopRow;

    //check to see if the vscrollbar should be shown
    //or hidden
    if(oldMaxRow == 0 && m_GI->m_maxTopRow > 0)
        AdjustComponentSizes();
    if(oldMaxRow > 0 && m_GI->m_maxTopRow == 0)
        AdjustComponentSizes();
}
/***************************************************
CalcLeftCol
    Purpose
        Calculates the MAX left column then adjusts the
        left column if needed (m_maxLeftCol). This function
        is called internally
****************************************************/
void CUGCtrl::CalcLeftCol(){

    //calc the max left col
    int col;
    int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
    m_GI->m_maxLeftCol = m_GI->m_numLockCols;
    for(col = m_GI->m_numberCols ; col >= 0; col --){
        width -= GetColWidth(col);
        if(width <0){
            m_GI->m_maxLeftCol = col+1;
            if( m_GI->m_maxLeftCol >= m_GI->m_numberCols)
                m_GI->m_maxLeftCol = m_GI->m_numberCols -1;
            break;
        }
    }

    //check to make sure the values are not below 0
    if(m_GI->m_maxLeftCol < m_GI->m_numLockCols)
        m_GI->m_maxLeftCol = m_GI->m_numLockCols;
    if(m_GI->m_leftCol < m_GI->m_numLockCols)
        m_GI->m_leftCol = m_GI->m_numLockCols;

    //adjust the left column if needed
    if(m_GI->m_leftCol > m_GI->m_maxLeftCol)
        m_GI->m_leftCol = m_GI->m_maxLeftCol;

}

/***************************************************
AdjustTopRow
    Purpose
        Adjusts the top row so that that current row
        is in within view. This function is called
        internally
****************************************************/
void CUGCtrl::AdjustTopRow(){

    //if the top row is greater then set it equal
    if(m_GI->m_topRow > m_GI->m_dragRow){
        if(m_GI->m_dragRow >= m_GI->m_numLockRows)
            m_GI->m_topRow = m_GI->m_dragRow;
        else
            m_GI->m_topRow = m_GI->m_numLockRows;
    }


    //if the top row is equal then return
    if(m_GI->m_topRow == m_GI->m_dragRow)
        return;

    //if less then check to see if it is within
    //the height of the grid for distance
    if(m_GI->m_uniformRowHeightFlag == FALSE){
        long row;
        int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
        for(row = m_GI->m_dragRow; row >= m_GI->m_topRow; row--){
            height -= GetRowHeight(row);
            if(height < 0){
                m_GI->m_topRow = row +1;
                break;
            }
        }
    }
    else{
        int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
        long row = m_GI->m_dragRow - height / m_GI->m_defRowHeight + 1;
        if(m_GI->m_topRow < row)
            m_GI->m_topRow = row;
    }
}
/***************************************************
AdjustLeftCol
    Purpose
        Adjusts the left column so that that current
        col is in view. This function is called
        internally
****************************************************/
void CUGCtrl::AdjustLeftCol(){

    //if the left col row is greater then set it equal
    if(m_GI->m_leftCol > m_GI->m_dragCol){
        if(m_GI->m_dragCol >= m_GI->m_numLockCols)
            m_GI->m_leftCol = m_GI->m_dragCol;
        else
            m_GI->m_leftCol = m_GI->m_numLockCols;
    }

    //if the left col is equal then return
    if(m_GI->m_leftCol == m_GI->m_dragCol)
        return;

    //if less then check to see if it is within
    //the width of the grid for distance
    int col;
    int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
    for(col = m_GI->m_dragCol; col >= m_GI->m_leftCol; col--){
        width -= GetColWidth(col);
        if(width < 0 && col < m_GI->m_maxLeftCol){
            m_GI->m_leftCol = col +1;
            break;
        }
    }
}
/***************************************************
Update
    Purpose
        General purpose update, updates are sent to all child
        windows - each child window is then responsible for
        checking all variables that affect it.
****************************************************/
void CUGCtrl::Update(){

    if(!m_enableUpdate)
        return;

    m_CUGGrid->Update();
    m_CUGSideHdg->Update();
    m_CUGCnrBtn->Update();
    m_CUGVScroll->Update();
    m_CUGHScroll->Update();
    m_CUGTab->Update();
    m_CUGTopHdg->Update();

    //ValidateRect(NULL);
}
/***************************************************
Moved
    Purpose
        -Movement notify function
        -This function checks to see what moved in the grid
        then notifies the appropriate child windows
        -Processes movements for the multiple-selection class
        -stores the current cell
****************************************************/
void CUGCtrl::Moved(){

    //call the multiselect manager
    if(m_GI->m_multiSelectFlag && ! m_editInProgress){
        if(m_GI->m_moveType == 0) //keyboard
            m_GI->m_multiSelect->OnKeyMove(m_GI->m_dragCol,m_GI->m_dragRow);
        else if(m_GI->m_moveType == 1)  //left click
            m_GI->m_multiSelect->OnLClick(m_GI->m_dragCol,m_GI->m_dragRow,m_GI->m_moveFlags);
        else if(m_GI->m_moveType == 2)  //right click
            m_GI->m_multiSelect->OnRClick(m_GI->m_dragCol,m_GI->m_dragRow,m_GI->m_moveFlags);
        else    //mouse move
            m_GI->m_multiSelect->OnMouseMove(m_GI->m_dragCol,m_GI->m_dragRow,m_GI->m_moveFlags);

        m_GI->m_multiSelect->GetOrigCell(&m_GI->m_currentCol,&m_GI->m_currentRow);
    }
    else{
        m_GI->m_currentCol = m_GI->m_dragCol;
        m_GI->m_currentRow = m_GI->m_dragRow;
    }

    //send notify messages
    BOOL cellChange = FALSE;
    //col change
    if(m_GI->m_lastCol != m_GI->m_currentCol){
        m_GI->m_defDataSource->OnColChange(m_GI->m_lastCol,m_GI->m_currentCol);
        OnColChange(m_GI->m_lastCol,m_GI->m_currentCol);
        cellChange = TRUE;
    }
    //row change
    if(m_GI->m_lastRow != m_GI->m_currentRow){
        m_GI->m_defDataSource->OnRowChange(m_GI->m_lastRow,m_GI->m_currentRow);
        OnRowChange(m_GI->m_lastRow,m_GI->m_currentRow);
        cellChange = TRUE;
    }
    //cell change
    if(cellChange){
        OnCellChange(m_GI->m_lastCol,m_GI->m_currentCol,m_GI->m_lastRow,m_GI->m_currentRow);
    }
    //left col change
    if(m_GI->m_lastLeftCol != m_GI->m_leftCol){
        OnLeftColChange(m_GI->m_lastLeftCol,m_GI->m_leftCol);
    }
    //top row change
    if(m_GI->m_lastTopRow != m_GI->m_topRow){
        OnTopRowChange(m_GI->m_lastTopRow,m_GI->m_topRow);
    }

    //move the tracking window if need be
    MoveTrackingWindow();


    //notify the cell types of the move
    GetCellIndirect(m_GI->m_lastCol,m_GI->m_lastRow,&m_cell);
    GetCellType(m_cell.GetCellType())->
        OnKillFocus(m_GI->m_lastCol,m_GI->m_lastRow,&m_cell);

    GetCellIndirect(m_GI->m_currentCol,m_GI->m_currentRow,&m_cell);
    GetCellType(m_cell.GetCellType())->
        OnSetFocus(m_GI->m_currentCol,m_GI->m_currentRow,&m_cell);

    //notify the components of the move
    m_CUGGrid->Moved();
    m_CUGSideHdg->Moved();
    m_CUGTopHdg->Moved();
    m_CUGVScroll->Moved();
    m_CUGHScroll->Moved();
    m_CUGCnrBtn->Moved();

    //update the last... variables
    m_GI->m_lastTopRow = m_GI->m_topRow;
    m_GI->m_lastRow = m_GI->m_currentRow;
    m_GI->m_lastLeftCol = m_GI->m_leftCol;
    m_GI->m_lastCol = m_GI->m_currentCol;
}

/***************************************************
GetCellType
    Purpose
        Returns the pointer to the specified cell type.
        When a cell type is added to the grid it is given
        an index number. This index can be used to look
        up its coresponding pointer
    Params
        type - index value to look up
    Return
        pointer to a cell type, if the index is not
        found then the default cell type pointer
        is returned
****************************************************/
CUGCellType* CUGCtrl::GetCellType(int type){

    if(type <= 0)
        return &m_normalCellType;

    CUGCellType* ct = (CUGCellType*)m_cellTypeList->GetPointer(type);
    if(ct == NULL)
        return &m_normalCellType;
    else
        return ct;
}

/***************************************************
GetCellType
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::GetCellType(CUGCellType* type){

    return m_cellTypeList->GetPointerIndex((void*)type);
}

/***************************************************
GetCellType
    Purpose
        Returns the cell type for a given cell
    Params
        col,row - cell to return the cell type of
    Return
        pointer to a cell type, if the cell is not
        found then the default cell type pointer
        is returned
****************************************************/
CUGCellType * CUGCtrl::GetCellType(int col,long row){

    if(GetCellIndirect(col,row,&m_cell) == UG_SUCCESS){
        if(m_cell.IsPropertySet(UGCELL_CELLTYPE_SET))
            return GetCellType(m_cell.GetCellType());
        else
            return &m_normalCellType;
    }

    return GetCellType(-1);
}

/***************************************************
****************************************************
    MOVEMENT FUNTIONS    MOVEMENT FUNCTIONS
****************************************************
****************************************************/

/***************************************************
VerifyTopRow
    Purpose
        Checks to see that the new given position of the
        top row is valid, and will adjust it if ness.
    Params

    Return

****************************************************/
int CUGCtrl::VerifyTopRow(long* newRow){

    if(*newRow > m_GI->m_maxTopRow)
        *newRow = m_GI->m_maxTopRow;

    if(*newRow < m_GI->m_numLockRows)
        *newRow = m_GI->m_numLockRows;

    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::VerifyCurrentRow(long* newRow){

    if(*newRow >= m_GI->m_numberRows){

        //perform hit bottom
        long rowsfound = 0;
        m_GI->m_defDataSource->OnHitBottom(m_GI->m_numberRows,
            *newRow - m_GI->m_numberRows +1,&rowsfound);
        OnHitBottom(m_GI->m_numberRows,*newRow - m_GI->m_numberRows +1,rowsfound);

        if(m_GI->m_dragRow == m_GI->m_numberRows-1)
            return 1;
        else
            *newRow = m_GI->m_numberRows -1;
    }

    if(*newRow < 0)
        *newRow = 0;

    if(m_GI->m_enableJoins){
        GetCell(m_GI->m_dragCol,*newRow,&m_cell);
        if(m_cell.IsPropertySet(UGCELL_JOIN_SET)){
            int startCol = m_GI->m_dragCol;
            long startRow = *newRow;
            int endCol;
            long endRow;
            GetJoinRange(&startCol,&startRow,&endCol,&endRow);
            if(m_GI->m_moveType == 0){ //keyboard
                if(*newRow > m_GI->m_dragRow){
                    if(m_GI->m_dragRow >= startRow){
                        if(endRow+1 < m_GI->m_numberRows)
                            *newRow = endRow+1;
                    }
                    else{
                        *newRow = startRow;
                    }
                }
                else{
                    if(m_GI->m_dragRow <= endRow){
                        if(startRow -1 >0)
                            *newRow = startRow-1;
                    }
                }
            }
            else{
                *newRow = startRow;
            }

        }
    }

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::VerifyLeftCol(int* newCol){

    if(*newCol > m_GI->m_maxLeftCol)
        *newCol = m_GI->m_maxLeftCol;

    if(*newCol < m_GI->m_numLockCols)
        *newCol = m_GI->m_numLockCols;

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::VerifyCurrentCol(int* newCol){

    if(*newCol >= m_GI->m_numberCols){
        if(m_GI->m_dragCol == m_GI->m_numberCols -1)
            return 1;
        else
            *newCol = m_GI->m_numberCols -1;
    }
    if(*newCol == m_GI->m_dragCol)
        return UG_SUCCESS;

    if(*newCol < 0)
        *newCol = 0;

    if(m_GI->m_enableJoins){
        GetCell(*newCol,m_GI->m_dragRow,&m_cell);
        if(m_cell.IsPropertySet(UGCELL_JOIN_SET)){
            int startCol = *newCol;
            long startRow = m_GI->m_dragRow;
            int endCol;
            long endRow;
            GetJoinRange(&startCol,&startRow,&endCol,&endRow);
            if(m_GI->m_moveType == 0){ //keyboard
                if(*newCol > m_GI->m_dragCol){
                    if(m_GI->m_dragCol >= startCol){
                        if(endCol+1 < m_GI->m_numberCols)
                            *newCol = endCol+1;
                    }
                    else{
                        *newCol = startCol;
                    }
                }
                else{
                    if(m_GI->m_dragCol <= endCol){
                        if(startCol -1 >0)
                            *newCol = startCol-1;
                    }
                }
            }
            else{
                *newCol = startCol;
            }
        }
    }

    return UG_SUCCESS;
}

/***************************************************
MoveTopRow
    Moves the grid's top row, calls the OnCanViewMove
    first to see if the process is allowed
    Defines are avialable for this function
    'flag'  0-lineup 1-linedown 2-pageup
            3-pagedown 4-top 5-bottom

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::MoveTopRow(int flag){

    switch(flag){

        case UG_LINEUP:{
            return SetTopRow(m_GI->m_topRow -1);
        }
        case UG_LINEDOWN:{
            return SetTopRow(m_GI->m_topRow +1);
        }
        case UG_TOP:{
            return SetTopRow(0);
        }
        case UG_BOTTOM:{
            return SetTopRow(m_GI->m_numberRows - 1);
        }
        case UG_PAGEUP:{
            int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
            long row = m_GI->m_topRow;
            while(height >=0){
                if(row >=0)
                    height -= GetRowHeight(row);
                else
                    height -= m_GI->m_defRowHeight;
                row --;
                if(GetRowHeight(row) > height)
                    break;
            }
            return SetTopRow(row);
        }
        case UG_PAGEDOWN:{
            int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
            long row = m_GI->m_topRow;
            while(height >=0){
                if(row < m_GI->m_maxTopRow)
                    height -= GetRowHeight(row);
                else
                    height -= m_GI->m_defRowHeight;
                row ++;
                if(GetRowHeight(row) > height)
                    break;
            }
            return SetTopRow(row);
        }
    }
    return UG_SUCCESS;
}

/***************************************************
AdjustTopRow
    Adjusts the position of the top row by the given
    amount (which can be positive or negative).
    OnCanViewMove is called to see if the process is
    allowed

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::AdjustTopRow(long adjust){
    return SetTopRow(m_GI->m_topRow + adjust);
}

/***************************************************
MoveCurrentRow
    Moves the grid's current row, calls the OnCanMove
    first to see if the process is allowed
    Defines are avialable for this function
    'flag'  0-lineup 1-linedown 2-pageup
            3-pagedown 4-top 5-bottom

   returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success

****************************************************/
int CUGCtrl::MoveCurrentRow(int flag){

    switch(flag){

        case UG_LINEUP:{
            return GotoRow(m_GI->m_dragRow -1);
        }
        case UG_LINEDOWN:{
            return GotoRow(m_GI->m_dragRow +1);
        }
        case UG_TOP:{
            return GotoRow(0);
        }
        case UG_BOTTOM:{
            return GotoRow(m_GI->m_numberRows - 1);
        }
        case UG_PAGEUP:{
            int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
            long row = m_GI->m_dragRow;
            while(height >=0){
                if(row >=0)
                    height -= GetRowHeight(row);
                else
                    height -= m_GI->m_defRowHeight;
                row --;
                if(GetRowHeight(row) > height)
                    break;
            }
            return GotoRow(row);
        }
        case UG_PAGEDOWN:{
            int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
            long row = m_GI->m_dragRow;
            while(height >=0){
                if(row < m_GI->m_numberRows)
                    height -= GetRowHeight(row);
                else
                    height -= m_GI->m_defRowHeight;
                row ++;
                if(GetRowHeight(row) > height)
                    break;
            }
            return GotoRow(row);
        }
    }

    return UG_SUCCESS;
}

/***************************************************
AdjustCurrentRow
    Adjusts the position of the current row by the given
    amount (which can be positive or negative).
    OnCanMove is called to see if the process is
    allowed

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::AdjustCurrentRow(long adjust){
    return GotoRow(m_GI->m_dragRow + adjust);
}

/***************************************************
GotoRow
    Moves the current row to the given row.
    OnCanMove is called to see if the process is
    allowed

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::GotoRow(long row){

    if(m_GI->m_moveType == 0){  //keyboard
        if(GetKeyState(VK_SHIFT) >=0 && GetKeyState(VK_CONTROL) >=0)
            m_GI->m_multiSelect->GetOrigCell(&m_GI->m_currentCol,&m_GI->m_currentRow);
    }

    if(row == m_GI->m_dragRow)
        return UG_SUCCESS;

    if( VerifyCurrentRow(&row) != UG_SUCCESS)
        return 1;

    if(row == m_GI->m_dragRow)
        return UG_SUCCESS;

    if(OnCanMove(m_GI->m_dragCol,m_GI->m_dragRow,m_GI->m_dragCol,row) == FALSE)
        return 2;

    m_GI->m_dragRow = row;


    AdjustTopRow();


    Moved();

    return UG_SUCCESS;
}
/***************************************************
SetTopRow
    Moves the top row to the given position.
    OnCanViewMoved is called to see if the procedure
    is allowed

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::SetTopRow(long row){

    if(row == m_GI->m_topRow)
        return UG_SUCCESS;

    if( VerifyTopRow(&row) != UG_SUCCESS)
        return 1;

    if(OnCanViewMove(m_GI->m_leftCol,m_GI->m_topRow,m_GI->m_leftCol,row) == FALSE)
        return 2;

    m_GI->m_topRow = row;

    Moved();

    return UG_SUCCESS;
}

/***************************************************
MoveLeftCol
    Moves the grid's left column, calls the OnCanViewMove
    first to see if the process is allowed
    Defines are avialable for this function
    'flag'  0-lineup 1-linedown 2-pageup
            3-pagedown 4-top 5-bottom

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::MoveLeftCol(int flag){

    switch(flag){

        case UG_LINEUP:{
            return SetLeftCol(m_GI->m_leftCol -1);
        }
        case UG_LINEDOWN:{
            return SetLeftCol(m_GI->m_leftCol +1);
        }
        case UG_TOP:{
            return SetLeftCol(0);
        }
        case UG_BOTTOM:{
            return SetLeftCol(m_GI->m_numberCols - 1);
        }
        case UG_PAGEUP:{
            int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
            int col = m_GI->m_leftCol;
            while(width >=0){
                if(col >=0)
                    width -= GetColWidth(col);
                else
                    width -= m_GI->m_defColWidth;
                col --;
            }
            return SetLeftCol(col);
        }
        case UG_PAGEDOWN:{
            int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
            int col = m_GI->m_leftCol;
            while(width >=0){
                if(col >=0)
                    width -= GetColWidth(col);
                else
                    width -= m_GI->m_defColWidth;
                col ++;
            }
            return SetLeftCol(col);
        }
    }
    return UG_SUCCESS;
}
/***************************************************
AdjustLeftCol
    Adjusts the position of the left column by the given
    amount (which can be positive or negative).
    OnCanViewMove is called to see if the process is
    allowed

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::AdjustLeftCol(int adjust){
    return SetLeftCol(m_GI->m_leftCol + adjust);
}
/***************************************************
MoveCurrentCol
    Moves the grid's current column, calls the OnCanMove
    first to see if the process is allowed
    Defines are avialable for this function
    'flag'  0-lineup 1-linedown 2-pageup
            3-pagedown 4-top 5-bottom

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::MoveCurrentCol(int flag){

    switch(flag){

        case UG_LINEUP:{
            return GotoCol(m_GI->m_dragCol -1);
        }
        case UG_LINEDOWN:{
            return GotoCol(m_GI->m_dragCol +1);
        }
        case UG_TOP:{
            return GotoCol(0);
        }
        case UG_BOTTOM:{
            return GotoCol(m_GI->m_numberCols - 1);
        }
        case UG_PAGEUP:{
            int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
            int col = m_GI->m_dragCol;
            while(width >=0){
                if(col >=0)
                    width -= GetColWidth(col);
                else
                    width -= m_GI->m_defColWidth;
                col --;
            }
            return GotoCol(col);
        }
        case UG_PAGEDOWN:{
            int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
            int col = m_GI->m_dragCol;
            while(width >=0){
                if(col >=0)
                    width -= GetColWidth(col);
                else
                    width -= m_GI->m_defColWidth;
                col ++;
            }
            return GotoCol(col);
        }
    }

    return UG_SUCCESS;
}
/***************************************************
AdjustCurrentCol
    Adjusts the position of the current column by the given
    amount (which can be positive or negative).
    OnCanMove is called to see if the process is
    allowed

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::AdjustCurrentCol(int adjust){
    return GotoCol(m_GI->m_dragCol + adjust);
}
/***************************************************
GotoCol
    Moves the current column to the specified postion
    OnCanMove is called to see if the process is allowed

  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::GotoCol(int col){

    if(m_GI->m_moveType == 0){  //keyboard
        if(GetKeyState(VK_SHIFT) >=0 && GetKeyState(VK_CONTROL) >=0)
            m_GI->m_multiSelect->GetOrigCell(&m_GI->m_currentCol,&m_GI->m_currentRow);
    }

    if(col == m_GI->m_dragCol)
        return UG_SUCCESS;

    if( VerifyCurrentCol(&col) != UG_SUCCESS)
        return 1;

    if(col == m_GI->m_dragCol)
        return UG_SUCCESS;

    // TD call the datasource...
    if(m_GI->m_defDataSource->OnCanMove(m_GI->m_dragCol,m_GI->m_dragRow,col,m_GI->m_dragRow) != TRUE)
        return 2;

    if(OnCanMove(m_GI->m_dragCol,m_GI->m_dragRow,col,m_GI->m_dragRow) == FALSE)
        return 2;

    m_GI->m_dragCol = col;

    AdjustLeftCol();

    Moved();

    return UG_SUCCESS;
}
/***************************************************
SetLeftCol
    Moves the left column to the specified postion
    OnCanViewMove is called to see if the process is allowed
  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::SetLeftCol(int col){

    if(col == m_GI->m_leftCol)
        return UG_SUCCESS;

    if( VerifyLeftCol(&col) != UG_SUCCESS)
        return 1;

    if(OnCanViewMove(m_GI->m_leftCol,m_GI->m_topRow,col,m_GI->m_topRow) == FALSE)
        return 2;

    if(m_GI->m_leftCol == col)
        return UG_SUCCESS;


    m_GI->m_leftCol = col;
    Moved();

    return UG_SUCCESS;
}
/***************************************************
GotoCell
    Moves the current row/column to the specified
    position
    OnCanMove is called to see if the process is allowed
  returns
    1           out of range
    2           OnCanMove did not allow the procedure
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::GotoCell(int col,long row){

    if(col == m_GI->m_dragCol && row == m_GI->m_dragRow)
        return UG_SUCCESS;

    if( VerifyCurrentRow(&row) != UG_SUCCESS)
        return 1;

    if(m_GI->m_enableJoins)
        GetJoinStartCell(&col,&row);

    // first the datasource...
    if(m_GI->m_defDataSource->OnCanMove(m_GI->m_currentCol,m_GI->m_currentRow,col,row) != TRUE)
        return 2;

//  if(OnCanMove(m_GI->m_leftCol,m_GI->m_topRow,col,row) == FALSE)
    if(OnCanMove(m_GI->m_lastCol,m_GI->m_lastRow,col,row) == FALSE)         // BMD 06 Jan 2002
        return 2;

    m_GI->m_dragCol = col;
    m_GI->m_dragRow = row;

    AdjustLeftCol();
    AdjustTopRow();

    Moved();

    return UG_SUCCESS;
}
/***************************************************
GetCellFromPoint
    Returns the column and row that lies over the
    given x,y co-ordinates. The co-ordinates are
    relative to the top left hand corner of the grid

  Return
    1           co-ords do not fall on a cell
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::GetCellFromPoint(int x,int y,int *ptcol,long *ptrow){

    RECT rect;
    return GetCellFromPoint(x,y,ptcol,ptrow,&rect);
}
/***************************************************
GetTopCellFromPoint
    ***GSF added
    Returns TRUE if point is in the top heading;
      ptcol is the col number (0 based) in that case
    Return FALSE if the point is not in the top heading
****************************************************/
BOOL CUGCtrl::GetTopCellFromPoint(int px,int py,int *ptcol){

    if(px < 0 || px > m_GI->m_gridWidth) {
        return FALSE;
    }
    if(py < 0 || py > m_GI->m_gridHeight) {
        return FALSE;
    }
    if (py >= m_GI->m_topHdgHeight) {
        return FALSE;
    }
    //find the col
    int width   = 0;
    for(int x=0;x<m_GI->m_numberCols;x++){

        if(x == m_GI->m_numLockCols)
            x = m_GI->m_leftCol;

        width += GetColWidth(x);

        if(width > px){
            *ptcol = x;
            break;
        }
    }
    return TRUE;
}
/***************************************************
GetSideCellFromPoint
    ***GSF added
    Returns TRUE if point is in the side heading;
      ptrow is the row number (0 based) in that case
    Return FALSE if the point is not in the side heading
****************************************************/
BOOL CUGCtrl::GetSideCellFromPoint(int px,int py,long *ptrow){

    if(px < 0 || px > m_GI->m_gridWidth) {
        return FALSE;
    }
    if(py < 0 || py > m_GI->m_gridHeight) {
        return FALSE;
    }
    if (px >= m_GI->m_sideHdgWidth) {
        return FALSE;
    }
    //find the row
    int height  = 0;
    for(int y=0;y<m_GI->m_numberRows;y++){

        if(y == m_GI->m_numLockRows)
            y = m_GI->m_topRow;

        height += GetRowHeight(y);

        if(height > py){
            *ptrow = y;
            break;
        }
    }
    return TRUE;
}

/***************************************************
GetCellFromPoint
    Returns the column and row that lies over the
    given x,y co-ordinates. The co-ordinates are
    relative to the top left hand corner of the grid
    area (not including the top headings);

    This function also retuns the rectangle of the
    cell that was found at that location

  Return
    1           co-ords do not fall on a cell
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::GetCellFromPoint(int px,int py,int *ptcol,long *ptrow,RECT *rect){

    int ptsFound = 0;

    int x;
    long y;

    if(px < 0 || px > m_GI->m_gridWidth)
        return 1;
    if(py < 0 || py > m_GI->m_gridHeight)
        return 1;

    int width   = 0;
    int height  = 0;

    //find the col
    for(x=0;x<m_GI->m_numberCols;x++){

        if(x == m_GI->m_numLockCols)
            x = m_GI->m_leftCol;

        width += GetColWidth(x);

        if(width > px){
            *ptcol = x;
            rect->right = width;
            rect->left = rect->right - GetColWidth(x);
            ptsFound ++;
            break;
        }
    }

    //find the row
    for(y=0;y<m_GI->m_numberRows;y++){

        if(y == m_GI->m_numLockRows)
            y = m_GI->m_topRow;

        height += GetRowHeight(y);

        if(height > py){
            *ptrow = y;
            rect->bottom = height;
            rect->top = rect->bottom - GetRowHeight(y);
            ptsFound ++;
            break;
        }
    }

    //if the cell was not found then return 1
    if(ptsFound != 2)
        return 1;

    //if cell joining then check to see if the see is joined
    if(m_GI->m_enableJoins){
        GetCell(x,y,&m_cell);
        if(m_cell.IsPropertySet(UGCELL_JOIN_SET)){
            GetCellRect(ptcol,ptrow,rect);
        }
    }
    return UG_SUCCESS;
}
/***************************************************
Does not take cell joining into account
****************************************************/
int CUGCtrl::GetAbsoluteCellFromPoint(int x,int y,int *ptcol,long *ptrow){

    int tempJoinFlag = m_GI->m_enableJoins;

    RECT rect;
    m_GI->m_enableJoins = FALSE;            //******* GSF bug fix!!!!
    int rt = GetCellFromPoint(x,y,ptcol,ptrow,&rect);

    m_GI->m_enableJoins = tempJoinFlag;

    return rt;
}
/***************************************************
GetCellRect
    Returns the rectangle of the given cell.
    Relative to the top-left corner of the grid
    area (not including the top headings);
  Return
    1           co-ords out of range
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::GetCellRect(int col,long row,RECT *rect){
    return GetCellRect(&col,&row,rect);
}
/***************************************************
GetCellRect
int CUGCtrl::GetCellRect(int *col,long *row,RECT *rect);

    Purpose
        Returns the rectangle of the given cell relative
        to the top-left corner of the grid area (not
        including the top/side headings). This function
        takes cell joining and locked regions into account
    Params
        col - (input) column to return the rect of
              (output) the start cell of a joined cell
              region if the specified col/row points to
              a cell within the region
        row - (input) row to return the rect of
              (output) the start cell of a joined cell
              region if the specified col/row points to
              a cell witin the region
        rect -  (output) returns the rectange of the specified
                cell in pixels. Joins and locked regions are
                taken into account
    Return
        UG_SUCCESS  success
        1           cell not visible
****************************************************/
int CUGCtrl::GetCellRect(int *col,long *row,RECT *rect){


    int x;
    long y;
    int width =0;
    int height =0;

    int startCol    = *col;
    int endCol      = *col;
    long startRow   = *row;
    long endRow     = *row;

    rect->left  = 0;
    rect->top   = 0;
    rect->right = 0;
    rect->bottom    = 0;

    //if the specified cell is within a join then find the joined range
    if(m_GI->m_enableJoins)
        if(GetJoinRange(&startCol,&startRow,&endCol,&endRow) == UG_SUCCESS){
            *col = startCol;
            *row = startRow;
        }


    //find the col
    if(startCol >= m_GI->m_numLockCols){//if the col is not within the lock region
        rect->left = m_GI->m_lockColWidth;
        rect->right = m_GI->m_lockColWidth;
    }
    for(x=0;x<m_GI->m_numberCols;x++){

        if(x == m_GI->m_numLockCols)
            x = m_GI->m_leftCol;

        if(x == startCol)
            rect->left = width;

        width += GetColWidth(x);

        if(x == endCol){
            rect->right = width;
            break;
        }
    }

    //find the row
    if(startRow >= m_GI->m_numLockRows){ //if the row is not within the lock region
        rect->top       = m_GI->m_lockRowHeight;
        rect->bottom    = m_GI->m_lockRowHeight;
    }
    for(y=0;y<m_GI->m_numberRows;y++){

        if(y == m_GI->m_numLockRows)
            y = m_GI->m_topRow;

        if(y == startRow)
            rect->top = height;

        height += GetRowHeight(y);

        if(y == endRow){
            rect->bottom = height;
            break;
        }
    }

    return UG_SUCCESS;
}

/***************************************************
GetRangeRect
    returns the rectangle for a range of cells
****************************************************/
int CUGCtrl::GetRangeRect(int startCol,long startRow,int endCol,long endRow,RECT *rect){


    int x;
    long y;
    int width =0;
    int height =0;

    rect->left      = - 10;
    rect->top       = - 10;
    rect->right     = m_GI->m_gridWidth + 10;
    rect->bottom    = m_GI->m_gridHeight + 10;

    //find the col
    if(startCol >= m_GI->m_numLockCols && m_GI->m_numLockCols >0){//if the col is not within the lock region
        rect->left = m_GI->m_lockColWidth;
    }
    for(x=0;x<m_GI->m_numberCols;x++){

        if(x == m_GI->m_numLockCols)
            x = m_GI->m_leftCol;

        if(x == startCol)
            rect->left = width;

        width += GetColWidth(x);

        if(x == endCol){
            rect->right = width;
            break;
        }
        if(width > m_GI->m_gridWidth)
            break;
    }

    //find the row
    if(startRow >= m_GI->m_numLockRows && m_GI->m_numLockRows >0){ //if the row is not within the lock region
        rect->top       = m_GI->m_lockRowHeight;
    }
    for(y=0;y<m_GI->m_numberRows;y++){

        if(y == m_GI->m_numLockRows)
            y = m_GI->m_topRow;

        if(y == startRow)
            rect->top = height;

        height += GetRowHeight(y);

        if(y == endRow){
            rect->bottom = height;
            break;
        }
        if(height > m_GI->m_gridHeight)
            break;
    }

    return UG_SUCCESS;
}
/***************************************************
returns UG_SUCCESS if the specified cell is part of
a block, also returns the col and row of the starting
cell of that block
****************************************************/
int CUGCtrl::GetJoinStartCell(int *col,long *row){
    return GetJoinStartCell(col,row,&m_cell);
}
/***************************************************
returns UG_SUCCESS if the specified cell is part of
a block, also returns the col and row of the starting
cell of that block
****************************************************/
int CUGCtrl::GetJoinStartCell(int *col,long *row,CUGCell *cell){

    if(m_GI->m_enableJoins == FALSE)
        return 1;

    GetCellIndirect(*col,*row,cell);
    if(cell->IsPropertySet(UGCELL_JOIN_SET) == FALSE)
        return 2;

    int startCol;
    long startRow;
    BOOL origin;

    cell->GetJoinInfo(&origin,&startCol,&startRow);

    if(!origin){
        *col += startCol;
        *row += startRow;
        GetCellIndirect(*col,*row,cell);
    }

    return UG_SUCCESS;
}


// GHM 20100216 for FixLines routine
void CUGCtrl::GetJoinStartCellFast(int *col,long *row,CUGCell * cell){

    if( m_GI->m_enableJoins == FALSE )
        return;

    GetCell(*col,*row,cell);
    if( cell->IsPropertySet(UGCELL_JOIN_SET) == FALSE )
        return;

    BOOL origin;
    int coladd;
    long rowadd;

    cell->GetJoinInfo(&origin,&coladd,&rowadd);

    if( !origin )
    {
        *col += coladd;
        *row += rowadd;
        GetCell(*col,*row,cell);
    }
}



/***************************************************

returns UG_SUCCESS if the specified cell is part of
a block, also returns the col and row of the starting
and ending cells of that block
col and row point to a cell within the join on input
and contain the start cell on output if successful
****************************************************/
int CUGCtrl::GetJoinRange(int *col,long *row,int *col2,long *row2){

    if(m_GI->m_enableJoins == FALSE)
        return 1;

    GetCellIndirect(*col,*row,&m_cell);
    if(m_cell.IsPropertySet(UGCELL_JOIN_SET) == FALSE)
        return 2;

    int startCol;
    long startRow;
    BOOL origin;

    m_cell.GetJoinInfo(&origin,&startCol,&startRow);

    if(!origin){
        *col += startCol;
        *row += startRow;
        GetCellIndirect(*col,*row,&m_cell);
    }

    m_cell.GetJoinInfo(&origin,col2,row2);
    *col2+= *col;
    *row2+= *row;

    return UG_SUCCESS;
}
/***************************************************
EnableJoins
    Purpose
    Params
    Return
        UG_SUCCESS(0)   - success
        UG_ERROR(1)     - error
****************************************************/
int CUGCtrl::EnableJoins(BOOL state){
    if(state)
        m_GI->m_enableJoins = TRUE;
    else
        m_GI->m_enableJoins = FALSE;

    return UG_SUCCESS;
}
/***************************************************
EnableCellOverLap
    Purpose
    Params
    Return
        UG_SUCCESS(0)   - success
        UG_ERROR(1)     - error
****************************************************/
int CUGCtrl::EnableCellOverLap(BOOL state){
    if(state)
        m_GI->m_enableCellOverLap = TRUE;
    else
        m_GI->m_enableCellOverLap = FALSE;

    return UG_SUCCESS;
}
/***************************************************
EnableColSwapping
    Purpose
    Params
    Return
        UG_SUCCESS(0)   - success
        UG_ERROR(1)     - error
****************************************************/
int CUGCtrl::EnableColSwapping(BOOL state){
    if(state)
        m_GI->m_enableColSwapping = TRUE;
    else
        m_GI->m_enableColSwapping = FALSE;

    return UG_SUCCESS;
}

/***************************************************
EnableExcelBorders
    Purpose
    Params
    Return
        UG_SUCCESS(0)   - success
        UG_ERROR(1)     - error
****************************************************/
int CUGCtrl::EnableExcelBorders(BOOL state){
    if(state)
        m_GI->m_enableExcelBorders = TRUE;
    else
        m_GI->m_enableExcelBorders = FALSE;

    return UG_SUCCESS;
}

/***************************************************
****************************************************
    EDITING FUNCTIONS
****************************************************
****************************************************/

/***************************************************
StartEdit
    Starts editing mode - if the current cell is
    not readonly and the cell type of the current
    cell allows editing
    An OnStartEdit notification is also sent that
    can allow or dis-allow the editing

  return
    1           editing not allowed
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::StartEdit(){
    return StartEdit(m_GI->m_currentCol,m_GI->m_currentRow,0);
}
/***************************************************
StartEdit
    Starts editing mode - if the current cell is
    not readonly and the cell type of the current
    cell allows editing
    An OnStartEdit notification is also sent that
    can allow or dis-allow the editing
    'key' allows to have a keystroke stuffed into
    the edit control when is starts, useful for
    when editing starts because of a OnCharDown
    notification

  return
    1           editing not allowed
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::StartEdit(int key){

    return StartEdit(m_GI->m_currentCol,m_GI->m_currentRow,key);
}
/***************************************************
StartEdit
    Starts editing mode - if the specified cell is
    not readonly and the cell type of the specified
    cell allows editing
    An OnStartEdit notification is also sent that
    can allow or dis-allow the editing
    'key' allows to have a keystroke stuffed into
    the edit control when is starts, useful for
    when editing starts because of a OnCharDown
    notification
    'col','row' specifies the cell to edit

  return
    1           editing not allowed
    2           could not move to cell
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::StartEdit(int col,long row,int key){

    m_editInProgress = TRUE;

    //clear any multiple sections
    int startCol,endCol;
    long startRow,endRow;
    m_GI->m_multiSelect->GetTotalRange(&startCol,&startRow,&endCol,&endRow);
    ClearSelections();
    if(startCol!=endCol || startCol!=col || startRow!=endRow || startRow!=row)
        RedrawRange(startCol,startRow,endCol,endRow);

    if(col != m_GI->m_currentCol || row != m_GI->m_currentRow){
        if(GotoCell(col,row) != UG_SUCCESS){
            m_editInProgress = FALSE;
            return 2;
        }
    }

    m_editCol = col;
    m_editRow = row;
    m_editCell.ClearAll();
    GetCellIndirect(m_editCol,m_editRow,&m_editCell);

    //check to see if this cell type is editable
    CUGCellType * ct;
    if(m_editCell.IsPropertySet(UGCELL_CELLTYPE_SET))
        ct = GetCellType(m_editCell.GetCellType());
    else
        ct = GetCellType(0);
    if(ct->CanTextEdit() == FALSE){
        m_editInProgress = FALSE;
        return 1;
    }

    RECT rect = { 0 };
    CUGCell cell;

    if(col >= 0 && row >= 0){  //grid editing
        GetCellRect(col,row,&rect);
        m_editParent = m_CUGGrid;
    }
    if(col < 0 && row >= 0){    //side heading editing
        m_CUGSideHdg->GetCellRect(col,row,&rect);
        m_editParent = m_CUGSideHdg;
    }
    if(col >= 0 && row < 0){    //top heading editing
        m_CUGTopHdg->GetCellRect(col,row,&rect);
        m_editParent = m_CUGTopHdg;
    }

    rect.right--;
    rect.bottom--;

    //get the edit area
    ct->GetEditArea(&rect);

    if(m_editCell.GetReadOnly()){
        m_editInProgress = FALSE;
        return 1;
    }

    m_GI->m_editCtrl->SetWindowText(m_editCell.GetText());

    //set the pointer to the current edit control
    //only use this var from now on as it may point to another
    //control after OnEditStart is called
    m_editCtrl = m_GI->m_editCtrl;

    // TD call the datasource first..
    if(m_GI->m_defDataSource->OnEditStart(m_editCol,m_editRow,&m_editCtrl) != FALSE){
        if(OnEditStart(m_editCol,m_editRow,&m_editCtrl) != FALSE){

            m_editInProgress = TRUE;

            m_editCtrl->SetWindowText(m_editCell.GetText());

            if(m_editCell.IsPropertySet(UGCELL_FONT_SET))
                m_editCtrl->SetFont(m_editCell.GetFont());
            else if(m_GI->m_defFont != NULL)
                m_editCtrl->SetFont(m_GI->m_defFont);

            m_editCtrl->SetParent(m_editParent);

            m_editCtrl->SetWindowPos(&wndTop,rect.left,rect.top,
                rect.right-rect.left,rect.bottom-rect.top,0);

            m_editCtrl->ShowWindow(SW_SHOW);

            m_editCtrl->SetActiveWindow();
            m_editCtrl->SetFocus();

            //stuff a key stroke in
            if(key >0){
                m_editCtrl->SendMessage(WM_CHAR,key,0);
            }

            return UG_SUCCESS;
        }
    }

    m_editInProgress = FALSE;
    return 1;
}

/***************************************************
ContinueEdit

    If already in edit mode this function moves the
    editing to another cell using the given adjustments
    if the specified cell is not readonly and the cell
    type of the specified cell allows editing
    An OnStartEdit notification is also sent that
    can allow or dis-allow the editing

  return
    1           editing not allowed
    UG_SUCCESS  success
****************************************************/
int CUGCtrl::ContinueEdit(int adjustCol,long adjustRow){
    return UG_SUCCESS;
}

/***************************************************
****************************************************
    GET/SET FUNTIONS    GET/SET FUNCTIONS
****************************************************
****************************************************/

/***************************************************
GetCell
    Fills the given cell class with the information
    from the cell at the given position

  return
        UG_SUCCESS  success
        1           cell not found
****************************************************/
int CUGCtrl::GetCell(int col,long row,CUGCell * cell){

    if(col >= m_GI->m_numberCols)
        return UG_ERROR;

    CUGDataSource * ds = NULL;

    cell->SetPropertyFlags(0);

    //column translation only is performed on cols >= 0
    if(col >=0 && col < m_GI->m_numberCols)
        ds = m_GI->m_colInfo[col].dataSource;
    if(ds == NULL)
        ds = m_GI->m_defDataSource;

    //get the cell from the datasource
    if(col >=0 && col < m_GI->m_numberCols)
        return ds->GetCell(m_GI->m_colInfo[col].colTranslation,row,cell);
    else
        return ds->GetCell(col,row,cell);
}
/***************************************************
GetCellIndirect
    Fills the given cell class with the information
    from the cell at the given position, this function
    starts by filling the cell with the default
    informaiton for the specified column, then it
    over-writes any information that is 'set' in the
    specified cell.

  return
        UG_SUCCESS  success
        1           cell not found
****************************************************/
int CUGCtrl::GetCellIndirect(int col,long row,CUGCell *cell){

    cell->SetPropertyFlags(0);
    //get the column default
    if(col >= 0 && row >= 0 && col < m_GI->m_numberCols){
        if(m_GI->m_colInfo[col].colDefault != NULL)
            cell->CopyInfoFrom(m_GI->m_colInfo[col].colDefault);
    }
    else{
        cell->CopyInfoFrom(m_GI->m_hdgDefaults);
    }

    if(row >= m_GI->m_numberRows)
        return UG_SUCCESS;

    //find the datasource
    CUGDataSource * ds = NULL;
    if(col >=0 && col < m_GI->m_numberCols)
        ds = m_GI->m_colInfo[col].dataSource;
    if(ds == NULL)
        ds = m_GI->m_defDataSource;

    //get the cell from the datasource
    if(col >=0 && col < m_GI->m_numberCols){
        ds->GetCell(m_GI->m_colInfo[col].colTranslation,row,cell);
        OnGetCell(m_GI->m_colInfo[col].colTranslation,row,cell);
    }
    else{
        ds->GetCell(col,row,cell);
        OnGetCell(col,row,cell);
    }

    //use format class
    if(cell->IsPropertySet(UGCELL_FORMAT_SET)){
        cell->GetFormatClass()->ApplyDisplayFormat(cell);
    }

    //use cell style object
    if(cell->IsPropertySet(UGCELL_STYLE_SET)){
        cell->GetCellStyle()->AddInfoTo(cell);
    }


    return UG_SUCCESS;
}
/***************************************************
SetCell
    Copies the given cell to the cell at the specified
    position. The information that is saved depends
    on the capabilities of the data source.
    (ex. the memory manager saves all the information,
     a basic database source will only save the text).

  return
    UG_SUCCESS  success
    1           cell not found
****************************************************/
int CUGCtrl::SetCell(int col,long row,CUGCell *cell){

    OnSetCell(col,row,cell);

    //find the datasource
    CUGDataSource * ds = NULL;
    if(col >=0)
        ds = m_GI->m_colInfo[col].dataSource;
    if(ds == NULL)
        ds = m_GI->m_defDataSource;

    //set the cell in the datasource
    //get the cell from the datasource
    if(col >=0)
        ds->SetCell(m_GI->m_colInfo[col].colTranslation,row,cell);
    else
        ds->SetCell(col,row,cell);

    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGCtrl::GetColTranslation(int col){

    if(col <0 || col >= m_GI->m_numberCols)
        return col;

    return m_GI->m_colInfo[col].colTranslation;
}
/***************************************************
****************************************************/
int CUGCtrl::SetColTranslation(int col,int transCol){

    if(col <0 || col >= m_GI->m_numberCols)
        return 1;

    m_GI->m_colInfo[col].colTranslation = transCol;


    return UG_SUCCESS;
}

/***************************************************
DeleteCell
    Deletes the specified cell
  Return
    UG_SUCCESS  success
    1           cell no found
****************************************************/
int CUGCtrl::DeleteCell(int col,long row){

    //find the datasource
    CUGDataSource * ds = NULL;
    if(col >=0)
        ds = m_GI->m_colInfo[col].dataSource;
    if(ds == NULL)
        ds = m_GI->m_defDataSource;

    return ds->DeleteCell(m_GI->m_colInfo[col].colTranslation,row);

}

/***************************************************
SetColDefault
    Sets the column default values. These values are
    used when the grid draws a cell, the defaults are
    retrieved first then only the informtion that
    the cell being drawn contains is copied overtop
    of the defauls

    return
        UG_SUCCESS success
        1           fail
****************************************************/
int CUGCtrl::SetColDefault(int col,CUGCell *cell){

    if(col <0 || col >= m_GI->m_numberCols)
        return UG_ERROR;

    cell->CopyInfoTo(m_GI->m_colInfo[col].colDefault);

    return UG_SUCCESS;
}

/***************************************************
SetGridDefault
    Sets the grid default values. These values are
    used when new columns are made (SetNumberCols)
    they are copied to the new column defaults
    (see above)

    return
        UG_SUCCESS success
        1           fail
****************************************************/
int CUGCtrl::SetGridDefault(CUGCell *cell){

    cell->CopyInfoTo(m_GI->m_gridDefaults);

    return UG_SUCCESS;
}

/***************************************************
GetColDefault
    copies the given column defaults into the supplied
    cell

  return
        UG_SUCCESS  success
        1           fail
****************************************************/
int CUGCtrl::GetColDefault(int col,CUGCell *cell){

    if(col <0 || col >= m_GI->m_numberCols)
        return UG_ERROR;

    cell->CopyInfoFrom(m_GI->m_colInfo[col].colDefault);

    return UG_SUCCESS;
}
/***************************************************
GetGridDefault
    copies the grid defaults into the supplied
    cell

  return
        UG_SUCCESS  success
        1           fail
****************************************************/
int CUGCtrl::GetGridDefault(CUGCell *cell){
    cell->CopyInfoFrom(m_GI->m_gridDefaults);
    return UG_SUCCESS;
}

/***************************************************
SetHeadingDefault
    copies the given cell properties to the heading
    defaults. Heading defaults are used when a
    top and side headings are being drawn ( as well
    as the top corner button). These values are applied
    first, then any properties set for the heading cell
    being drawn are applied overtop

  return
        UG_SUCCESS  success
        1           fail
****************************************************/
int CUGCtrl::SetHeadingDefault(CUGCell *cell){
    m_GI->m_hdgDefaults->CopyInfoFrom(cell);
    return UG_SUCCESS;
}

/***************************************************
GetHeadingDefault
    Fills the given cell with the heading default
    properties

  return
        UG_SUCCESS  success
        1           fail
****************************************************/
int CUGCtrl::GetHeadingDefault(CUGCell *cell){
    cell->CopyInfoFrom(m_GI->m_hdgDefaults);
    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGCtrl::JoinCells(int startCol,long startRow,int endCol,long endRow){

    if(startCol > endCol)
        return 1;
    if(startRow > endRow)
        return 1;
    if(endCol >= m_GI->m_numberCols)
        return 1;
    if(endRow >= m_GI->m_numberRows)
        return 1;
    if(startCol < (m_GI->m_numberSideHdgCols *-1))
        return 1;
    if(startRow < (m_GI->m_numberTopHdgRows *-1))
        return 1;

    int x;
    long y;
    CUGCell cell;

    for(y=startRow;y<=endRow;y++){
        for(x = startCol;x<=endCol;x++){
            GetCell(x,y,&cell);
            cell.SetJoinInfo(FALSE,startCol-x,startRow-y);
            SetCell(x,y,&cell);
        }
    }

    GetCell(startCol,startRow,&cell);
    cell.SetJoinInfo(TRUE,endCol - startCol,endRow - startRow);
    SetCell(startCol,startRow,&cell);

    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGCtrl::UnJoinCells(int col,long row){

    int endCol;
    long endRow;
    CUGCell cell;

    if(GetJoinRange(&col,&row,&endCol,&endRow) == UG_SUCCESS){
        for(long y = row; y <= endRow; y++){
            for(int x = col; x <= endCol; x++){
                 GetCell(x,y,&cell);
                 cell.ClearProperty(UGCELL_JOIN_SET);
                 SetCell(x,y,&cell);
            }
        }
    }
    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGCtrl::QuickGetText(int col,long row,CString *string){

    ASSERT(string);

    GetCell(col,row,&m_cell);
    *string = m_cell.GetText();

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
LPCTSTR CUGCtrl::QuickGetText(int col,long row){

    GetCell(col,row,&m_cell);
    return m_cell.GetText();
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetText(int col,long row,LPCTSTR string){

    ASSERT(string);

    GetCell(col,row,&m_cell);
    m_cell.SetText(string);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetNumber(int col,long row,double number){

    GetCell(col,row,&m_cell);
    m_cell.SetNumber(number);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetMask(int col,long row,LPCTSTR string){

    GetCell(col,row,&m_cell);
    m_cell.SetMask(string);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetLabelText(int col,long row,LPCTSTR string){

    GetCell(col,row,&m_cell);
    m_cell.SetLabelText(string);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGCtrl::QuickSetTextColor(int col,long row,COLORREF color){

    GetCell(col,row,&m_cell);
    m_cell.SetTextColor(color);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetHTextColor(int col,long row,COLORREF color){

    GetCell(col,row,&m_cell);
    m_cell.SetHTextColor(color);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGCtrl::QuickSetBackColor(int col,long row,COLORREF color){

    GetCell(col,row,&m_cell);
    m_cell.SetBackColor(color);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetHBackColor(int col,long row,COLORREF color){

    GetCell(col,row,&m_cell);
    m_cell.SetHBackColor(color);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetBitmap(int col,long row,CBitmap * bitmap){

    GetCell(col,row,&m_cell);
    m_cell.SetBitmap(bitmap);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetBitmap(int col,long row,int index){
    return QuickSetBitmap(col,row,GetBitmap(index));
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetFont(int col,long row,CFont * font){

    ASSERT(font);

    GetCell(col,row,&m_cell);
    m_cell.SetFont(font);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetFont(int col,long row,int index){

    return QuickSetFont(col,row,GetFont(index));
}

/***************************************************
****************************************************/
int CUGCtrl::QuickSetAlignment(int col,long row,int align){

    GetCell(col,row,&m_cell);
    m_cell.SetAlignment((short)align);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGCtrl::QuickSetBorder(int col,long row,int border){

    GetCell(col,row,&m_cell);
    m_cell.SetBorder((short)border);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetBorderColor(int col,long row,CPen *pen){

    GetCell(col,row,&m_cell);
    m_cell.SetBorderColor(pen);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::QuickSetCellType(int col,long row,long type){

    GetCell(col,row,&m_cell);
    m_cell.SetCellType(type);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}

/***************************************************
****************************************************/
int CUGCtrl::QuickSetCellTypeEx(int col,long row,long typeEx){

    GetCell(col,row,&m_cell);
    m_cell.SetCellTypeEx(typeEx);
    SetCell(col,row,&m_cell);
    return UG_SUCCESS;
}
/***************************************************
QuickSetRange
    Sets a range of cells to use the same attributes as
    the given cell
Return
    UG_SUCCESS - success
    otherwise the number of SetCell errors are returned
    this may be caused if part or all of the range is
    outside of the grids total column/row range
****************************************************/
int CUGCtrl::QuickSetRange(int startCol,long startRow,int endCol,long endRow,
                           CUGCell *cell){

    int error = UG_SUCCESS;

    for(long row = startRow; row <= endRow;row++){
        for(int col = startCol;col <= endCol;col++){
            if(SetCell(col,row,cell) != UG_SUCCESS)
                error++;
        }
    }
    return error;
}


/***************************************************
SetNumberRow
    Sets the number of rows that are displayed in the
    grid. Up to 64,000 rows are allowed with non-uniform
    row height. Up to 2 billion rows are allowed with
    uniform row height. If non-uniform row height is set
    and the number of row exced 65,000 then the grid
    automatically goes to uniform row height

  return
        UG_SUCCESS  success
        1           fail
****************************************************/
int CUGCtrl::SetNumberRows(long rows,BOOL redraw){

    //range checking
    if(rows <0)
        return 1;

    if(rows <= m_GI->m_numLockRows && rows > 0)
        LockRows(rows-1);

    //store the old number of rows
    long oldNumRows = m_GI->m_numberRows;

    //check to see if uniform row height should be used
    if(rows > 640000/*64000*/ && m_GI->m_uniformRowHeightFlag == FALSE) // GHM 20100215 see no need for the 64000 limitation
        SetUniformRowHeight(TRUE);

    //store the number of rows and create the new row height array
    m_GI->m_numberRows = rows;
    if(m_GI->m_uniformRowHeightFlag == FALSE){
        //create the array
        int *tempRowHeights = new int[rows];

        //copy the old row heights over
        for(long loop = 0;loop < rows ; loop++){
            if(loop < oldNumRows)
                tempRowHeights[loop] = m_GI->m_rowHeights[loop];
            else
                tempRowHeights[loop] = m_GI->m_defRowHeight;
        }
        //delete the old row height array
        if(m_GI->m_rowHeights != NULL)
            delete[] m_GI->m_rowHeights;
        //use the new array
        m_GI->m_rowHeights = tempRowHeights;
    }

    //make sure that the current grid positions are within range
    int oldTop = m_GI->m_topRow;
    CalcTopRow();
    if(oldTop != m_GI->m_topRow)
        Moved();

    if(m_GI->m_currentRow >= rows){
        m_GI->m_currentRow =  rows -1;
        m_GI->m_lastRow = m_GI->m_currentRow;
        m_GI->m_dragRow = m_GI->m_currentRow;
    }
    if(m_GI->m_dragRow >= rows){
        m_GI->m_dragRow = rows - 1;
    }


    SetLockRowHeight();

    if(m_GI->m_currentRow < 0 && rows > 0)
        GotoRow(0);

    if(redraw){
        RedrawAll();
    }

    return UG_SUCCESS;
}


/***************************************************
GetNumberRow
    Returns the number of rows
****************************************************/
long CUGCtrl::GetNumberRows(){

    return m_GI->m_numberRows;
}
/***************************************************
SetNumberCols
    Sets the number of columns (0 - 32,000)

  return
        UG_SUCCESS  success
        1           fail
****************************************************/
int CUGCtrl::SetNumberCols(int cols,BOOL redraw){

    //range checking
    if(cols <0 || cols > 32000)
        return 1;

    if(cols <= m_GI->m_numLockCols && cols > 0)
        LockColumns(cols-1);

    //store the old number of cols
    int oldNumCols = m_GI->m_numberCols;

    //store the number of cols and create the new col width array
    m_GI->m_numberCols = cols;

    //create the array
    UGCOLINFO * tempColWidths   = new UGCOLINFO[cols];

    //copy the col information over
    for(int loop = 0;loop < cols ; loop++){
        if(loop < oldNumCols){
            tempColWidths[loop].width = m_GI->m_colInfo[loop].width;
            tempColWidths[loop].dataSource = m_GI->m_colInfo[loop].dataSource;
            tempColWidths[loop].colDefault = m_GI->m_colInfo[loop].colDefault;
            tempColWidths[loop].colTranslation = m_GI->m_colInfo[loop].colTranslation;
        }
        else{
            tempColWidths[loop].width = m_GI->m_defColWidth;
            tempColWidths[loop].dataSource = m_GI->m_defDataSource;
            tempColWidths[loop].colDefault = new CUGCell;
            tempColWidths[loop].colDefault->CopyInfoFrom(m_GI->m_gridDefaults);
            tempColWidths[loop].colTranslation = loop;
        }
    }

    //delete the old information
    for(int loop = cols;loop <oldNumCols;loop++){
        delete m_GI->m_colInfo[loop].colDefault;
    }
    if(m_GI->m_colInfo != NULL)
    //delete the old col width array
        delete[] m_GI->m_colInfo;
    m_GI->m_colInfo = tempColWidths;

    //make sure that the current grid positions are within range
    CalcLeftCol();
    if(m_GI->m_currentCol >= cols){
        m_GI->m_currentCol =  cols -1;
        m_GI->m_dragCol = m_GI->m_currentCol;
    }

    SetLockColWidth();

    if(m_GI->m_currentCol < 0 && cols > 0)
        GotoCol(0);

    if(redraw)
        RedrawAll();

    return UG_SUCCESS;
}
/***************************************************
GetNumberCols
    Returns the number of columns that are currently
    set
****************************************************/
int CUGCtrl::GetNumberCols(){
    return m_GI->m_numberCols;
}
/***************************************************
SetColWidth
    Sets the width of the given column in pixels

    return
        UG_SUCCESS  success
        1           specifed col is out of range
                    or a negative width
***************************************************/
int CUGCtrl::SetColWidth(int col,int width){

    if(col <0 || col >= m_GI->m_numberCols){
        if(col <0 || col >=  -m_GI->m_numberSideHdgCols){
            SetSH_ColWidth(col,width);
        }
        return 1;
    }

    m_GI->m_colInfo[col].width = width;

    //check to see if a column within the lock is
    //being changed, then set the lock width
    if(col < m_GI->m_numLockCols){
        SetLockColWidth();
    }

    return UG_SUCCESS;
}
/***************************************************
GetColWidth
    Retrieves the width of the specied column.

    return
        UG_SUCCESS  success
        1           specifed col is out of range
***************************************************/
int CUGCtrl::GetColWidth(int col,int *width){

    if(col <0 || col >= m_GI->m_numberCols)
        return 1;

    *width =  m_GI->m_colInfo[col].width;

    return UG_SUCCESS;
}
/***************************************************
GetColWidth
***************************************************/
int CUGCtrl::GetColWidth(int col){

    if(col <0 || col >= m_GI->m_numberCols)
        return 1;

    return m_GI->m_colInfo[col].width;
}

/***************************************************
SetDefColWidth
    Sets the default column width. This value is used
    when new columns are created.

  return
    UG_SUCCESS  success
    1           fail
***************************************************/
int CUGCtrl::SetDefColWidth(int width){

    if(width <0)
        return 1;

    m_GI->m_defColWidth = width;

    return UG_SUCCESS;
}
/***************************************************
BestFit
    Purpose
        Adjusts the widths of the secified columns
        so that they are the 'best fit' for the information
        being displayed
    Params
        startCol -  the start column in a range of cols
                    to be 'best fitted'
        endCol -    the end column in the range of cols
        calcRange - the number of rows to base the calculation
                    on. use 0 for all rows
        flag -      options for the best fit
                    bit1: include top headings (UG_BESTFIT_TOPHEADINGS)
                    bit2: use average width (UG_BESTFIT_AVERAGE)
    Return
****************************************************/
int CUGCtrl::BestFit(int startCol,int endCol,int CalcRange,int flag){

    if(startCol < (m_GI->m_numberSideHdgCols *-1))
        return 1;
    if(endCol >= m_GI->m_numberCols)
        return 2;
    if(startCol > endCol)
        return 3;

    CUGCell cell;
    CUGCellType * cellType;
    CSize size;
    int count       = 0;
    int x;
    long y;
    int numCols     = endCol-startCol+1;
    int *bestWidth  = new int[numCols];
    CDC * dc        = GetDC();

    int startRow = 0;
    if(flag&UG_BESTFIT_TOPHEADINGS)
        startRow -= m_GI->m_numberTopHdgRows;

    //set the best width to 3 pixels (3 for min width)
    for(x= 0;x < numCols;x++)
        bestWidth[x] = 3;

    for(y = startRow;y <m_GI->m_numberRows;y++){

        if(y == m_GI->m_numLockRows)
            y = m_GI->m_topRow;

        for(x= startCol;x <= endCol;x++){

            GetCellIndirect(x,y,&cell);

            if(cell.IsPropertySet(UGCELL_JOIN_SET) == FALSE){
                cellType = GetCellType(cell.GetCellType());
                cellType->GetBestSize(dc,&size,&cell);
                if(size.cx > bestWidth[x-startCol])
                    bestWidth[x-startCol] = size.cx;
            }
        }

        count++;
        if(count == CalcRange)
            break;
    }

    //update the col widths - restrict to width of grid window
    int bw;
    RECT rect;
    m_CUGGrid->GetClientRect(&rect);
    int maxw = rect.right - m_GI->m_vScrollWidth;

    for(x= 0;x < numCols;x++){
        bw = bestWidth[x];

        if(bw > maxw)
            bw = maxw;
        SetColWidth(startCol+x,bw);
    }

    delete[] bestWidth;

    ReleaseDC(dc);

    AdjustComponentSizes();

    return UG_SUCCESS;
}
/***************************************************
FitToWindow
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::FitToWindow(int startCol,int endCol){

    RECT rect;
    int screenwidth;
    int columnwidths,width;
    int loop;
    float ratio;
    int newwidth;

    //validation checking
    if(startCol < 0 || endCol < startCol || endCol >= m_GI->m_numberCols)
        return 1;

    //get the grid screen width
    m_CUGGrid->GetClientRect(&rect);
    screenwidth = rect.right;

    //get the total column widths
    columnwidths = 0;
    for(loop = startCol;loop <=endCol;loop++){
        GetColWidth(loop,&width);
        columnwidths += width;
    }

    //get the ratio
    ratio = ((float)screenwidth) / ((float)columnwidths);

    //reset the column widths
    for(loop = startCol;loop <endCol;loop++){
        GetColWidth(loop,&width);
        newwidth = (int)((float)width*ratio);
        SetColWidth(loop,newwidth);
        screenwidth -= newwidth;
    }
    SetColWidth(endCol,screenwidth);

    Update();
    return UG_SUCCESS;
}
/***************************************************
SetDefRowHeight
    Sets the default row height. This value is used
    for all rows when uniform row height is set.
    For non-uniform row height, this value is used
    when a new row is created

  return
    UG_SUCCESS  success
    1           fail

***************************************************/
int CUGCtrl::SetDefRowHeight(int height){

    if(height <1 || height > 1024)
        return UG_ERROR;

    m_GI->m_defRowHeight = height;

    return UG_SUCCESS;
}
/***************************************************
SetUniformRowHeight
    Sets the grid in uniform row height or no-uniform
    row height.
    'flag'  TRUE:  then uniform row height is set
               all rows then become the same height
               which is set by 'SetDefRowHeight'
               The height can still be adjusted by the
               used, but all rows are adjusted at the
               same time
    'flag' FALSE: then non-uniform row height is set
                if the grid have less than 64001 rows
                A maximum of 64000 rows are allowed
                with this mode.
                This allows each row to have its own
                height 'SetRowHeight'

  return
    UG_SUCCESS  success
    1           fail
***************************************************/
int CUGCtrl::SetUniformRowHeight(int flag){

    //uniform row height on
    if(flag){
        if(m_GI->m_rowHeights != NULL){
            delete[] m_GI->m_rowHeights;
            m_GI->m_rowHeights = NULL;
        }
        m_GI->m_uniformRowHeightFlag = TRUE;
    }
    //uniform row height off
    else{
        if(m_GI->m_rowHeights == NULL){
            if(m_GI->m_uniformRowHeightFlag && m_GI->m_numberRows >0){
                m_GI->m_rowHeights = new int[m_GI->m_numberRows];
                for(long row = 0;row <m_GI->m_numberRows;row++){
                    m_GI->m_rowHeights[row] =m_GI->m_defRowHeight;
                }
            }
        }
        m_GI->m_uniformRowHeightFlag = FALSE;
    }

    return UG_SUCCESS;
}
/***************************************************
SetRowHeight
    Sets the height of the specified row. If uniform
    row height is specied then the default row height
    is set

  return
    UG_SUCCESS  success
    1           fail  - out of range
***************************************************/
int CUGCtrl::SetRowHeight(long row,int height)
{
//  if(row < 0 || row >= m_GI->m_numberRows)
//      return 0;
//
// smg: two lines above are orig; next if blk is mine;
// basically, if i passed in a negative row, it wouldn't
// change the top hdg height! (whereas SetColWidth would
// adjust the side hdg width if negative)

    if(row <0 || row >= m_GI->m_numberRows)
    {
        if(row <0 || row >=  -m_GI->m_numberTopHdgRows)
        {
            SetTH_RowHeight (row,height);
        }
        return 1;
    }

    if(m_GI->m_uniformRowHeightFlag)
        m_GI->m_defRowHeight = height;
    else
        m_GI->m_rowHeights[row] = height;

//  SetLockRowHeight();

    if (row < m_GI->m_numLockRows)  // smg: i think this tst nds to be made...
    {
        SetLockRowHeight();
    }

    return UG_SUCCESS;
}
/***************************************************
GetRowHeight
    Retrives the height of the specified row.
    If uniform row height is specified then the
    default row height is returned

  return
    UG_SUCCESS  success
    1           fail - out of range
***************************************************/
int CUGCtrl::GetRowHeight(long row,int *height){
    if(row < 0 || row >= m_GI->m_numberRows)
        return 1;
    *height = GetRowHeight(row);
    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::GetRowHeight(long row){

    if(row < 0 || row >= m_GI->m_numberRows)
        return 0;

    if(m_GI->m_uniformRowHeightFlag)
        return m_GI->m_defRowHeight;

    return m_GI->m_rowHeights[row];

}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::InsertCol(int col){

    //find the datasource
    CUGDataSource * ds = NULL;
    if(col >=0)
        ds = m_GI->m_colInfo[col].dataSource;
    if(ds == NULL)
        ds = m_GI->m_defDataSource;

    int rt = ds->InsertCol(m_GI->m_colInfo[col].colTranslation);

    if(rt == UG_SUCCESS){

        SetNumberCols(GetNumberCols()+1);

        //adjust the column information structure
        int numCols = GetNumberCols() -1;
        UGCOLINFO temp;
        temp.width          = m_GI->m_colInfo[numCols].width;
        temp.dataSource     = m_GI->m_colInfo[numCols].dataSource;
        temp.colDefault     = m_GI->m_colInfo[numCols].colDefault;
        temp.colTranslation = m_GI->m_colInfo[numCols].colTranslation;

        for(int loop = numCols ; loop > col; loop--){
            m_GI->m_colInfo[loop].width         = m_GI->m_colInfo[loop-1].width;
            m_GI->m_colInfo[loop].dataSource    = m_GI->m_colInfo[loop-1].dataSource;
            m_GI->m_colInfo[loop].colDefault    = m_GI->m_colInfo[loop-1].colDefault;
            m_GI->m_colInfo[loop].colTranslation= m_GI->m_colInfo[loop-1].colTranslation;
        }
        m_GI->m_colInfo[col].width          = temp.width;
        m_GI->m_colInfo[col].dataSource     = temp.dataSource;
        m_GI->m_colInfo[col].colDefault     = temp.colDefault;
        m_GI->m_colInfo[col].colTranslation = col;
    }

    return rt;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::AppendCol(){

    //find the datasource
    CUGDataSource * ds = NULL;
    ds = m_GI->m_defDataSource;

    int rt = ds->AppendCol();

    if(rt == UG_SUCCESS)
        SetNumberCols(GetNumberCols()+1);

    return rt;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::DeleteCol(int col){

    if(GetNumberCols() <= 0)
        return UG_ERROR;

    //find the datasource
    CUGDataSource * ds = NULL;
    if(col >=0)
        ds = m_GI->m_colInfo[col].dataSource;
    if(ds == NULL)
        ds = m_GI->m_defDataSource;

    int rt = ds->DeleteCol(m_GI->m_colInfo[col].colTranslation);

    if(rt == UG_SUCCESS){

        //adjust the column information structures
        int numCols = GetNumberCols()-1;
        if(numCols > 0){
            UGCOLINFO temp;
            temp.colDefault     = m_GI->m_colInfo[col].colDefault;
            for(int loop = col ; loop < numCols;loop++){
                m_GI->m_colInfo[loop].width         = m_GI->m_colInfo[loop+1].width;
                m_GI->m_colInfo[loop].dataSource    = m_GI->m_colInfo[loop+1].dataSource;
                m_GI->m_colInfo[loop].colDefault    = m_GI->m_colInfo[loop+1].colDefault;
                m_GI->m_colInfo[loop].colTranslation= m_GI->m_colInfo[loop+1].colTranslation;
            }
            m_GI->m_colInfo[numCols].colDefault = temp.colDefault;
        }

        //reset the number of cols
        SetNumberCols(GetNumberCols()-1);
    }

    return rt;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::InsertRow(long row){

    if(row > m_GI->m_numberRows)
        return UG_ERROR;

    //find the datasource
    CUGDataSource * ds = NULL;
    ds = m_GI->m_defDataSource;

    int rt = ds->InsertRow(row);
    if(rt == UG_SUCCESS){
        SetNumberRows(m_GI->m_numberRows + 1);
    }
    return rt;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::AppendRow(){

    int rt = m_GI->m_defDataSource->AppendRow();
    if(rt == UG_SUCCESS){
        SetNumberRows(m_GI->m_numberRows + 1);
    }
    return rt;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::DeleteRow(long row){

    if(row < 0 || row >= m_GI->m_numberRows)
        return UG_ERROR;

    int rt = m_GI->m_defDataSource->DeleteRow(row);
    if(rt == UG_SUCCESS){
        SetNumberRows(m_GI->m_numberRows - 1);
    }
    return rt;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::FindDialog(){

    if(m_findDialogRunning != FALSE)
        return 1;

    m_findDialogRunning = TRUE;
    m_findDialogStarted = TRUE;

#ifdef UG_ENABLE_FINDDIALOG
    m_findReplaceDialog = new CFindReplaceDialog;
    m_findReplaceDialog->Create(TRUE,_T(""),NULL,FR_DOWN,this);
    ASSERT(m_findReplaceDialog->m_hWnd);
#endif

    CString * strPair = new CString[2];
    strPair[0] = _T("&whole word");
    strPair[1] = _T("Match &entire cell text");

#ifdef UG_ENABLE_FINDDIALOG
    EnumChildWindows(m_findReplaceDialog->m_hWnd, ModifyDlgItemText, (LPARAM)strPair);
#endif

    delete [] strPair;


    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::ReplaceDialog(){

    if(m_findDialogRunning != FALSE)
        return 1;

    m_findDialogRunning = TRUE;
    m_findDialogStarted = TRUE;

#ifdef UG_ENABLE_FINDDIALOG
    m_findReplaceDialog = new CFindReplaceDialog;
    m_findReplaceDialog->Create(FALSE,_T(""),NULL,FR_DOWN,this);
#endif

    CString * strPair = new CString[2];
    strPair[0] = _T("&whole word");
    strPair[1] = _T("Match &entire cell text");

#ifdef UG_ENABLE_FINDDIALOG
    EnumChildWindows(m_findReplaceDialog->m_hWnd, ModifyDlgItemText, (LPARAM)strPair);
#endif

    delete [] strPair;

    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::FindInAllCols(BOOL state){
    if(state)
        m_findInAllCols = TRUE;
    else
        m_findInAllCols = FALSE;

    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
long CUGCtrl::ProcessFindDialog(UINT,long){
#ifdef UG_ENABLE_FINDDIALOG
    if(m_findReplaceDialog == NULL)
        return 0;
    if(m_findReplaceDialog->GetSafeHwnd() == NULL)
        return 0;
    if(m_findReplaceDialog->IsTerminating()){
        m_findDialogStarted = FALSE;
        m_findDialogRunning = FALSE;
        return 1;
    }

    int col = m_GI->m_currentCol;
    long row = m_GI->m_currentRow;

    CString string = m_findReplaceDialog->GetFindString();
    CString string2 = m_findReplaceDialog->GetReplaceString();
    int findFlags = 0;
    if(!m_findReplaceDialog->MatchCase())
        findFlags |= UG_FIND_CASEINSENSITIVE;
    if(!m_findReplaceDialog->MatchWholeWord())
        findFlags |= UG_FIND_PARTIAL;
    if(!m_findReplaceDialog->SearchDown())
        findFlags |= UG_FIND_UP;
    if(m_findInAllCols)
        findFlags |= UG_FIND_ALLCOLUMNS;

    m_findDialogStarted = FALSE;

    if( m_findReplaceDialog->ReplaceCurrent()){
        QuickSetText(m_GI->m_currentCol,row,string2);
        RedrawCell(m_GI->m_currentCol,row);
    }
    if( m_findReplaceDialog->ReplaceAll()){
        if(string2 == string)
            return 0;
        if(findFlags&UG_FIND_PARTIAL) {
            if(string2.Find(string) != -1)
                return 0;
        }
        while(FindNext(&string,&col,&row,findFlags) ==UG_SUCCESS) {
            GotoCell(col,row);
            QuickSetText(m_GI->m_currentCol,row,string2);
            RedrawCell(col,row);
        }
    }

    if(FindNext(&string,&col,&row,findFlags) !=UG_SUCCESS){
        MessageBox(_T("Item not found"),_T("Not Found"),MB_OK);
        return 0;
    }
    else
        GotoCell(col,row);


    GotoCell(col,row);

    return 0;
#else //!UG_ENABLE_FINDDIALOG
    return 0;
#endif
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::FindFirst(CString *string,int *col,long *row,long flags){

    *col = m_GI->m_colInfo[*col].colTranslation;
    return m_GI->m_defDataSource->FindFirst(string,
        col,row,flags);
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::FindNext(CString *string,int *col,long *row,int flags){
    *col = m_GI->m_colInfo[*col].colTranslation;

    return m_GI->m_defDataSource->FindNext(string,
        col,row,flags);
}

/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::SortBy(int col,int flag){

    int cols[1];
    cols[0] = col;
    return SortBy(cols,1,flag);

}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::SortBy(int *cols,int num,int flag){

    //perform data column translation
    for(int loop = 0;loop < num;loop++){
        cols[loop] = m_GI->m_colInfo[cols[loop]].colTranslation;
    }

    return m_GI->m_defDataSource->SortBy(cols,num,flag);
}

/***************************************************
SetTH_Height
    Sets the total height of the top heading
    A value of 0 hides the top heading

  return
    UG_SUCCESS  success
    1           fail - out of range
***************************************************/
int CUGCtrl::SetTH_Height(int height){

    if(height <0 || height >1024)
        return 1;

    m_GI->m_topHdgHeight = height;

    //adjust the height for each top heading row
    int totalHeight = 0;
    int loop;
    double adjust;
    //find the total old height
    for(loop = 0;loop < m_GI->m_numberTopHdgRows;loop++){
        totalHeight += m_GI->m_topHdgHeights[loop];
    }
    //find the adjustment value
    adjust = (double)height / (double)totalHeight;
    //adjust each row height
    for(loop = 0;loop < m_GI->m_numberTopHdgRows;loop++){
        m_GI->m_topHdgHeights[loop] = (int)(m_GI->m_topHdgHeights[loop] * adjust +0.5);
    }

    AdjustComponentSizes();

    return UG_SUCCESS;
}

/***************************************************
GetTH_Height
    Returns the height of the top heading
***************************************************/
int CUGCtrl::GetTH_Height(){
    return m_GI->m_topHdgHeight;
}
/***************************************************
SetSH_Width
    Sets the total widht of the side heading
    A value of 0 hides the side heading

  return
    UG_SUCCESS  success
    1           fail - out of range
***************************************************/
int CUGCtrl::SetSH_Width(int width){

    if(width <0 || width >1024)
        return 1;

    m_GI->m_sideHdgWidth = width;

    //adjust the width
    int totalWidth = 0;
    int loop;
    double adjust;
    //find the total old height
    for(loop = 0;loop < m_GI->m_numberSideHdgCols;loop++){
        totalWidth += m_GI->m_sideHdgWidths[loop];
    }
    //find the adjustment value
    adjust = (double)width / (double)totalWidth;
    //adjust each col width
    for(loop = 0;loop < m_GI->m_numberSideHdgCols;loop++){
        m_GI->m_sideHdgWidths[loop] = (int)(m_GI->m_sideHdgWidths[loop] * adjust +0.5);
    }

    AdjustComponentSizes();

    return UG_SUCCESS;
}
/***************************************************
GetSH_Width
    Returns the width of the side heading
***************************************************/
int CUGCtrl::GetSH_Width(){
    return m_GI->m_sideHdgWidth;
}
/***************************************************
SetVS_Width
    Sets the width of the vertical scroll bar

  return
    UG_SUCCESS  success
    1           fail - out of range
***************************************************/
int CUGCtrl::SetVS_Width(int width){

    if(width <0 || width > 1024)
        return 1;

    m_GI->m_vScrollWidth = width;

    AdjustComponentSizes();

    return UG_SUCCESS;
}
/***************************************************
GetVS_Width
    returns the width of the vertical scroll bar
***************************************************/
int CUGCtrl::GetVS_Width(){
    return m_GI->m_vScrollWidth;
}
/***************************************************
SetHVS_Height
    Sets the height of the horizontal scroll bar

  return
    UG_SUCCESS  success
    1           fail - out of range
***************************************************/
int CUGCtrl::SetHS_Height(int height){

    if(height <0 || height > 1024)
        return 1;

    m_GI->m_hScrollHeight = height;

    AdjustComponentSizes();

    return UG_SUCCESS;
}
/***************************************************
GetHS_Height
    returns the height of the horizontal scroll bar
***************************************************/
int CUGCtrl::GetHS_Height(){
    return m_GI->m_hScrollHeight;
}
/***************************************************
SetCurrentCellMode
    Sets the highlighting and focus mode of the
    current cell
    value of 1 by default

    'mode' if bit 0 ( value of 1) is set then
            the current cell gets a focus rectangle
           if bit 1 ( value of 2) is set then
            the current cell is highlighted
           (a value of three is for both)
***************************************************/
int CUGCtrl::SetCurrentCellMode(int mode){

    if(mode < 0 || mode > 3)
        return UG_ERROR;

    m_GI->m_currentCellMode = mode;

    return UG_SUCCESS;
}
/***************************************************
SetHightlightRow
    Set the grid into rowhighlighting mode.
    FALSE by default
    'mode'  TRUE: row highlighting mode - a whole row
            is highlighted at a time - useful for
            multi-column list boxes

            FALSE: standard cell hightlighting
***************************************************/
int CUGCtrl::SetHighlightRow(int mode){

    if(mode){
        m_GI->m_highlightRowFlag = TRUE;

        mode = m_GI->m_multiSelect->GetSelectMode();
        if( mode & UG_MULTISELECT_CELL )
            mode -= UG_MULTISELECT_CELL;
        mode |= UG_MULTISELECT_ROW;

        m_GI->m_multiSelect->SelectMode(mode);
    }
    else{
        m_GI->m_highlightRowFlag = FALSE;

        mode = m_GI->m_multiSelect->GetSelectMode();
        if( mode & UG_MULTISELECT_ROW )
            mode -= UG_MULTISELECT_ROW;
        mode |= UG_MULTISELECT_CELL;

        m_GI->m_multiSelect->SelectMode(mode);
    }

    return UG_SUCCESS;
}
/***************************************************
SetMultiSelectMode
    Turns multiple selection on and off
    off by default
    'mode'  TRUE or FALSE

***************************************************/
int CUGCtrl::SetMultiSelectMode(int mode){

    m_GI->m_multiSelectFlag = mode;

    //0: off  1:cell  2:row
    if(mode){
        if(m_GI->m_highlightRowFlag)
            return m_GI->m_multiSelect->SelectMode(2);
        else
            return m_GI->m_multiSelect->SelectMode(1);
    }
    return m_GI->m_multiSelect->SelectMode(mode);
}
/***************************************************
Set3DHeight
    Sets the height of the 3D border styles
    This value is used every time the border draws
    a rasied or recessed border
    a value of 1 is default

  return
    UG_SUCCESS  success
    1           fail - out of range

***************************************************/
int CUGCtrl::Set3DHeight(int height){

    if(height <1 || height > 16)
        return 1;

    m_GI->m_threeDHeight = height;

    return UG_SUCCESS;
}
/***************************************************
SetPaintMode
    Turns painting on/off.
    Useful for making many changes to the grid's
    properties, with paint mode off the grid will
    not try and update itself
***************************************************/
int CUGCtrl::SetPaintMode(int mode){

    if(mode){
        m_GI->m_paintMode = TRUE;
        RedrawAll();
    }
    else
        m_GI->m_paintMode = FALSE;

    return UG_SUCCESS;
}

/***************************************************
GetPaintMode
    Returns the current paint mode
***************************************************/
int CUGCtrl::GetPaintMode(){
    return m_GI->m_paintMode;
}

/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::SetVScrollMode(int mode){

    m_GI->m_vScrollMode = mode;
    m_CUGVScroll->Update();

    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
***************************************************/
int CUGCtrl::SetHScrollMode(int mode){

    m_GI->m_hScrollMode = mode;
    m_CUGHScroll->Update();

    return UG_SUCCESS;
}
/***************************************************
GetCurrentCol
    Returns the current column within the grid
***************************************************/
int CUGCtrl::GetCurrentCol(){

    return m_GI->m_currentCol;
}
/***************************************************
GetCurrentRow
    Returns the curent row within the grid
***************************************************/
long CUGCtrl::GetCurrentRow(){

    return m_GI->m_currentRow;
}
/***************************************************
GetLeftCol
    Returns the left visible column within the grid
***************************************************/
int CUGCtrl::GetLeftCol(){

    return m_GI->m_leftCol;
}
/***************************************************
GetRightCol
    Returns the right visible column within the grid
***************************************************/
int CUGCtrl::GetRightCol(){

    return m_GI->m_rightCol;

}
/***************************************************
GetTopRow
    Returns the top visible row within the grid
***************************************************/
long CUGCtrl::GetTopRow(){
    return m_GI->m_topRow;
}
/***************************************************
GetBottomRow
    Returns the bottom visible row within the grid
***************************************************/
long CUGCtrl::GetBottomRow(){

    return m_GI->m_bottomRow;
}

/***************************************************
****************************************************
    REDRAW FUNTIONS
****************************************************
****************************************************/

/***************************************************
RedrawAll
    Redraws all child windows
***************************************************/
int CUGCtrl::RedrawAll(){
    Update();
    return UG_SUCCESS;
}
/***************************************************
RedrawCell
    Redraws the specified cell within the grid
***************************************************/
int CUGCtrl::RedrawCell(int col,long row){
    if(col < 0){
        m_CUGSideHdg->Invalidate();
        m_CUGSideHdg->UpdateWindow();
    }
    else if(row < 0){
        m_CUGTopHdg->Invalidate();
        m_CUGTopHdg->UpdateWindow();
    }
    else{
        m_CUGGrid->RedrawCell(col,row);
    }
    return UG_SUCCESS;
}
/***************************************************
RedrawRow
    Redraws the specfied row within the grid
***************************************************/
int CUGCtrl::RedrawRow(long row){
    m_CUGGrid->RedrawRow(row);
    return UG_SUCCESS;
}
/***************************************************
RedrawCol
    Redraws the specified column within the grid
***************************************************/
int CUGCtrl::RedrawCol(int col){
    m_CUGGrid->RedrawCol(col);
    return UG_SUCCESS;
}
/***************************************************
RedrawRange
    Redraws the specified range of cells within the
    grid
***************************************************/
int CUGCtrl::RedrawRange(int startCol,long startRow,int endCol,long endRow){
    m_CUGGrid->RedrawRange(startCol,startRow,endCol,endRow);
    return UG_SUCCESS;
}

/***************************************************
****************************************************
    CELL TYPE
****************************************************
****************************************************/

/***************************************************
AddCellType
    Adds a custom cell type to the grid's cell type
    list. Once added a cell can use this type.

     Returns the ID number of the cell type that
    was added. This is the value to use when setting
    a cells 'celltype' property

    NOTE: The grid does not delete the cell type when
    the grid is destroyed.

    -1 error
***************************************************/
long CUGCtrl::AddCellType(CUGCellType *celltype){

    if(celltype == NULL)
        return -1;

    int index = m_cellTypeList->AddPointer(celltype);

    if(index  == -1)
        return -1;

    celltype->m_ctrl = this;
    celltype->SetID(index);

    return index;
}
/****************************************************
RemoveCellType
    Removes the specified cell type from the grid's
    cell type list. It does not delete the cell type
    class
****************************************************/
int CUGCtrl::RemoveCellType(int index){

    m_cellTypeList->DeletePointer(index);

    return UG_SUCCESS;
}

/***************************************************
****************************************************
    Multiple Selection
****************************************************
****************************************************/

/***************************************************
ClearSelections
    Clears any selections made in the grid
  return
    UG_SUCCESS  success
    1           fail - out of range
****************************************************/
int CUGCtrl::ClearSelections(){

    m_GI->m_multiSelect->ClearAll();

    m_GI->m_dragCol = m_GI->m_currentCol;
    m_GI->m_dragRow = m_GI->m_currentRow;

    return UG_SUCCESS;
}
/***************************************************
Select
    Selects a specified cell
    Multiple selection must be turned on for this
    function to work
  return
    UG_SUCCESS  success
    1           fail - out of range
****************************************************/
int CUGCtrl::Select(int col,long row){
    m_GI->m_multiSelect->StartBlock(col,row);
    m_GI->m_multiSelect->EndBlock(col,row);
    RedrawCell(col,row);
    return UG_SUCCESS;
}
/***************************************************
****************************************************/
BOOL CUGCtrl::IsSelected(int col,long row,int *blockNum){
    return m_GI->m_multiSelect->IsSelected(col,row,blockNum);
}

/***************************************************
SelectRange
    Selects a specified range of cells
    Multiple selection must be turned on for this
    function to work
  return
    UG_SUCCESS  success
    1           fail - out of range
****************************************************/
int CUGCtrl::SelectRange(int startCol,long startRow,int endCol,long endRow){
    if(m_GI->m_multiSelectFlag&3){
        m_GI->m_multiSelect->StartBlock(startCol,startRow);
        m_GI->m_multiSelect->EndBlock(endCol,endRow);
    }
    RedrawRange(startCol,startRow,endCol,endRow);
    return UG_SUCCESS;
}
/***************************************************
EnumFirstSelected
    Returns the co-ords of the first cell that
    has been selected. the current cell is also included
    in this enumeration
    ( if Multiple selection is off then the current cell
    is returned)

  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGCtrl::EnumFirstSelected(int *col,long *row){

    return m_GI->m_multiSelect->EnumFirstSelected(col,row);
}
/***************************************************
EnumNextSelected
    Returns the co-ords of the next cell that
    has been selected. the current cell is also included
    in this enumeration
    (Multiple selection mode must be set first)
  return
    UG_SUCCESS  success
    1           no more items selected
****************************************************/
int CUGCtrl::EnumNextSelected(int *col,long *row){

    return m_GI->m_multiSelect->EnumNextSelected(col,row);
}


// count the number of rows selected
long CUGCtrl::GetNumberRowsSelected()
{
    long number_rows = 0;

    std::optional<long> this_row_num;
    int col_num;
    long row_num;

    if( EnumFirstSelected(&col_num, &row_num) == UG_SUCCESS )
    {
        do
        {
            if( row_num != this_row_num )
            {
                ++number_rows;
                this_row_num = row_num;
            }

        } while( EnumNextSelected(&col_num, &row_num) == UG_SUCCESS );
    }

    return number_rows;
}


/***************************************************
CopySelected
    Copies the grid's currently selected items to the
    clipboard

  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGCtrl::CopySelected(){

    return CopySelected(FALSE);
}
/***************************************************
CopySelected
    Copies the grid's currently selected items to the
    clipboard
    'cutFlag' if true then the selected items are cut
        from the grid
  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGCtrl::CopySelected(int cutFlag){

    int rt;
    CString  clipString =_T("");

    //get the selected items in a TAB delimited string
    CreateSelectedString(clipString,cutFlag);

    //copy the items to the clipboard
    rt = CopyToClipBoard(&clipString);

    if(rt == UG_SUCCESS){
        ClearSelections();
        RedrawAll();
    }

    return rt;
}
/***************************************************
****************************************************/
void CUGCtrl::CreateSelectedString(CString& string,int cutFlag){

    int rt;
    int col;
    long row,lastrow;
    CUGCell cell;

    string = _T("");

    //enum selected items and add them to the string
    rt = m_GI->m_multiSelect->EnumFirstSelected(&col,&row);
    lastrow = row;
    while(rt == UG_SUCCESS){

        //get the selected cell then copy the string
        GetCellIndirect(col,row,&cell);
        string += cell.GetText();

        //check the cut flag
        if(cutFlag){
            cell.ClearAll();
            SetCell(col,row,&cell);
        }

        //update the last row flag
        lastrow = row;

        //find the next selected item
        rt = m_GI->m_multiSelect->EnumNextSelected(&col,&row);

        //add line feeds between rows
        if(rt == UG_SUCCESS){
            if(row != lastrow)
                string += _T("\n");
            //otherwise add tabs between cols
            else
                string += _T("\t");
        }
    }
}


/***************************************************
CutSelected
    Copies the grid's currently selected items to the
    clipboard then removes the contents of the selected
    cells
  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGCtrl::CutSelected(){

    return CopySelected(TRUE);
}
/***************************************************
CopyToClipBoard
    Copies the specified string to the clipboard
  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGCtrl::CopyToClipBoard(CString* string){

    HGLOBAL hg;          //memory handle
    LPTSTR data;          //memory pointer
    int size;           //size of clipboard data

    //open clipboard , copy , then close
    OpenClipboard();
    //empty the clipboard
    EmptyClipboard();

    //get the size of the Buffers string
    size = string->GetLength();

    //alloc enough memory for the string
    hg=GlobalAlloc(GMEM_ZEROINIT,size+1);

    //make sure there are no errors
    if(hg==NULL)
        return FALSE;

    //lock the memory
    data=(LPTSTR)GlobalLock(hg);
    lstrcpy(data,(LPCTSTR)*string);

    //unlock the memory
    GlobalUnlock(hg);
    //save the memory to the clipboard
    SetClipboardData(CF_TEXT,hg);

    //close the clipboard
    CloseClipboard();

   return UG_SUCCESS;
}

/***************************************************
CopyFromClipBoard
    Copies the contents from the clipboard to the
    specified string
  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGCtrl::CopyFromClipBoard(CString* string){

    HGLOBAL hg;          //memory handle
    LPTSTR data;          //memory pointer
    unsigned long size;  //size of clipboard data

    //open clipboard
    OpenClipboard();

    //get a pointer to the text
    hg=GetClipboardData(CF_TEXT);

    //if no text was found then return FALSE
    if(hg==NULL)
        return 1;

    //lock the memory and get a pointer to it
    data=(LPTSTR)GlobalLock(hg);
    //get the size of the text
    size=GlobalSize(hg);

    *string = data;

    //unlock the global memory
    GlobalUnlock(hg);

    //close the clipboard
    CloseClipboard();

    return UG_SUCCESS;
}
/***************************************************
Paste
    Pastes the contents of the clipboard into the
    grid - starting from the current cell
  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGCtrl::Paste(){

    CString string;

    //get the text from the clipboard
    if(CopyFromClipBoard(&string)!= UG_SUCCESS)
        return 1;

    Paste(string);

    return UG_SUCCESS;
}
/***************************************************
Paste
    Pastes the contents of the clipboard into the
    grid - starting from the current cell
  return
    UG_SUCCESS  success
    1           fail
****************************************************/
int CUGCtrl::Paste(CString &string){

    int     col     = m_GI->m_currentCol;
    long    row     = m_GI->m_currentRow;
    int     pos     =0;
    int     endpos;
    int     len;
    CString sub;
    LPTSTR  buf;
    CUGCell cell;
    csprochar endchar = 0;
    int     maxcol = col;
    long    maxrow = row;


    buf = string.GetBuffer(1);
    len = string.GetLength();

    while(pos < len){

        //check to see if the field is blank
        if(buf[pos]==_T('\t')|| buf[pos]==_T('\r') || buf[pos]==_T('\n')){
            endchar = buf[pos];
            endpos = pos;
            //set a blank cell
            GetCell(col,row,&cell);
            cell.SetText(_T(""));
            SetCell(col,row,&cell);
        }
        //find the end of the item then copy the item to the cell
        else{
            endpos = pos+1;
            while(endpos < len){
                endchar = buf[endpos];
                if(endchar == _T('\n') || endchar == _T('\r') || endchar == _T('\t'))
                    break;
                endpos++;
            }

            //copy the item
            GetCell(col,row,&cell);
            sub = string.Mid(pos,endpos-pos);
            cell.SetText(sub);
            SetCell(col,row,&cell);

            //store the max col and row
            if(col > maxcol)
                maxcol = col;
            if(row > maxrow)
                maxrow = row;
        }

        //set up the position for the next field
        if(endchar == _T('\t'))
            col++;
        if(endchar == _T('\r') || endchar == _T('\n')){
            col = m_GI->m_currentCol;
            row++;
            if(buf[endpos] == _T('\r') && buf[endpos+1]== _T('\n'))
                endpos++;
        }
        pos = endpos +1;

    }

    string.ReleaseBuffer();

    ClearSelections();
    SelectRange(m_GI->m_currentCol,m_GI->m_currentRow,maxcol,maxrow);

    RedrawAll();

    return UG_SUCCESS;
}
/***************************************************
return
    -1 failure
    otherwise datasource index number is returned
****************************************************/
int CUGCtrl::AddDataSource(CUGDataSource * ds){

    if(ds == NULL)
        return UG_ERROR;

    int loop;

    ds->m_ctrl = this;

    //alloc a larger DataSource List array if needed
    if(m_dataSrcListLength == m_dataSrcListMaxLength){
        m_dataSrcListMaxLength += 16;
        CUGDataSource ** temp = new CUGDataSource*[m_dataSrcListMaxLength];
        for(loop = 0;loop <m_dataSrcListMaxLength; loop++){
            if(loop < m_dataSrcListLength)
                temp[loop] = m_dataSrcList[loop];
            else
                temp[loop] = NULL;
        }
        delete[] m_dataSrcList;
        m_dataSrcList = temp;
    }
    for(loop = 1;loop <m_dataSrcListMaxLength; loop++){
        if(m_dataSrcList[loop] == NULL){
            m_dataSrcList[loop] = ds;
            m_dataSrcListLength ++;
            ds->SetID(loop);
            return loop;
        }
    }

    return -1;
}
/***************************************************
GetDataSource
    Purpose
        Returns the pointer to the datasource at the
        specified 0 based index. Datasources are added
        to the list through the AddDataSource member
        function
    Params
        index - 0 based index of datasource
    Returns
        NULL - failure
        otherwise the pointer to the datasource
****************************************************/
CUGDataSource * CUGCtrl::GetDataSource(int index){

    if(index <0 || index > m_dataSrcListLength)
        return NULL;

    //return the memory manager as the first datasource
    if(index == 0)
        return m_GI->m_CUGMem;
    else
        return m_dataSrcList[index];
}
/***************************************************
SetDefDataSource
    Purpose
        Sets the datasource for the grid to use as
        its default datasource. The datasource must
        first be in the datasource list (which the
        memory manager is by default, index 0).
    Params
        index - index of datasource to use as the default
    Return
        UG_SUCCESS  - success
        1           - failure
****************************************************/
int CUGCtrl::SetDefDataSource(int index){

    if(index <0 || index > m_dataSrcListLength)
        return 1;

    //if the index is 0 then use the memory manager
    //that was created in each sheet, otherwise use
    //a datasource from the datasource list
    m_GI->m_defDataSourceIndex = index;

    if(index == 0)
        m_GI->m_defDataSource = m_GI->m_CUGMem;
    else
        m_GI->m_defDataSource = m_dataSrcList[index];

    for(int cols = 0;cols < m_GI->m_numberCols;cols++){
        m_GI->m_colInfo[cols].dataSource = m_GI->m_defDataSource;
    }

    return UG_SUCCESS;
}
/***************************************************
GetDefDataSource
    Purpose
        Returns the index number of the default
        datasource for the grid
    Params
        none
    Return
        default data source index
****************************************************/
int CUGCtrl::GetDefDataSource(){
    return m_GI->m_defDataSourceIndex;
}
/***************************************************
RemoveDataSource
    Purpose
        Removes a datasource from the grids datasource
        list. This function DOES NOT DELETE the datasource
        object that was in the list.
    Params
        index -index of datasource to remove
    Return
        UG_SUCCESS  - success
        1           - failure
****************************************************/
int CUGCtrl::RemoveDataSource(int index){

    if(index <= 0 || index > m_dataSrcListLength)
        return 1;

    if(m_dataSrcList[index] != NULL){
        m_dataSrcList[index] = NULL;
//      m_dataSrcListLength --;
        return UG_SUCCESS;
    }
    else
        return 1;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetGridUsingDataSource(int index){

    if(index <0 || index >= m_dataSrcListMaxLength)
        return NULL;

    CUGDataSource * ds= m_dataSrcList[index];

    if(ds == NULL)
        return 1;

    int cols = ds->GetNumCols();
    if(cols <1)
        return 2;

    SetNumberCols(cols,FALSE);

    long rows = ds->GetNumRows();
    if(rows >=0)
        SetNumberRows(rows,FALSE);

    AdjustComponentSizes();


    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
CMenu * CUGCtrl::GetPopupMenu(){

    return m_menu;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::EmptyMenu(){

    while(m_menu->DeleteMenu(0,MF_BYPOSITION) != FALSE);

    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::AddMenuItem(int ID,LPCTSTR string){

    if(lstrlen(string) == 0 || ID == -1){
        if(m_menu->AppendMenu(MF_SEPARATOR,0,_T("")) != FALSE)
            return UG_SUCCESS;
    }
    else{
        if(m_menu->AppendMenu(MF_STRING,ID,string) != FALSE)
            return UG_SUCCESS;
    }
    return 1;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::RemoveMenuItem(int ID){

    if(m_menu->DeleteMenu(ID,MF_BYCOMMAND) != FALSE)
        return UG_SUCCESS;

    return 1;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::EnableMenu(BOOL state){

    if(state)
        m_GI->m_enablePopupMenu = TRUE;
    else
        m_GI->m_enablePopupMenu = FALSE;

    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::UseHints(BOOL state){
    if(state){
        m_GI->m_enableHints = TRUE;
        m_CUGGrid->EnableToolTips(TRUE);
    }
    else{
        m_GI->m_enableHints = FALSE;
        m_CUGGrid->EnableToolTips(FALSE);
    }
    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::UseVScrollHints(BOOL state){
    if(state){
        m_GI->m_enableVScrollHints = TRUE;
        m_CUGVScroll->EnableToolTips(TRUE);
    }
    else{
        m_GI->m_enableVScrollHints = FALSE;
        m_CUGVScroll->EnableToolTips(FALSE);
    }
    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::UseHScrollHints(BOOL state){
    if(state){
        m_GI->m_enableHScrollHints = TRUE;
        m_CUGHScroll->EnableToolTips(TRUE);
    }
    else{
        m_GI->m_enableHScrollHints = FALSE;
        m_CUGHScroll->EnableToolTips(FALSE);
    }
    return UG_SUCCESS;
}
/***************************************************
AddTab - int AddTab(LPCTSTR label,long ID);
    Purpose
        Adds a new tab to the grid to the end of the
        existing tabs. This function does not resize
        the visible tab area. Use SetTabWidth for setting
        the size.
    Params
        label   - label to show in the tab
        ID      - ID number returned when the tab is
                clicked
    Return
        UG_SUCCESS  - success
        1           - error
****************************************************/
int CUGCtrl::AddTab(LPCTSTR label,long ID){
    return m_CUGTab->AddTab(label,ID);
}
/****************************************************
*****************************************************/
int CUGCtrl::InsertTab(int pos,LPCTSTR label,long ID){
    return m_CUGTab->InsertTab(pos,label,ID);
}
/****************************************************
*****************************************************/
int CUGCtrl::DeleteTab(long ID){
    return m_CUGTab->DeleteTab(ID);
}
/****************************************************
*****************************************************/
int CUGCtrl::SetTabWidth(int width){
    m_GI->m_tabWidth = width;
    return UG_SUCCESS;
}
/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::SetCurrentTab(long ID){
    if(m_CUGTab->SetCurrentTab(ID)){
        OnTabSelected(ID);
        return UG_SUCCESS;
    }
    else
        return UG_ERROR;
}
/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::GetCurrentTab(){
    return m_CUGTab->GetCurrentTab();
}
/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::GetCurrentTabID(){
    return m_CUGTab->GetCurrentTabID();
}
/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::GetTabID(int iIndex){
    return m_CUGTab->GetTabID(iIndex);
}
/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::SetTabBackColor(long ID,COLORREF color){
    return m_CUGTab->SetTabBackColor(ID,color);
}
/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::SetTabTextColor(long ID,COLORREF color){
    return m_CUGTab->SetTabTextColor(ID,color);
}
/****************************************************
    Purpose 10/23/98 Savy added this for changing the tab text
    Params
    Return
*****************************************************/
int CUGCtrl::SetTabText(long ID,CString sString){
    return m_CUGTab->SetTabText(ID,sString);
}
/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::SetNumberSheets(int numSheets){

    if(numSheets < 1 || numSheets > 1024)
        return UG_ERROR;

    CUGGridInfo * origGI = m_GI;

    //create the new array
    CUGGridInfo ** temp = new CUGGridInfo *[numSheets];

    //copy the existing gridinfo into the new array
    for(int loop = 0;loop < m_numberSheets;loop++){
        if(loop < numSheets)
            temp[loop] = m_GIList[loop];
        else
            delete m_GIList[loop];
    }

    //add new gridinfo elements to the new array
    for(int loop = m_numberSheets ; loop < numSheets ;loop++){

        temp[loop] = new CUGGridInfo;

        //set up the default information
        temp[loop]->m_multiSelect->m_ctrl   = this;
        temp[loop]->m_CUGMem->m_ctrl        = this;
        temp[loop]->m_multiSelect->m_GI     = temp[loop];
        temp[loop]->m_editCtrl              = &m_defEditCtrl;

        m_GI = temp[loop];

        OnSheetSetup(loop);
    }

    //delete the old array, and use the new array
    if(m_GIList != NULL)
        delete[] m_GIList;
    m_GIList = temp;

    m_numberSheets = numSheets;

    m_GI = origGI;

    if(m_currentSheet >= numSheets)
        SetSheetNumber(numSheets -1);

    return UG_SUCCESS;
}

/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::GetNumberSheets(){
    return m_numberSheets;
}

/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::SetSheetNumber(int index,BOOL update){

    //check to see if the number is valid
    if(index <0 || index >= m_numberSheets)
        return UG_ERROR;

    //check to see if the display should be updated
    BOOL origValue = m_enableUpdate;
    if(!update){
        m_enableUpdate = FALSE;
    }

    m_CUGGrid->SetFocus();

    //switch in the new CUGGridInfo Class
    //copy over the tab width to the new class
    //update all components so that they point to
    //the new CUGGridInfo class
    //update the multiSelect pointer so that it points
    //to the new CUGGridInfo class multiSelect class
    CUGGridInfo* oldInfo    = m_GI;
    m_GI                    = m_GIList[index];

    m_GI->m_tabWidth = oldInfo->m_tabWidth;

    m_CUGGrid->m_GI         = m_GI;
    m_CUGTopHdg->m_GI       = m_GI;
    m_CUGSideHdg->m_GI      = m_GI;
    m_CUGCnrBtn->m_GI       = m_GI;
    m_CUGVScroll->m_GI      = m_GI;
    m_CUGHScroll->m_GI      = m_GI;
    m_CUGTab->m_GI          = m_GI;
    #ifdef UG_ENABLE_PRINTING
        m_CUGPrint->m_GI    = m_GI;
    #endif

    //store the current sheet number
    m_currentSheet = index;

    m_GI->m_lastTopRow = -1;
    m_GI->m_lastLeftCol = -1;

    //adjust the child window sizes - this also
    //causes the grid to re-draw
    AdjustComponentSizes();

    m_enableUpdate = origValue;

    return UG_SUCCESS;
}

/****************************************************
    Purpose
    Params
    Return
*****************************************************/
int CUGCtrl::GetSheetNumber(){
    return m_currentSheet;
}

/***************************************************
SetTH_NumberRows - int SetTH_NumberRows(int rows)
    Purpose
        Sets the number of rows available in the top
        heading of the grid. This function will not
        make the top heading disapear if set to 0, use
        the SetTH_Height for this purpose.
    Params
        rows - the number of rows to use in the top heading
    Return
        UG_SUCCESS  success
        1           invalid number of rows
****************************************************/
int CUGCtrl::SetTH_NumberRows(int rows){

    //range checking
    if(rows < 1 || rows > 64)
        return 1;

    //copy the old height information
    if(m_GI->m_topHdgHeights != NULL){
        int * temp = new int[rows];
        for(int loop = 0;loop <rows;loop++){
            if(loop < m_GI->m_numberTopHdgRows)
                temp[loop] = m_GI->m_topHdgHeights[loop];
            else
                temp[loop] = m_GI->m_defRowHeight;
        }
        delete[] m_GI->m_topHdgHeights;
        m_GI->m_topHdgHeights = temp;
    }
    else{
        m_GI->m_topHdgHeights = new int[rows];
        for(int loop = 0;loop <rows;loop++)
            m_GI->m_topHdgHeights[loop] = m_GI->m_defRowHeight;
    }

    m_GI->m_numberTopHdgRows = rows;

    return UG_SUCCESS;
}

/***************************************************
SetTH_RowHeight
    Sets the height of a row in the top heading
    Top heading rows start from -1 then -2 and so on
    -1 row is the one closest to the grid it self
****************************************************/
int CUGCtrl::SetTH_RowHeight(int row,int height){

    //translate the row number into a 0 based positive index
    row = (row * -1) -1;

    if(row <0 || row >= m_GI->m_numberTopHdgRows)
        return 1;

    if(height < 0 || height > 1024)
        return 2;

//  m_GI->m_topHdgHeights[row] = height;

//  smg: auuuuuuugh!! instead of the above line, which i've commented out,
//  the full func (using SetSH_ColWidth as a guide) shld be the following:

    if(m_GI->m_topHdgHeights [row] == height)
        return UG_SUCCESS;

    m_GI->m_topHdgHeights [row] = height;

    if(m_GI->m_numberTopHdgRows == 1)
        SetTH_Height (height);

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetSH_NumberCols(int cols){

    //range checking
    if(cols < 1 || cols > 64)
        return 1;

    //copy the old width information
    if(m_GI->m_sideHdgWidths != NULL){
        int * temp = new int[cols];
        for(int loop = 0;loop <cols;loop++){
            if(loop < m_GI->m_numberSideHdgCols)
                temp[loop] = m_GI->m_sideHdgWidths[loop];
            else
                temp[loop] = m_GI->m_defColWidth;
        }
        delete[] m_GI->m_sideHdgWidths;
        m_GI->m_sideHdgWidths = temp;
    }
    else{
        m_GI->m_sideHdgWidths = new int[cols];
        for(int loop = 0;loop <cols;loop++)
            m_GI->m_sideHdgWidths[loop] = m_GI->m_defColWidth;
    }

    m_GI->m_numberSideHdgCols = cols;

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetSH_ColWidth(int col,int width){

    //translate the col number into a 0 based positive index
    col = (col * -1) -1;

    if(col < 0 || col >= m_GI->m_numberSideHdgCols)
        return 1;

    if(width < 0 || width > 1024)
        return 2;

    if(m_GI->m_sideHdgWidths[col] == width)
        return UG_SUCCESS;

    m_GI->m_sideHdgWidths[col] = width;

    if(m_GI->m_numberSideHdgCols == 1)
        SetSH_Width(width);

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::SetLockRowHeight(){

    if(m_GI->m_numLockRows <1)
        return;

    m_GI->m_lockRowHeight = 0;
    for(int loop = 0;loop < m_GI->m_numLockRows;loop++){
        m_GI->m_lockRowHeight += GetRowHeight(loop);
    }

}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::SetLockColWidth(){

    if(m_GI->m_numLockCols <1)
        return;

    m_GI->m_lockColWidth = 0;
    for(int loop = 0;loop < m_GI->m_numLockCols;loop++){
        m_GI->m_lockColWidth += GetColWidth(loop);
    }

}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetArrowCursor(HCURSOR cursor){
    if(cursor == NULL)
        return UG_ERROR;
    m_GI->m_arrowCursor = cursor;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetWESizingCursor(HCURSOR cursor){
    if(cursor == NULL)
        return UG_ERROR;
    m_GI->m_WEResizseCursor = cursor;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetNSSizingCursor(HCURSOR cursor){
    if(cursor == NULL)
        return UG_ERROR;
    m_GI->m_NSResizseCursor = cursor;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::OnSetFocus(int section){
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::OnKillFocus(int section){
}


/*********************************************
drag and drop
**********************************************/
#ifdef UG_ENABLE_DRAGDROP

/*********************************************
**********************************************/
int CUGCtrl::StartDragDrop(){

    ReleaseCapture();

    CString SelectString;
    int     len;

    CreateSelectedString(SelectString,FALSE);
    len = SelectString.GetLength();

    HGLOBAL hglobal = GlobalAlloc(GMEM_ZEROINIT,sizeof(csprochar)*len+1);
    LPTSTR string = (LPTSTR)GlobalLock(hglobal);
    lstrcpy(string,SelectString);
    GlobalUnlock(hglobal);
    m_dataSource.CacheGlobalData(CF_TEXT,hglobal,NULL);

    m_dataSource.DoDragDrop(DROPEFFECT_COPY,NULL,NULL);

    m_dataSource.Empty();

    return UG_SUCCESS;
}
/*********************************************
COleDropTarget
**********************************************/
int CUGCtrl::DragDropTarget(BOOL state){

    if(state == FALSE){
        m_dropTarget.Revoke();
    }
    else{
        m_dropTarget.Register(m_CUGGrid);
    }
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
DROPEFFECT  CUGCtrl::OnDragEnter(COleDataObject* pDataObject){
    return DROPEFFECT_NONE;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
DROPEFFECT  CUGCtrl::OnDragOver(COleDataObject* pDataObject,int col,long row){
    return DROPEFFECT_NONE;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
DROPEFFECT  CUGCtrl::OnDragDrop(COleDataObject* pDataObject,int col,long row){
    return DROPEFFECT_NONE;
}
void CUGCtrl::OnDragLeave(CWnd* pWnd)
{
}
DROPEFFECT CUGCtrl::OnDragScroll(CWnd* pWnd, DWORD dwKeyState, CPoint point)
{
    return DROPEFFECT_NONE;
}
#endif

/***************************************************
****************************************************
    VIRTUAL FUNTIONS    VIRTUAL FUNCTIONS
****************************************************
****************************************************/

/***************************************************
OnSetup
    This function is called just after the grid window
    is created or attached to a dialog item.
    It can be used to initially setup the grid
****************************************************/
void CUGCtrl::OnSetup(){
}
/***************************************************
OnSheetSetup
****************************************************/
void CUGCtrl::OnSheetSetup(int sheetNumber){
}

/***************************************************
OnCanMove
    Sent when the current cell in the grid is about
    to move
    A return of TRUE allows the move, a return of
    FALSE stops the move
****************************************************/
int CUGCtrl::OnCanMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}
/***************************************************
OnCanMove
    Sent when the top row or left column in the grid is about
    to move
    A return of TRUE allows the move, a return of
    FALSE stops the move
****************************************************/
int CUGCtrl::OnCanViewMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::OnHitBottom(long numrows,long rowspast,long rowsfound){
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::OnHitTop(long numrows,long rowspast){

}

/***************************************************
OnCanSizeCol
    Sent when the user is over a separation line on
    the top heading
    A return value of TRUE allows the possibiliy of
    a resize
****************************************************/
int CUGCtrl::OnCanSizeCol(int col){
    return TRUE;
}

/***************************************************
OnColSizing
    Sent when the user is sizing a column
    The column that is being sized is given as
    well as the width. Plus the width can be modified
    at this point. This makes it easy to set min and
    max widths
****************************************************/
void CUGCtrl::OnColSizing(int col,int *width){
}

/***************************************************
OnColSized
    This is sent when the user finished sizing the
    given column (see above for more details)
****************************************************/
void CUGCtrl::OnColSized(int col,int *width){
}

/***************************************************
OnCanSizeRow
    Sent when the user is over a separation line on
    the side heading
    A return value of TRUE allows the possibiliy of
    a resize
****************************************************/
int  CUGCtrl::OnCanSizeRow(long row){
    return TRUE;
}

/***************************************************
OnRowSizing
    Sent when the user is sizing a row
    The row that is being sized is given as
    well as the height. Plus the height can be modified
    at this point. This makes it easy to set min and
    max heights
****************************************************/
void CUGCtrl::OnRowSizing(long row,int *height){
}

/***************************************************
OnRowSized
    This is sent when the user is finished sizing hte
    given row ( see above for more details)
****************************************************/
void CUGCtrl::OnRowSized(long row,int *height){
}

/***************************************************
OnCanSizeSideHdg
    This is sent when the user moves into position
    for sizing the width of the side heading
    return TRUE to allow the sizing
    or FALSE to not allow it
****************************************************/
int CUGCtrl::OnCanSizeSideHdg(){
    return TRUE;
}

/***************************************************
OnCanSizeTopHdg
    This is sent when the user moves into position
    for sizing the height of the top heading
    return TRUE to allow the sizing
    or FALSE to not allow it
****************************************************/
int CUGCtrl::OnCanSizeTopHdg(){
    return TRUE;
}

/***************************************************
OnSideHdgSizing
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::OnSideHdgSizing(int *width){
    return TRUE;

}

/***************************************************
OnTopHdgSizing
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::OnTopHdgSizing(int *height){
    return TRUE;

}

/***************************************************
OnSideHdgSized
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::OnSideHdgSized(int *width){
    return TRUE;

}

/***************************************************
OnTopHdgSized
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::OnTopHdgSized(int *height){
    return TRUE;

}

/***************************************************
OnColChange
    Sent whenever the current column changes
    The old and the new columns are given
****************************************************/
void CUGCtrl::OnColChange(int oldcol,int newcol){
}

/***************************************************
OnRowChange
    Sent whenever the current row changes
    The old and the new rows are given
****************************************************/
void CUGCtrl::OnRowChange(long oldrow,long newrow){
}

/***************************************************
OnCellChange
    Sent whenever the current cell changes rows or
    columns
****************************************************/
void CUGCtrl::OnCellChange(int oldcol,int newcol,long oldrow,long newrow){
}

/***************************************************
OnLeftColChange
    Sent whenever the left visible column in the grid
    changes
****************************************************/
void CUGCtrl::OnLeftColChange(int oldcol,int newcol){
}

/***************************************************
OnTopRowChange
    Sent whenever the top visible row in the grid changes
****************************************************/
void CUGCtrl::OnTopRowChange(long oldrow,long newrow){
}

/***************************************************
OnLClicked
    Sent whenever the user clicks the left mouse
    button within the grid
    this message is sent when the button goes down
    then again when the button goes up

    'col' and 'row' are negitive if the area clicked
    in is not a valid cell
    'rect' the rectangle of the cell that was clicked in
    'point' the point where the mouse was clicked
    'updn'  TRUE if the button is down FALSE if the
            button just when up
****************************************************/
void CUGCtrl::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnRClicked
    Sent whenever the user clicks the right mouse
    button within the grid
    this message is sent when the button goes down
    then again when the button goes up

    'col' and 'row' are negitive if the area clicked
    in is not a valid cell
    'rect' the rectangle of the cell that was clicked in
    'point' the point where the mouse was clicked
    'updn'  TRUE if the button is down FALSE if the
            button just when up
****************************************************/
void CUGCtrl::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnDClicked
    Sent whenever the user double clicks the left mouse
    button within the grid

    'col' and 'row' are negitive if the area clicked
    in is not a valid cell
    'rect' the rectangle of the cell that was clicked in
    'point' the point where the mouse was clicked
****************************************************/
void CUGCtrl::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnMouseMove
****************************************************/
void CUGCtrl::OnMouseMove(int col,long row,POINT *point,UINT nFlags,BOOL processed){

}

/***************************************************
OnTH_LClicked
    Sent whenever the user clicks the left mouse
    button within the top heading
    this message is sent when the button goes down
    then again when the button goes up

    'col' is negitive if the area clicked in is not valid
    'updn'  TRUE if the button is down FALSE if the
            button just when up
****************************************************/
void CUGCtrl::OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnTH_RClicked
    Sent whenever the user clicks the right mouse
    button within the top heading
    this message is sent when the button goes down
    then again when the button goes up

    'col' is negitive if the area clicked in is not valid
    'updn'  TRUE if the button is down FALSE if the
            button just when up
****************************************************/
void CUGCtrl::OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnTH_LClicked
    Sent whenever the user double clicks the left mouse
    button within the top heading

    'col' is negitive if the area clicked in is not valid
****************************************************/
void CUGCtrl::OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnSH_LClicked
    Sent whenever the user clicks the left mouse
    button within the side heading
    this message is sent when the button goes down
    then again when the button goes up

    'row' is negitive if the area clicked in is not valid
    'updn'  TRUE if the button is down FALSE if the
            button just when up
****************************************************/
void CUGCtrl::OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnSH_RClicked
    Sent whenever the user clicks the right mouse
    button within the side heading
    this message is sent when the button goes down
    then again when the button goes up

    'row' is negitive if the area clicked in is not valid
    'updn'  TRUE if the button is down FALSE if the
            button just when up
****************************************************/
void CUGCtrl::OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnSH_DClicked
    Sent whenever the user double clicks the left mouse
    button within the side heading

    'row' is negitive if the area clicked in is not valid
****************************************************/
void CUGCtrl::OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnCB_LClicked
    Sent whenever the user clicks the left mouse
    button within the top corner button
    this message is sent when the button goes down
    then again when the button goes up

    'updn'  TRUE if the button is down FALSE if the
            button just when up
****************************************************/
void CUGCtrl::OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnCB_RClicked
    Sent whenever the user clicks the right mouse
    button within the top corner button
    this message is sent when the button goes down
    then again when the button goes up

    'updn'  TRUE if the button is down FALSE if the
            button just when up
****************************************************/
void CUGCtrl::OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnCB_DClicked
    Sent whenever the user double clicks the left mouse
    button within the top corner button
****************************************************/
void CUGCtrl::OnCB_DClicked(RECT *rect,POINT *point,BOOL processed){
}

/***************************************************
OnKeyDown
    Sent whenever the user types when the grid has
    focus. The keystroke can be modified here as well.
    (See WM_KEYDOWN for more information)
****************************************************/
void CUGCtrl::OnKeyDown(UINT *vcKey,BOOL processed){
}

/***************************************************
OnCharDown
    Sent whenever the user types when the grid has
    focus. The keystroke can be modified here as well.
    (See WM_CHAR for more information)
****************************************************/
void CUGCtrl::OnCharDown(UINT *vcKey,BOOL processed){
}

/***************************************************
OnGetCell
    This message is sent everytime the grid needs to
    draw a cell in the grid. At this point the cell
    class has been filled with the information to be
    used to draw the cell. The information can now be
    changed before it is used for drawing
****************************************************/
void CUGCtrl::OnGetCell(int col,long row,CUGCell *cell){

}

/***************************************************
OnSetCell
    This message is sent everytime the a cell is about
    to change.
****************************************************/
void CUGCtrl::OnSetCell(int col,long row,CUGCell *cell){
}

/***************************************************
OnDataSourceNotify
    This message is sent from a data source , message
    depends on the data source - check the information
    on the data source(s) being used
    - The ID of the Data source is also returned
****************************************************/
void CUGCtrl::OnDataSourceNotify(int ID,long msg,long param){
}

/***************************************************
OnCellTypeNotify
    This message is sent from a cell type , message
    depends on the cell type - check the information
    on the cell type classes
    - The ID of the cell type is given
****************************************************/
int CUGCtrl::OnCellTypeNotify(long ID,int col,long row,long msg,long param){
    return TRUE;
}

/***************************************************
OnEditStart
    This message is sent whenever the grid is ready
    to start editing a cell
    A return of TRUE allows the editing a return of
    FALSE stops editing
    Plus the properties of the CEdit class can be modified
****************************************************/
int CUGCtrl::OnEditStart(int col, long row,CWnd **edit){
    return TRUE;
}

/***************************************************
OnEditVerify
    This is send when the editing is about to end
****************************************************/
int CUGCtrl::OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey){
    return TRUE;
}

/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CUGCtrl::OnEditFinish(int col, long row,CWnd* edit,LPCTSTR string,BOOL cancelFlag){

    return TRUE;
}

/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CUGCtrl::OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow){
    return TRUE;
}

/***************************************************
sections - UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
            UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
****************************************************/
void CUGCtrl::OnMenuCommand(int col,long row,int section,int item){
}

/***************************************************
return UG_SUCCESS to allow the menu to appear
return 1 to not allow the menu to appear
****************************************************/
int CUGCtrl::OnMenuStart(int col,long row,int section){
    return TRUE;
}

/***************************************************
//
//  Ultimate Grid v3.x  additions
//  These functions have been added since the
//  Original release
//
****************************************************/

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::HideCurrentCell(){
    return UG_SUCCESS;
}

/***************************************************
    Purpose

    Params
        dc - device context
        db_dc - double buffer device context, may be NULL
    Return
****************************************************/
void CUGCtrl::OnScreenDCSetup(CDC *dc,CDC *db_dc,int section){

}

/***************************************************
    Purpose
    Params
    Return
return      -1  if cell1 goes before cell2
            0   if the cells are equal
            1   if cell2 goes before cell1
****************************************************/
int CUGCtrl::OnSortEvaluate(CUGCell *cell1,CUGCell *cell2,int flags){

    if(flags&UG_SORT_DESCENDING){
        CUGCell *ptr = cell1;
        cell1 = cell2;
        cell2 = ptr;
    }

    if(cell1 == NULL && cell2 == NULL)
        return 0;
    else if(cell1 == NULL)
        return -1;
    else if(cell2 == NULL)
        return 1;

    if(cell1->IsPropertySet(UGCELL_TEXT_SET) == FALSE){
        if(cell2->IsPropertySet(UGCELL_TEXT_SET) == FALSE)
            return 0;
        return -1;
    }
    switch(cell1->GetDataType()){

        case UGCELLDATA_STRING:
            return _tcscmp( cell1->GetText() ,cell2->GetText());
        case UGCELLDATA_NUMBER:
        case UGCELLDATA_BOOL:
        case UGCELLDATA_CURRENCY:
                double n1 = cell1->GetNumber();
                double n2 = cell2->GetNumber();
                if(n1 < n2)
                    return -1;
                if(n1 > n2)
                    return 1;
                return 0;
    }
    return _tcscmp(cell1->GetText(),cell2->GetText());
}

/***************************************************
    Purpose
    Params
    Return
0:off  1:on  2:on-squared 3:on-cubed
****************************************************/
int CUGCtrl::SetBallisticMode(int mode){

    if(mode <0 || mode >3)
        return 1;

    m_GI->m_ballisticMode = mode;

    return UG_SUCCESS;

}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetBallisticDelay(int milisec){

    if(milisec < 0 || milisec > 32000)
        return 1;

    m_GI->m_ballisticDelay = milisec;

    return UG_SUCCESS;
}

/***************************************************
SetKeyBallisticMode
    Purpose
        Sets the ballistic mode for scrolling with
        the keyboard. The mode value is the number
        of repeats the key must make before an
        increment in speed is made. Every time
        a multiple of the repeat number is reached
        the speed is doubled.
    Params
        mode    0:off  n:number of key repeats
                (0 - 1000 are valid)
    Return
        UG_ERROR - invalid mode number
        UG_SUCCESS - success
****************************************************/
int CUGCtrl::SetBallisticKeyMode(int mode){

    if(mode <0 || mode > 1000)
        return 1;

    m_GI->m_ballisticKeyMode = mode;

    return UG_SUCCESS;
}

/***************************************************
SetKeyBallisticDelay
    Purpose
        Sets the initial delay for ballistic keys
        (if ballistic keys are on)
        the delay is only used when the key repeat
        count is lower than the mode set
    Params
        milisec - delay time
    Params
        UG_ERROR - invalid time
        UG_SUCCESS - success
****************************************************/
int CUGCtrl::SetBallisticKeyDelay(int milisec){

    if(milisec < 0)
        return 1;

    m_GI->m_ballisticKeyDelay = milisec;

    return UG_SUCCESS;
}

/***************************************************
SetDoubleBufferMode
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetDoubleBufferMode(int mode){

    m_GI->m_doubleBufferMode = mode;
    m_CUGGrid->SetDoubleBufferMode(mode);

    return UG_SUCCESS;
}

/***************************************************
LockColumns
    Purpose
        Sets the number of columns to lock in place
        on the left side of the grid (so they do not
        scroll).
        The following variable are set:
            m_GI->m_numLockCols
            m_GI->m_lockColWidth
    Params
        numCols - the number of columns to lock
    Return
        UG_SUCCESS  - success
        1           - numCols was out of range
****************************************************/
int CUGCtrl::LockColumns(int numCols){

    //do range checking
    if(numCols <0 || numCols > m_GI->m_numberCols)
        return 1;

    //if the new value is the same as the old then just return
    if(numCols == m_GI->m_numLockCols)
        return UG_SUCCESS;

    //set the number of locked columns
    m_GI->m_numLockCols = numCols;

    //set the lock width
    m_GI->m_lockColWidth = 0;
    for(int col = 0;col < numCols;col++){
        m_GI->m_lockColWidth += GetColWidth(col);
    }

    m_GI->m_leftCol = numCols;
    m_GI->m_lastLeftCol = numCols;

    CalcLeftCol();

    return UG_SUCCESS;
}

/***************************************************
LockRows
    Purpose
        Sets the number of rows to lock in place
        on the top of the grid (so they do not
        scroll).
        The following variable are set:
            m_GI->m_numLockRows
            m_GI->m_lockRowHeight
    Params
        numRows - the number of rows to lock
    Return
        UG_SUCCESS  - success
        1           - numRows was out of range
****************************************************/
int CUGCtrl::LockRows(int numRows){

    //do range checking
    if(numRows <0 || numRows > m_GI->m_numberRows)
        return 1;

    //if the new value is the same as the old then just return
    if(numRows == m_GI->m_numLockRows)
        return UG_SUCCESS;

    //set the number of locked rows
    m_GI->m_numLockRows = numRows;

    //set the lock height
    m_GI->m_lockRowHeight = 0;
    for(int row = 0;row < numRows;row++){
        m_GI->m_lockRowHeight += GetRowHeight(row);
    }

    m_GI->m_topRow = numRows;
    m_GI->m_lastTopRow = numRows;

    CalcTopRow();

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::OnTabSelected(int ID){
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::OnAdjustComponentSizes(RECT *grid,RECT *topHdg,RECT *sideHdg,
        RECT *cnrBtn,RECT *vScroll,RECT *hScroll,RECT *tabs){
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::OnDrawFocusRect(CDC *dc,RECT *rect){

    //DrawExcelFocusRect(dc,rect);

    rect->bottom --;
    rect->right --;
    dc->DrawFocusRect(rect);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
COLORREF CUGCtrl::OnGetDefBackColor(int section){
    if(section == UG_GRID)
        return GetSysColor(COLOR_WINDOW);
    else
        return GetSysColor(COLOR_BTNFACE);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::DrawExcelFocusRect(CDC *dc,RECT *rect){

    BOOL innerRectOnly = FALSE;
    BOOL hideTop = FALSE;
    BOOL hideLeft = FALSE;

    CPen pen,wPen;


    int dcID = dc->SaveDC();


    //find the rect to draw - may be a range of cells
    //if multi-select is on
    if(m_GI->m_multiSelectFlag&3){
        if(m_GI->m_multiSelect->GetNumberBlocks() == 1){
            int startCol,endCol;
            long startRow,endRow;
            m_GI->m_multiSelect->GetCurrentBlock(&startCol,&startRow,&endCol,&endRow);

            if(startCol != endCol || startRow != endRow){
                GetRangeRect(startCol,startRow,endCol,endRow,rect);
            }
            if(startCol >= m_GI->m_numLockCols && startCol < m_GI->m_leftCol){
                //hide the left
                hideLeft = TRUE;
            }
            if(startRow >= m_GI->m_numLockRows && startRow < m_GI->m_topRow){
                //hide the top
                hideTop = TRUE;
            }
            if(rect->bottom <= rect->top)
                GetRangeRect(startCol,startRow,endCol,endRow,rect);
            if(rect->right <= rect->left)
                GetRangeRect(startCol,startRow,endCol,endRow,rect);

            //add draw hints for the grid so it will clear up the
            //focus rect next time
            //m_CUGGrid->m_drawHint.AddHint(startCol-1,startRow-1,endCol+1,endRow+1);
        }
        else if(m_GI->m_multiSelect->GetNumberBlocks() > 1){
            innerRectOnly = TRUE;
        }
        else{
            //add draw hints for the grid so it will clear up the
            //focus rect next time
            //m_CUGGrid->m_drawHint.AddHint(m_GI->m_currentCol-1,m_GI->m_currentRow-1,
            //  m_GI->m_currentCol+1,m_GI->m_currentRow+1);
        }
    }
    else{
        //add draw hints for the grid so it will clear up the
        //focus rect next time
        //m_CUGGrid->m_drawHint.AddHint(m_GI->m_currentCol-1,m_GI->m_currentRow-1,
        //  m_GI->m_currentCol+1,m_GI->m_currentRow+1);
    }

    int top = rect->top;
    int left = rect->left;


    //inner black rect
    dc->SelectObject(GetStockObject(BLACK_PEN));
    if(rect->top == 0)
        rect->top ++;
    if(rect->left == 0)
        rect->left ++;
    if(!hideTop){
        dc->MoveTo(rect->left,rect->top);
        dc->LineTo(rect->right-1,rect->top);
    }
    else{
        dc->MoveTo(rect->right-1,rect->top);
    }
    dc->LineTo(rect->right-1,rect->bottom-1);
    dc->LineTo(rect->left,rect->bottom-1);
    if(!hideLeft){
        dc->LineTo(rect->left,rect->top-1);
    }
    rect->top = top;
    rect->left = left;


    //inner white rect
    dc->SelectObject(GetStockObject(WHITE_PEN));
    if(rect->top == 0)
        rect->top ++;
    if(rect->left == 0)
        rect->left ++;
    if(!hideTop){
        dc->MoveTo(rect->left+1,rect->top+1);
        dc->LineTo(rect->right-2,rect->top+1);
    }
    else{
        dc->MoveTo(rect->right-2,rect->top+1);
    }
    dc->LineTo(rect->right-2,rect->bottom-2);
    dc->LineTo(rect->left+1,rect->bottom-2);
    if(!hideLeft){
        dc->LineTo(rect->left+1,rect->top);
    }
    rect->top = top;
    rect->left = left;



    //draw the rest if innerRectOnly is false
    if(!innerRectOnly){

        dc->SelectObject(GetStockObject(BLACK_PEN));


        //outer rect
        if(rect->top == 0)
            rect->top +=2;
        if(rect->left == 0)
            rect->left +=2;
        if(hideLeft)
            rect->left++;
        if(!hideTop){
            dc->MoveTo(rect->left-2,rect->top-2);
            dc->LineTo(rect->right+1,rect->top-2);
        }
        else{
            rect->top++;
            dc->MoveTo(rect->right+1,rect->top-2);
        }
        dc->LineTo(rect->right+1,rect->bottom+1);
        dc->LineTo(rect->left-2,rect->bottom+1);
        if(!hideLeft){
            dc->LineTo(rect->left-2,rect->top -3);
        }

        rect->top = top;
        rect->left = left;


        //bottom right square
        dc->MoveTo(rect->right-2,rect->bottom-2);
        dc->LineTo(rect->right,rect->bottom-2);
        dc->MoveTo(rect->right+2,rect->bottom-2);
        dc->LineTo(rect->right+2,rect->bottom+1);
        dc->MoveTo(rect->right+2,rect->bottom+1);
        dc->LineTo(rect->right+2,rect->bottom+2);
        dc->LineTo(rect->right,rect->bottom+2);
        dc->MoveTo(rect->right-2,rect->bottom+2);
        dc->LineTo(rect->right,rect->bottom+2);

        wPen.CreatePen(PS_SOLID,1,RGB(255,255,255));
        dc->SelectObject(wPen);
        dc->MoveTo(rect->right+3,rect->bottom-3);
        dc->LineTo(rect->right-3,rect->bottom-3);
        dc->LineTo(rect->right-3,rect->bottom+3);


        pen.CreatePen(PS_SOLID,1,RGB(120,120,120));
        dc->SelectObject(pen);


        //dark gray middle rect
        if(rect->top >0 && !hideTop){
            dc->MoveTo(rect->left-2,rect->top-1);
            dc->LineTo(rect->right+2,rect->top-1);
        }
        else
            rect->top +=2;

        dc->MoveTo(rect->right,rect->top-1);
        dc->LineTo(rect->right,rect->bottom+3);
        if(rect->left >0 && !hideLeft){
            dc->MoveTo(rect->left-1,rect->bottom+1);
            dc->LineTo(rect->left-1,rect->top-2);
        }
        else
            rect->left +=3;
        dc->MoveTo(rect->right+2,rect->bottom);
        dc->LineTo(rect->left-3,rect->bottom);

    }

    rect->top = top;
    rect->left = left;


    dc->RestoreDC(dcID);

    pen.DeleteObject();
    wPen.DeleteObject();

}

/***************************************************
StartMenu
    starts the pop-up menu if enabled

    col - col to start the menu for
    row - row to start the menu for
    point - point in screen co-ords
    seciton - UG_GRID,UG_TOPHEADING,UG_SIDEHEADING
              UG_CORNERBUTTON,UG_TAB,UG_VSCROLL,UG_HSCROLL
Return
    UG_SUCCESS  - success
    1           - menu not enabled
    2           - OnStartMenu did not allow the menu to appear
    3           - menu failed
****************************************************/
int CUGCtrl::StartMenu(int col,long row,POINT *point,int section){

    if(!m_GI->m_enablePopupMenu)
        return 1;

    GetJoinStartCell(&col,&row);

    if(OnMenuStart(col,row,section) == FALSE)
        return 2;

    m_menuCol = col;
    m_menuRow = row;
    m_menuSection = section;

    if( m_menu->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,
        point->x,point->y,this,NULL) == FALSE){

        return 3;
    }
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
BOOL CUGCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
    // TODO: Add your specialized code here and/or call the base class
    int ID = LOWORD(wParam);

    //send a notification for menu items
    if(ID >= 1000){
        OnMenuCommand(m_menuCol,m_menuRow,m_menuSection,ID);
    }

    return CWnd::OnCommand(wParam, lParam);
}

/***************************************************
SetMargin
    Purpose
        Sets the left/right margins that cell types
        will use when drawing cells.
    Params
        pixels - pixels wide to set the margin to
    Return
        UG_SUCCESS(0)   - success
        UG_ERROR(1)     - error
****************************************************/
int CUGCtrl::SetMargin(int pixels){

    if(pixels < 0)
        return 1;

    m_GI->m_margin = pixels;

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::AddFont(LPCTSTR fontName,int height,int weight){

    return AddFont(height,0,0,0,weight,0,0,0,0,0,0,0,0,fontName);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::AddFont(int height,int width,int escapement,int orientation,
            int weight,BYTE italic,BYTE underLine,BYTE strikeOut,
            BYTE charSet,BYTE outputPrecision,BYTE clipPrecision,
            BYTE quality,BYTE pitchAndFamily,LPCTSTR fontName){

    int count,loop;
    LOGFONT lf;
    LOGFONT lf2;
    int lfBaseSize = sizeof(LOGFONT) - LF_FACESIZE;

    //create a log font structure from the params
    lf.lfHeight         = height;
    lf.lfWidth          = width;
    lf.lfEscapement     = escapement;
    lf.lfOrientation    = orientation;
    lf.lfWeight         = weight;
    lf.lfItalic         = italic;
    lf.lfUnderline      = underLine;
    lf.lfStrikeOut      = strikeOut;
    lf.lfCharSet        = charSet;
    lf.lfOutPrecision   = outputPrecision;
    lf.lfClipPrecision  = clipPrecision;
    lf.lfQuality        = quality;
    lf.lfPitchAndFamily = pitchAndFamily;
    if(lstrlen(fontName) < LF_FACESIZE)
        lstrcpy(lf.lfFaceName, fontName );
    else
        lstrcpy(lf.lfFaceName,_T(""));

    //check to see if a font with the same attribs already exists
    //else add the font
    count = m_fontList->GetCount();
    for(loop = 0;loop < count;loop++){

        //get the LOGFONT for an existing font
        CFont * font = (CFont *)m_fontList->GetPointer(loop);
        font->GetLogFont(&lf2);

        //compare the LOGFONTs
        if(memcmp(&lf,&lf2,lfBaseSize) == 0){
            if(lstrcmp(lf.lfFaceName,lf2.lfFaceName) == 0){
                //if the LOGFONTs are the same then just
                //increment the use count andreturn a pointer
                //to the existing font
                m_fontList->UpdateParam(loop,m_fontList->GetParam(loop)+1);
                return loop;
            }
        }
    }

    //if a matching font was not found above then create it
    CFont * font = new CFont();
    font->CreateFontIndirect(&lf);
    return m_fontList->AddPointer(font);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::RemoveFont(int index){

    //decrement the use count
    int count = m_fontList->GetParam(index) - 1;
    m_fontList->UpdateParam(index,count);

    //if the use count is 0 then delete it
    if(count <=0)
        return m_fontList->DeletePointer(index);
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::ClearAllFonts(){

    m_fontList->EmptyList();

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
CFont * CUGCtrl::GetFont(int index){

    return (CFont *)m_fontList->GetPointer(index);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetDefFont(CFont *font){

    if(font == NULL)
        return 1;

    m_GI->m_defFont = font;

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetDefFont(int index){

    CFont * font = (CFont *)m_fontList->GetPointer(index);
    if(font == NULL)
        return 1;

    m_GI->m_defFont =  font;

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::AddBitmap( UINT resourceID,LPCTSTR resourceName ){

    CBitmap * bitmap = new CBitmap();
    if(resourceName == NULL)
        bitmap->LoadBitmap(resourceID);
    else
        bitmap->LoadBitmap(resourceName);

    return m_bitmapList->AddPointer(bitmap);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::AddBitmap( LPCTSTR fileName){
    BITMAPFILEHEADER    m_bmfh;
    BITMAPINFOHEADER    m_bmih;
    BITMAPINFO *        m_lpbmi;
    void *              m_lpvBits;

    FILE * fptr;

    // open the file
    fptr = _tfopen(fileName,_T("rb"));

    //return if the open file failed
    if(fptr==NULL){
        return -1;
    }

    //Retrieve the BITMAPFILEHEADER structure
    fread(&m_bmfh,1,sizeof(BITMAPFILEHEADER),fptr);

    //Retrieve the BITMAPINFOHEADER structure
    fread(&m_bmih,1,sizeof(BITMAPINFOHEADER),fptr);

    //Allocate memory for the BITMAPINFO structure
    if(m_bmih.biBitCount <16)
        m_lpbmi = (BITMAPINFO *)new csprochar[sizeof(BITMAPINFOHEADER)+((1<<m_bmih.biBitCount) * sizeof(RGBQUAD))];
    else
        m_lpbmi = (BITMAPINFO *)new csprochar[sizeof(BITMAPINFOHEADER)+ sizeof(RGBQUAD)];

    //Load BITMAPINFOHEADER into the BITMAPINFO structure
    memcpy(m_lpbmi,&m_bmih,sizeof(BITMAPINFO));

    //Retrieve the color table
    if(m_bmih.biBitCount <16)
        fread(m_lpbmi->bmiColors,1,((1<<m_bmih.biBitCount) * sizeof(RGBQUAD)),fptr);

    //Allocate memory for the required number of bytes
    m_lpvBits = new csprochar[m_bmfh.bfSize - m_bmfh.bfOffBits];

    //Retrieve the bitmap data
    fread(m_lpvBits,1,(m_bmfh.bfSize - m_bmfh.bfOffBits),fptr);

    //close the file
    fclose(fptr);


    //check to see if a DIB was already loaded
    //if it was then delete the old information

    CBitmap* bitmap = new CBitmap();

    CDC *dc = m_CUGGrid->GetDC();
    bitmap->Attach(CreateDIBitmap(dc->m_hDC,&m_bmih,CBM_INIT,m_lpvBits,m_lpbmi, DIB_RGB_COLORS));
    m_CUGGrid->ReleaseDC(dc);

    delete[] m_lpbmi;
    delete[] m_lpvBits;

    return m_bitmapList->AddPointer(bitmap);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::RemoveBitmap(int index){

    return m_bitmapList->DeletePointer(index);

}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::ClearAllBitmaps(){

    m_bitmapList->EmptyList();

    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
CBitmap* CUGCtrl::GetBitmap(int index){

    return (CBitmap *)m_bitmapList->GetPointer(index);

}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
BOOL CUGCtrl::OnColSwapStart(int col){

    return TRUE;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
BOOL CUGCtrl::OnCanColSwap(int fromCol,int toCol){

    return TRUE;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::MoveColPosition(int fromCol,int toCol,BOOL insertBefore){

    //check the instert before flag
    if(insertBefore == FALSE)
        toCol++;

    //range checking
    if(fromCol <0 || fromCol >= m_GI->m_numberCols)
        return 1;
    if(toCol <0 || toCol > m_GI->m_numberCols)
        return 1;

    //check to see if the col position needs to be changed
    if(fromCol == toCol || fromCol+1 == toCol)
        return UG_SUCCESS;

    if(fromCol < toCol)
        toCol --;

    //store the column info for the fromCol - since it will be over-written
    int fTransCol               = m_GI->m_colInfo[fromCol].colTranslation;
    int fWidth                  = m_GI->m_colInfo[fromCol].width;
    CUGDataSource * fDataSource = m_GI->m_colInfo[fromCol].dataSource;
    CUGCell * fColDefault       = m_GI->m_colInfo[fromCol].colDefault;

    //shift the column information over to the new position
    if(fromCol < toCol){
        //shift everything bwtween the to and from cols down one column
        for(int loop = fromCol;loop < toCol;loop++){
            m_GI->m_colInfo[loop].colTranslation = m_GI->m_colInfo[loop+1].colTranslation;
            m_GI->m_colInfo[loop].width         = m_GI->m_colInfo[loop+1].width;
            m_GI->m_colInfo[loop].dataSource    = m_GI->m_colInfo[loop+1].dataSource;
            m_GI->m_colInfo[loop].colDefault    = m_GI->m_colInfo[loop+1].colDefault;
        }
    }
    else{
        //shift everything bwtween the to and from cols up one column
        for(int loop = fromCol;loop > toCol;loop--){
            m_GI->m_colInfo[loop].colTranslation = m_GI->m_colInfo[loop-1].colTranslation;
            m_GI->m_colInfo[loop].width         = m_GI->m_colInfo[loop-1].width;
            m_GI->m_colInfo[loop].dataSource    = m_GI->m_colInfo[loop-1].dataSource;
            m_GI->m_colInfo[loop].colDefault    = m_GI->m_colInfo[loop-1].colDefault;
        }
    }

    //copy the prev. saved col informtion into the to column
    m_GI->m_colInfo[toCol].colTranslation = fTransCol;
    m_GI->m_colInfo[toCol].width        = fWidth;
    m_GI->m_colInfo[toCol].dataSource   = fDataSource;
    m_GI->m_colInfo[toCol].colDefault   = fColDefault;

    SetLockColWidth();

    return UG_SUCCESS;
}



#ifdef UG_ENABLE_PRINTING


/***************************************************
    Purpose
    Params
    Return
this function returns the number of pages needed
to print the entire range, using the current
print scale and options
****************************************************/
int CUGCtrl::PrintInit(CDC * pDC, CPrintDialog* pPD, int startCol,long startRow,
              int endCol,long endRow){

    return m_CUGPrint->PrintInit(pDC,pPD,startCol,startRow,endCol,endRow);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::PrintPage(CDC * pDC, int pageNum){

    return m_CUGPrint->PrintPage(pDC,pageNum);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::PrintSetMargin(int whichMargin,int size){
    return m_CUGPrint->PrintSetMargin(whichMargin,size);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::PrintSetScale(double scale){

    return m_CUGPrint->PrintSetScale(scale);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::PrintSetOption(int option,long param){

    return m_CUGPrint->PrintSetOption(option,param);
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::PrintGetOption(int option,long *param){

    return m_CUGPrint->PrintGetOption(option,param);
}

#endif

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetNewTopHeadingClass(CUGTopHdg * topHeading){

    if(m_CUGTopHdg != NULL)
        delete m_CUGTopHdg;

    m_CUGTopHdg = topHeading;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetNewSideHeadingClass(CUGSideHdg * sideHeading){

    if(m_CUGSideHdg!= NULL)
        delete m_CUGSideHdg;

    m_CUGSideHdg = sideHeading;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetNewGridClass(CUGGrid * grid){

    if(m_CUGGrid != NULL)
        delete m_CUGGrid;

    m_CUGGrid = grid;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetNewMultiSelectClass(CUGMultiSelect * multiSelect){

    if(m_GI->m_multiSelect != NULL)
        delete m_GI->m_multiSelect;

    m_GI->m_multiSelect = multiSelect;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetNewTabClass(CUGTab * tab){

    if(m_CUGTab != NULL)
        delete m_CUGTab;

    m_CUGTab = tab;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetNewVScrollClass(CUGVScroll * scroll){

    if(m_CUGVScroll != NULL)
        delete m_CUGVScroll;

    m_CUGVScroll = scroll;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetNewHScrollClass(CUGHScroll * scroll){

    if(m_CUGHScroll != NULL)
        delete m_CUGHScroll;

    m_CUGHScroll = scroll;
    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetNewEditClass(CWnd * edit){

    m_GI->m_editCtrl = edit;

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
CWnd * CUGCtrl::GetEditClass(){

    return m_GI->m_editCtrl;

}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::AddOverlayObject(CWnd *wnd,int startCol,int startRow,
                     int endCol,int endRow){

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetOverlayObjectPos(int index,int startCol,int startRow,
                        int endCol,int endRow,int leftOffset,
                        int topOffset,int rightOffset,int bottomOffset){


    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::GetOverlayObjectPos(int index,int* startCol,int* startRow,
                        int* endCol,int* endRow,int* leftOffset,
                        int* topOffset,int* rightOffset,
                        int* bottomOffset){

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::GetOverlayObjectPos(int index,int* startCol,int* startRow,
                        int* endCol,int* endRow){

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::RemoveOverlayObject(int index){

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
CWnd * CUGCtrl::GetOverlayObject(int index){

    return UG_SUCCESS;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetTrackingWindow(CWnd *wnd){

    if(wnd == NULL)
        return UG_ERROR;

    m_trackingWnd = wnd;
    m_trackingWnd->SetWindowPos(&wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);

    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::SetTrackingWindowMode(int mode){
    m_GI->m_trackingWndMode = mode;
    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::MoveTrackingWindow(){

    if(m_trackingWnd == NULL)
        return;

    RECT tRect,origTRect;
    RECT rect;

    BOOL moveWindow = FALSE;

    int sWidth = GetSystemMetrics(SM_CXSCREEN);
    int sHeight = GetSystemMetrics(SM_CYSCREEN);

    m_trackingWnd->GetWindowRect(&origTRect);
    m_trackingWnd->GetWindowRect(&tRect);
    m_CUGGrid->ScreenToClient(&tRect);

    GetCellRect(m_GI->m_dragCol,m_GI->m_dragRow,&rect);

    if(m_GI->m_trackingWndMode == 1){  //stay close mode
        int tWidth = tRect.right - tRect.left;
        int tHeight = tRect.bottom - tRect.top;

        tRect.left = rect.right +30;
        tRect.right = tRect.left + tWidth;
        tRect.top = rect.top;
        tRect.bottom = rect.top + tHeight;

        m_CUGGrid->ClientToScreen(&tRect);
        moveWindow = TRUE;
    }

    if(tRect.top < rect.bottom && tRect.bottom > rect.top &&
        tRect.left < rect.right && tRect.right > rect.left){

        int rightInc = rect.right - tRect.left + 5;
        int downInc = rect.bottom - tRect.top + 5;

        //move the tracking rect over to the right if possible
        if(origTRect.right+rightInc < sWidth){
            m_trackingWnd->GetWindowRect(&tRect);
            tRect.left += rightInc;
            tRect.right += rightInc;
            moveWindow = TRUE;
        }
        //else move the tracking rect down
        else if(origTRect.bottom + downInc < sHeight){
            m_trackingWnd->GetWindowRect(&tRect);
            tRect.top += downInc;
            tRect.bottom += downInc;
            moveWindow = TRUE;
        }
        else{
            rightInc = rect.left - tRect.right - 5;
            downInc = rect.top  - tRect.bottom - 5;

            //move the tracking rect to the left
            if(origTRect.left+rightInc >0){
                m_trackingWnd->GetWindowRect(&tRect);
                tRect.left += rightInc;
                tRect.right += rightInc;
                moveWindow = TRUE;
            }
            //move the tracking rect up
            else if(origTRect.bottom + downInc < sHeight){
                m_trackingWnd->GetWindowRect(&tRect);
                tRect.top += downInc;
                tRect.bottom += downInc;
                moveWindow = TRUE;
            }
        }
    }

    if(moveWindow){
        OnTrackingWindowMoved(&origTRect,&tRect);
        m_trackingWnd->MoveWindow(&tRect,TRUE);
        m_trackingWnd->UpdateWindow();
        RedrawAll();
    }
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGCtrl::OnTrackingWindowMoved(RECT *origRect,RECT *newRect){

}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::EnableUpdate(BOOL state){

    m_enableUpdate = state;

    return UG_SUCCESS;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::OnHint(int col,long row,int section,CString *string){
    string->Format(_T("Col:%d Row:%ld"),col,row);
    return TRUE;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::OnVScrollHint(long row,CString *string){
    return TRUE;
}
/***************************************************
    Purpose
    Params
    Return
****************************************************/
int CUGCtrl::OnHScrollHint(int col,CString *string){
    return TRUE;
}
/***************************************************
****************************************************/
int CUGCtrl::SetUserSizingMode(BOOL state){

    m_GI->m_userSizingMode = state;

    return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::SetColDataSource(int col,int dataSrcIndex){

    if(dataSrcIndex < 0 || dataSrcIndex > m_dataSrcListLength)
        return UG_ERROR;

    if(dataSrcIndex == 0)
        return SetColDataSource(col,m_GI->m_CUGMem);
    else
        return SetColDataSource(col,m_dataSrcList[dataSrcIndex]);
}

/***************************************************
****************************************************/
int CUGCtrl::SetColDataSource(int col,CUGDataSource * dataSrc){

    if(col < 0 || col >= m_GI->m_numberCols)
        return UG_ERROR;
    if(dataSrc == NULL)
        return UG_ERROR;

    m_GI->m_colInfo[col].dataSource = dataSrc;
    return UG_SUCCESS;
}

/***************************************************
    ModifyDialogItemText - used in conjunction with EnumChildWindows
    to let us change text of a control on a common dialog
    (e.g. m_findReplaceDialog)
****************************************************/
BOOL CALLBACK CUGCtrl::ModifyDlgItemText(HWND hWnd, LPARAM lParam){
    // if the text of the window passed in matches the
    // text of the first CSring in the array pair pointed
    // to by lParam, set the text to the 2nd CString
    // and return FALSE;
    CString strTemp;
    CString * strP = (CString*) lParam;

    // get the existing text
    CWnd * wnd = CWnd::FromHandle(hWnd);
    wnd->GetWindowText(strTemp);

    if(strTemp.Find((LPCTSTR)strP[0]) != -1) {
        wnd->SetWindowText( strP[1] );
        return FALSE;
    }
    return TRUE;
}



void CUGCtrl::ForeachSelectedRow(const std::function<void(long)>& callback_function)
{
    if( GetNumberRows() == 0 )
        return;

    int col;
    long row;

    if( EnumFirstSelected(&col, &row) == UG_SUCCESS )
    {
        callback_function(row);

        long prev_row = row;

        while( EnumNextSelected(&col, &row) == UG_SUCCESS )
        {
            if( row != prev_row )
            {
                callback_function(row);
                prev_row = row;
            }
        }
    }
}

std::vector<long> CUGCtrl::GetSelectedRows()
{
    std::vector<long> selected_rows;

    ForeachSelectedRow(
        [&](long row)
        {
            selected_rows.emplace_back(row);
        });

    return selected_rows;
}
