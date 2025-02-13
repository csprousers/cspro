/***************************************************
****************************************************
Skeleton Class for a Derived CTblGrid v3.5
****************************************************
****************************************************/
#include "StdAfx.h"
#include "Tblgrid.h"
#include "ASCIIExp.h"
#include "HTMLExpt.h"
#include "RTFExpt.h"
#include "TabChWnd.h"
#include "TabDoc.h"
#include "TabDropSource.h"
#include "TabView.h"
#include "TbHdrClc.h"
#include <fstream>
#include <strstream>

const int GRID_DEFAULT_ROWHEIGHT =  5;
const int GRID_DEFAULT_COLWIDTH  = 75;
const int GRID_DEFAULT_COLS      =  5;
const int GRID_DEFAULT_ROWS      = 10;



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int iTotalWidth;

class CMyTimer
{
public:
    CTime m_StartTime;
    int   m_TotalSeconds;
    void Start() { m_StartTime = CTime::GetCurrentTime();}
    int  Stop() {
        CTimeSpan ts  = CTime::GetCurrentTime() - m_StartTime;
        m_TotalSeconds =  (int)ts.GetTotalSeconds();
        return m_TotalSeconds;
                }
    void ShowMsg(CIMSAString sMsg){
        CIMSAString sLocalMsg;
        sLocalMsg = sMsg + _T("%d");
        sMsg.Format(sLocalMsg,m_TotalSeconds);
        AfxMessageBox(sMsg);
    }

};

CPoint oldPoint;

BEGIN_MESSAGE_MAP(CTblGrid,CUGCtrl)
//{{AFX_MSG_MAP(CTblGrid)
// NOTE - the ClassWizard will add and remove mapping macros here.
//    DO NOT EDIT what you see in these blocks of generated code !
//}}AFX_MSG_MAP
    ON_MESSAGE(UWM::Table::Update, OnUpdateTable)
    ON_WM_PAINT()
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
END_MESSAGE_MAP()


/***************************************************
****************************************************/
CTblGrid::CTblGrid()
{
    m_pTable = nullptr;
    m_pSpec = nullptr;
    m_pTblColRoot = nullptr;
    m_pTblRowRoot = nullptr;
    m_iGridHeaderRows = 0;
    m_iNumGridRows =0;
    m_bGridVarDrop = false;
    m_iCurrSelArea=0;
    m_bReconcileSpecialCells = false;
    m_bFixLines = false;
}
/***************************************************
****************************************************/
CTblGrid::~CTblGrid()
{
    if(m_pTblRowRoot) {
        delete  m_pTblRowRoot;
    }
    if(m_pTblColRoot) {
        delete  m_pTblColRoot;
    }
    DestroyCells();
}


void CTblGrid::DefaultGrid() {

    EnableExcelBorders(FALSE);
    SetNumberCols(GRID_DEFAULT_COLS);
    SetNumberRows(GRID_DEFAULT_ROWS);

    for (int iCol=0; iCol<GRID_DEFAULT_COLS; iCol++) {
        for (int iRow=0; iRow<GRID_DEFAULT_ROWS; iRow++) {
            QuickSetText(iCol, iRow, _T(""));
            QuickSetBorder (iCol, iRow, 0);
            //Make sure that the extramem is cleared
            CUGCell cellGrid ;
            GetCell(iCol,iRow,&cellGrid);
            cellGrid.SetText(_T(""));
            cellGrid.ClearExtraMem();
            SetCell(iCol,iRow,&cellGrid);
        }
        SetColWidth(iCol, GRID_DEFAULT_COLWIDTH);
    }
    SetSH_Width(20);
}

/***************************************************
OnSetup
This function is called just after the grid window
is created or attached to a dialog item.
It can be used to initially setup the grid
****************************************************/
HCURSOR     _hCursorSize;
void CTblGrid::OnSetup(){
    _hCursorSize = ::LoadCursor(nullptr,IDC_SIZEALL) ;

    m_GI->m_bCrossTab  = true;
    m_blackPen.CreatePen(PS_SOLID,1,RGB(0,0,0));
    m_whitePen.CreatePen(PS_SOLID,1,RGB(255,0,0));

    EnableExcelBorders(FALSE);

    SetMultiSelectMode(TRUE);
    DragDropTarget(TRUE);

    AdjustComponentSizes();

    RECT rect = {0,0,0,0};
    m_InlineEdit.Create(WS_CHILD|ES_MULTILINE|ES_AUTOVSCROLL,rect,this,125);
    m_InlineEdit.m_ctrl =this;
    //m_InlineEdit.Create(WS_VISIBLE,rect,this,125);
}

/***************************************************
OnSheetSetup
****************************************************/
void CTblGrid::OnSheetSetup(int sheetNumber){
//  SetMultiSelectMode(TRUE);
}

/***************************************************
OnCanMove
Sent when the current cell in the grid is about
to move
A return of TRUE allows the move, a return of
FALSE stops the move
****************************************************/
int CTblGrid::OnCanMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}
/***************************************************
OnCanMove
Sent when the top row or left column in the grid is about
to move
A return of TRUE allows the move, a return of
FALSE stops the move
****************************************************/
int CTblGrid::OnCanViewMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}
/***************************************************
****************************************************/
void CTblGrid::OnHitBottom(long numrows,long rowspast,long rowsfound){
}
/***************************************************
****************************************************/
void CTblGrid::OnHitTop(long numrows,long rowspast){

}
/***************************************************
OnCanSizeCol
Sent when the user is over a separation line on
the top heading
A return value of TRUE allows the possibiliy of
a resize
****************************************************/
int CTblGrid::OnCanSizeCol(int col){
    if(GetColWidth(col) ==0){// Do not allow to open up hidden cols
        return FALSE;
    }
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
void CTblGrid::OnColSizing(int col,int *width){
}
/***************************************************
OnColSized
This is sent when the user finished sizing the
given column (see above for more details)
****************************************************/
void CTblGrid::OnColSized(int col,int *width){
    if(*width > 5) {//for now . what shld be the min
        SaveStateInfo();
    }
    else {
        *width =5;
    }
}
/***************************************************
OnCanSizeRow
Sent when the user is over a separation line on
the side heading
A return value of TRUE allows the possibiliy of
a resize
****************************************************/
int  CTblGrid::OnCanSizeRow(long row)
{
    if(GetRowHeight(row) ==0){// Do not allow to open up hidden rows
        return FALSE;
    }
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
void CTblGrid::OnRowSizing(long row,int *height){
}
/***************************************************
OnRowSized
This is sent when the user is finished sizing hte
given row ( see above for more details)
****************************************************/
void CTblGrid::OnRowSized(long row,int *height){
    if(*height > 5){//for now . what shld be the min
        SaveStateInfo();
    }
    else {
        *height  = 5;
    }
}
/***************************************************
OnCanSizeSideHdg
This is sent when the user moves into position
for sizing the width of the side heading
return TRUE to allow the sizing
or FALSE to not allow it
****************************************************/
int CTblGrid::OnCanSizeSideHdg(){
    return TRUE;
}
/***************************************************
OnCanSizeTopHdg
This is sent when the user moves into position
for sizing the height of the top heading
return TRUE to allow the sizing
or FALSE to not allow it
****************************************************/
int CTblGrid::OnCanSizeTopHdg(){
    return TRUE;
}
/***************************************************
OnSideHdgSizing
****************************************************/
int CTblGrid::OnSideHdgSizing(int *width){
    return TRUE;

}
/***************************************************
OnSideHdgSized
****************************************************/
int CTblGrid::OnSideHdgSized(int *width){
    SaveStateInfo();
    return TRUE;

}
/***************************************************
OnTopHdgSized
****************************************************/
int CTblGrid::OnTopHdgSized(int *height){
    SaveStateInfo();
    return TRUE;

}
/***************************************************
OnTopHdgSizing
****************************************************/
int CTblGrid::OnTopHdgSizing(int *height){
    return TRUE;

}
/***************************************************
OnColChange
Sent whenever the current column changes
The old and the new columns are given
****************************************************/
void CTblGrid::OnColChange(int oldcol,int newcol){
}
/***************************************************
OnRowChange
Sent whenever the current row changes
The old and the new rows are given
****************************************************/
void CTblGrid::OnRowChange(long oldrow,long newrow){
}
/***************************************************
OnCellChange
Sent whenever the current cell changes rows or
columns
****************************************************/
void CTblGrid::OnCellChange(int oldcol,int newcol,long oldrow,long newrow){
}
/***************************************************
OnLeftColChange
Sent whenever the left visible column in the grid
changes
****************************************************/
void CTblGrid::OnLeftColChange(int oldcol,int newcol){
}
/***************************************************
OnTopRowChange
Sent whenever the top visible row in the grid changes
****************************************************/
void CTblGrid::OnTopRowChange(long oldrow,long newrow){
}
/***************************************************
OnLClicked
Sent whenever the user clicks the left mouse
button within the grid
this message is sent when the button goes down
then again when the button goes up

  'col' and 'row' are negative if the area clicked
  in is not a valid cell
  'rect' the rectangle of the cell that was clicked in
  'point' the point where the mouse was clicked
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CTblGrid::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed){

    if (m_pTable == nullptr) {
        // no table attached to the grid
        return;
    }

    if (!updn) {    // button just went up
        UpdateTableBlocking();
    }

    if (updn) {     // button just went down
        if (IsValidDragDrop(col, row)) {
            // we're in stubs or column headers
            m_iDragDropRow = row;
            m_iDragDropCol = col;

            StartDragDrop(row,col);
        }
    }
   // DrawSubTableLines();
   DrawSubTableRects(true);
}

/***************************************************
OnRClicked
Sent whenever the user clicks the right mouse
button within the grid
this message is sent when the button goes down
then again when the button goes up

  'col' and 'row' are negative if the area clicked
  in is not a valid cell
  'rect' the rectangle of the cell that was clicked in
  'point' the point where the mouse was clicked
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CTblGrid::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
{
    if (!updn) {    // button just went up
        UpdateTableBlocking();
        CWnd* pWnd = GetParent();
        /*this->ClientToScreen(point);
        pWnd->ScreenToClient(point);*/ //Dont do this 'cos we need this point to get the row,col
        LPARAM lParam = MAKELONG(point->x , point->y);
        pWnd->SendMessage(WM_RBUTTONUP, 0 , lParam);
    }
}
/***************************************************
OnDClicked
Sent whenever the user double clicks the left mouse
button within the grid

  'col' and 'row' are negative if the area clicked
  in is not a valid cell
  'rect' the rectangle of the cell that was clicked in
  'point' the point where the mouse was clicked
****************************************************/
void CTblGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
    StartEdit();
}
/***************************************************
OnMouseMove
****************************************************/
void CTblGrid::OnMouseMove(int col,long row,POINT *point,UINT nFlags,BOOL processed){
}
/***************************************************
OnTH_LClicked
Sent whenever the user clicks the left mouse
button within the top heading
this message is sent when the button goes down
then again when the button goes up

  'col' is negative if the area clicked in is not valid
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CTblGrid::OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed) {

    if(!updn){      // button just went up
        // force focus rect to topmost cell
        m_GI->m_currentRow = 0;
        m_GI->m_currentCol = col;

        if(GetKeyState(VK_CONTROL) <0){
            SelectRange(col,0,col,GetNumberRows()-1);
        }
        else if(GetKeyState(VK_SHIFT) <0){
            m_GI->m_multiSelect->EndBlock(col,GetNumberRows()-1);
        }
        else{
            ClearSelections();
            SelectRange(col,0,col,GetNumberRows()-1);
        }

        UpdateTableBlocking();

    }
}
/***************************************************
OnTH_RClicked
Sent whenever the user clicks the right mouse
button within the top heading
this message is sent when the button goes down
then again when the button goes up

  'col' is negative if the area clicked in is not valid
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CTblGrid::OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnTH_LClicked
Sent whenever the user double clicks the left mouse
button within the top heading

  'col' is negative if the area clicked in is not valid
****************************************************/
void CTblGrid::OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnSH_LClicked
Sent whenever the user clicks the left mouse
button within the side heading
this message is sent when the button goes down
then again when the button goes up

  'row' is negative if the area clicked in is not valid
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CTblGrid::OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){

    if(!updn){      // button just went up
        // force focus rect to leftmost cell
        m_GI->m_currentRow = row;
        m_GI->m_currentCol = 0;

        if(GetKeyState(VK_CONTROL) <0){

            SelectRange(0,row,GetNumberCols()-1,row);
        //  m_GI->m_multiSelect->ToggleCell(GetNumberCols()-1,row);


        }
        else if(GetKeyState(VK_SHIFT) <0){
            m_GI->m_multiSelect->EndBlock(GetNumberCols()-1,row);
        }
        else{
            ClearSelections();
            SelectRange(0,row,GetNumberCols()-1,row);
        }

        UpdateTableBlocking();

    }
/*  //Select the row using multi select
    if(TRUE) {
        if(updn) {


            //Select all the  cells in this row
            if( col <= 0 ) {

                CUGCell cell;
                GetCell(col, row, &cell);
                short type = cell.GetBorder();

                //if this button is recessed then the selection has been already made
                //So deselect the row  and remove recessed

                //Make the side heading cell depressed
                if( type & UG_BDR_RECESSED ) {
                    cell.SetBorder(UG_BDR_RAISED);
                    SetCell(col, -1, &cell);
                    m_CUGSideHdg->Invalidate();
                    m_CUGSideHdg->SendMessage(WM_PAINT,0,0);
                    //Deselect the row cells
                    ClearSelection
                }

                else {



                    cell.SetBorder(UG_BDR_RECESSED);
                    SetCell(col, row, &cell);
                    m_CUGSideHdg->Invalidate();
                    m_CUGSideHdg->SendMessage(WM_PAINT,0,0);
                    SelectRange(0,row,GetNumberCols(),row);
                    RedrawRow(row);
                }




            }


        }

        else {

            //Set the button up and deselect the selected stuff

            updn = FALSE;


        }

    }*/

    TRACE(_T("The Col is %d , The Row is %d \n"),col,row);
    TRACE(_T("The point is (x,y) %d,%d \n"),point->x,point->y);
}
/***************************************************
OnSH_RClicked
Sent whenever the user clicks the right mouse
button within the side heading
this message is sent when the button goes down
then again when the button goes up

  'row' is negative if the area clicked in is not valid
  'updn'  TRUE if the button is down FALSE if the
  button just when up
****************************************************/
void CTblGrid::OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnSH_DClicked
Sent whenever the user double clicks the left mouse
button within the side heading

  'row' is negative if the area clicked in is not valid
****************************************************/
void CTblGrid::OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
    StartEdit(col,row,0);
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
void CTblGrid::OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed){

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
void CTblGrid::OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnCB_DClicked
Sent whenever the user double clicks the left mouse
button within the top corner button
****************************************************/
void CTblGrid::OnCB_DClicked(RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnKeyDown
Sent whenever the user types when the grid has
focus. The keystroke can be modified here as well.
(See WM_KEYDOWN for more information)
****************************************************/
void CTblGrid::OnKeyDown(UINT *vcKey,BOOL processed){

    if (*vcKey==VK_TAB) {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SETFOCUS);
    }
}
/***************************************************
OnCharDown
Sent whenever the user types when the grid has
focus. The keystroke can be modified here as well.
(See WM_CHAR for more information)
****************************************************/
void CTblGrid::OnCharDown(UINT *vcKey,BOOL processed){
}

void CTblGrid::OnPaint()
{
    CUGCtrl::OnPaint();
    //DrawSubTableLines();
    DrawSubTableRects(true);

}

void CTblGrid::Moved()
{
    CUGCtrl::Moved();
    DrawSubTableRects(true);

}
/***************************************************
OnGetCell
This message is sent everytime the grid needs to
draw a cell in the grid. At this point the cell
class has been filled with the information to be
used to draw the cell. The information can now be
changed before it is used for drawing
****************************************************/
void CTblGrid::OnGetCell(int col,long row,CUGCell *cell)
{
    if(!m_bGridUpdate || !IsDataCell(col,row) ){
        return; //we are not concerned about non-data cells
    }
    if(m_CUGGrid->m_drawHint.IsInvalid(col,row) == FALSE){
        //    TRACE("Cell need not be drawn col =%d , row = %d \n",col,row);
        return;
    }

    bool bDesignView = false;
    /*CTabView* pView = (CTabView*)GetParent();
    if(pView){
        pView ? bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView() : bDesignView = true;
    }*/
    bool bFound =false;


    if(!bDesignView){
        if(m_arrCells.GetSize() ==0){
            return;
        }

        CGTblCell* pGTblCell = GetGTblCell(col,row);
        if(!pGTblCell || !pGTblCell->IsDirty()){
            return;
        }
        //Is Cell Visible
        CRect rect(0,0,0,0);
        int  localCol,x;
        long y;
        CUGCell gridCell;

        long localRow;
/*
        int iLeftCol = GetLeftCol();
        int iRightCol = GetRightCol();
        int iTopRow  = GetTopRow();
        int iBottomRow = GetBottomRow();
        iBottomRow - iTopRow > 60 ? iBottomRow = iTopRow+60 :iBottomRow =iBottomRow ;
        iRightCol - iLeftCol > 40 ? iRightCol = iLeftCol+40 : iRightCol =iRightCol;

        bool bCellColVisible = col >= iLeftCol&& col <= iRightCol;
        bool bCellRowVisible = row >= iTopRow&& row <= iBottomRow;
*/

        for(y = 0; y < m_GI->m_numberRows;y++){
            if(bFound){
                break; //we got the rect for our cell;
            }
            //skip rows hidden under locked rows
            if(y == m_GI->m_numLockRows)
                y = m_GI->m_topRow;

            localRow = y;

            //calc the top bottom and right side of the rect
            //for the current cell to be drawn
            rect.top = rect.bottom;

            if(m_GI->m_uniformRowHeightFlag)
                rect.bottom += m_GI->m_defRowHeight;
            else
                rect.bottom += m_GI->m_rowHeights[localRow];

            if(rect.top == rect.bottom)
                continue;

            rect.right = 0;

            //check all visible cells in the current row to
            //see if they need drawing
            for(x = 0;x < m_GI->m_numberCols;x++){
                if(bFound){
                    break;
                }
                //skip cols hidden under locked cols
                if(x == m_GI->m_numLockCols)
                    x = m_GI->m_leftCol;

                localRow = y;
                localCol = x;

                //calc the left and right side of the rect
                rect.left = rect.right;
                rect.right  += m_GI->m_colInfo[localCol].width;

                if(rect.left == rect.right)
                    continue;


                //copy the rect, then use the cellRect from here
                //this is done since the cellRect may be modifiec
                //CopyRect(&cellRect,&rect);

                if(x == col && y == row) {
                    bFound =true;
                    //check to see if the rect is with in the grid rect
                   if(rect.right > m_GI->m_gridWidth || rect.bottom > m_GI->m_gridHeight){
                        //No need to compute format information
                    //    TRACE("Cell does not need format info col =%d , row = %d \n",col,row);
                    }
                    else{
                        //compute format information
                    //    TRACE("Cell needs  format info col =%d , row = %d \n",col,row);
                        void** pOb = (void**)cell->GetExtraMemPtr();
                        if(true){
                            //Is cell data cell
                            //If so get the fmt and apply to cell
                            //set it in the cellgrid
                            //Get the CGTblCell and set it
                            if(!pOb){
                                pOb = (void**)cell->AllocExtraMem(sizeof(LPVOID));
                                *pOb = pGTblCell;
                            }
                            //Apply format to data cell
                            ApplyFormat2DataCells2(col,row,cell);
                            SetCell(col,row,cell);

                            pGTblCell->SetDirty(false);
                            if(m_bFixLines){
                                FixLines(row);
                            }
                        }
                    }
                    break;
                }

            }
        }
    }
}
/***************************************************
OnSetCell
This message is sent everytime the a cell is about
to change.
****************************************************/
void CTblGrid::OnSetCell(int col,long row,CUGCell *cell){
}
/***************************************************
OnDataSourceNotify
This message is sent from a data source , message
depends on the data source - check the information
on the data source(s) being used
- The ID of the Data source is also returned
****************************************************/
void CTblGrid::OnDataSourceNotify(int ID,long msg,long param){
}
/***************************************************
OnCellTypeNotify
This message is sent from a cell type , message
depends on the cell type - check the information
on the cell type classes
- The ID of the cell type is given
****************************************************/
int CTblGrid::OnCellTypeNotify(long ID,int col,long row,long msg,long param){
    return 0;
}
/***************************************************
OnEditStart
This message is sent whenever the grid is ready
to start editing a cell
A return of TRUE allows the editing a return of
FALSE stops editing
Plus the properties of the CEdit class can be modified
****************************************************/
int CTblGrid::OnEditStart(int col, long row,CWnd **edit){
   bool bRet = IsValidCell4InlineEdit(col,row);
   if(bRet) {
        CUGCell cell;
        GetCell(col,row,&cell);

        // JH 9/22/06 remove spaces added for indentation before edit.  They get added back in after edit.
        // if they aren't removed here, then they get added in twice.
        std::unique_ptr<CFmt> pFmt = GetFmt4Cell(col, row);
        if(!pFmt.get()){
            return FALSE;
        }
        ASSERT(pFmt.get());
        CFont* pFont = pFmt->GetFont();
        if (pFont == nullptr) {
            // nullptr font means use the font from default fmt
            FMT_ID eGridComp = FMT_ID_INVALID;
            CGTblOb* pGTblOb = nullptr;
            GetComponent(col,row,eGridComp,&pGTblOb);
            ASSERT(eGridComp != FMT_ID_INVALID);
            CTabView* pView = (CTabView*)GetParent();
            CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
            const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(eGridComp));
            pFont = pFmt->GetFont();
            ASSERT(pFont != nullptr);
        }
        CIMSAString sText = QuickGetText(col, row);
        int iIndent = pFmt->GetIndent(LEFT);
        if(iIndent>0){
            iIndent = Twips2NumSpaces(iIndent, pFont);
            sText = sText.AdjustLenLeft(sText.GetLength() - iIndent);
        }
        iIndent = pFmt->GetIndent(RIGHT);
        if(iIndent>0){
            iIndent = Twips2NumSpaces(iIndent, pFmt->GetFont());
            sText = sText.AdjustLenRight(sText.GetLength() - iIndent);
        }
        m_editCell.SetText(sText);

/*      if(cell.GetBackColor() ==RGB(192,192,192) )
            return FALSE;*/
        *edit =&m_InlineEdit;
        return TRUE;
    }
    else {
        return FALSE;
    }
}
/***************************************************
OnEditVerify
This is send when the editing is about to end
****************************************************/
int CTblGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey){
    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CTblGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag){

    if( cancelFlag ) // 20130102 not sure why this wasn't handled
        return TRUE;

    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    CFmt* pFmt = nullptr;
    CDataCellFmt* pDataCellFmt = nullptr;
    CFmt* pFmtChanged = nullptr;
    CDataCellFmt* pDataCellFmtChanged = nullptr;
    CUSTOM custom;
    custom.m_bIsCustomized = true;
    CIMSAString sOriginal = QuickGetText(col,row);
    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    CFmtReg* pFmtReg = m_pTable->GetFmtRegPtr();

    bool bChanged = sOriginal.Compare(string)!=0;
    if(!bChanged){
        return TRUE;
    }
    else {
        pDoc->SetModifiedFlag(TRUE);
        GetComponent(col,row,eGridComp,&pGTblOb);
        switch(eGridComp){
        case FMT_ID_TITLE:
            if(pGTblOb){
                CTblOb* pTblOb = m_pTable->GetTitle();
                pTblOb->RemoveAllPrtViewInfo();
                CFmt* pFmtChanged = nullptr;
                if(pTblOb->GetFmt() && pTblOb->GetFmt()->GetIndex()!=0){
                    pFmtChanged = DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,pFmtReg->GetFmt(FMT_ID_TITLE));
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pFmtChanged,pFmt);
                    pFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pFmtChanged));

                    pFmtReg->AddFmt(pFmtChanged);
                    pTblOb->SetFmt(pFmtChanged);
                }
                custom.m_sCustomText = string;
                pFmtChanged->SetCustom(custom);
            }
            break;
        case FMT_ID_AREA_CAPTION:
            if(pGTblOb){
                CTblOb* pTblOb = m_pTable->GetAreaCaption();
                pTblOb->RemoveAllPrtViewInfo();
                CFmt* pFmtChanged = nullptr;
                if(pTblOb->GetFmt() && pTblOb->GetFmt()->GetIndex()!=0){
                    pFmtChanged = DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,pFmtReg->GetFmt(FMT_ID_AREA_CAPTION));
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pFmtChanged,pFmt);
                    pFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pFmtChanged));

                    pFmtReg->AddFmt(pFmtChanged);
                    pTblOb->SetFmt(pFmtChanged);
                }
                custom.m_sCustomText = string;
                pFmtChanged->SetCustom(custom);
            }
            break;
        case FMT_ID_SUBTITLE:
            if(pGTblOb){
                CTblOb* pTblOb = m_pTable->GetSubTitle();
                pTblOb->RemoveAllPrtViewInfo();
                CFmt* pFmtChanged = nullptr;
                if(pTblOb->GetFmt() && pTblOb->GetFmt()->GetIndex()!=0){
                    pFmtChanged = DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,pFmtReg->GetFmt(FMT_ID_SUBTITLE));
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pFmtChanged,pFmt);
                    pFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pFmtChanged));

                    pFmtReg->AddFmt(pFmtChanged);
                    pTblOb->SetFmt(pFmtChanged);
                }
                custom.m_sCustomText = string;
                pFmtChanged->SetCustom(custom);
            }
            break;
        case FMT_ID_STUBHEAD:
            if(pGTblOb){
                CTblOb* pTblOb = m_pTable->GetStubhead(0);
                pTblOb->RemoveAllPrtViewInfo();
                CFmt* pFmtChanged = nullptr;
                if(pTblOb->GetFmt() && pTblOb->GetFmt()->GetIndex()!=0){
                    pFmtChanged = DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,pFmtReg->GetFmt(FMT_ID_STUBHEAD));
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pFmtChanged,pFmt);
                    pFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pFmtChanged));

                    pFmtReg->AddFmt(pFmtChanged);
                    pTblOb->SetFmt(pFmtChanged);
                }
                custom.m_sCustomText = string;
                pFmtChanged->SetCustom(custom);
            }
            break;
        case FMT_ID_SPANNER:
            if(pGTblOb){
                CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                ASSERT(pCol);
                CTabVar* pTabVar = pCol->GetTabVar();
                CTabValue* pTabVal = pCol->GetTabVal();
                ASSERT(pTabVar);
                pTabVar->RemoveAllPrtViewInfo();
                pTabVal ? pDataCellFmt = pTabVal->GetDerFmt(): pDataCellFmt =pTabVar->GetDerFmt();
                if(pDataCellFmt && pDataCellFmt->GetIndex()!=0){
                    pDataCellFmtChanged =pDataCellFmt;
                }
                else {
                    pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmtReg->GetFmt(FMT_ID_SPANNER));
                    pDataCellFmtChanged = new CDataCellFmt(*pDataCellFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pDataCellFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pDataCellFmtChanged,pDataCellFmt);
                    pDataCellFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pDataCellFmtChanged));
                    pFmtReg->AddFmt(pDataCellFmtChanged);
                    pTabVal ? pTabVal->SetFmt(pDataCellFmtChanged) : pTabVar->SetFmt(pDataCellFmtChanged) ;
                }
                ASSERT(pDataCellFmtChanged);
                custom.m_sCustomText = string;
                pDataCellFmtChanged->SetCustom(custom);
            }
            break;
        case FMT_ID_COLHEAD:
            if(pGTblOb){
                CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                ASSERT(pCol);
                CTabValue* pTabVal = pCol->GetTabVal();
                ASSERT(pTabVal);
                pTabVal->RemoveAllPrtViewInfo();
                CDataCellFmt* pFmtChanged = nullptr;
                CDataCellFmt* pFmt = nullptr;
                if(pTabVal->GetFmt() && pTabVal->GetFmt()->GetIndex()!=0){
                    pFmtChanged = DYNAMIC_DOWNCAST(CDataCellFmt,pTabVal->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmtReg->GetFmt(FMT_ID_COLHEAD));
                    pFmtChanged = new CDataCellFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pFmtChanged,pFmt);
                    pFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pFmtChanged));

                    pFmtReg->AddFmt(pFmtChanged);
                    pTabVal->SetFmt(pFmtChanged);
                }
                ASSERT(pFmtChanged);
                custom.m_sCustomText = string;
                pFmtChanged->SetCustom(custom);
            }
            break;
        case FMT_ID_CAPTION:
            if(pGTblOb){
                CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                ASSERT(pRow);
                CTabVar* pTabVar = pRow->GetTabVar();
                CTabValue* pTabVal =pRow->GetTabVal();
                pTabVal ? pDataCellFmt =pTabVal->GetDerFmt() : pDataCellFmt =pTabVar->GetDerFmt();
                ASSERT(pTabVar);
                pTabVar->RemoveAllPrtViewInfo();
                if(pDataCellFmt && pDataCellFmt->GetIndex()!=0){
                    pDataCellFmtChanged = pDataCellFmt;
                }
                else {
                    pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmtReg->GetFmt(FMT_ID_CAPTION));
                    pDataCellFmtChanged = new CDataCellFmt(*pDataCellFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pDataCellFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pDataCellFmtChanged,pDataCellFmt);
                    pDataCellFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pDataCellFmtChanged));

                    pFmtReg->AddFmt(pDataCellFmtChanged);
                    pTabVal ? pTabVal->SetFmt(pDataCellFmtChanged) : pTabVar->SetFmt(pDataCellFmtChanged) ;
                }
                ASSERT(pDataCellFmtChanged);
                custom.m_sCustomText = string;
                pDataCellFmtChanged->SetCustom(custom);
            }
            break;
        case FMT_ID_STUB:
            if(pGTblOb){
                CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                ASSERT(pRow);
                CTabValue* pTabVal = pRow->GetTabVal();
                ASSERT(pTabVal);
                pTabVal->RemoveAllPrtViewInfo();
                CDataCellFmt* pFmt = nullptr;
                CDataCellFmt* pFmtChanged = nullptr;
                if(pTabVal->GetFmt() && pTabVal->GetFmt()->GetIndex()!=0){
                    pFmtChanged = DYNAMIC_DOWNCAST(CDataCellFmt,pTabVal->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmtReg->GetFmt(FMT_ID_STUB));
                    pFmtChanged = new CDataCellFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pFmtChanged,pFmt);
                    pFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pFmtChanged));

                    pFmtReg->AddFmt(pFmtChanged);
                    pTabVal->SetFmt(pFmtChanged);
                }
                ASSERT(pFmtChanged);
                custom.m_sCustomText = string;
                pFmtChanged->SetCustom(custom);
            }
            break;
        case FMT_ID_DATACELL:
            if(pGTblOb){
                CGTblCell* pGTblCell = DYNAMIC_DOWNCAST(CGTblCell,pGTblOb);
                ASSERT(pGTblCell);
                CGTblRow* pGTblRow = pGTblCell->GetTblRow();
                ASSERT(pGTblRow);

                CTabValue* pTabVal = pGTblRow->GetTabVal();

                CDataCellFmt* pFmt = nullptr;
                CDataCellFmt* pFmtChanged = nullptr;
                if(pTabVal){
                    //find if we have special cells .else make one
                    CSpecialCell* pSpecialCell = nullptr;
                    int iPanel = GetRowPanel(row);
                    if(iPanel > 0 ){
                        pSpecialCell = pTabVal->FindSpecialCell(iPanel,col);
                    }
                    if(pSpecialCell){
                        pFmtChanged = pSpecialCell->GetDerFmt();
                        ASSERT(pFmtChanged); // Special cells must have special fmt
                    }
                    else {
                        CArray<CSpecialCell, CSpecialCell&>& arrSpecials = pTabVal->GetSpecialCellArr();
                        CSpecialCell specialCell(iPanel,col);
                        pFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmtReg->GetFmt(FMT_ID_DATACELL));
                        pFmtChanged = new CDataCellFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                        pFmtChanged->Init();
                        pView->MakeDefFmt4Dlg(pFmtChanged,pFmt);
                        pFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pFmtChanged));
                        specialCell.SetFmt(pFmtChanged);

                        pFmtReg->AddFmt(pFmtChanged);
                        arrSpecials.Add(specialCell);
                    }
                }

                ASSERT(pFmtChanged);
                custom.m_sCustomText = string;
                pFmtChanged->SetCustom(custom);

            }
            //AfxMessageBox("Modifying Cells");
            break;
        case FMT_ID_PAGENOTE:
            if(pGTblOb){
                CTblOb* pTblOb = m_pTable->GetPageNote();
                CFmt* pFmtChanged = nullptr;
                pTblOb->RemoveAllPrtViewInfo();
                if(pTblOb->GetFmt() && pTblOb->GetFmt()->GetIndex()!=0){
                    pFmtChanged = DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,pFmtReg->GetFmt(FMT_ID_PAGENOTE));
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pFmtChanged,pFmt);
                    pFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pFmtChanged));

                    pFmtReg->AddFmt(pFmtChanged);
                    pTblOb->SetFmt(pFmtChanged);
                }
                custom.m_sCustomText = string;
                pFmtChanged->SetCustom(custom);
            }
            break;
        case FMT_ID_ENDNOTE:
            if(pGTblOb){
                CTblOb* pTblOb = m_pTable->GetEndNote();
                CFmt* pFmtChanged = nullptr;
                pTblOb->RemoveAllPrtViewInfo();
                if(pTblOb->GetFmt() && pTblOb->GetFmt()->GetIndex()!=0){
                    pFmtChanged = DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,pFmtReg->GetFmt(FMT_ID_ENDNOTE));
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                    pFmtChanged->Init();
                    pView->MakeDefFmt4Dlg(pFmtChanged,pFmt);
                    pFmtChanged->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pFmtChanged));

                    pFmtReg->AddFmt(pFmtChanged);
                    pTblOb->SetFmt(pFmtChanged);
                }
                custom.m_sCustomText = string;
                pFmtChanged->SetCustom(custom);
            }
            break;
        default:
            break;
        }
        //Begin recompute the cell width
        CUGCell cell;
        GetCell(col,row,&cell);
        CRect rcDraw(0,0,GetColWidth(col),0);
        //int offset =0;
        short alignment = cell.GetAlignment();
        if(cell.GetPropertyFlags() &UGCELL_ALIGNMENT_SET){
            alignment = cell.GetAlignment();
        }
        else{
            alignment = 0;
        }

        CDC dcMem;
        CBitmap bmp;  // for use with mem DC
        CDC* pDC = GetDC();
        int iSavedDC = pDC->SaveDC();
        bmp.CreateCompatibleBitmap(pDC, m_GI->m_gridWidth,m_GI->m_gridHeight);
        dcMem.CreateCompatibleDC(pDC);
        CBitmap* pOldBmp = dcMem.SelectObject(&bmp);

        CFont*pFont = cell.GetFont();
        if(pFont){
            dcMem.SelectObject(pFont);
        }
        UINT format = DT_WORDBREAK | DT_NOPREFIX;
        // check alignment - multiline
        if(alignment) {
            if(alignment & UG_ALIGNCENTER) {
                format |= DT_CENTER;
            }
            else if(alignment & UG_ALIGNRIGHT) {
                format |= DT_RIGHT;
                rcDraw.right -= 2;
            }
            else if(alignment & UG_ALIGNLEFT) {
                format |= DT_LEFT;
                rcDraw.left += 2;
            }

        }
        rcDraw.top =0;
        dcMem.DrawText(string, &rcDraw, format|DT_CALCRECT);
        if(rcDraw.Height() > GetRowHeight(row)){
            SetRowHeight(row,rcDraw.Height());
        }
        if(rcDraw.Height() > GetColWidth(col)){
            SetColWidth(col,rcDraw.Width());
        }
        SaveStateInfo(); //once it is set save state info for update later

        // nuke our special DC
        dcMem.SelectStockObject(OEM_FIXED_FONT);
        dcMem.SelectObject(pOldBmp);
        dcMem.DeleteDC();
        bmp.DeleteObject();
        pDC->RestoreDC(iSavedDC);
        //End compute cell width
    }
    this->PostMessage(UWM::Table::Update);
    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CTblGrid::OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow){
    //return TRUE;
    return FALSE;
}
/***************************************************
sections - UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
****************************************************/
void CTblGrid::OnMenuCommand(int col,long row,int section,int item){
}
/***************************************************
return UG_SUCCESS to allow the menu to appear
return 1 to not allow the menu to appear
****************************************************/
int CTblGrid::OnMenuStart(int col,long row,int section){
    return TRUE;
}
/***************************************************
OnHint
****************************************************/
int CTblGrid::OnHint(int col,long row,int section,CString *string){
    string->Format(_T("Col:%d Row:%ld"),col,row);
    return TRUE;
}
/***************************************************
OnVScrollHint
****************************************************/
int CTblGrid::OnVScrollHint(long row,CString *string){
    return TRUE;
}
/***************************************************
OnHScrollHint
****************************************************/
int CTblGrid::OnHScrollHint(int col,CString *string){
    return TRUE;
}

/*********************************************
OLE
**********************************************/
#ifdef __AFXOLE_H__

/***************************************************
****************************************************/
DROPEFFECT  CTblGrid::OnDragEnter(COleDataObject* pDataObject){
    oldPoint = CPoint(-1,-1);
    m_bGridVarDrop = true;
    return DROPEFFECT_MOVE;
}
void CTblGrid::OnDragLeave(CWnd* pWnd)
{
    m_bGridVarDrop = false;
    DrawBitMap();
}
/***************************************************
****************************************************/
DROPEFFECT  CTblGrid::OnDragOver(COleDataObject* pDataObject,int col,long row){

    if (IsValidDragDrop(col, row)) {
        DrawBitMap(true);
        return DROPEFFECT_MOVE;
    }
    else {
        DrawBitMap(true);
        return DROPEFFECT_NONE;
    }
}
/***************************************************
****************************************************/
DROPEFFECT  CTblGrid::OnDragDrop(COleDataObject* pDataObject,int col,long row){
    if(true){
       DrawBitMap();
    }
    if(!m_pDragSourceItem){
        return DROPEFFECT_NONE;
    }
    m_bGridVarDrop = true;

    CTabView* pView = (CTabView*)GetParent();
    DictTreeNode* dict_tree_node = nullptr;

    CPoint point;
    ::GetCursorPos(&point);

    pView->SendMessage(WM_IMSA_DROPITEM, MAKEWPARAM(point.x, point.y), reinterpret_cast<LPARAM>(dict_tree_node));

    m_bGridVarDrop = false;
    return DROPEFFECT_NONE;
}
DROPEFFECT CTblGrid::OnDragScroll(CWnd* pWnd, DWORD dwKeyState, CPoint point)
{
    //if pWnd is kind of CView, we let COleDropTarget to handle it
/*  if (pWnd->IsKindOf(RUNTIME_CLASS(CView))) We are not working with CView.
        return COleDropTarget::OnDragScroll(pWnd,dwKeyState,point);*/

/*   if (!ms_bBeginDrop) //SAVY put here to  check if drag is not being done
        return DROPEFFECT_NONE;*/

    CRect rectClient;
    m_CUGGrid->GetClientRect(&rectClient);
    CRect rect = rectClient;
    //nScrollInset is a COleDropTarget's static member variable
    //rect.InflateRect(-nScrollInset, -nScrollInset);
    int iRet = __max(m_GI->m_vScrollWidth,m_GI->m_hScrollHeight);

    int nScrollInset = 2*iRet;
    rect.InflateRect(-nScrollInset, -nScrollInset);
    // hit-test against inset region
    if (rectClient.PtInRect(point) && !rect.PtInRect(point)) {
        UINT        uMsg;
        int         nCode;
        CScrollBar* pScrollBar=nullptr;
        // determine which way to scroll along both X & Y axis
        if (point.x<rect.left) {
            uMsg=WM_HSCROLL;
            nCode=SB_LINELEFT;
        }
        else if (point.x>=rect.right) {
            uMsg=WM_HSCROLL;
            nCode=SB_LINERIGHT;
        }
        if (point.y<rect.top) {
            uMsg=WM_VSCROLL;
            nCode=SB_LINEUP;
        }
        else if (point.y>=rect.bottom) {
            uMsg=WM_VSCROLL;
            nCode=SB_LINEDOWN;
        }
        if(m_GI->m_vScrollWidth != 0 && uMsg==WM_VSCROLL){// Vertical scroll is  present
           DrawBitMap();
           OnVScroll(nCode, 0, nullptr);
           DrawBitMap(true);
        }
        if(m_GI->m_hScrollHeight != 0 && uMsg==WM_HSCROLL){// horizontal scroll is  present
           DrawBitMap();
           OnHScroll(nCode,0, nullptr);
           DrawBitMap(true);
        }

        if (dwKeyState & MK_CONTROL)
            return DROPEFFECT_SCROLL | DROPEFFECT_COPY;
        else
            return DROPEFFECT_SCROLL | DROPEFFECT_MOVE;
    }
    return DROPEFFECT_NONE;
}
/***************************************************
****************************************************/
//SAVY&&& Fix the prototype Or Remove this function and use the view's validation function
BOOL CTblGrid::IsValidDragDrop(int col,long row){

    if (m_pTable == nullptr) {
        // no table attached to the grid
        return FALSE;
    }
    CTabView* pTabView = (CTabView*)GetParent();
    CPoint point;
    ::GetCursorPos(&point);
    return pTabView->IsDropPointValid(point);

    if (row < m_iGridHeaderRows && col == 0) {
        return FALSE;
    }
    // no, if on $AreaName$ cell (area processing)
    //&&& worry about levels later
    /*if (m_pTable->GetNumAreaLevels() > 0) {
        if (row == m_iGridHeaderRows && col == 0) {
            return FALSE;
        }
    }*/
    if (row < m_iGridHeaderRows || col == 0) {
        return TRUE;
    }
    return FALSE;

}

/////////////////////////////////////////////////
// trying to determine if there is anything on our
// table, i.e., no dict items in rows or cols

bool CTblGrid::IsGridEmpty()
{
    if (GetNumGridRows() == 1)
        return true;
    else
        return false;
}


/***************************************************
****************************************************/
#endif
void CTblGrid::OnScreenDCSetup(CDC *dc,int section){
}
/***************************************************
OnSortEvaluate
return      -1  <
0   ==
1   >
****************************************************/
int CTblGrid::OnSortEvaluate(CUGCell *cell1,CUGCell *cell2,int flags){

    if(flags&UG_SORT_DESCENDING){
        CUGCell *ptr = cell1;
        cell1 = cell2;
        cell2 = ptr;
    }

    //  if(cell1->IsPropertySet(UGCELL_TEXT_SET) == FALSE){
    //      if(cell2->IsPropertySet(UGCELL_TEXT_SET) == FALSE)
    //          return 0;
    //      return -1;
    //  }
    switch(cell1->GetDataType()){

    case UGCELLDATA_STRING:
        if(nullptr == cell1->GetText() && nullptr == cell2->GetText())
            return 0;
        if(nullptr == cell1->GetText())
            return 1;
        if(nullptr == cell2->GetText())
            return -1;
        return _tcscmp(cell1->GetText(),cell2->GetText());
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
    if(nullptr == cell1->GetText())
        return 1;
    if(nullptr == cell2->GetText())
        return -1;
    return _tcscmp(cell1->GetText(),cell2->GetText());
}


/***************************************************
OnAdjustComponentSizes
****************************************************/
void CTblGrid::OnAdjustComponentSizes(RECT *grid,RECT *topHdg,RECT *sideHdg,
                                      RECT *cnrBtn,RECT *vScroll,RECT *hScroll,RECT *tabs){
}

/***************************************************
OnDrawFocusRect
****************************************************/
void CTblGrid::OnDrawFocusRect(CDC *dc,RECT *rect){

    //  DrawExcelFocusRect(dc,rect);

    //  rect->bottom --;
    //  rect->right --;
    //  dc->DrawFocusRect(rect);

    rect->bottom --;
    dc->DrawFocusRect(rect);
    rect->left++;
    rect->top++;
    rect->right--;
    rect->bottom--;
    dc->DrawFocusRect(rect);

}

/***************************************************
OnGetDefBackColor
****************************************************/
COLORREF CTblGrid::OnGetDefBackColor(int section){
    return RGB(255,255,255);
}
/***************************************************
OnSetFocus
****************************************************/
void CTblGrid::OnSetFocus(int section){
}

/***************************************************
OnKillFocus
****************************************************/
void CTblGrid::OnKillFocus(int section){
}
/***************************************************
OnColSwapStart
****************************************************/
BOOL CTblGrid::OnColSwapStart(int col){

    return TRUE;
}

/***************************************************
OnCanColSwap
****************************************************/
BOOL CTblGrid::OnCanColSwap(int fromCol,int toCol){

    return TRUE;
}
/***************************************************
OnTrackingWindowMoved
****************************************************/
void CTblGrid::OnTrackingWindowMoved(RECT *origRect,RECT *newRect){
}


int CTblGrid::DragDropTarget(BOOL state){

    if(state == FALSE){
        m_dropTarget.Revoke();
    }
    else{
        m_dropTarget.Register(this->GetParent());
    }
    return UG_SUCCESS;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  int CTblGrid::StartDragDrop(int iRow , int iCol)
//
/////////////////////////////////////////////////////////////////////////////////
int CTblGrid::StartDragDrop(int iRow , int iCol)
{
    m_pDragSourceItem = nullptr; //intialise
    m_bGridVarDrop = false;
    ReleaseCapture();

    CString sString  = GetRowColString(iRow,iCol);


    int iSourceRow = 0;
    int iSourceCol= 0;
    int iFound = sString.Find(_T("-"));
    CTabVar* pTItem = nullptr;
    CTabVar* pSItem = nullptr;

    if(iCol != 0) {
        CUGCell cellGrid;
        GetCell(iCol,iRow,&cellGrid);

        CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
            m_pDragSourceItem = ((CGTblCol*)(*pTblOb))->GetTabVar();
        }
    }
    else {
        CUGCell cellGrid;
        GetCell(0,iRow,&cellGrid);
        CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
           m_pDragSourceItem = ((CGTblRow*)(*pTblOb))->GetTabVar();
        }
    }


    int len = sString.GetLength();
    HGLOBAL hglobal = GlobalAlloc(GMEM_ZEROINIT,sizeof(TCHAR)*(len+1));
    LPTSTR string = (LPTSTR)GlobalLock(hglobal);
    lstrcpy(string,sString);
    GlobalUnlock(hglobal);

    m_dataSource.CacheGlobalData(CF_UNICODETEXT,hglobal,nullptr);

    m_dataSource.DoDragDrop(DROPEFFECT_COPY, nullptr, std::make_unique<TabDropSource>().get());

    m_dataSource.Empty();
    m_CUGGrid->RedrawWindow(); //Try and remove this SAVY sep26 2003
    return UG_SUCCESS;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  CString CTblGrid::GetRowColString(int iRow, int iCol)
//
/////////////////////////////////////////////////////////////////////////////////
CString CTblGrid::GetRowColString(int iRow, int iCol)
{
    if( iCol != 0 )
        return IntToString(iRow) + _T("-") + IntToString(iCol);

    else
        return IntToString(iRow);
}

/////////////////////////////////////////////////////////////////////////////
// reset the grid
void CTblGrid::ResetGrid() {

    //SetRedraw(FALSE);
    LockColumns(0);
    LockRows(0);
    SetNumberCols(0);
    SetNumberRows(0);
    GetDataSource(0)->Empty();
    //this->m_GI->m_defDataSource->Empty();

     /*//Grid re initialise for the Tblob
    // Make sure that the extramem is cleared
    int iNumCols = GetNumberCols();
    long lNumRows = GetNumberRows();
    for( int iColIndex =0 ; iColIndex <iNumCols ; iColIndex++) {
        for( long lRowIndex =0 ; lRowIndex < lNumRows ; lRowIndex++) {
            CUGCell cellGrid ;
            GetCell(iColIndex,lRowIndex,&cellGrid);
            cellGrid.SetText("");
            CTblOb** pOb = (CTblOb**)cellGrid.GetExtraMemPtr();
            cellGrid.ClearExtraMem();
            SetCell(iColIndex,lRowIndex,&cellGrid);

        }
    }

    int iNumberofCols = GetNumberCols() ;
    for(int iIndex = 0; iIndex < iNumberofCols ; iIndex++)
    {
        DeleteCol(0);
    }

    int iNumberofRows  = GetNumberRows() ;
    for( iIndex = 0; iIndex <iNumberofRows; iIndex++)
    {
        DeleteRow(0);
    }
    */
    DefaultGrid();

   //SetRedraw(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTblGrid::DoFlip(COleDataObject* pDataObject,int col,long row)
//
/////////////////////////////////////////////////////////////////////////////////
DROPEFFECT  CTblGrid::DoFlip(COleDataObject* pDataObject,int col,long row)
{

    HGLOBAL hGlobal=pDataObject->GetGlobalData(CF_TEXT);
    CString sString =(TCHAR*)GlobalLock(hGlobal);
    // Unlock memory
    GlobalUnlock(hGlobal);
    if(sString.IsEmpty())
        return DROPEFFECT_NONE;

    int iSourceRow = 0;
    int iSourceCol= 0;
    int iFound = sString.Find(_T("-"));
    CTabVar* pTItem = nullptr;
    CTabVar* pSItem = nullptr;

    if (iFound != -1) {
        CString sRow = sString.Left(iFound);
        CString sCol = sString.Right(sString.GetLength()-iFound-1);
        iSourceRow = _ttoi(sRow);
        iSourceCol= _ttoi(sCol);

        CUGCell cellGrid;
        GetCell(iSourceCol,iSourceRow,&cellGrid);

        CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
            pSItem = ((CGTblCol*)(*pTblOb))->GetTabVar();
        }
    }
    else {
        //Get the variable and do a flip
        iSourceRow = _ttoi(sString);
        pSItem  = GetRowVar(iSourceRow);
    }



    //Do you consider the drop to be on Col /Row
    BOOL bDropRow = FALSE;

    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();


    if(col == 0 ) {
        //From the row number and determine the row variable
        bDropRow = TRUE;
        pTItem = GetRowVar(row);
    }
    else {
        if(IsValidDragDrop(col,row)) {
            CUGCell cellGrid;
            GetCell(col,row,&cellGrid);
            CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
            if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
                pTItem = ((CGTblCol*)(*pTblOb))->GetTabVar();
            }
            bDropRow = FALSE;
        }
        else {
            return DROPEFFECT_NONE;
        }
    }


    //if the  drop is on the row
    if(bDropRow) {
        //Get the origin
        if(col == 0 && iSourceCol !=0) {
            //we have a column being dropped on the row
            //Col ----> Row
            if(m_pTable->GetRowRoot()->GetNumChildren() == 0 ){
                return DROPEFFECT_NONE;
            }
            //SAVY &&& TO DO
            //if the target row is the lastrow + 1 then it is a "+"
            bool bLastRow = false;
            if(pSItem && bLastRow) {//SAVY&&& TO DO
                pTItem->Remove();
                pSItem->InsertSibling(pTItem);//determine after / before
            }
            else if(pSItem){
                //Remove the source item from its current position with out deleting it
                pSItem->Remove();
                //Add the source item tot the target item as its child
                if(!pTItem) {
                    if(m_pTable->GetRowRoot()->GetNumChildren() == 0 ) {
                        m_pTable->GetRowRoot()->AddChildVar(pSItem);
                    }
                }
                else if(pTItem->GetParent() == m_pTable->GetRowRoot() ) {//Add Child
                    pTItem->AddChildVar(pSItem);
                }
                else {//Add sibling
                    pTItem->InsertSibling(pSItem);//determine after / before
                }
            }
            m_bReconcileSpecialCells = true;
            return DROPEFFECT_COPY;

        }
        else {
        //the drop is from Row ---> Row
        if(m_pTable->GetRowRoot()->GetNumChildren() ==0  ) {
            return DROPEFFECT_NONE;
        }
        else {
            //Get the variable and do a flip
            int iSRow = _ttoi(sString);
            CTabVar* pSItem = GetRowVar(iSRow); //Source item
            if(pSItem == pTItem) {
                return DROPEFFECT_NONE;
            }
            //SAVY &&& TO DO
            //if the target row is the lastrow + 1 then it is a "+"
            bool bLastRow = false;
            if(pSItem && bLastRow) {//SAVY&&& TO DO
                //Remove the target item from its current position with out deleting it
                pTItem->Remove();
                pSItem->InsertSibling(pTItem);
            }
            else if(pTItem && pSItem){
                //Remove the source item from its current position with out deleting it
                 pSItem->Remove();
                //Add the source item tot the target item as its child
                if(pTItem->GetParent() == m_pTable->GetRowRoot() ) {//Add Child
                    pTItem->AddChildVar(pSItem);
                }
                else {//Add sibling
                    pTItem->InsertSibling(pSItem);
                }
            }
            return DROPEFFECT_COPY;
        }

        }

    }
    else { // the drop is on Col
        //Get the origin
        if(iSourceCol != 0 && col !=0) {
            //the drop is from Col ---> Col
            if(m_pTable->GetColRoot()->GetNumChildren() == 0 ) {
                return DROPEFFECT_NONE;
            }
            else {
                //Get the variable and do a flip
                if(pSItem == pTItem) {
                    return DROPEFFECT_NONE;
                }
                //SAVY &&& TO DO
                //if the target row is the lastrow + 1 then it is a "+"
                bool bLastCol = false;
                if(pTItem && pSItem && bLastCol) {//SAVY&&& TO DO
                    //Remove the target item from its current position with out deleting it
                    pTItem->Remove();
                    pSItem->InsertSibling(pTItem); //got to determine berfore /after
                }
                else if(pSItem && pTItem){
                    //Remove the source item from its current position with out deleting it
                    pSItem->Remove();
                    //Add the source item tot the target item as its child
                    if(pTItem->GetParent() == m_pTable->GetColRoot() ) {//Add Child
                        pTItem->AddChildVar(pSItem);
                    }
                    else {//Add sibling
                        pTItem->InsertSibling(pSItem);
                    }
                }
                m_bReconcileSpecialCells = true;
                return DROPEFFECT_COPY;
            }

        }
        else {
            //the drop is from Row ---> Col
            if(m_pTable->GetRowRoot()->GetNumChildren() == 0 ) {
                return DROPEFFECT_NONE;
            }
            else {
                //Get the variable and do a flip
                if(pSItem == pTItem) {
                    return DROPEFFECT_NONE;
                }
                //SAVY &&& TO DO
                //if the target row is the lastrow + 1 then it is a "+"
                bool bLastCol = false;
                if(pSItem && bLastCol) {//SAVY&&& TO DO
                    //Remove the target item from its current position with out deleting it
                    pTItem->Remove();
                    pSItem->InsertSibling(pTItem);
                }
                else if(pSItem){
                    //Remove the source item from its current position with out deleting it
                    pSItem->Remove();
                    //Add the source item tot the target item as its child
                    if(!pTItem ) {
                        if(m_pTable->GetColRoot()->GetNumChildren() == 0) {
                            m_pTable->GetColRoot()->AddChildVar(pSItem);
                        }
                    }
                    else if(pTItem->GetParent() == m_pTable->GetColRoot() ) {//Add Child
                        pTItem->AddChildVar(pSItem);
                    }
                    else {//Add sibling
                        pTItem->InsertSibling(pSItem);
                    }
                }
                m_bReconcileSpecialCells = true;
                return DROPEFFECT_COPY;
            }

        }

    }

    return DROPEFFECT_NONE;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::Update(bool bOnlyData /*=false*/)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::Update(bool bOnlyData /*=false*/)
{
    CWaitCursor wait;

    CTabView* pView = (CTabView*)GetParent();

    // notify the frame holding this grid that the table grid is being updated
    if( pView != nullptr ) {
        pView->GetParentFrame()->PostMessage(UWM::Table::TableGridUpdated);
    }

    if(m_pTable == nullptr){
        ResetGrid();
        return;
    }

    bool bDesignView;
    pView ? bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView() : bDesignView = true;

    m_bGridUpdate = false;

    if(!bOnlyData){
        DestroyCells();
        if(pView) {
            bool bViewer = ((CTableChildWnd*)pView->GetParentFrame())->IsViewer();
            if(!bViewer){
                CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
                CTabSet* pTabSet = pDoc->GetTableSpec();
                const CDataDict* pDict = pTabSet->GetDict();
                m_pTable->ReconcileTabVals(m_pTable->GetRowRoot(),pDict,pTabSet->GetWorkDict());
                m_pTable->ReconcileTabVals(m_pTable->GetColRoot(),pDict,pTabSet->GetWorkDict());
            }
        }

        ResetGrid(); //Get the grid back to default

        SetupGrid(); //Do the tbl spec and new row /col groups
        FixAreaTokensInOneRowTable();

        DrawGridTitle(); //Set up title

        DrawGridCols(m_pTblColRoot,0); //Set up cols
        m_arrGTblRows.RemoveAll();
        DrawGridRows(m_pTblRowRoot,0); //Set up rows

        HandleAStarBPlusCModeDisplay();
        if(m_bReconcileSpecialCells){
            ReconcileTabValFmts();//First set the formats right
            ReconcileSpecialCells();//Then do reconcile of the special cells
            m_bReconcileSpecialCells=false;
        }

        DrawPageOrEndNote();
        ApplyFormat2SpannersNColHeads();
        ApplyFormat2StubCol();

        ProcessCells(); //Allocate cells to the table


        DrawGridStubHead(); //&&& Do Stub head

        ApplyGridColsJoins(m_pTblColRoot,0); //Set up cols

        if(pView) {
            CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();

            POSITION pos = pDoc->GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CTabView* pView = (CTabView*)pDoc->GetNextView(pos);

            //  Open the dictionary, if not already open
            bool bViewer = ((CTableChildWnd*)pView->GetParentFrame())->IsViewer();
            if(true) { //for now do for all  . If speed problems then do it on a need basis
                UpdateData();
            }
        }
    }
    else {
        UpdateData();
    }

    ClearAllRunTimeFmts();

    BuildAllRunTimeFmts();

    m_bGridUpdate = true;
    for(int iCell =0; iCell < m_arrCells.GetSize(); iCell++){
        m_arrCells[iCell]->SetDirty(true);
    }
    //formatting for datacells comes through ongetcell .
    /*if(bDesignView){
        ApplyFormat2DataCells(); //Apply Format to data cells when in design view
    }*/
    if(HasArea()){
        DrawLines4AreaCaptionRows();
    }
    m_bFixLines = true;
    FixLines(); //&&& mnage lines
    m_bFixLines = false;
  //  DrawSubTableLines();
    DrawSubTableRects(true);
    ResetSizes();
    ProcessHideSpanners(true);

    if(IsOneRowVarTable() && HasArea()){
        ProcessHideCaptions();
    }
    if(!bDesignView){
        ProcessHideCaptions();
        ProcessHideSpanners();
        ProcessHideStubs();
        ProcessHideColHeads();
        ProcessAreaTokensinRows();
    }
    if(m_pTable->m_bHasFreqStats){//redraw pagenote to set the height correctly
        //now resetsizes would set the iTotalwidth correctly
        DrawPageOrEndNote();
    }
   // LockColumns(1);
    LockRows(this->m_iGridHeaderRows);
    ClearSelections();
 //   SetRedraw(TRUE);
    CRect rect;
    pView->GetClientRect(&rect);
    //DrawSubTableLines();
    DrawTblBorders();
    MoveWindow(&rect);
    AdjustComponentSizes();
    m_CUGGrid->RedrawWindow();
    DrawSubTableRects(true);
}

/////////////////////////////////////////////////////////////////////////////
//
//                           CTblGrid::DrawGridTitle
//
/////////////////////////////////////////////////////////////////////////////

void CTblGrid::DrawGridTitle ()
{
    int iCols = GetNumberCols();

    if(iCols > 1) {
        JoinCells(0,0,iCols-1,0);
    }
    QuickSetText(0,0,m_pTable->GetTitleText());

    //Set the pTbloB for the Title Cell
    CUGCell cellGrid;
    GetCell(0,0,&cellGrid);
    m_GTitle.SetTblOb(m_pTable->GetTitle());
    void** pTblOb = (void**)cellGrid.AllocExtraMem(sizeof(LPVOID));
    *pTblOb = &m_GTitle;
    SetCell(0, 0 ,  &cellGrid);

    CFmt fmt;
    GetFmt4NonDataCell(&m_GTitle ,FMT_ID_TITLE, fmt);
    ApplyFormat2Cell(cellGrid,&fmt);
    SetCell(0, 0 ,  &cellGrid);

    //Do SubTitle if it has one
    bool bHasSubTitle =false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));
    pTblFmt ?  bHasSubTitle = pTblFmt->HasSubTitle() : bHasSubTitle = pDefTblFmt->HasSubTitle();
    if(bHasSubTitle){
        int iSubTitleRow =1;
        if(iCols > 1) {
            JoinCells(0,iSubTitleRow,iCols-1,iSubTitleRow);
        }
        QuickSetText(0,iSubTitleRow,m_pTable->GetSubTitle()->GetText());

        //Set the pTbloB for the SubTitle Cell
        CUGCell cellGrid;
        GetCell(0,iSubTitleRow,&cellGrid);
        m_GSubTitle.SetTblOb(m_pTable->GetSubTitle());
        void** pTblOb = (void**)cellGrid.AllocExtraMem(sizeof(LPVOID));
        *pTblOb = &m_GSubTitle;
        SetCell(0, iSubTitleRow ,  &cellGrid);

        CFmt fmt;
        GetFmt4NonDataCell(&m_GSubTitle ,FMT_ID_SUBTITLE, fmt);
        ApplyFormat2Cell(cellGrid,&fmt);
        SetCell(0, iSubTitleRow ,  &cellGrid);
    }

}
/*
/////////////////////////////////////////////////////////////////////////////
//
//                           CTblGrid::DrawAreaCaption
//
/////////////////////////////////////////////////////////////////////////////
void CTblGrid::DrawAreaCaption(int iAreaCaptionRow)
{
    bool bHasArea = false;
    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();

    int iNumAreaLevels = pDoc->GetTableSpec().GetConsolidate()->GetNumAreas();
    iNumAreaLevels > 0 : bHasArea = true : bHasArea = false;
    if(bHasArea ) {
        CTblOb* pAreaCaption = m_pTable->GetAreaCaption();

        CTblFmt* pTblFmt = m_pTable->GetDerFmt();
        CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_AREA_CAPTION));

        //int iNumHeaderRows = GetNumHeaderRows();
        int iCols = GetNumberCols();

        //int iAreaCaptionRow = iNumHeaderRows+1;

        //Do PageNote if it has one
        if(true){
            bool bSpanCells  = true; //Get it from the area caption fmt;
            if(bSpanCells) {
                JoinCells(0,iAreaCaptionRow,iCols-1,iAreaCaptionRow);
            }
            QuickSetText(0,iAreaCaptionRow,pAreaCaption)->GetText());

            //Set the pTbloB for the PageNote Cell
            CUGCell cellGrid;
            GetCell(0,iAreaCaptionRow,&cellGrid);
            m_GAreaCaption.SetTblOb(pAreaCaption);
            void** pTblOb = (void**)cellGrid.AllocExtraMem(sizeof(LPVOID));
            *pTblOb = &m_GAreaCaption;
            SetCell(0, iAreaCaptionRow ,  &cellGrid);

            CFmt fmt;
            GetFmt4NonDataCell(&m_GAreaCaption ,FMT_ID_AREA_CAPTION, fmt);
            ApplyFormat2Cell(cellGrid,&fmt);
            SetCell(0, iAreaCaptionRow ,  &cellGrid);
        }
}
*/

/////////////////////////////////////////////////////////////////////////////
//
//                           CTblGrid::DrawPageOrEndNote
//
/////////////////////////////////////////////////////////////////////////////

void CTblGrid::DrawPageOrEndNote()
{
    bool bHasPageNote =false;
    bool bHasEndNote = false;
    int iPageNoteRow = -1;
    int iEndNoteRow = -1;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

    pTblFmt ?  bHasPageNote = pTblFmt->HasPageNote() : bHasPageNote = pDefTblFmt->HasPageNote();
    pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();


    if(bHasPageNote && bHasEndNote){
        iPageNoteRow = m_iNumGridRows-2;
        iEndNoteRow = m_iNumGridRows-1;

    }
    else if (bHasPageNote){
        iPageNoteRow = m_iNumGridRows-1;
    }
    else if (bHasEndNote){
        iEndNoteRow = m_iNumGridRows-1;
    }

    int iCols = GetNumberCols();

  //Do PageNote if it has one
    if(iPageNoteRow != -1){
        if(iCols > 1) {
            JoinCells(0,iPageNoteRow,iCols-1,iPageNoteRow);
        }
        QuickSetText(0,iPageNoteRow,m_pTable->GetPageNote()->GetText());

        //Set the pTbloB for the PageNote Cell
        CUGCell cellGrid;
        GetCell(0,iPageNoteRow,&cellGrid);
        m_GPageNote.SetTblOb(m_pTable->GetPageNote());
        void** pTblOb = (void**)cellGrid.AllocExtraMem(sizeof(LPVOID));
        *pTblOb = &m_GEndNote;
        SetCell(0, iPageNoteRow ,  &cellGrid);

        CFmt fmt;
        GetFmt4NonDataCell(&m_GPageNote ,FMT_ID_PAGENOTE, fmt);
        ApplyFormat2Cell(cellGrid,&fmt);
        SetCell(0, iPageNoteRow ,  &cellGrid);
    }

    //Do EndNote if it has one
    if(iEndNoteRow !=-1){
        if(iCols > 1) {
            JoinCells(0,iEndNoteRow,iCols-1,iEndNoteRow);
        }
        QuickSetText(0,iEndNoteRow,m_pTable->GetEndNote()->GetText());

        //Set the pTbloB for the EndNote Cell
        CUGCell cellGrid;
        GetCell(0,iEndNoteRow,&cellGrid);
        m_GEndNote.SetTblOb(m_pTable->GetEndNote());
        void** pTblOb = (void**)cellGrid.AllocExtraMem(sizeof(LPVOID));
        *pTblOb = &m_GEndNote;
        SetCell(0, iEndNoteRow ,  &cellGrid);
        CFmt fmt;
        GetFmt4NonDataCell(&m_GEndNote ,FMT_ID_ENDNOTE, fmt);

        if(m_pTable && m_pTable->m_bHasFreqStats && iTotalWidth !=0) {
            CDC* pDC = GetDC();
            int iSavedDC = pDC->SaveDC();
            pDC->SelectObject(fmt.GetFont());
            CSize sz = pDC->GetTextExtent(m_pTable->GetEndNote()->GetText());
            int iTotalLines = 1 + (sz.cx + 5) / iTotalWidth;
            iTotalLines++;
            SetRowHeight(iEndNoteRow, iTotalLines*sz.cy);
            pDC->RestoreDC(iSavedDC);
            ReleaseDC(pDC);
        }

        ApplyFormat2Cell(cellGrid,&fmt);
        SetCell(0, iEndNoteRow ,  &cellGrid);
    }

}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::DrawGridCols (CGTblCol* pCol, int iLevel)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::DrawGridCols (CGTblCol* pCol, int iLevel)
{
    CUGCell cellGrid;

    // COLGROUP
    if (pCol->HasChildren()) {
        int iCount = pCol->GetNumChildren();

        ///&&& Do borders
        int iStartRow = this->m_iGridHeaderRows;
        int iTotalRows = GetNumberRows();

        //&&&
        //Do Cell Styles Later
        //Here process  the cells and attach Cell objects may be ?

        // recurse through children
        for (int iIndex =0; iIndex < pCol->GetNumChildren() ; iIndex ++) {
            CGTblCol* pChild = (CGTblCol*) pCol->GetChild(iIndex);
            DrawGridCols(pChild, iLevel + 1);
        }
    }

    // don't draw for root object
    if (pCol->GetCurLevel() == 0) {
        return;
    }

    bool bHasSubTitle =false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));
    pTblFmt ?  bHasSubTitle = pTblFmt->HasSubTitle() : bHasSubTitle = pDefTblFmt->HasSubTitle();

    int iRow = pCol->GetCurLevel();
    int iCol = pCol->GetStartCol();
    if(bHasSubTitle){
        iRow++;
    }
    GetCell(iCol, iRow, &cellGrid);

    const CFmt* const pFmt = GetFmtFromRowCol(iCol,iRow);
    //Alignment
    if(pFmt){
        short iAlign = GetAlignmentFlags(pFmt);
        cellGrid.SetAlignment(iAlign);
    }
    else {
        cellGrid.SetAlignment(UG_ALIGNVCENTER|UG_ALIGNRIGHT);
    }
    // text
    cellGrid.SetText(pCol->GetText());

    void** pGTblOb = (void**)cellGrid.AllocExtraMem(sizeof(LPVOID));
    *pGTblOb = pCol;

    // justification

    // lines

    SetCell(iCol, iRow, &cellGrid);

    // join cells into spanners
    if (pCol->GetNumGridCols() > 0) {
        const CFmt* const pFmt = GetFmtFromRowCol(iCol, iRow);

        int iAdditionalJoinCols = 0;
        int iNumJoinSpanners = 0;
        if(pFmt){
            const CDataCellFmt* const pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmt);
            iNumJoinSpanners = pDataCellFmt->GetNumJoinSpanners();
            //sEndTabVarName = "P05_MS_VS1";
        }
        iAdditionalJoinCols = GetAdditionalJoinColsForSpanners(pCol->GetTabVar(),iNumJoinSpanners);
        JoinCells(iCol, iRow, iCol + pCol->GetNumGridCols() -1 +iAdditionalJoinCols , iRow);
        //Alignment
        if(pFmt){
            short iAlign = GetAlignmentFlags(pFmt);
            QuickSetAlignment(iCol, iRow, iAlign);
        }
        else {
            QuickSetAlignment(iCol, iRow, UG_ALIGNVCENTER|UG_ALIGNCENTER);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ApplyGridColsJoins (CGTblCol* pCol, int iLevel)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ApplyGridColsJoins (CGTblCol* pCol, int iLevel)
{
    CUGCell cellGrid;

    // COLGROUP
    if (pCol->HasChildren()) {
        int iCount = pCol->GetNumChildren();

        ///&&& Do borders
        int iStartRow = this->m_iGridHeaderRows;
        int iTotalRows = GetNumberRows();

        //&&&
        //Do Cell Styles Later
        //Here process  the cells and attach Cell objects may be ?

        // recurse through children in the reverse order. so that the
        //custom spanners  join does not over ride the spanners join
        for (int iIndex =pCol->GetNumChildren()-1; iIndex >=0 ; iIndex --) {
            CGTblCol* pChild = (CGTblCol*) pCol->GetChild(iIndex);
            ApplyGridColsJoins(pChild, iLevel + 1);
        }
    }

    // don't draw for root object
    if (pCol->GetCurLevel() == 0) {
        return;
    }

    bool bHasSubTitle =false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));
    pTblFmt ?  bHasSubTitle = pTblFmt->HasSubTitle() : bHasSubTitle = pDefTblFmt->HasSubTitle();

    int iRow = pCol->GetCurLevel();
    int iCol = pCol->GetStartCol();
    if(bHasSubTitle){
        iRow++;
    }
    //GetCell(iCol, iRow, &cellGrid);

    //const CFmt* const pFmt = GetFmtFromRowCol(iCol,iRow);
    //Alignment
    /*if(pFmt){
        short iAlign = GetAlignmentFlags(pFmt);
        cellGrid.SetAlignment(iAlign);
    }
    else {
        cellGrid.SetAlignment(UG_ALIGNVCENTER|UG_ALIGNRIGHT);
    }*/
    // text
    //cellGrid.SetText(pCol->GetText());

   /* void** pGTblOb = (void**)cellGrid.AllocExtraMem(sizeof(LPVOID));
    *pGTblOb = pCol;*/

    // justification

    // lines

    //SetCell(iCol, iRow, &cellGrid);

    // join cells into spanners
    if (pCol->GetNumGridCols() > 0) {
        const CFmt* const pFmt = GetFmtFromRowCol(iCol, iRow);

        int iAdditionalJoinCols = 0;
        int iNumJoinSpanners = 0;


        if(pFmt){
            const CDataCellFmt* const pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmt);
            iNumJoinSpanners = pDataCellFmt->GetNumJoinSpanners();
            //sEndTabVarName = "P05_MS_VS1";
        }
        iAdditionalJoinCols = GetAdditionalJoinColsForSpanners(pCol->GetTabVar(),iNumJoinSpanners);
        if(iAdditionalJoinCols){//if we have a custom join
            JoinCells(iCol, iRow, iCol + pCol->GetNumGridCols() -1 +iAdditionalJoinCols , iRow);
        }
        ////Alignment
        //if(pFmt){
        //    short iAlign = GetAlignmentFlags(pFmt);
        //    QuickSetAlignment(iCol, iRow, iAlign);
        //}
        //else {
        //    QuickSetAlignment(iCol, iRow, UG_ALIGNVCENTER|UG_ALIGNCENTER);
        //}
    }
}
int CTblGrid::GetAdditionalJoinColsForSpanners(CTabVar* pStartTabVar,  int iNumJoinSpanners)
{
    int additionalJoinCols =0;
    if(pStartTabVar && iNumJoinSpanners > 0 ){
        CTabVar* pParent = pStartTabVar->GetParent();
        bool bEndVarFound = false;
        bool bStartVarFound  = false;
        int iJoinPos = 0;
        for(int iChildVar =0; iChildVar < pParent->GetNumChildren() ; iChildVar++){
            CTabVar* pCurChildVar = pParent->GetChild(iChildVar);

            if(pCurChildVar == pStartTabVar){
                bStartVarFound = true;
                continue;
            }
            else if(!bStartVarFound){
                continue;
            }
            if (iJoinPos < iNumJoinSpanners){
                additionalJoinCols += pCurChildVar->GetNumValues();
                iJoinPos++;
            }
            else{
                break;
            }

        }

    }
    return additionalJoinCols;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::DrawGridRows (CGTblRow* pRow, int iLevel)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::DrawGridRows (CGTblRow* pRow, int iLevel)
{
    CUGCell cellGrid;

    if (iLevel > 0) {       // don't do root
        // draw our own caption
        CIMSAString csText = pRow->GetText();
        if(pRow->GetTabVar() == nullptr && pRow->GetTabVal() == nullptr ){
            if(pRow->GetTblOb() == m_pTable->GetAreaCaption()) {
                //Draw Area Caption
                CTblOb* pAreaCaption = m_pTable->GetAreaCaption();
                pAreaCaption->SetText(AREA_TOKEN);
                QuickSetText(0, pRow->GetStartRow(), pAreaCaption->GetText());
                bool bSpanCells = true;
                int iCols = GetNumberCols();
                CFmt retFmt;
                GetFmt4NonDataCell(pRow,FMT_ID_AREA_CAPTION, retFmt);
                bSpanCells = retFmt.GetSpanCells() == SPAN_CELLS_YES;
                if(bSpanCells) {
                    JoinCells(0,pRow->GetStartRow(),iCols-1,pRow->GetStartRow());
                }
                else {
                  // JoinCells(1,pRow->GetStartRow(),iCols-1,pRow->GetStartRow());
                }
            }
            else if(pRow->GetTblOb() == m_pTable->GetOneRowColTotal()) {
                //Draw OneRowColTotal
                CTblOb* pOneRowColTotal = m_pTable->GetOneRowColTotal();
                pOneRowColTotal->SetText(TOTAL_LABEL);
                QuickSetText(0, pRow->GetStartRow(), pOneRowColTotal->GetText());
                bool bSpanCells = false;
                int iCols = GetNumberCols();
                CFmt retFmt =*pOneRowColTotal->GetDerFmt();
            }
        }
        else if(pRow->GetTabVar() != nullptr && pRow->GetTabVal() == nullptr) {
            QuickSetText(0, pRow->GetStartRow(), csText);
        }
        else {
            QuickSetText(0, pRow->GetStartRow(), csText);
        }
        GetCell(0, pRow->GetStartRow(), &cellGrid);

        void** pTblOb = (void**)cellGrid.AllocExtraMem(sizeof(LPVOID));
        *pTblOb = pRow;
        //pRow->SetTblOb(m_pTable->GetAreaCaption());
        SetCell(0, pRow->GetStartRow(),&cellGrid);
        if(pRow->GetStartRow() >= m_iGridHeaderRows){
            m_arrGTblRows.Add(pRow);
        }
        // draw our own cells along the row you can probably stuff the rowvar in the cell objects
    }

    // recurse through children
    if (pRow->HasChildren()) {
        int iCount = pRow->GetNumChildren();
        for (int iIndex =0; iIndex <  iCount ; iIndex ++) {
            CGTblRow* pChild = (CGTblRow*) pRow->GetChild(iIndex);
            DrawGridRows(pChild, iLevel + 1);
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
//:DrawGridStub
//                           CTblGrid:Head
//
/////////////////////////////////////////////////////////////////////////////

void CTblGrid::DrawGridStubHead ()
{
    int iStubHeadStartRow =1;
    bool bHasSubTitle =false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));
    pTblFmt ?  bHasSubTitle = pTblFmt->HasSubTitle() : bHasSubTitle = pDefTblFmt->HasSubTitle();
    if(bHasSubTitle){
        iStubHeadStartRow++;
    }
    if(m_iGridHeaderRows > 1){
        JoinCells(0,iStubHeadStartRow,0,m_iGridHeaderRows-1);
        m_GStubHead.SetTblOb(m_pTable->GetStubhead(0));
        CUGCell cellGrid;
        GetCell(0,iStubHeadStartRow,&cellGrid);
        void** pTblOb = (void**)cellGrid.AllocExtraMem(sizeof(LPVOID));
        *pTblOb = &m_GStubHead;
        SetCell(0, iStubHeadStartRow ,  &cellGrid);
        CFmt fmt;
        GetFmt4NonDataCell(&m_GStubHead ,FMT_ID_STUBHEAD, fmt);
        //if(m_pTable->GetRowRoot()->GetNumChildren() == 1 ) {
            //if(m_pTable->GetRowRoot()->GetChild(0)->GetNumChildren()==0){
        if(IsOneRowVarTable()){
                CIMSAString sStubText = m_pTable->GetStubhead(0)->GetText();
                if(!fmt.IsTextCustom()){
                    if(m_pTable->GetRowRoot()->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                       // bHasSystemTotalInRow = true;
                        sStubText = _T("");
                    }
                    else {
                        sStubText = m_pTable->GetRowRoot()->GetChild(0)->GetText();
                    }
                    m_pTable->GetStubhead(0)->SetText(sStubText);
                }
                QuickSetText(0,iStubHeadStartRow,sStubText);
        }
        else {
            CIMSAString sStubText = m_pTable->GetStubhead(0)->GetText();
            if(!fmt.IsTextCustom()){
                m_pTable->GetStubhead(0)->SetText(_T(""));
            }
        }
        GetCell (0,iStubHeadStartRow, &cellGrid);
        ApplyFormat2Cell(cellGrid,&fmt);
        SetCell(0, iStubHeadStartRow ,&cellGrid);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CTblGrid::DrawLineAtBottom
//
/////////////////////////////////////////////////////////////////////////////

void CTblGrid::DrawLineAtBottom ()
{
    CUGCell cell;
    int iBorder;

    int iRows = GetNumberRows();
    int iCols = GetNumberCols();

    for (int i=0; i<iCols; i++) {
        GetCell (i, iRows-1, &cell);
        iBorder = cell.GetBorder();
        iBorder |= UG_BDR_BTHIN;
        cell.SetBorder((short)iBorder);
        cell.SetBorderColor(&(m_blackPen));
        SetCell (i, iRows-1, &cell);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CTblGrid::SetupGrid
//
/////////////////////////////////////////////////////////////////////////////

void CTblGrid::SetupGrid ()
{

    EnableExcelBorders(FALSE);

    //Grid re initialise for the Tblob
    int iNumCols = GetNumberCols();
    long lNumRows = GetNumberRows();
    /*for( int iColIndex =0 ; iColIndex <iNumCols ; iColIndex++) {
        for( long lRowIndex =0 ; lRowIndex < lNumRows ; lRowIndex++) {
            CUGCell cellGrid ;
            GetCell(iColIndex,lRowIndex,&cellGrid);
            cellGrid.SetText("");
            CGTblOb** pOb = (CGTblOb**)cellGrid.GetExtraMemPtr();
            cellGrid.ClearExtraMem();
            SetCell(iColIndex,lRowIndex,&cellGrid);

        }
    }*/

    SpecToTable();

    // default font for grid
    LOGFONT lf;
    lf.lfHeight = -13;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = 400;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = 0;
    lf.lfOutPrecision = 3;
    lf.lfClipPrecision = 2;
    lf.lfQuality = 1;
    lf.lfPitchAndFamily = 34;
    _tcscpy(lf.lfFaceName, _T("Arial"));

    m_font.DeleteObject();
    m_font.CreateFontIndirect(&lf);
    SetDefFont(&m_font);


    for (int iCol =0 ; iCol  < GetNumberCols()  ; iCol++) {
        for (int iRow =0 ; iRow  < GetNumberRows() ; iRow++) {
            QuickSetCellTypeEx(iCol,iRow,UGCT_NORMALMULTILINE);
        }
    }
}

void CTblGrid::DrawSubTableLines()
{
    CTabView* pView = (CTabView*)GetParent();
    ASSERT(pView);
    bool bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView();
    if(!bDesignView || !m_pTable){
        return;
    }

    CTabVar* pRowRootVar = m_pTable->GetRowRoot();
    int iNumChildren = pRowRootVar->GetNumChildren();
    bool bDone = false;
    if(iNumChildren <=1){
        if(iNumChildren ==0){
            bDone =true;
        }
        else {
            CTabVar* pTabVar = pRowRootVar->GetChild(0);
            if(pTabVar->GetNumChildren() <=1){
                bDone =true;
            }
        }
    }
    if(!bDone){
        //go through all the rows
        FMT_ID eGridComp = FMT_ID_INVALID;
        CGTblOb* pGTblOb = nullptr;
        int iStartRow = GetNumHeaderRows()-1;
        int iNumCols = GetNumberCols();
        for(int iRow = iStartRow; iRow < GetNumberRows(); iRow++){
            eGridComp = FMT_ID_INVALID;
            pGTblOb = nullptr;
            GetComponent(0,iRow,eGridComp,&pGTblOb);
            if(eGridComp != FMT_ID_CAPTION){
                continue;
            }
            else {//Draw the horizontal lines 4 subtables
                //Get DC .
                CDC *pDC = m_CUGGrid->GetDC();
                int iSavedDC = pDC->SaveDC();
                int iStartCol =0;
                int iEndCol = iNumCols-1;
                int iStartRow =iRow;
                int iEndRow = iRow;
                CRect rect;
                //Get the rectangle
                GetRangeRect(iStartCol,iStartRow,iEndCol,iEndRow,rect);
                CPen penThickLine(PS_SOLID, 3, RGB(0,0,255));
                pDC->SelectObject(&penThickLine);
                //pDC->SelectObject(GetStockObject(BLACK_PEN));
                pDC->MoveTo(rect.left,rect.top);
                pDC->LineTo(rect.right,rect.top);
                //Draw line from start to end

                pDC->RestoreDC(iSavedDC);

            }
        }
        //Go through all the columns
        //Go thro
        eGridComp = FMT_ID_INVALID;
        pGTblOb = nullptr;
        //For each column
        //For each row until the numheader -1
        //check if spanner . If hidden . get row and make the rowwidth zero
        int iNumHeaders = GetNumHeaderRows();
        for(int iCol =1; iCol < iNumCols ; iCol++){
            for(int iRow =1; iRow < iNumHeaders-1; iRow++){
                eGridComp = FMT_ID_INVALID;
                pGTblOb = nullptr;
                int iStartCol = iCol;
                long iStartRow = iRow;
                GetJoinStartCell(&iStartCol, &iStartRow);
                if(iStartCol != iCol && iStartRow != iRow)
                    continue;

                GetComponent(iCol,iRow,eGridComp,&pGTblOb);
                if(!pGTblOb || eGridComp != FMT_ID_SPANNER){
                    continue;
                }
                else {
                    CGRowColOb* pSpannerOb = DYNAMIC_DOWNCAST(CGRowColOb,pGTblOb);
                    //Check if it has a child which is a tabvar. if so ignore.
                    //cos we need the lowest spanner
                    if(pSpannerOb->GetTabVar() && pSpannerOb->GetTabVar()->GetNumChildren() ==0) {
                        int iEndCol = iStartCol;
                        long iEndRow = iStartRow;
                        GetJoinRange(&iStartCol, &iStartRow, &iEndCol, &iEndRow);
                        int iFinalCol = iEndCol;
                        int iFinalRow = GetNumberRows() -1;

                        //Get DC .
                        CDC *pDC = m_CUGGrid->GetDC();
                        int iSavedDC = pDC->SaveDC();
                        CRect rect;
                        //Get the rectangle
                        GetRangeRect(iEndCol,iEndRow,iFinalCol,iFinalRow,rect);
                        CPen penThickLine(PS_SOLID, 3, RGB(0,0,255));
                        pDC->SelectObject(&penThickLine);
                        //pDC->SelectObject(GetStockObject(BLACK_PEN));
                        pDC->MoveTo(rect.right,rect.top);
                        pDC->LineTo(rect.right,rect.bottom);
                        //Draw line from start to end

                        pDC->RestoreDC(iSavedDC);
                    }
                }
            }
        }
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           CTblGrid::FixLines
//
//          If adjoining cells both have borders, they can combine to
//          make a "double" thick line; this routine gets rid of these
//
/////////////////////////////////////////////////////////////////////////////

void CTblGrid::FixLines (long lFixRow /*= -1*/)
{
    //Set Header lines
    int iNumRows = GetNumberRows();

    int iNumCols = GetNumberCols();

    CUGCell cellTop, cellCurrent, cellLeft;
    int iTopCellBrdr, iCurCellBrdr, iLeftCellBrdr;
    bool bInRange = false;
    int  iStartCol,iLeftStartCol,iTopStartCol;
    long iStartRow,iLeftStartRow,iTopStartRow;

    for (int iCol=0; iCol<iNumCols; iCol++) {
        for (int iRow=0; iRow<iNumRows; iRow++) {
            if(lFixRow != -1 ){
                bInRange = (iRow >=  lFixRow-1  && iRow <=  lFixRow+1);
                if(!bInRange){
                    continue;
                }
            }
            iStartCol = iCol;
            iStartRow = iRow;
            GetJoinStartCell(&iStartCol, &iStartRow);
            if (iStartRow > 0) {
                iTopStartCol = iCol;
                iTopStartRow = iStartRow-1;

                GetJoinStartCell(&iTopStartCol, &iTopStartRow);
                GetCell(iTopStartCol,iTopStartRow,&cellTop);

                iTopCellBrdr = cellTop.GetBorder();
            }
            if (TRUE) {
                GetCell (iStartCol,   iStartRow,   &cellCurrent);
                iCurCellBrdr = cellCurrent.GetBorder();
            }
            if (iStartCol > 0) {
                iLeftStartCol = iStartCol-1;
                iLeftStartRow = iStartRow;

                GetJoinStartCell(&iLeftStartCol, &iLeftStartRow);
                GetCell(iLeftStartCol,iLeftStartRow,&cellLeft);

                iLeftCellBrdr = cellLeft.GetBorder();
            }

            if (iStartRow > 0) {
                // cell on top has a bottom line;
                // current cell has a top line
                // take away line attrib which has least prcedence thin/thick
                bool bRmThin4TopBBrdr = (iTopCellBrdr & UG_BDR_BTHIN) && ((iCurCellBrdr & UG_BDR_TTHIN) ||(iCurCellBrdr & UG_BDR_TTHICK));
                bool  bRmThick4TopBBrdr = (iTopCellBrdr & UG_BDR_BTHICK) && (iCurCellBrdr & UG_BDR_TTHICK);
                bool  bRmThin4CurTBrdr = (iTopCellBrdr & UG_BDR_BTHICK) && (iCurCellBrdr & UG_BDR_TTHIN);
                if (bRmThin4TopBBrdr) {//covers (Thin-->Thin / Thin-Thick)
                    cellTop.SetBorder ((short)(iTopCellBrdr - UG_BDR_BTHIN));
                    SetCell (iTopStartCol, iTopStartRow, &cellTop);
                }
                else if (bRmThick4TopBBrdr) {//covers (thick-->thick)
                    cellTop.SetBorder ((short)(iTopCellBrdr - UG_BDR_BTHICK));
                    SetCell (iTopStartCol, iTopStartRow, &cellTop);
                }
                else if (bRmThin4CurTBrdr) {//covers (thick-->thin)
                    cellCurrent.SetBorder ((short)(iCurCellBrdr - UG_BDR_TTHIN));
                    SetCell (iStartCol, iStartRow, &cellCurrent);
                }
            }
            if (iStartCol > 0) {
                 // cell on left has a right line;
                // current cell has a left line
                // take away line attrib which has least prcedence thin/thick
                bool bRmThin4LeftRBrdr = (iLeftCellBrdr & UG_BDR_RTHIN) && (iCurCellBrdr & UG_BDR_LTHIN ||iCurCellBrdr & UG_BDR_LTHICK);
                bool  bRmThick4LeftRBrdr = (iLeftCellBrdr & UG_BDR_RTHICK) && (iCurCellBrdr & UG_BDR_LTHICK);
                bool  bRmThin4CurLBrdr = (iLeftCellBrdr & UG_BDR_RTHICK) && (iCurCellBrdr & UG_BDR_LTHIN);

                 if (bRmThin4LeftRBrdr) {//covers (Thin-->Thin / Thin-Thick)
                    cellLeft.SetBorder ((short)(iLeftCellBrdr - UG_BDR_RTHIN));
                    SetCell (iLeftStartCol, iLeftStartRow, &cellLeft);
                }
                else if (bRmThick4LeftRBrdr) {//covers (thick-->thick)
                    cellLeft.SetBorder ((short)(iLeftCellBrdr - UG_BDR_RTHICK));
                    SetCell (iLeftStartCol, iLeftStartRow, &cellLeft);
                }
                else if (bRmThin4CurLBrdr) {//covers (thick-->thin)
                    cellCurrent.SetBorder ((short)(iCurCellBrdr - UG_BDR_LTHIN));
                    SetCell (iStartCol, iStartRow, &cellCurrent);
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblGrid::IsMultiSelectState()
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::IsMultiSelectState()
{
    int iSelCol=NONE;
    long lSelRow=NONE;
    int iFound=m_GI->m_multiSelect->EnumFirstSelected(&iSelCol, &lSelRow);
    int iCount = 0;
    while (iFound==UG_SUCCESS) {
        iCount++;
        iFound=m_GI->m_multiSelect->EnumNextSelected(&iSelCol, &lSelRow);
    }
    return (iCount > 1);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblGrid::GetMultiSelObj(CArray<CTblBase*,CTblBase*>&arrTblBase,FMT_ID eFmtID)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::GetMultiSelObj(CArray<CTblBase*,CTblBase*>&arrTblBase,FMT_ID eFmtID)
{
    bool bRet = true;
    ASSERT(IsMultiSelectState());
    int iSelCol=NONE;
    long lSelRow=NONE;
    CGTblOb* pGTblOb = nullptr;
    int iFound=m_GI->m_multiSelect->EnumFirstSelected(&iSelCol, &lSelRow);
    CTblBase* pTblBase = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;
    while (iFound==UG_SUCCESS) {
        pTblBase = nullptr;
        //For each selected object
        if (IsDataCell(iSelCol,lSelRow)) {
            bRet = false;
            break;
        }
        else {
            GetComponent(iSelCol,lSelRow,eGridComp,&pGTblOb);
            if(eGridComp != eFmtID){
                bRet = false;
                break;
            }
            else if(eGridComp == FMT_ID_STUB || eGridComp == FMT_ID_COLHEAD){
                pTblBase = ((CGRowColOb*)pGTblOb)->GetTabVal();
            }
            else if(eGridComp == FMT_ID_CAPTION || eGridComp == FMT_ID_SPANNER){
                pTblBase = ((CGRowColOb*)pGTblOb)->GetTabVar();
            }
            if(pTblBase){
                bool bFound = false;
                for(int iIndex = 0; iIndex < arrTblBase.GetSize(); iIndex++){
                    if(pTblBase == arrTblBase[iIndex]){
                        bFound = true;
                        break;
                    }
                }
                if(!bFound){
                    arrTblBase.Add(pTblBase);
                }
            }
        }
        iFound=m_GI->m_multiSelect->EnumNextSelected(&iSelCol, &lSelRow);
    }
    if(!bRet){
        arrTblBase.RemoveAll();
    }
    bRet = arrTblBase.GetSize() > 1;
    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::UpdateTableBlocking(bool bBlock)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::UpdateTableBlocking(bool bBlock /*=false*/)
{
    CMap<int, int, bool, bool> colMask;
    CMap<long, long, bool, bool> rowMask;

    //////////////////////////////////////////////////////////////////////
    // enumerate selected cells
    CGTblOb* pGTblOb = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;
    if(bBlock){
        int iSelCol=NONE;
        long lSelRow=NONE;
        int iFound=m_GI->m_multiSelect->EnumFirstSelected(&iSelCol, &lSelRow);
        while (iFound==UG_SUCCESS) {

            //For each selected object
            if (IsDataCell(iSelCol,lSelRow)) {
                pGTblOb = GetGTblCell(iSelCol, lSelRow);
                eGridComp = FMT_ID_DATACELL;
            }
            else {
                GetComponent(iSelCol,lSelRow,eGridComp,&pGTblOb);
            }
            ASSERT(pGTblOb);
            pGTblOb->SetBlocked(true);
            BlockGTblObParents(pGTblOb,bBlock);

            if(eGridComp == FMT_ID_DATACELL){
                CGTblCell* pGTblCell = DYNAMIC_DOWNCAST(CGTblCell,pGTblOb);
                ASSERT(pGTblCell);

                //Mark the Stub/caption as blocked
                if (rowMask.PLookup(lSelRow) == nullptr) {
                    rowMask.SetAt(lSelRow, true);

                    CGTblRow* pGTblRow = pGTblCell->GetTblRow();
                    ASSERT(pGTblRow);
                    BlockGTblObParents(pGTblRow,bBlock);
                }

                // first check if we hit this column already
                if (colMask.PLookup(iSelCol) == nullptr) {

                    // mark this column as visited
                    colMask.SetAt(iSelCol, true);

                    //Mark the colheads and their parents blocked
                    CGTblCol* pGTblCol = pGTblCell->GetTblCol();
                    ASSERT(pGTblCol);
                    BlockGTblObParents(pGTblCol,bBlock);

                    // mark empty cells in caption rows, above this column
                    CGTblOb* pParent = pGTblCell->GetTblRow()->GetParent();
                    while (pParent && pParent->GetParent()) {
                        FMT_ID id;
                        CGTblOb* pOb = nullptr;

                        CGTblRow* pGParentTblRow = DYNAMIC_DOWNCAST(CGTblRow, pParent);
                        int iCaptionRow = pGParentTblRow->GetStartRow();

    #ifdef _DEBUG
                        // sanity check
                        GetComponent(0, iCaptionRow,id,&pOb);
                        ASSERT(id==FMT_ID_CAPTION);
    #endif

    //CString s1=QuickGetText(0, iCaptionRow);
    //CString s2=QuickGetText(iSelCol, iCaptionRow);

                        // get the cell above our column, in the caption row
                        GetComponent(iSelCol, iCaptionRow, id, &pOb);

                        if (pOb != nullptr) {
                            pOb->SetBlocked(bBlock);
                        }

                        pParent = pParent->GetParent();
                    }
                }
            }
            iFound=m_GI->m_multiSelect->EnumNextSelected(&iSelCol, &lSelRow);
        }

        // the stubhead is always blocked ...
        int iStubHeadRow=1;  // below the title

        // ... or below the subtitle (if one is present)
        if (m_pTable->GetDerFmt() != nullptr && m_pTable->GetDerFmt()->HasSubTitle()) {
            iStubHeadRow++;
        }

        GetComponent(0, iStubHeadRow,eGridComp,&pGTblOb);
        pGTblOb->SetBlocked(bBlock);
    }
    else {//Reset the flags by going throug all the CGTblobs  of the table
        for(int iRow =0; iRow < GetNumberRows(); iRow++){
            for(int iCol =0; iCol< GetNumberCols(); iCol++){
                if (IsDataCell(iCol, iRow)) {
                    pGTblOb = GetGTblCell(iCol,iRow);
                }
                else {
                    GetComponent(iCol,iRow,eGridComp,&pGTblOb);
                }
                if(!pGTblOb){
                    continue;
                }
                ASSERT(pGTblOb);
                pGTblOb->SetBlocked(bBlock);
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::BlockGTblObParents(CGTblOb* pGTblOb , bool bFlag)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::BlockGTblObParents(CGTblOb* pGTblOb , bool bFlag)
{
    CGTblOb* pParent = pGTblOb;    //pGTblOb->GetParent();
    while(pParent){
        pParent->SetBlocked(bFlag);
        pParent = pParent->GetParent();
    }
    return;
}

bool CTblGrid::SpecToTable()
{
    if(m_pTblRowRoot) {
        delete  m_pTblRowRoot;
    }
    if(m_pTblColRoot) {
        delete  m_pTblColRoot;
    }

    m_pTblRowRoot = new  CGTblRow();
    m_pTblColRoot = new  CGTblCol();
    m_iGridHeaderRows = 0;
    m_iNumGridRows =0;


    //Go through the spec and create Row/Col objects
    CTabVar* pColRootVar = m_pTable->GetColRoot();

    AddChildCols(pColRootVar,m_pTblColRoot);

    CTabVar* pRowRootVar = m_pTable->GetRowRoot();

    int iNumSlices = 1;
    CTabView* pView = (CTabView*)GetParent();
    ASSERT(pView);
    bool bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView();

    if(!bDesignView){
       iNumSlices = m_pTable->GetTabDataArray().GetSize();
       if(m_iCurrSelArea != 0){//We need to show only the current slice
           iNumSlices =1;
       }
    }
    if(iNumSlices == 0 ) {
        iNumSlices = 1;
    }
    for(int iSlice =0; iSlice < iNumSlices ; iSlice++){
        AddChildRows(pRowRootVar,m_pTblRowRoot);
    }

    //Now compute number of headers / stubs  and dimension of the table

    int iStartCol = 1;      // leftmost is for stubs
    m_pTblColRoot->SetNumGridCols(ComputeGridCols(m_pTblColRoot, iStartCol, 0));

    m_iNumGridRows = m_iGridHeaderRows ;
    int iStartRow  = m_iGridHeaderRows;
    if(IsOneRowVarTable()) {
      bool bHasArea = HasArea();
      if(!bHasArea){
            iStartRow = iStartRow -1;
            m_iNumGridRows = m_iNumGridRows -1;
        }
    }
    m_pTblRowRoot->SetNumGridRows(ComputeGridRows(m_pTblRowRoot, &iStartRow, 0));

    bool bHasPageNote =false;
    bool bHasEndNote = false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

    pTblFmt ?  bHasPageNote = pTblFmt->HasPageNote() : bHasPageNote = pDefTblFmt->HasPageNote();
    bHasPageNote  ? m_iNumGridRows++ : m_iNumGridRows;

    pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();
    bHasEndNote  ? m_iNumGridRows++ : m_iNumGridRows;


    SetNumberRows(m_iNumGridRows);

    if(m_pTable->GetColRoot() && m_pTable->GetColRoot()->GetNumChildren() == 0) {
        SetNumberCols(m_pTblColRoot->GetNumGridCols());
    }
    else {
        SetNumberCols(m_pTblColRoot->GetNumGridCols() + 1);      // one extra for stubs
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::AddChildCols(CTabVar* pTabVar , CGTblCol* pParentCol)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::AddChildCols(CTabVar* pTabVar , CGTblCol* pParentCol)
{
    //Recursive routine for adding child cols .
    if(!pTabVar->IsRoot()){
        CGTblCol* pChildCol = new  CGTblCol();
        pParentCol->AddChild(pChildCol);
        pChildCol->SetParent(pParentCol);
        pChildCol->SetTabVar(pTabVar);

        if(IsAStarBPlusCMode(pTabVar)){
            //generate the same var again . We will fix the display in the drawgridcols later
            //now we are only  generating "dummies" to get the right structure
            CGTblCol* pLocalChildCol = new  CGTblCol();
            pChildCol->AddChild(pLocalChildCol);
            pLocalChildCol->SetParent(pChildCol);
            pLocalChildCol->SetTabVar(pTabVar);

            //Add one more "dummy" and to this add the vals.
            pChildCol = new  CGTblCol();
            pLocalChildCol->AddChild(pChildCol);
            pChildCol->SetParent(pLocalChildCol);
            pChildCol->SetTabVar(pTabVar);
            // now let it go through to add the vals
            //The strucutre will look like this for "C" of A*B +C
            /*
                    --------------------------
                   |        VarC             |
                   --------------------------
                   |        VarC             |
                   --------------------------
                   |        VARC             |
                   ---------------------------
                   |valc1 | valc2| valc3|valc4|
                   ----------------------------
                   in the display we wil join row 1 && 2 and expand "3" and join 3& 4 to get the required display for the "C" in A*B+C in the column
                   --------------------------
                   |        VarC             |
                   --------------------------
                   |        VarC             |
                   --------------------------
                   |      |      |      |     |
                   |valc1 | valc2| valc3|valc4|
                   ----------------------------
            */
        }
        //for each tab value
        int iNumValues = pTabVar->GetNumValues();
        for(int iIndex =0; iIndex < iNumValues ; iIndex++) {
            CTabValue* pTabVal = pTabVar->GetValue(iIndex);

            CGTblCol* pChildColVal = new  CGTblCol();
            pChildCol->AddChild(pChildColVal);
            pChildColVal->SetParent(pChildCol);
            pChildColVal->SetTabVar(pTabVar);
            pChildColVal->SetTabVal(pTabVal);

            //Now for each TabVar do the same
            int iNumCrossVars = pTabVar->GetNumChildren();
            for (int iCVar =0; iCVar < iNumCrossVars; iCVar++){
                CTabVar* pChildTabVar = pTabVar->GetChild(iCVar);
                AddChildCols(pChildTabVar,pChildColVal);
            }

        }
    }
    else {
        //Now for each TabVar do the same
        int iNumCrossVars = pTabVar->GetNumChildren();
        for (int iCVar =0; iCVar < iNumCrossVars; iCVar++){
            CTabVar* pChildTabVar = pTabVar->GetChild(iCVar);
            AddChildCols(pChildTabVar,pParentCol);
        }

        //TO DO //if NumCross Vars are zero //we should put in stuff for freq/total here

    }

}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::AddChildRows(CTabVar* pTabVar , CGTblRow* pParentRow)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::AddChildRows(CTabVar* pTabVar , CGTblRow* pParentRow)
{
    //Recursive routine for adding child Rows .
    if(!pTabVar->IsRoot()){
        CGTblRow* pChildRow = new  CGTblRow();
        pParentRow->AddChild(pChildRow);
        pChildRow->SetParent(pParentRow);
        pChildRow->SetTabVar(pTabVar);

        //for each tab value
        int iNumValues = pTabVar->GetNumValues();
        for(int iIndex =0; iIndex < iNumValues ; iIndex++) {
            CTabValue* pTabVal = pTabVar->GetValue(iIndex);

            CGTblRow* pChildRowVal = new  CGTblRow();
            pChildRow->AddChild(pChildRowVal);
            pChildRowVal->SetParent(pChildRow);
            pChildRowVal->SetTabVar(pTabVar);
            pChildRowVal->SetTabVal(pTabVal);
            if(pTabVal->GetTabValType() == RDRBRK_TABVAL){
                continue;
            }
            //Now for each TabVar do the same
            int iNumCrossVars = pTabVar->GetNumChildren();
            for (int iCVar =0; iCVar < iNumCrossVars; iCVar++){
                CTabVar* pChildTabVar = pTabVar->GetChild(iCVar);
                AddChildRows(pChildTabVar,pChildRowVal);
            }

        }
    }
    else {
        //Allocate an empty grid row for the area caption
        bool bHasArea = false;
        CTabView* pView = (CTabView*)GetParent();
        CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();

        int iNumAreaLevels = pDoc->GetTableSpec()->GetConsolidate()->GetNumAreas();
        iNumAreaLevels > 0 ? bHasArea = true : bHasArea = false;
        if(bHasArea && pTabVar->IsRoot() ) {
            //CTblOb* pAreaCaption = m_pTable->GetAreaCaption();
            CGTblRow* pChildRow = new  CGTblRow();
            pParentRow->AddChild(pChildRow);
            pChildRow->SetParent(pParentRow);
            pChildRow->SetTabVal(nullptr);
            pChildRow->SetTabVar(nullptr);
            pChildRow->SetTblOb(m_pTable->GetAreaCaption());
        }

        //Now for each TabVar do the same
        int iNumCrossVars = pTabVar->GetNumChildren();
        for (int iCVar =0; iCVar < iNumCrossVars; iCVar++){
            CTabVar* pChildTabVar = pTabVar->GetChild(iCVar);
            AddChildRows(pChildTabVar,pParentRow);
        }
        //TO DO //if NumCross Vars are zero //we should put in stuff for freq/total here
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  int CTblGrid::ComputeGridCols(CGTblCol* pCol, int iStartGridCol, int iLevel)
//
/////////////////////////////////////////////////////////////////////////////////
int CTblGrid::ComputeGridCols(CGTblCol* pCol, int iStartGridCol, int iLevel)
{
    // set our own properties
    pCol->SetCurLevel(iLevel);
    pCol->SetStartCol(iStartGridCol);

    int iTitleOrSubTitleRow = 1;
    bool bHasSubTitle =false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));
    pTblFmt ?  bHasSubTitle = pTblFmt->HasSubTitle() : bHasSubTitle = pDefTblFmt->HasSubTitle();

    bHasSubTitle ? iTitleOrSubTitleRow =2 : iTitleOrSubTitleRow =1;
    // set table property
    if (m_iGridHeaderRows < (iLevel+iTitleOrSubTitleRow)) {       // + iTitleOrSubTitleRow is for the title /subtitle
        m_iGridHeaderRows = iLevel+iTitleOrSubTitleRow;
    }

    // recurse through children to figure out how many cells our spanner should take up
    // and start cell for each child
    int iGridCols =0;
    if (pCol->HasChildren()) {
        int iCount = pCol->GetNumChildren();
        iGridCols = 0;
        for (int iIndex =0; iIndex < iCount ; iIndex ++) {
            CGTblCol* pChild = (CGTblCol*)pCol->GetChild(iIndex);
            int iChildCols = ComputeGridCols(pChild, iStartGridCol, iLevel + 1);
            iGridCols += iChildCols;
            iStartGridCol += iChildCols;
        }
    }
    else {
        iGridCols = 1;
    }
    pCol->SetNumGridCols(iGridCols);

    return iGridCols;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  int CTblGrid::ComputeGridRows(CGTblRow* pRow, int iStartGridRow, int iLevel)
//
/////////////////////////////////////////////////////////////////////////////////
int CTblGrid::ComputeGridRows(CGTblRow* pRow, int* iStartGridRow, int iLevel)
{
    // set our own properties
    pRow->SetCurLevel(iLevel);

    // set number of grid rows
    if (iLevel > 0) {
        pRow->SetStartRow(*iStartGridRow);
        (*iStartGridRow)++;
        m_iNumGridRows ++ ;
    }

    // recurse through children to figure out how many cells our spanner should take up
    // and start cell for each child
    int iGridRows =0;
    if (pRow->HasChildren()) {
        int iCount = pRow->GetNumChildren();
        iGridRows = 0;
        for (int iIndex =0; iIndex < iCount ; iIndex ++) {
            CGTblRow* pChild = (CGTblRow*)pRow->GetChild(iIndex);
            int iChildRows = ComputeGridRows(pChild, iStartGridRow, iLevel + 1);
            iGridRows += iChildRows;
        //  *iStartGridRow += iChildRows;
        }
    }
    else {
        iGridRows = 1;
    }
    pRow->SetNumGridRows(iGridRows);

    return iGridRows;
}

void CTblGrid::DeleteGTblObs()
{
    //Delete Table  row /col objects
    if(m_pTblRowRoot) {
        delete  m_pTblRowRoot;
        m_pTblRowRoot = nullptr;
    }
    if(m_pTblColRoot) {
        delete  m_pTblColRoot;
        m_pTblColRoot = nullptr;
    }

    //Delete the cell objects
    DestroyCells();
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ProcessCells()
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ProcessCells()
{
    CUGCell gridCell;
    //Get number of data rows
    int iStartRow = m_iGridHeaderRows;
    int iMaxRows = m_iNumGridRows;
    int iNumCols = GetNumberCols();
    DestroyCells();
    int iTotalCells = (iMaxRows - iStartRow)* (iNumCols-1);
    m_arrCells.SetSize(iTotalCells,0);

    for(int iCell =0; iCell < iTotalCells;iCell++){
       m_arrCells[iCell]= new CGTblCell();
    }

    CArray<CGTblCol*,CGTblCol*> arrTblCols;
    int iIndex =0;
    for(int iRow = iStartRow; iRow < iMaxRows; iRow++) {//for each row
        GetCell(0,iRow,&gridCell);
        CTblOb** pOb = (CTblOb**)gridCell.GetExtraMemPtr();
        CGTblRow* pRow = (CGTblRow*)*pOb ;

        for (int iCol = 1; iCol < GetNumberCols(); iCol ++ ) {
            if(iRow == iStartRow){
                GetCell(iCol,m_iGridHeaderRows-1,&gridCell);
                CTblOb** pOb = (CTblOb**)gridCell.GetExtraMemPtr();
                ASSERT(pOb);
                arrTblCols.Add((CGTblCol*)*pOb );
            }
            CGTblCol* pCol = arrTblCols[iCol-1];


          //  CGTblCell* pCell = new CGTblCell();
            CGTblCell* pCell = m_arrCells[iIndex];
            iIndex++;

            pCell->SetTblCol(pCol);
            pCell->SetTblRow(pRow);
            pCell->SetDirty(true);
            //m_arrCells.Add(pCell);

        }
    }
    ASSERT(iIndex == iTotalCells);

}

/////////////////////////////////////////////////////////////////////////////////
//
//  CGTblCell* CTblGrid::GetGTblCell(int col , long row)
/////////////////////////////////////////////////////////////////////////////////
CGTblCell* CTblGrid::GetGTblCell(int col, long row)
{
    CGTblCell*  pGTblCell = nullptr;
    //Get number of data rows
    int iStartRow = m_iGridHeaderRows;
    int iMaxRows = m_iNumGridRows;
    int iNumCols = GetNumberCols();

    ASSERT(row >= iStartRow);
    ASSERT(col >= 1);

    int lCurrCell = (row-iStartRow)*(iNumCols-1) + (col-1);

    ASSERT(lCurrCell < m_arrCells.GetSize());
    return m_arrCells[lCurrCell];
#ifdef _OLD_
    int iIndex =0;
    for(long iRow = iStartRow; iRow < iMaxRows; iRow++) {//for each row
        for (int iCol = 1; iCol < GetNumberCols(); iCol ++ ) {
            if(iRow != row || iCol != col){
                iIndex++;
                continue;
            }
            pGTblCell = m_arrCells[iIndex];
            return pGTblCell;
        }
    }
    return pGTblCell;
#endif
}
void CTblGrid::DestroyCells()
{
    int iSize = m_arrCells.GetSize();
    for(int iIndex =0; iIndex  < iSize ; iIndex ++  ) {
        delete m_arrCells[iIndex];
    }
    m_arrCells.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTabVar* CTblGrid::GetRowVar(long iRowNumber)
//
/////////////////////////////////////////////////////////////////////////////////
CTabVar* CTblGrid::GetRowVar(long iRowNumber)
{
    CUGCell cellGrid;
    GetCell(0,iRowNumber,&cellGrid);

    CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
    if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
        return ((CGTblRow*)(*pTblOb))->GetTabVar();
    }
    else {
        return nullptr;
    }
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           CTblGrid::ResetSizes
//
//      so that they're at least as wide as the widest string in them
//      Make sure the title grid row is tall enough to accommodate wrapping
//
/////////////////////////////////////////////////////////////////////////////
void CTblGrid::ResetSizes()
{
    if(!m_pTable)
        return;

    CDC* pDC = GetDC();

    int i, j;
    CSize sz;
    CIMSAString csText;

    UINT uRow =m_iGridHeaderRows - 1;
    int iNumRows = GetNumberRows();
    int iNumCols = GetNumberCols();
    int iHeight = GRID_DEFAULT_ROWHEIGHT;
    iTotalWidth = 0;
    for (i=0; i<iNumCols; i++) {
        int iWidest = 0;
        if (i == 0) {       // take into account stubhead
            CUGCell cellGrid;
            GetCell(0,uRow,&cellGrid);
            int iSavedDC =pDC->SaveDC();
            CFont*pFont = cellGrid.GetFont();
            if(pFont){
                pDC->SelectObject(pFont);
            }
            csText = QuickGetText(0, uRow);
            csText += _T("  ");
            sz = pDC->GetOutputTextExtent(csText);
                //use margins from dundas code ugcelltype
            sz.cx += 6;
            sz.cy += 2;
            pDC->RestoreDC(iSavedDC);
            if (sz.cx > iWidest) {
                iWidest = sz.cx;
            }
        }
        int iMaxRows4WidthProcess = GetLastDataRow();
        for (j=uRow; j<iMaxRows4WidthProcess; j++) {
            CUGCell cellGrid;
            GetCell(i,j,&cellGrid);
            int iSavedDC =pDC->SaveDC();
            CFont*pFont = cellGrid.GetFont();
            if(pFont){
                pDC->SelectObject(pFont);
            }
            csText = QuickGetText(i, j);
            csText += _T("  ");
            sz = pDC->GetOutputTextExtent(csText);
                //use margins from dundas code ugcelltype
            sz.cx += 6;
            sz.cy += 4;
            pDC->RestoreDC(iSavedDC);

            if (sz.cx > iWidest) {
                iWidest = sz.cx;
            }
        }


        int iCurWidth = GetColWidth(i);
        if (iCurWidth > iWidest) {
            iTotalWidth += iCurWidth;
            continue;
        }
        if (iWidest > GRID_DEFAULT_COLWIDTH ) {
            iTotalWidth += iWidest;
            SetColWidth (i, iWidest);
        }
    }

    // Set title's row height, depending on whether it will wrap
    CIMSAString csTitle =  m_pTable->GetTitleText();
    CUGCell cellGrid;
    GetCell(0,0,&cellGrid);
    int iSavedDC =pDC->SaveDC();
    CFont*pFont = cellGrid.GetFont();
    if(pFont){
        pDC->SelectObject(pFont);
    }
    if(csTitle.IsEmpty()){
        csTitle = _T("Title"); //just for computation  of height
    }
    sz = pDC->GetOutputTextExtent(csTitle);
    pDC->RestoreDC(iSavedDC);
    int iTitleLines = 1 + (sz.cx + 5) / iTotalWidth;
    SetRowHeight(0, iTitleLines*sz.cy);

    // Set subtitle's row height, depending on whether it will wrap
    bool bHasSubTitle =false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));
    pTblFmt ?  bHasSubTitle = pTblFmt->HasSubTitle() : bHasSubTitle = pDefTblFmt->HasSubTitle();

    if(bHasSubTitle){
        int iSubTitleRow =1;

        CFmt fmt;
        GetFmt4NonDataCell(&m_GSubTitle ,FMT_ID_SUBTITLE, fmt);
        ApplyFormat2Cell(cellGrid,&fmt);

        CIMSAString csSubTitle= m_pTable->GetSubTitle()->GetText();
        CFont*pFont = cellGrid.GetFont();
        if(fmt.IsTextCustom()){
            csSubTitle = fmt.GetCustomText();
            pFont = fmt.GetFmtFont().GetFont();
        }
        if(!pFont){
            cellGrid.GetFont();
        }
        CUGCell cellGrid;
        GetCell(0,iSubTitleRow,&cellGrid);
        int iSavedDC =pDC->SaveDC();
        if(pFont){
            pDC->SelectObject(pFont);
        }
        if(csSubTitle.IsEmpty()){
            csSubTitle = _T("SubTitle"); //just for computation  of height
        }
        sz = pDC->GetOutputTextExtent(csSubTitle);
        pDC->RestoreDC(iSavedDC);
        int iSubTitleLines = 1 + (sz.cx + 5) / iTotalWidth;
        SetRowHeight(iSubTitleRow, iSubTitleLines*sz.cy);
    }
    //Do the row heights
    for (int iRow =1; iRow <iNumRows; iRow++) { //for each row
        int iTallest = 0;
        for (int iCol =0; iCol < iNumCols ; iCol++) {
            CUGCell cellGrid;
            GetCell(iCol,iRow,&cellGrid);
            int iSavedDC =pDC->SaveDC();
            CFont*pFont = cellGrid.GetFont();
            if(pFont){
                pDC->SelectObject(pFont);
            }
            csText = QuickGetText(iCol,iRow);
            csText += _T("  ");
            sz = pDC->GetOutputTextExtent(csText);
            //use margins from dundas code ugcelltype
            sz.cx += 6;
            sz.cy += 4;
            pDC->RestoreDC(iSavedDC);
            if (sz.cy > iTallest) {
                iTallest = sz.cy;
            }
        }
        int iCurHeight = GetRowHeight(iRow);
        if (iCurHeight > iTallest) {
            continue;
        }
        if (iTallest > GRID_DEFAULT_ROWHEIGHT ) {
            SetRowHeight(iRow, iTallest);
        }
    }
    ApplyStateInfo();
    ReleaseDC(pDC);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::UpdateData()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::UpdateData()
{
    CWaitCursor wait;
    //Go through the CTable data area and format the text to put into the cells
    CMyTimer myTimer;
    int iIndex =0;
    int iNumcols = GetNumberCols();

    if(!m_pTable)
        return;

    myTimer.Start();
    CArray<CTabData*, CTabData*>&arrTabData =  m_pTable->GetTabDataArray();
    if(arrTabData.GetSize()  < 1)
        return;


    bool bHasPageNote =false;
    bool bHasEndNote = false;
    int iLastDataRow = m_iNumGridRows;
    int iPageNoteRow = -1;
    int iEndNoteRow = -1;
    double dValue =0;

    int iCol =0;

    CGTblCell* pGTblCell = nullptr;
    CGTblRow* pGTblRow = nullptr;
    CTabData* pTabData  = nullptr;

    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

    pTblFmt ?  bHasPageNote = pTblFmt->HasPageNote() : bHasPageNote = pDefTblFmt->HasPageNote();
    pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();


    if(bHasPageNote && bHasEndNote){
        iLastDataRow = iLastDataRow-2;

    }
    else if (bHasPageNote || bHasEndNote){
        iLastDataRow = iLastDataRow-1;
    }

    int iStartRow = m_iGridHeaderRows;
    int iEndRow = iLastDataRow;
    bool bIsDataCell =false;
    long lCurrCell = -1;
    int iRow  =-1;
    for(int iTbdSlice =0; iTbdSlice < arrTabData.GetSize(); iTbdSlice++){
        if(m_iCurrSelArea !=0 && iTbdSlice != m_iCurrSelArea -1){
            continue;
        }
        pTabData = arrTabData[iTbdSlice]; //zero for now when areas come in do the rest
        CArray<double, double&>& arrCells = pTabData->GetCellArray();
        //SetCell Data
        iIndex =0;
        int iNumCells = arrCells.GetSize();
        for(iRow = iStartRow; iRow < iLastDataRow; iRow++) {
            if(iIndex == iNumCells){
                break; //We are done with the slice
            }
            //if it is row group continue;
            // CUGCell cellGrid;
            pGTblRow = m_arrGTblRows[iRow-iStartRow];
            ASSERT(pGTblRow);
            if(!pGTblRow->GetTabVal() || pGTblRow->GetNumChildren() > 0){
                continue;
            }
            if(pGTblRow && pGTblRow->GetTblOb() != m_pTable->GetOneRowColTotal()){
                CTabValue* pTabVal = pGTblRow->GetTabVal();
                if(pTabVal && pTabVal->GetTabValType() == RDRBRK_TABVAL){//reader break no data
                    continue;
                }
                else if ( pGTblRow->GetNumChildren() > 0 ){
                    continue; //row group no data
                }
            }

            for (iCol = 1; iCol < iNumcols ; iCol++) {

                //   pGTblCell->SetDirty(true);
                bIsDataCell = pGTblRow->GetTabVal() && !pGTblRow->HasChildren();
                if(!bIsDataCell){
                    continue; //only data cells
                }
                dValue =0;
                //pGTblCell = GetGTblCell(iCol ,iRow);
                lCurrCell = (iRow-m_iGridHeaderRows)*(iNumcols-1) + (iCol-1);
                ASSERT(lCurrCell < m_arrCells.GetSize());
                pGTblCell  = m_arrCells[lCurrCell];

                ASSERT(pGTblCell);
                //pGTblCell->SetData(dValue);

                if(iIndex > iNumCells ){
                    ASSERT(FALSE); //grid cellls exceeds data cells
                }
                if(iIndex> iNumCells){
                    ASSERT(FALSE); //SAVY To add stuff for Notapplicable/Default/missing  etc into the
                    //crosstab command .
                }
                else {
                    dValue=  arrCells [iIndex];  // csc 11/12/03
                }
                iIndex++;

                ASSERT(pGTblCell);
                pGTblCell->SetData(dValue);
                //  pGTblCell->SetDirty(true);
            }

        }
        iStartRow = iRow;
    }
    myTimer.Stop();
    //myTimer.ShowMsg("time for updatedata");

    myTimer.Start();
    ApplyTransformation();
    myTimer.Stop();
    //myTimer.ShowMsg("time for transformation");

    if(m_pTable->m_bHasFreqStats){
        RenderFreqStats();
    }
    if(m_pTable->m_bHasFreqNTiles){
        RenderFreqNTiles();
    }
}
bool  CTblGrid::IsDataCell(int col, long row)
{
    bool bRet = false;
    if(row >= GetLastDataRow() || col < 1 || row < m_iGridHeaderRows){
        bRet = false;
    }
    else {
        bRet = true;
    }
    return bRet;
}


bool CTblGrid::IsCellHidden(int iCol, long iRow)
{
    CGTblOb* pGTblOb = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;
    GetComponent(iCol, iRow, eGridComp, &pGTblOb);
    ASSERT(pGTblOb);
    ASSERT(eGridComp != FMT_ID_INVALID);
    if (eGridComp==FMT_ID_DATACELL) {
        CDataCellFmt fmtTmp;
        GetFmt4DataCell(iCol, iRow, fmtTmp);
        return fmtTmp.GetHidden()==HIDDEN_YES;
    }
    else {
        CFmt fmtTmp;
        GetFmt4NonDataCell(pGTblOb, eGridComp, fmtTmp);
        return fmtTmp.GetHidden()==HIDDEN_YES;
    }
}

bool CTblGrid::IsCellBlocked(int iCol, long iRow)
{
    CGTblOb* pGTblOb = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;
    GetComponent(iCol, iRow, eGridComp, &pGTblOb);
    ASSERT(pGTblOb);
    return pGTblOb->GetBlocked();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ShowSpannerOrHorizMergedCell()
//
// only show spanner or merged cell if the corresponding col header is visible
// and not unblocked (and block only is on).
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::ShowSpannerOrHorizMergedCell(int iCol, bool bBlockedOnly)
{
    // its hidden if all of the col hdrs in the joined cells are not hidden
    // in which case this shouldn't be hidden either
    int iColHeadRow=GetNumHeaderRows()-1;
    int iJoinStartCol = iCol;
    long iJoinStartRow = iColHeadRow;
    int iJoinEndCol = iCol;
    long iJoinEndRow = iColHeadRow;
    GetJoinRange(&iJoinStartCol, &iJoinStartRow, &iJoinEndCol, &iJoinEndRow);
    ASSERT(iCol >= iJoinStartCol && iCol <= iJoinEndCol);
    for (int iHdrCol = iJoinStartCol; iHdrCol <= iJoinEndCol; ++iHdrCol) {
        if (!IsCellHidden(iCol, iColHeadRow) && (IsCellBlocked(iCol, iColHeadRow) || !bBlockedOnly)) {
            return true;
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ShowVertMergedCell()
//
// only show vertically merged cell if one or more unmerged cells in the
// corresponding row is visible
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::ShowVertMergedCell(long iRow, bool bBlockedOnly, bool bIncludeParents)
{
    for (int iCol = 0; iCol < GetNumberCols(); ++iCol) {
        int iJoinColStart = iCol;
        int iJoinColEnd = iCol;
        long iJoinRowStart = iRow;
        long iJoinRowEnd = iRow;
        GetJoinRange(&iJoinColStart, &iJoinRowStart, &iJoinColEnd, &iJoinRowEnd);
        if (iJoinRowStart != iJoinRowEnd) {
            continue; // vertically merged cell, don't consider it
        }
        CFmt f;
        CTableGridExporter::CJoinRegion j;
        if (ShouldExportCell(iCol, iRow, bBlockedOnly, bIncludeParents, f, j)) {
            return true;
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::FormatDataAfterRowTransformation()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::FormatDataAfterRowTransformation()
{
    ASSERT(FALSE); //not used any more . delete this . performance issues
    //Go through the CTable data area and format the text to put into the cells
    int iIndex =0;
    int iNumcols = GetNumberCols();

    if(!m_pTable)
        return;

    CArray<CTabData*, CTabData*>&arrTabData =  m_pTable->GetTabDataArray();
    if(arrTabData.GetSize()  < 1)
        return;

    bool bHasPageNote =false;
    bool bHasEndNote = false;
    int iLastDataRow = m_iNumGridRows;
    int iPageNoteRow = -1;
    int iEndNoteRow = -1;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

    CTabSetFmt* pTabSetFmt=DYNAMIC_DOWNCAST(CTabSetFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABSET));
    CIMSAString sZeroFill =pTabSetFmt->GetZeroMask();

    pTblFmt ?  bHasPageNote = pTblFmt->HasPageNote() : bHasPageNote = pDefTblFmt->HasPageNote();
    pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();


    if(bHasPageNote && bHasEndNote){
        iLastDataRow = iLastDataRow-2;

    }
    else if (bHasPageNote || bHasEndNote){
        iLastDataRow = iLastDataRow-1;
    }

    int iStartRow = m_iGridHeaderRows;
    int iEndRow = iLastDataRow;
    CUGCell cellGrid;
    CDataCellFmt retFmt;
    CIMSAString sVal;
    double dValue =0;
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    int iRow =-1;
    for(int iTbdSlice =0; iTbdSlice < arrTabData.GetSize(); iTbdSlice++){
        CTabData* pTabData = arrTabData[iTbdSlice]; //zero for now when areas come in do the rest
        CArray<double, double&>& arrCells = pTabData->GetCellArray();
        iIndex =0;
        int iNumCells = arrCells.GetSize();
        for(iRow = iStartRow; iRow < iLastDataRow; iRow++) {
            if(iIndex == iNumCells){
                break; //We are done with the slice
            }
            //if it is row group continue;
           // CUGCell cellGrid;
            GetCell(0,iRow,&cellGrid);
            CTblOb** pOb = (CTblOb**)cellGrid.GetExtraMemPtr();
            if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblRow)) ){
                CTabValue* pTabVal = ((CGTblRow*)(*pOb))->GetTabVal();
                if(pTabVal && pTabVal->GetTabValType() == RDRBRK_TABVAL){//reader break no data
                    continue;
                }
                else if ( ((CGTblRow*)(*pOb))->GetNumChildren() > 0 ){
                    continue; //row group no data
                }
            }


            for (int iCol = 1; iCol < iNumcols ; iCol++) {

                pGTblOb = nullptr;
                eGridComp = FMT_ID_INVALID;
                GetComponent(iCol,iRow,eGridComp,&pGTblOb);
                if(eGridComp != FMT_ID_DATACELL){
                    continue;
                }

                dValue =0;
                sVal =_T("");

                GetCell(iCol,iRow,&cellGrid);
                CTblOb** pOb = (CTblOb**)cellGrid.GetExtraMemPtr();
                if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblCell))){
                    dValue = ((CGTblCell*)(*pOb))->GetData();
                }

//                GetFmt4DataCell(iCol,iRow, retFmt);
                if(dValue <= 1.0e50){
                    sVal = m_pTable->FormatDataCell(dValue, m_pSpec->GetTabSetFmt(),&retFmt);
                }
                else {
                    //sVal = "";
                    sVal =sZeroFill;
                }
                cellGrid.SetText(sVal);
                SetCell(iCol,iRow,&cellGrid);
                //QuickSetText(iCol, iRow, sVal);
            }

        }
        iStartRow = iRow;
    }
}

void CTblGrid::RenderFreqStats()
{
    if(m_pTable->m_bHasFreqStats && m_pTable){
        bool bHasEndNote = false;
        CTblFmt* pTblFmt = m_pTable->GetDerFmt();
        CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

        pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();

        ASSERT(bHasEndNote);

        int iNumRows = GetNumberRows();
        int iNumCols = GetNumberCols();
        //Add Two rows to separate the data
      //  AppendRow();
      //  AppendRow();
        //Add a Row .
        /*int iTotalRows = GetNumberRows();
        //Join all the columns
        JoinCells(1,iTotalRows-1,iNumCols-1,iTotalRows-1);*/
        CIMSAString sString,sMsg;

        int iNumCats = (int)m_pTable->m_iTotalCategories;

        if (m_pTable->m_bAlphaFreqStats) {
            sMsg.Format(_T("%d alphanumeric categories"), iNumCats);
            sString += sMsg;
        }

        else {
            int iMax = (int)m_pTable->m_dFrqMaxCode;
            int iMin = (int)m_pTable->m_dFrqMinCode;
            sMsg.Format(_T("%d numeric categories, Min %d , Max %d , ") , iNumCats,iMin,iMax);
            sString += sMsg;

            sMsg.Format(_T("Mean: %4.2f , Std.Dev: %6.4f , Variance: %6.4f , ") , m_pTable->m_dFrqMean,m_pTable->m_dFrqStdDev,m_pTable->m_dFrqVariance) ;
            sString += sMsg;

            sMsg.Format(_T("Mode: %3.1f , Median: %6.4f") , m_pTable->m_dFrqModeCode,m_pTable->m_dFrqMedianInt) ;
            sString += sMsg;
        }

        sString = _T("Statistics : ") + sString;
        m_pTable->GetEndNote()->SetText(sString);
        return;
      /*  QuickSetCellTypeEx(0,iTotalRows-1,UGCT_NORMALMULTILINE);
        QuickSetText(0,iTotalRows-1,"Statistics : ");

        QuickSetCellTypeEx(1,iTotalRows-1,UGCT_NORMALMULTILINE);
        QuickSetText(1,iTotalRows-1,sString);*/


        /*CDC* pDC = GetDC();

        CSize sz = pDC->GetTextExtent(sString);
        int iTotalLines = 1 + (sz.cx + 5) / iTotalWidth;
        iTotalLines++;
        SetRowHeight(iTotalRows-1, iTotalLines*sz.cy);


        //Remove Border
        {
            CUGCell cell;

            int iRows = GetNumberRows();
            int iCols = GetNumberCols();
            for(int iCol = 0 ; iCol  < iCols ; iCol++) {
                QuickSetBorder(iCol,iTotalRows-2,0);
                if(iCol == iCols -1){
                    QuickSetBorder(iCol,iTotalRows-2,UG_BDR_RTHIN);
                }
            }
            for(iCol = 0 ; iCol  < iCols ; iCol++) {
                QuickSetBorder(iCol,iTotalRows-1,UG_BDR_BTHIN|UG_BDR_RTHIN);
            }

            for(iCol =0 ; iCol < iCols ; iCol++) {
                for(int iRow =0 ; iRow < iTotalRows-2 ; iRow++) {
                    QuickSetBorder(iCol,iRow,UG_BDR_BTHIN|UG_BDR_RTHIN);
                }
            }
        }
        ReleaseDC(pDC);*/
        /*Statistics : 2 numeric categories. Min 1, Max 2
        Mean: 1.51, Std.Dev: 0.4998, Variance: 0.2498
        Mode: 2, Median: 2 (1.0279)
        SumsNumCats: (freq) 24134.0, (cat*freq) 36547.0, (cat*cat*freq) 61373.0*/

        // make row multiline
        //Format the text to fit in to the cell
    }

}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::RenderFreqNTiles()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::RenderFreqNTiles()
{
    if( m_pTable != nullptr && m_pTable->m_bHasFreqNTiles )
    {
        AppendRow();

        const int EntriesPerNTile = 3;
        int iSize = m_pTable->m_arrFrqNTiles.GetSize();

        if( iSize == 0 || iSize % EntriesPerNTile != 0 || GetNumberCols() < EntriesPerNTile )
        {
             AfxMessageBox(_T("Invalid nTile Structure"));
             return;
         }

        auto add_and_set_cells = [&](const auto& c1, const auto& c2, const auto& c3)
        {
            int row_number = GetNumberRows();
            AppendRow();

            QuickSetText(0, row_number, c1);
            QuickSetText(1, row_number, c2);
            QuickSetText(2, row_number, c3);
        };

        add_and_set_cells(_T("Percentiles"), _T("Discontinuous"), _T("Continuous"));

        for( int iIndex = 0; iIndex < iSize; iIndex += EntriesPerNTile )
        {
            add_and_set_cells(m_pTable->m_arrFrqNTiles[iIndex],
                m_pTable->m_arrFrqNTiles[iIndex + 1], m_pTable->m_arrFrqNTiles[iIndex + 2]);
        }
    }
}




/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::DrawBitMap(bool bDrawNew /*=false*/)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::DrawBitMap(bool bDrawNew /*=false*/)
{
    CDC *pDC = m_CUGGrid->GetDC();
    int iSave = pDC->SaveDC();
    CPoint point;
    ::GetCursorPos(&point);
    this->ScreenToClient(&point);


    pDC->SelectObject(&m_font);
    if(oldPoint != CPoint(-1,-1) && m_pDragSourceItem) { //erase the old point
        //get size of text
        CSize sz;
        sz = pDC->GetOutputTextExtent(m_pDragSourceItem->GetText());

        CDC dcMemory;
        //create memory compatible HDC
        dcMemory.CreateCompatibleDC(pDC);

        int iMemDC = dcMemory.SaveDC();
        dcMemory.SetBkColor(RGB(0,0,0));
        dcMemory.SetTextColor(RGB(255,255,255));
        dcMemory.SelectObject(&m_font);

        CBitmap bitmap;
        //create small monochrome bitmap
        bitmap.CreateBitmap(sz.cx,sz.cy,1,1,nullptr);
        CBitmap* pOldBMap = dcMemory.SelectObject(&bitmap);

        //draw the text into the rectangle
        dcMemory.TextOut(0,0,m_pDragSourceItem->GetText());

        //draw text at POINT p
        pDC->BitBlt(oldPoint.x,oldPoint.y,sz.cx,sz.cy,&dcMemory,0,0,SRCINVERT);

        //cleanup
        dcMemory.RestoreDC(iMemDC);
        DeleteDC(dcMemory);

        oldPoint  =CPoint(-1,-1); // reset the old point we have erased

    }
    if(bDrawNew && m_pDragSourceItem){//draw new bitmap in the new place
        oldPoint = point;
        //get size of text
        CSize sz;
        sz = pDC->GetOutputTextExtent(m_pDragSourceItem->GetText());

        CDC dcMemory;
        //create memory compatible HDC
        dcMemory.CreateCompatibleDC(pDC);

        int iMemDC = dcMemory.SaveDC();
        dcMemory.SetBkColor(RGB(0,0,0));
        dcMemory.SetTextColor(RGB(255,255,255));
        dcMemory.SelectObject(&m_font);

        CBitmap bitmap;
        //create small monochrome bitmap
        bitmap.CreateBitmap(sz.cx,sz.cy,1,1,nullptr);
        CBitmap* pOldBMap = dcMemory.SelectObject(&bitmap);

        //draw the text into the rectangle
        dcMemory.TextOut(0,0,m_pDragSourceItem->GetText());

        //draw text at POINT p
        pDC->BitBlt(point.x,point.y,sz.cx,sz.cy,&dcMemory,0,0,SRCINVERT);

        //cleanup
        dcMemory.RestoreDC(iMemDC);
        DeleteDC(dcMemory);
    }
    pDC->RestoreDC(iSave);
    m_CUGGrid->ReleaseDC(pDC);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::DoDragScroll()
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::DoDragScroll()
{
    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();

    CDDTreeCtrl* pDictTree = pDoc->GetTabTreeCtrl()->GetDDTreeCtrl();

    CRect rectClient;
    m_CUGGrid->GetClientRect(&rectClient);
    CRect rect = rectClient;
    //nScrollInset is a COleDropTarget's static member variable
    //rect.InflateRect(-nScrollInset, -nScrollInset);
    int iRet = __max(m_GI->m_vScrollWidth,m_GI->m_hScrollHeight);

    int nScrollInset = 2*iRet;
    rect.InflateRect(-nScrollInset, -nScrollInset);
    CPoint point;
    ::GetCursorPos(&point);
    m_CUGGrid->ScreenToClient(&point);
    // hit-test against inset region
    if (rectClient.PtInRect(point) && !rect.PtInRect(point)) {
        UINT        uMsg;
        int         nCode;
        CScrollBar* pScrollBar=nullptr;
        // determine which way to scroll along both X & Y axis
        if (point.x<rect.left) {
            uMsg=WM_HSCROLL;
            nCode=SB_LINELEFT;
        }
        else if (point.x>=rect.right) {
            uMsg=WM_HSCROLL;
            nCode=SB_LINERIGHT;
        }
        if (point.y<rect.top) {
            uMsg=WM_VSCROLL;
            nCode=SB_LINEUP;
        }
        else if (point.y>=rect.bottom) {
            uMsg=WM_VSCROLL;
            nCode=SB_LINEDOWN;
        }
        if(m_GI->m_vScrollWidth != 0 && uMsg==WM_VSCROLL){// Vertical scroll is  present
           pDictTree->m_pDragImage->DragShowNolock(FALSE);
           OnVScroll(nCode, 0, nullptr);
           pDictTree->m_pDragImage->DragShowNolock(TRUE);
        }
        if(m_GI->m_hScrollHeight != 0 && uMsg==WM_HSCROLL){// horizontal scroll is  present
           pDictTree->m_pDragImage->DragShowNolock(FALSE);
           OnHScroll(nCode,0, nullptr);
           pDictTree->m_pDragImage->DragShowNolock(TRUE);
        }

    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ApplyStateInfo()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ApplyStateInfo()
{
    CTabView* pView = (CTabView*)GetParent();
    bool bDesignView = true;
    pView ? bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView() : bDesignView = true;

    int iRow  = GetNumHeaderRows() -1;
    int iNumCols = GetNumberCols();
    CMap<CTblBase*, CTblBase*, int, int> tblObMap;
    for(int iIndex = 1; iIndex < iNumCols;iIndex++){
        CUGCell cellGrid;
        GetCell(iIndex,iRow,&cellGrid);

        CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
            CTabValue* pTabVal = ((CGTblCol*)(*pTblOb))->GetTabVal();
            if(!pTabVal)
                continue;
            int iCurIndex = 0;
            if(tblObMap.Lookup(pTabVal,iCurIndex)){
                iCurIndex = iCurIndex + 1;
                tblObMap[pTabVal] = iCurIndex;
            }
            else {
                tblObMap[pTabVal] = iCurIndex;
            }
            int iSize = pTabVal->GetGrdViewInfoSize();
            if(iSize >0 && iCurIndex < iSize){
                CGrdViewInfo gridViewInfo = pTabVal->GetGrdViewInfo(iCurIndex);
                //if(((CGTblCol*)(*pTblOb))->m_szCurr.cx > GetColWidth(iIndex)){//if it is smaller than the minimal width do not apply stateinfo
               //if(gridViewInfo.GetCurrSize().cx > GetColWidth(iIndex)){//if it is smaller than the minimal width do not apply stateinfo
                if(gridViewInfo.GetCurrSize().cx !=0){//
                    ((CGTblCol*)(*pTblOb))->m_szCurr = CSize(gridViewInfo.GetCurrSize().cx,gridViewInfo.GetCurrSize().cy);
                    SetColWidth(iIndex,((CGTblCol*)(*pTblOb))->m_szCurr.cx);
                }
            }
        }
    }
    tblObMap.RemoveAll();
    iRow  = GetNumHeaderRows();
    int iNumRows  = GetNumberRows();
    for(int iIndex = iRow; iIndex < iNumRows;iIndex++){
        CUGCell cellGrid;
        GetCell(0,iIndex,&cellGrid);

        CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();

        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
            CTabValue* pTabVal = ((CGTblRow*)(*pTblOb))->GetTabVal();
            if(!pTabVal)
                continue;
            int iCurIndex = 0;
            if(tblObMap.Lookup(pTabVal,iCurIndex)){
                iCurIndex = iCurIndex + 1;
                tblObMap[pTabVal] = iCurIndex;
            }
            else {
                tblObMap[pTabVal] = iCurIndex;
            }
            int iSize = pTabVal->GetGrdViewInfoSize();
            if(iSize >0 && iCurIndex < iSize){
                CGrdViewInfo gridViewInfo = pTabVal->GetGrdViewInfo(iCurIndex);
                // if(((CGTblRow*)(*pTblOb))->m_szCurr.cy > GetRowHeight(iIndex)){
                //if(gridViewInfo.GetCurrSize().cy > GetRowHeight(iIndex)){
                if(gridViewInfo.GetCurrSize().cy != 0){
                    ((CGTblRow*)(*pTblOb))->m_szCurr = CSize(gridViewInfo.GetCurrSize().cx,gridViewInfo.GetCurrSize().cy);
                    SetRowHeight(iIndex,((CGTblRow*)(*pTblOb))->m_szCurr.cy);
                }
            }
        }
    }

    //Now Make sure that all the spanners are visible if not increase their row height
    CDC *pDC = m_CUGGrid->GetDC();
    int iStartCol , iEndCol ;
    long lStartRow,lEndRow;
    int iSavedDC = pDC->SaveDC();
    CDC dcMem;
    CBitmap bmp;  // for use with mem DC
    bmp.CreateCompatibleBitmap(pDC, m_GI->m_gridWidth,m_GI->m_gridHeight);
    dcMem.CreateCompatibleDC(pDC);
    CBitmap* pOldBmp = dcMem.SelectObject(&bmp);

    for(int iRow =0 ;iRow <GetNumHeaderRows(); iRow++){
        int iCurRowHeight = GetRowHeight(iRow);
        for(int iCol =0;iCol < GetNumberCols(); iCol++){
            CUGCell cell;
            iStartCol = iEndCol =iCol;
            lStartRow=lEndRow  = iRow;

            GetJoinRange(&iStartCol, &lStartRow, &iEndCol, &lEndRow);
            if(iStartCol != iCol ){
                continue;
            }
            GetJoinStartCell(&iStartCol,&lStartRow,&cell);
            int iWidth =0;
            bool bZeroWidth =false;
            for(int iColIndx = iStartCol; iColIndx <= iEndCol; iColIndx++){
                bZeroWidth =false;
                CGTblOb *pGTblOb = nullptr;
                FMT_ID eGridComp;
                GetComponent(iColIndx,GetNumHeaderRows() -1,eGridComp,&pGTblOb);
                if(!(eGridComp == FMT_ID_COLHEAD || eGridComp == FMT_ID_SPANNER)){
                    continue;
                }
                else {
                    CGTblCol* pTblCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                    if(pTblCol && pTblCol->GetTabVal()){
                        CFmt retFmt;
                        GetFmt4NonDataCell(pGTblOb,eGridComp, retFmt);
                        if(retFmt.GetHidden() == HIDDEN_YES){
                            //SetColWidth(iCol,0);
                            bDesignView ? bZeroWidth =false : bZeroWidth = true;
                        }
                    }
                }
                bZeroWidth ? iWidth = iWidth : iWidth+=GetColWidth(iColIndx);

            }


            CIMSAString sText = cell.GetText();
            CSize sz;
            sText += _T("  ");
            CRect rcDraw(0,0,iWidth,0);
            int offset =0;
            short alignment = cell.GetAlignment();
            if(cell.GetPropertyFlags() &UGCELL_ALIGNMENT_SET){
                alignment = cell.GetAlignment();
            }
            else{
                alignment = 0;
            }

            //text alignment
           /* if(alignment){
                CSize size(0,0);
                if(alignment&UG_ALIGNCENTER){
                    GetTextExtentPoint(dcMem.m_hDC,sText,cell.GetTextLength(),&size);
                    left = rcDraw.left + (rcDraw.right - rcDraw.left - size.cx) /2;
                }
                else if(alignment&UG_ALIGNRIGHT){
                    GetTextExtentPoint(dcMem.m_hDC,sText,cell.GetTextLength(),&size);
                    left = rcDraw.right - size.cx - m_GI->m_margin;
                }
                else{
                    left =  rcDraw.left + m_GI->m_margin + offset;
                }

                if(alignment & UG_ALIGNVCENTER){
                    CRect rtTemp = rcDraw;
                    iVertOffset = dcMem.DrawText(sText, cell.GetTextLength(), rtTemp, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT);
                    top = rcDraw.top + (rcDraw.bottom -rcDraw.top - iVertOffset) / 2;
                }
                else if(alignment & UG_ALIGNBOTTOM){
                    CRect rtTemp = rcDraw;
                    iVertOffset = dcMem.DrawText(sText, cell.GetTextLength(), rtTemp, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT);
                    top =  rcDraw.bottom - iVertOffset - 1;
                }
                else{
                    top = rcDraw.top + 1;
                }
            }
            else{
                left = rcDraw.left + m_GI->m_margin + offset;
                top = rcDraw.top + 1;
            }*/

            if(true){ // multiline
                // set up a default format
                UINT format = DT_WORDBREAK | DT_NOPREFIX;

                // check alignment - multiline
                if(alignment) {
                    if(alignment & UG_ALIGNCENTER) {
                        format |= DT_CENTER;
                    }
                    else if(alignment & UG_ALIGNRIGHT) {
                        format |= DT_RIGHT;
                        rcDraw.right -= 2;
                    }
                    else if(alignment & UG_ALIGNLEFT) {
                        format |= DT_LEFT;
                        rcDraw.left += 2;
                    }
                    /*if (alignment & UG_ALIGNVCENTER) {                      // BMD
                        rcDraw.top = top;
                    }
                    if (alignment & UG_ALIGNBOTTOM) {                       // BMD
                        rcDraw.top = top;
                    }*/

                }
                CFont*pFont = cell.GetFont();
                if(pFont){
                    dcMem.SelectObject(pFont);
                }
                // calc bounding rectangle for this string, based on formatting; store result in a tmp rect so we can adjust
                /*int tmpAdd = 0;
                rcDraw.top <0? tmpAdd = abs(rcDraw.top) : tmpAdd =0;*/
                rcDraw.top =0;
                dcMem.DrawText(sText, &rcDraw, format|DT_CALCRECT);

                // nuke our special DC
                dcMem.SelectStockObject(OEM_FIXED_FONT);

                if (rcDraw.Height() /*+tmpAdd*/ > iCurRowHeight) {
                    iCurRowHeight =rcDraw.Height() ;//+tmpAdd;
                }
            }
        }
        SetRowHeight(iRow,iCurRowHeight);

    }
    int iStubHeadRowIndex = 0;
    for(int iRow = 0; iRow < GetNumHeaderRows() ; iRow++){
        int iCurRowHeight = GetRowHeight(iRow);
        CUGCell cellGrid;
        CGTblOb *pGTblOb = nullptr;
        FMT_ID eGridComp;
        GetCell(0,iRow,&cellGrid);
        GetComponent(0,iRow,eGridComp,&pGTblOb);
        CTblOb* pTblOb = pGTblOb->GetTblOb();
        if(!pTblOb){
            continue;
        }
        CGrdViewInfo  gridViewInfo ;
        switch(eGridComp){
            case FMT_ID_TITLE:
            case FMT_ID_SUBTITLE:
                if(pTblOb->GetGrdViewInfoSize() > 0) {
                    CGrdViewInfo gridViewInfo = pTblOb->GetGrdViewInfo(0);
                    //if(gridViewInfo.GetCurrSize().cy > iCurRowHeight){
                    if(gridViewInfo.GetCurrSize().cy != 0){
                        SetRowHeight(iRow,gridViewInfo.GetCurrSize().cy);
                    }
                }
                break;
            case FMT_ID_STUBHEAD:
                if(pTblOb->GetGrdViewInfoSize() > 0 && pTblOb->GetGrdViewInfoSize() > iStubHeadRowIndex) {
                    CGrdViewInfo gridViewInfo = pTblOb->GetGrdViewInfo(iStubHeadRowIndex++);
                    //if(gridViewInfo.GetCurrSize().cy > iCurRowHeight){
                    if(gridViewInfo.GetCurrSize().cy != 0 ){
                        SetRowHeight(iRow,gridViewInfo.GetCurrSize().cy);
                        if(iStubHeadRowIndex ==1 && gridViewInfo.GetCurrSize().cx != 0 ){
                            SetColWidth(0,gridViewInfo.GetCurrSize().cx);
                        }
                    }
                }
                break;
            default:
                break;

        }

    }
    ApplyPageAndEndNoteStateInfo();
    dcMem.SelectObject(pOldBmp);
    dcMem.DeleteDC();
    bmp.DeleteObject();
    pDC->RestoreDC(iSavedDC);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::SaveStateInfo()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::SaveStateInfo()
{

    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    bool bModified = false;
    int iRow  = GetNumHeaderRows() -1;
    int iNumCols = GetNumberCols();
    CMap<CTblBase*, CTblBase*, int, int> tblObMap;
    for(int iIndex = 1; iIndex < iNumCols;iIndex++){
        CUGCell cellGrid;
        GetCell(iIndex,iRow,&cellGrid);

        CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
            CTabValue* pTabVal = ((CGTblCol*)(*pTblOb))->GetTabVal();
            int iSize = pTabVal->GetGrdViewInfoSize();
            int iCurIndex = 0;
            if(tblObMap.Lookup(pTabVal,iCurIndex)){
                iCurIndex = iCurIndex + 1;
                if(iSize -1 >= iCurIndex) {
                    CGrdViewInfo& gridViewInfo = pTabVal->GetGrdViewInfo(iCurIndex);
                    gridViewInfo.SetCurrSize(CSize(GetColWidth(iIndex),0));
                }
                else {
                    CGrdViewInfo  gridViewInfo ;
                    gridViewInfo.SetCurrSize(CSize(GetColWidth(iIndex),0));
                    pTabVal->AddGrdViewInfo(gridViewInfo);
                }
                bModified = true;
            }
            else {
                if(iSize -1 >= iCurIndex) {
                    CGrdViewInfo& gridViewInfo = pTabVal->GetGrdViewInfo(iCurIndex);
                    gridViewInfo.SetCurrSize(CSize(GetColWidth(iIndex),0));
                }
                else {
                    CGrdViewInfo  gridViewInfo ;
                    gridViewInfo.SetCurrSize(CSize(GetColWidth(iIndex),0));
                    pTabVal->AddGrdViewInfo(gridViewInfo);
                }
                bModified = true;
            }

        }
    }
    tblObMap.RemoveAll();

    iRow  = GetNumHeaderRows();
    int iNumRows  = GetNumberRows();
    for(int iIndex = iRow; iIndex < iNumRows;iIndex++){
        CUGCell cellGrid;
        GetCell(0,iIndex,&cellGrid);

        CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
            CTabValue* pTabVal = ((CGTblRow*)(*pTblOb))->GetTabVal();
            if(!pTabVal)
                continue;
            int iSize = pTabVal->GetGrdViewInfoSize();
            int iCurIndex = 0;
            if(tblObMap.Lookup(pTabVal,iCurIndex)){
                iCurIndex = iCurIndex + 1;
                if(iSize -1 >= iCurIndex) {
                    CGrdViewInfo& gridViewInfo = pTabVal->GetGrdViewInfo(iCurIndex);
                    gridViewInfo.SetCurrSize(CSize(0,GetRowHeight(iIndex)));
                }
                else {
                    CGrdViewInfo  gridViewInfo ;
                    gridViewInfo.SetCurrSize(CSize(0,GetRowHeight(iIndex)));
                    pTabVal->AddGrdViewInfo(gridViewInfo);
                }
                bModified = true;
            }
            else {
                if(iSize -1 >= iCurIndex) {
                    CGrdViewInfo& gridViewInfo = pTabVal->GetGrdViewInfo(iCurIndex);
                    gridViewInfo.SetCurrSize(CSize(0,GetRowHeight(iIndex)));
                }
                else {
                    CGrdViewInfo  gridViewInfo ;
                    gridViewInfo.SetCurrSize(CSize(0,GetRowHeight(iIndex)));
                    pTabVal->AddGrdViewInfo(gridViewInfo);
                }
                bModified = true;
            }

        }
    }
    //Save row heights of Title/ SubTitle/Row
    bool bFirstStubhead = true;
    for(int iRow = 0; iRow < GetNumHeaderRows() ; iRow++){
        CUGCell cellGrid;
        CGTblOb *pGTblOb = nullptr;
        FMT_ID eGridComp;
        GetCell(0,iRow,&cellGrid);
        GetComponent(0,iRow,eGridComp,&pGTblOb);
        CTblOb* pTblOb = pGTblOb->GetTblOb();
        if(!pTblOb){
            continue;
        }
        CGrdViewInfo  gridViewInfo ;
        switch(eGridComp){
            case FMT_ID_TITLE:
            case FMT_ID_SUBTITLE:
                {
                    pTblOb->RemoveAllGrdViewInfo();
                    gridViewInfo.SetCurrSize(CSize(0,GetRowHeight(iRow)));
                    pTblOb->AddGrdViewInfo(gridViewInfo);
                }
                break;
            case FMT_ID_STUBHEAD:
                if(bFirstStubhead){
                    pTblOb->RemoveAllGrdViewInfo();
                    bFirstStubhead = false;
                }
                gridViewInfo.SetCurrSize(CSize(GetColWidth(0),GetRowHeight(iRow)));
                pTblOb->AddGrdViewInfo(gridViewInfo);
                break;
            default:
                break;

        }

    }
    SavePageAndEndNoteStateInfo();

    if(bModified){
        pDoc->SetModifiedFlag(TRUE);
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblGrid::IsValidCell4InlineEdit(int iCol, long lRow)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::IsValidCell4InlineEdit(int iCol, long lRow)
{
    bool bRet = true;
    if (iCol < 0 || iCol > GetNumberCols() - 1) {
        bRet = false;
    }
    if(lRow < 0 || lRow > GetNumberRows() - 1){
        bRet = false;
    }
    return bRet;
}


void CTblGrid::DoRowTransformation(CGTblRow* pStartRow)
{
    CTabValue* pTabVal = nullptr;
    CTabVar* pTabVar = pStartRow->GetTabVar();
    ASSERT(pTabVar);

    //Get the percent position
    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();

    bool bRow = m_pTable->IsRowVar(pTabVar);
    CTallyFmt* pDefTallyFmt = nullptr;
    FMT_ID eFmtID = FMT_ID_INVALID;
    bRow ? eFmtID = FMT_ID_TALLY_ROW :eFmtID = FMT_ID_TALLY_COL;
    pDefTallyFmt= DYNAMIC_DOWNCAST(CTallyFmt,m_pSpec->GetFmtReg().GetFmt(eFmtID));
    ASSERT(pDefTallyFmt);

    CTallyFmt* pTallyFmt = pTabVar->GetTallyFmt();
//    CTallyFmt* pDefTallyFmt =DYNAMIC_DOWNCAST(CTallyFmt,m_pSpec->GetFmtReg().GetFmt(FMT_ID_TALLY));
    (!pTallyFmt) ? pTallyFmt =pDefTallyFmt: pTallyFmt;
    ASSERT(pTallyFmt);
    ASSERT(pTallyFmt->GetID() == eFmtID);

    CArray<CTallyFmt::InterleavedStatPair> aInterleavedStats;
    pTallyFmt->GetInterleavedStats(aInterleavedStats);
    for (int iPair = 0; iPair < aInterleavedStats.GetCount(); ++ iPair) {
        const CTallyFmt::InterleavedStatPair& p = aInterleavedStats.GetAt(iPair);
        int iFirstStat = p.m_first;
        int iSecondStat = p.m_second;

        int iStartRow = pStartRow->GetStartRow();
        int iMaxRows = GetLastDataRow();
        CGTblRow* pGTblRow = nullptr;

        CArray<CGTblCell*,CGTblCell*> arrDataCells;
        CArray<double,double> arrFirst;
        CArray<double,double> arrSecond;
        CGTblCell* pGTblCell = nullptr;
        int iIndex = iStartRow;
        bool bAreaBreak =false;
        bool bVarDone = false;
        while(iIndex < iMaxRows) {
            if(bAreaBreak){
                if(!IsOneRowVarTable()){//done with this area for this var
                    break;
                }
                else {//proceed to the next area for this var
                    iIndex++;
                    bAreaBreak = false;
                }
            }
            else if(bVarDone){
                break;
            }

            arrFirst.RemoveAll();
            arrSecond.RemoveAll();
            arrDataCells.RemoveAll();
            pGTblCell = nullptr;
            iStartRow = iIndex;
            for(iIndex =iStartRow; iIndex < iMaxRows; iIndex++){
                CGTblRow* pGTblRow = m_arrGTblRows[iIndex-m_iGridHeaderRows];
                if(pGTblRow->GetTblOb() == m_pTable->GetAreaCaption()){
                    bAreaBreak = true;
                    break;
                }
                else if(pGTblRow->GetTabVar() != pTabVar ){
                    bVarDone = true;
                    break;
                }
                pTabVal = pGTblRow->GetTabVal();
                if(!pTabVal){
                    continue;
                }
                if(pTabVal->GetStatId() == iFirstStat || pTabVal->GetStatId() == iSecondStat) {
                        //Add to Array Of Counts
                        for(int iCol = 1; iCol <GetNumberCols(); iCol++){
                            pGTblCell = GetGTblCell(iCol,iIndex);
                            ASSERT(pGTblCell);
                            arrDataCells.Add(pGTblCell);
                        }
                    }
                else if(pTabVal->GetTabValType()== RDRBRK_TABVAL){
                    continue;
                }
            }

            //Separate Cells into first and second vals
            int iCell = 0;
            for(; iCell < arrDataCells.GetSize() /2 ; iCell++){
                arrFirst.Add(arrDataCells[iCell]->GetData());
            }
            for(; iCell < arrDataCells.GetSize() ; iCell++){
                arrSecond.Add(arrDataCells[iCell]->GetData());
            }

            ASSERT(arrFirst.GetSize() == arrSecond.GetSize());
            bool bFromFirst = true;

            int iFirstIndex=0;
            int iSecondIndex=0;
            iCell =0;
            while(iCell < arrDataCells.GetSize()){
                double before = arrDataCells[iCell]->GetData();
                for(int iCol =1; iCol < GetNumberCols(); iCol++){
                    if(bFromFirst){
                        arrDataCells[iCell]->SetData(arrFirst[iFirstIndex]);
                        iFirstIndex++;
                    }
                    else {
                        arrDataCells[iCell]->SetData(arrSecond[iSecondIndex]);
                        iSecondIndex++;
                    }
                    iCell++;
                }
                bFromFirst = ! bFromFirst;
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ApplyTransformation()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ApplyTransformation()
{

    int iIndex =0;
    int iNumcols = GetNumberCols();

    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();

    if(!m_pTable)
        return;

    CArray<CTabData*, CTabData*>&arrTabData =  m_pTable->GetTabDataArray();
    if(arrTabData.GetSize()  < 1)
        return;

    CUGCell cellGrid;
    //CTabData* pTabData = arrTabData[0]; //zero for now when areas come in do the rest
///////////////////////////////////////////////////////////////////////////////////////////
    ///BEGIN ROW TRANSFORMATION
//////////////////////////////////////////////////////////////////////////////////////////
    //if you have only one element in the row then start the process from the stub
    if(IsOneRowVarTable()) {
        CTabVar* pTabVar = m_pTable->GetRowRoot()->GetChild(0);
        //bool bPercents  = pDoc->GetTableSpec()->HasCountsNPercent(pTabVar);
        CGTblRow* pGTblRow = nullptr;
        for(int iIndex =0; iIndex < m_arrGTblRows.GetSize(); iIndex++){
            if(m_arrGTblRows [iIndex]->GetTabVar() == pTabVar){
                pGTblRow = m_arrGTblRows [iIndex];
                break;
            }
        }
        if(pGTblRow){
            DoRowTransformation(pGTblRow);
        }
    }
    else {
        //Do the transformation on rows
        int iRow =0;
        long lMaxRows = GetLastDataRow();
        for(iRow = m_iGridHeaderRows; iRow < lMaxRows; iRow++) {
            //if it is row group continue;
            CGTblRow* pGTblRow = m_arrGTblRows[iRow-m_iGridHeaderRows];
            ASSERT(pGTblRow);
            if(pGTblRow ){
                if (pGTblRow->GetNumChildren() > 0 ) { //Row Col group
                    CTabVar* pTabVar = pGTblRow->GetTabVar();
                    if(pTabVar->GetNumChildren()>0){
                        continue ;
                    }
                    else {
                        // if children present
                        //Check if "Percent" is present
                        //bool bPercents = pDoc->GetTableSpec()->HasCountsNPercent(pTabVar);
                        DoRowTransformation(pGTblRow);
                    }

                }
                else {
                    continue;
                }
            }


        }
    }
//////////////////////////////////////////////////////////////////////////////////////////
    ///END ROW TRANSFORMATION
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
    //BEGIN COLUMN TRANSFORMATION
//////////////////////////////////////////////////////////////////////////////////////////
    //Do the transformation on columns

    int iRow  = GetNumHeaderRows() -1;
    int iNumCols = GetNumberCols();
    CMap<CTblOb*, CTblOb*, int, int> tblObMap;
    for(iIndex = 1; iIndex < iNumCols;iIndex++){
        CUGCell cellGrid;
        GetCell(iIndex,iRow,&cellGrid);

        CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
            CGTblCol* pTblCol = (CGTblCol*)(*pTblOb);
            CGRowColOb* pParent = DYNAMIC_DOWNCAST(CGRowColOb,pTblCol->GetParent());
            ASSERT(pParent);

            CTallyFmt* pTallyFmt = pTblCol->GetTabVar()->GetTallyFmt();
            if(!pTallyFmt){
                bool bRow = m_pTable->IsRowVar(pTblCol->GetTabVar());
                FMT_ID eFmtID = FMT_ID_INVALID;
                bRow ? eFmtID = FMT_ID_TALLY_ROW :eFmtID = FMT_ID_TALLY_COL;
                pTallyFmt= DYNAMIC_DOWNCAST(CTallyFmt,m_pSpec->GetFmtReg().GetFmt(eFmtID));
            }

            CArray<CTallyFmt::InterleavedStatPair> aInterleavedStats;
            pTallyFmt->GetInterleavedStats(aInterleavedStats);
            for (int iPair = 0; iPair < aInterleavedStats.GetCount(); ++ iPair) {

                const CTallyFmt::InterleavedStatPair& p = aInterleavedStats.GetAt(iPair);
                int iFirstStat = p.m_first;
                int iSecondStat = p.m_second;
                int iChild = 0;
                for (iChild = 0; iChild < pParent->GetNumChildren(); iChild++){
                    CTabValue* pTabVal = pParent->GetChild(iChild)->GetTabVal();
                    if (pTabVal && pTabVal->GetStatId() == iFirstStat) {
                        break;
                    }
                }
                ASSERT(iChild < pParent->GetNumChildren());
                int iStartChild = iChild;
                for (iChild = pParent->GetNumChildren()-1; iChild >=0  ; iChild--){
                    CTabValue* pTabVal = pParent->GetChild(iChild)->GetTabVal();
                    if (pTabVal && pTabVal->GetStatId() == iSecondStat) {
                        break;
                    }
                }
                ASSERT(iChild >= 0);
                int iEndChild;
                iEndChild = iChild;

                //Get first half which has firsts
                //Store them in firsts array

                CArray<double,double&> arrFirsts;
                double dValue =0;

                int iTotalCat = (iEndChild-iStartChild+1)/2;
                int iFirstsCol = 0;
                for(iFirstsCol = iIndex+iStartChild; iFirstsCol < iIndex+iStartChild+iTotalCat ;iFirstsCol++){
                    for (int iRow = m_iGridHeaderRows; iRow < m_iNumGridRows ; iRow++) {
                        dValue =0;
                        CGTblCell* pGTblCell = GetGTblCell(iFirstsCol,iRow);
                        dValue = pGTblCell->GetData();
                        arrFirsts.Add(dValue);
                    }
                }
                //Get next half which has seconds //store the  seconds in seconds array
                CArray<double,double&> arrSeconds;
                for(int iSecondsCol = iFirstsCol; iSecondsCol < iFirstsCol+iTotalCat ;iSecondsCol++){
                    for (int iRow = m_iGridHeaderRows; iRow < m_iNumGridRows ; iRow++) {
                        dValue =0;
                        CGTblCell* pGTblCell = GetGTblCell(iSecondsCol,iRow);
                        ASSERT(pGTblCell);
                        dValue = pGTblCell->GetData();
                        arrSeconds.Add(dValue);
                    }
                }
                //Now arrange using alternate mode;
                int iFirstsIndex=0;
                int iSecondsIndex=0;
                int iLocalIndex=0;
                for(int iCountsCol = iIndex+iStartChild; iCountsCol < iIndex+iStartChild+iTotalCat*2 ;iCountsCol++){
                    for (int iRow = m_iGridHeaderRows; iRow < m_iNumGridRows ; iRow++) {
                        CUGCell cellGrid;
                        CIMSAString sVal;
                        if((iLocalIndex)% 2 == 0){
                            dValue =0;
                            CGTblCell* pGTblCell = GetGTblCell(iCountsCol,iRow);
                            ASSERT(pGTblCell);

                            pGTblCell->SetData(arrFirsts[iFirstsIndex]);
                            iFirstsIndex++;
                        }
                        else{
                            dValue =0;
                            CGTblCell* pGTblCell = GetGTblCell(iCountsCol,iRow);
                            ASSERT(pGTblCell);
                            pGTblCell->SetData(arrSeconds[iSecondsIndex]);
                            iSecondsIndex++;
                        }
                    }
                    iLocalIndex++;
                }

            }

            iIndex += ((CGTblCol*)pTblCol->GetParent())->GetNumGridCols()-1;

        }

//////////////////////////////////////////////////////////////////////////////////////////
    //END COLUMN TRANSFORMATION
//////////////////////////////////////////////////////////////////////////////////////////
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//          PutHTMLTable
//
/////////////////////////////////////////////////////////////////////////////////
int CTblGrid::PutHTMLTable(_tostream& os, bool bBlockedOnly /*=true*/, bool bIncludeParents /*=true*/)
{
    CTabView* pView = (CTabView*)GetParent();
    CTableGridExporterHTML htmlExporter(pView->GetDC()->GetDeviceCaps(LOGPIXELSY));
    PutTable(htmlExporter, os, bBlockedOnly, bIncludeParents);

    return 1;
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CTblGrid::PutRTFTable
//
/////////////////////////////////////////////////////////////////////////////
int CTblGrid::PutRTFTable(_tostream& os, bool bBlockedOnly /*=true*/, bool bIncludeParents /*=true*/)
{
    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();

    // tbl print fmt
    CTblPrintFmt fmtTblPrint;
    if (nullptr!=m_pTable->GetTblPrintFmt()) {
        fmtTblPrint=*m_pTable->GetTblPrintFmt();
        fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));
    }
    else {
        fmtTblPrint=*DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
    }

    CTableGridExporterRTF rtfExporter(fmtTblPrint.GetPageMargin().left, fmtTblPrint.GetPageMargin().right,
                                      pView->GetDC()->GetDeviceCaps(LOGPIXELSY));
    PutTable(rtfExporter, os, bBlockedOnly, bIncludeParents);

    return 1;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CTblGrid::PutASCIITable
//
//      Tab-delimited format
//
/////////////////////////////////////////////////////////////////////////////
int CTblGrid::PutASCIITable(_tostream& os, bool bBlockedOnly /*=true*/, bool bIncludeParents /*=true*/)
{
    CTableGridExporterASCII asciiExporter;
    PutTable(asciiExporter, os, bBlockedOnly, bIncludeParents);

    return 1;
}

/*
void CTblGrid::GetRowHeaders(long iRow, CDWordArray& aRowHeaders)
{
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    GetComponent(0, iRow, eGridComp, &pGTblOb);
    bool bIsAreaCaption = (eGridComp == FMT_ID_AREA_CAPTION);
    CGRowColOb* pRow = DYNAMIC_DOWNCAST(CGRowColOb,pGTblOb);
    if (pRow) {
        if (pRow != GetRowRoot()) { // stop recursion when you get to topmost parent
            // find the parent in the grid
            CGTblOb* pParent = pRow->GetParent();
            bool bIsTopLevelNotArea = (HasArea() && pParent == GetRowRoot() && !bIsAreaCaption);
            int iIndexInParent = 1;
            long i = iRow - 1;
            for (; i >= 0; --i) {
                GetComponent(0, i, eGridComp, &pGTblOb);
                CGRowColOb* pR = DYNAMIC_DOWNCAST(CGRowColOb,pGTblOb);
                if (pR) {
                    if (!bIsAreaCaption && !bIsTopLevelNotArea) {
                        if (pR == pParent) {
                            // found parent - stop here
                            break;
                        }
                        else if (pR->GetParent() == pParent) {
                            // found a sibling (same parent), increment index in parent
                            ++iIndexInParent;
                        }
                    }
                    else if (bIsTopLevelNotArea) {
                        // if table has area, need to treat top level items that are not
                        // area captions as if they were children of area captions
                        if (eGridComp == FMT_ID_AREA_CAPTION) {
                            // found parent - stop here
                            break;
                        }
                        else if (pR->GetParent() == pParent) {
                            // found a sibling (same parent), increment index in parent
                            ++iIndexInParent;
                        }
                    }
                    else {
                        // must be area caption
                        // area captions are special - parent is always row root
                        // so don't stop until we get to top of table
                        // siblings are other area captions
                        if (eGridComp == FMT_ID_AREA_CAPTION) {
                            // found a sibling (same parent), increment index in parent
                            ++iIndexInParent;
                        }
                    }
                }
            }
            // either found parent and hit break above or got to top of table
            // in which case parent must be row root
            aRowHeaders.InsertAt(0, iIndexInParent);
            if (i >= 0) {
                GetRowHeaders(i, aRowHeaders); // recurse to get parent headers
            }
        }
    }
}
*/

int CTblGrid::GetRowStubLevel(long iRow)
{
    ASSERT(iRow >= 0 && iRow < GetNumberRows());

    // skip this row if stub is not first cell of vertical join
    int iJoinStartCol = 0;
    int iJoinEndCol = 0;
    long iJoinStartRow = iRow;
    long iJoinEndRow = iRow;
    GetJoinRange(&iJoinStartCol, &iJoinStartRow, &iJoinEndCol, &iJoinEndRow);
    if (iRow != iJoinStartRow) {
        return NONE;
    }

    int iLevel = NONE;

    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    GetComponent(0, iRow, eGridComp, &pGTblOb);
    bool bHasStubHeader = !SO::IsBlank(m_GStubHead.GetTblOb()->GetText());
    bool bIsStubHeader = (pGTblOb == &m_GStubHead);
    bool bIsAreaCaption = (eGridComp == FMT_ID_AREA_CAPTION);
    if (bIsAreaCaption && HasArea()) {
        iLevel = 1; // area caption is always level one - direct child of row root
        if (bHasStubHeader) {
            ++iLevel; // actually that isn't always true, if there is stub header
                      // then use the stub header as level 1 and the area caption as level 2
        }
    }
    else if (bIsStubHeader && bHasStubHeader) {
        iLevel = 1; // if there is a stub header, it will be level 1
    }
    else {
        CGRowColOb* pRow = DYNAMIC_DOWNCAST(CGRowColOb,pGTblOb);
        if (pRow) {
            iLevel = pRow->GetCurLevel();

            if (HasArea()) {
                // if table has area then increase the level of everything
                // other than area caption and stub header by one since grid treats area captions
                // at same level as other top-level captions (level 1) but we want
                // other captions to be at a lower level for HTML output (headers)
                ++iLevel;
            }
        }
    }
    return iLevel;
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CTblGrid::PutTable
//
/////////////////////////////////////////////////////////////////////////////
void CTblGrid::PutTable(CTableGridExporter& exporter, _tostream& os,
                       bool bBlockedOnly /* =true */, bool bIncludeParents /* =true */)
{
    ForceUpdate();

    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();

    exporter.StartFile(os);

    if( !exporter.IgnoreFormatting() ) // 20100818 will reduce time when copying to ASCII
        PutFormats(exporter, os, bBlockedOnly, bIncludeParents);

    // count number of columns to export (can be less than total number of columns if
    // some are hidden or not selected)
    int iNumExportColumns = 0;
    const int iColHeadRow=GetNumHeaderRows()-1;
    for (int iCol=0 ; iCol<GetNumberCols() ; iCol++) {
        CFmt fmt;
        CTableGridExporter::CJoinRegion join;

        if (ShouldExportCell(iCol, iColHeadRow, bBlockedOnly, bIncludeParents, fmt, join)) {
            ++iNumExportColumns;
        }
    }

    exporter.StartTable(os, iNumExportColumns);

    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;

    ///////////////////////////////////////////////////////////
    // Put out title, if there is one ...
    if (bIncludeParents) {
        CTblOb* pTitleOb=m_pTable->GetTitle();
        ASSERT(pTitleOb);

        CString sTitle(pTitleOb->GetText());

        // get format attributes
        CFmt fmt;
        if (nullptr!=pTitleOb->GetDerFmt()) {
            fmt=*pTitleOb->GetDerFmt();
            fmt.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_TITLE,0)));
        }
        else {
            fmt=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_TITLE,0));
        }

        if (fmt.IsTextCustom()) {
            sTitle=fmt.GetCustom().m_sCustomText;
        }
        if (!sTitle.IsEmpty()) {
            exporter.WriteTitle(os, fmt, sTitle);
        }
    }

    ///////////////////////////////////////////////////////////
    // Put out sub title, if there is one ...
    CTblOb* pSubTitleOb=m_pTable->GetSubTitle();
    ASSERT(pSubTitleOb);

    // get format attributes
    CFmt fmt;
    if (nullptr!=pSubTitleOb->GetDerFmt()) {
        fmt=*pSubTitleOb->GetDerFmt();
        fmt.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SUBTITLE,0)));
    }
    else {
        fmt=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SUBTITLE,0));
    }

    if (m_pTable->GetDerFmt() != nullptr && m_pTable->GetDerFmt()->HasSubTitle() && bIncludeParents) {
        CString sSubTitle(pSubTitleOb->GetText());
        if (fmt.IsTextCustom()) {
            sSubTitle=fmt.GetCustom().m_sCustomText;
        }
        if (!sSubTitle.IsEmpty()) {
            exporter.WriteSubTitle(os, fmt, sSubTitle);
        }
    }

    //////////////////////////////////////////////////////////////////////
    // put out cells

    // start down 1 row if we have a subtitle...
    int iRowStart=1;
    if (m_pTable->GetDerFmt() && m_pTable->GetDerFmt()->HasSubTitle()) {
        iRowStart++;
    }

    CRowHeaderCalculator rowHeaders(GetNumberRows());
    CColHeaderCalculator colHeaders(GetNumberCols());

    exporter.StartHeaderRows(os);

    int iVisRowsSoFar = 0;

    for (int iRow=iRowStart ; iRow < GetNumberRows() ; iRow++) {

        if (iRow == GetNumHeaderRows()) {
            exporter.EndHeaderRows(os);
        }

        // don't show rows with captions when you have area breaks
        // since already in header
        if (CheckHideCaptionRow(iRow) || GetRowHeight(iRow) == 0)
        {
            continue;
        }

        // check for row that is hidden becuase of all zero cell vals
        if (CheckHideAllZeroRow(iRow)) {
            continue;
        }

        int iVisColsSoFar = 0;

        // see if there are any cells this row to export
        int iNumCellsToExportThisRow = 0;
        //for (int iCol=0 ; iCol<GetNumberCols() ; iCol++) {
        for (int iCol=0 ; iCol<GetNumberCols() && !iNumCellsToExportThisRow; iCol++) { // 20100818 some optimization
            CFmt fmt;
            CTableGridExporter::CJoinRegion join;

            if (ShouldExportCell(iCol, iRow, bBlockedOnly, bIncludeParents, fmt, join)) {
                ++iNumCellsToExportThisRow;
            }
        }

        bool bRowHidden = (iNumCellsToExportThisRow == 0);

        // get header info for this row
        rowHeaders.AddRow(iRow, bRowHidden, GetRowStubLevel(iRow), iVisRowsSoFar);

        // skip this row if no cells to export
        if (bRowHidden) {
            continue;
        }

        const CDWordArray& aRowHeaders = rowHeaders.GetHeaders(iRow);
        exporter.StartRow(os, iRow, aRowHeaders);

        for (int iCol=0 ; iCol<GetNumberCols() ; iCol++) {
            CFmt fmt;
            CTableGridExporter::CJoinRegion join;

            if (ShouldExportCell(iCol, iRow, bBlockedOnly, bIncludeParents, fmt, join)) {

                CTableGridExporter::CJoinRegion adjJoin;
                AdjustJoinForVisibleExport(adjJoin, join, iCol, iRow,
                                           iVisColsSoFar, iVisRowsSoFar,
                                           bBlockedOnly, bIncludeParents);
                CString sVal=QuickGetText(join.iStartCol, join.iStartRow);

                // get col headers for this cell
                const CArray<CTableGridExporter::CJoinRegion>& aColHeaders = colHeaders.GetHeaders(iCol);

                // write out the cell
                exporter.WriteCell(os, iVisColsSoFar, iVisRowsSoFar, fmt, sVal, adjJoin, aColHeaders);

                // update col headers for this column (do this after writing cell so that a col doesn't get itself as a header)
                if (iRow < GetNumHeaderRows() && iCol > 0) {
                    colHeaders.AddColHeader(iRow, iCol, adjJoin);
                }

                ++iVisColsSoFar;
            }
        }

        exporter.EndRow(os);

        ++iVisRowsSoFar;
    }

    exporter.EndTable(os);
    exporter.EndFile(os);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CTblGrid::PutFormats
// Export formats from format reg to exported tbl file
/////////////////////////////////////////////////////////////////////////////
void CTblGrid::PutFormats(CTableGridExporter& exporter, _tostream& os,
                          bool bBlockedOnly, bool bIncludeParents)
{
    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();

    exporter.StartFormats(os);

    ///////////////////////////////////////////////////////////
    // Put out title format, if there is one ...
    CTblOb* pTitleOb=m_pTable->GetTitle();
    ASSERT(pTitleOb);
    // get format attributes
    CFmt fmt;
    if (nullptr!=pTitleOb->GetDerFmt()) {
        fmt=*pTitleOb->GetDerFmt();
        fmt.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_TITLE,0)));
    }
    else {
        fmt=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_TITLE,0));
    }
    exporter.WriteFormat(os, fmt);

    ///////////////////////////////////////////////////////////
    // Put out sub title, if there is one ...
    CTblOb* pSubTitleOb=m_pTable->GetSubTitle();
    ASSERT(pSubTitleOb);

    // get format attributes
    if (nullptr!=pSubTitleOb->GetDerFmt()) {
        fmt=*pSubTitleOb->GetDerFmt();
        fmt.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SUBTITLE,0)));
    }
    else {
        fmt=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SUBTITLE,0));
    }
    exporter.WriteFormat(os, fmt);

    // start down 1 row if we have a subtitle...
    int iRowStart=1;
    if (m_pTable->GetDerFmt() && m_pTable->GetDerFmt()->HasSubTitle()) {
        iRowStart++;
    }

    for (int iRow=iRowStart ; iRow < GetNumberRows() ; iRow++) {

        // don't show rows with captions when you have area breaks
        // since already in header
        if (CheckHideCaptionRow(iRow) || GetRowHeight(iRow) == 0)
        {
            continue;
        }

        // check for row that is hidden becuase of all zero cell vals
        if (CheckHideAllZeroRow(iRow)) {
            continue;
        }

        for (int iCol=0 ; iCol<GetNumberCols() ; iCol++) {
            CFmt fmt;
            CTableGridExporter::CJoinRegion join;

            if (ShouldExportCell(iCol, iRow, bBlockedOnly, bIncludeParents, fmt, join)) {
                exporter.WriteFormat(os, fmt);
            }
        }
    }

    exporter.EndFormats(os);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CTblGrid::AdjustJoinForVisibleExport
// adjust the row and col coords and merge region to only count visible/unblocked cells
/////////////////////////////////////////////////////////////////////////////
void CTblGrid::AdjustJoinForVisibleExport(CTableGridExporter::CJoinRegion& adjustJoin,
                                          const CTableGridExporter::CJoinRegion& origJoin,
                                          int iCol, int iRow,
                                          int iVisColsToJoinStart, int iVisRowsToJoinStart,
                                          bool bBlockedOnly, bool bIncludeParents)
{
    // join starts at num vis/unblocked cells encountered b/f join start
    adjustJoin.iStartCol = adjustJoin.iEndCol = iVisColsToJoinStart;
    adjustJoin.iStartRow = adjustJoin.iEndRow = iVisRowsToJoinStart;

    CFmt f;
    CTableGridExporter::CJoinRegion j;

    //////
    // Adjust start of join by counting vis rows/cols from curr cell
    //////

    for (int i = iCol - 1; i >= origJoin.iStartCol; --i) {
        if (ShouldExportCell(i,iRow, bBlockedOnly, bIncludeParents, f, j)) {
            --adjustJoin.iStartCol;
        }
    }

    for (int i = iRow - 1; i >= origJoin.iStartRow; --i) {
        if (ShouldExportCell(iCol, i, bBlockedOnly, bIncludeParents, f, j)) {
            --adjustJoin.iStartRow;
        }
    }

    //////
    // Adjust end of join by counting vis rows/cols from curr cell
    //////

    for (int i = iCol+1; i <= origJoin.iEndCol; ++i) {
        if (ShouldExportCell(i,iRow, bBlockedOnly, bIncludeParents, f, j)) {
            ++adjustJoin.iEndCol;
        }
    }

    for (int i = iRow+1; i <= origJoin.iEndRow; ++i) {
        if (ShouldExportCell(iCol, i, bBlockedOnly, bIncludeParents, f, j)) {
            ++adjustJoin.iEndRow;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CTblGrid::ShouldExportCell
// Return true if a cell is visible, blocked (if blocking is enabled),
// non-parent (if include parents is disabled).
/////////////////////////////////////////////////////////////////////////////
bool CTblGrid::ShouldExportCell(int iCol, long iRow, bool bBlockedOnly, bool bIncludeParents,
                                CFmt& fmtComponent,
                                CTableGridExporter::CJoinRegion& join)
{
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;

    if (IsDataCell(iCol, iRow)) {
        eGridComp = FMT_ID_DATACELL;
        pGTblOb = GetGTblCell(iCol, iRow);
    }
    else {
        GetComponent(iCol, iRow, eGridComp, &pGTblOb);
    }

    if (nullptr==pGTblOb) {
        return false;
    }

    // only process this cell if it is selected ...
    if (!pGTblOb->GetBlocked() && bBlockedOnly) {
        // cell is not blocked, but the user only wants blocked cells ...
        return false;
    }

    // only include non-data cells if including parents
    if (!bIncludeParents) {
        if (eGridComp!=FMT_ID_DATACELL &&
           !(IsAreaCaptionRow(iRow) && iCol !=0)) {
                return false;
        }
    }

    // hide spanner if it is "System Total" variable
    if (eGridComp==FMT_ID_SPANNER) {
        if (IsSystemTotal(pGTblOb)) {
            return false;
        }
        //Savy (R) sampling app 20081209
        else if(IsSystemStat(pGTblOb)){
            return false;
        }
    }

    // is this cell part of a vertical or horizontal join?
    join.iStartCol = join.iEndCol = iCol;
    join.iStartRow = join.iEndRow = iRow;
    GetJoinRange(&join.iStartCol, &join.iStartRow, &join.iEndCol, &join.iEndRow);

    bool bHorzMerge= (join.iStartCol!=iCol);  //this is cell is merged into prev cell horizontally

    // see if we are dealing with a horizontally merged cell or spanner
    // that sits on top of a hidden or unblocked colhead
    if (bHorzMerge || eGridComp==FMT_ID_SPANNER) {
        if (!ShowSpannerOrHorizMergedCell(iCol, bBlockedOnly)) {
            return false;
        }
    }

    // only export vertically merged cell if there is a visible cell in the corresponding row
    bool bVertMerge= (join.iStartRow!=join.iEndRow);  //this is cell is merged into prev cell vertically
    if (bVertMerge) {
        if (!ShowVertMergedCell(iRow, bBlockedOnly, bIncludeParents)) {
            return false;
        }
    }

    // get format for this cell (use start of merge if this cell is merged)
    FMT_ID eJoinStartGridComp = FMT_ID_INVALID;
    CGTblOb* pGJoinStartTblOb = nullptr;
    if (IsDataCell(join.iStartCol, join.iStartRow)) {
        eJoinStartGridComp = FMT_ID_DATACELL;
        pGJoinStartTblOb = GetGTblCell(join.iStartCol, join.iStartRow);
    }
    else {
        GetComponent(join.iStartCol, join.iStartRow, eJoinStartGridComp, &pGJoinStartTblOb);
    }

   if (IsAreaCaptionRow(join.iStartRow) && join.iStartCol != 0) {
        // this one of the "dead" cells that follows the area caption
        int iColHeadRow=GetNumHeaderRows()-1;
        FMT_ID eGridComp = FMT_ID_INVALID;
        CGTblOb* pGTblOb = nullptr;
        GetComponent(join.iStartCol,iColHeadRow,eGridComp,&pGTblOb);
        ASSERT(eGridComp == FMT_ID_COLHEAD);
        CGTblCol* pGTblCol  = DYNAMIC_DOWNCAST(CGTblCol ,pGTblOb);
        ASSERT(pGTblCol);
        GetFmt4AreaCaptionCells(pGTblCol, fmtComponent);
        CTabView* pView = (CTabView*)GetParent();
        CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
        const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();
        fmtComponent.CopyNonDefaultValues(fmtReg.GetFmt(FMT_ID_AREA_CAPTION));
    }
    else if (eJoinStartGridComp==FMT_ID_DATACELL) {
        CDataCellFmt fmtTmp;
        GetFmt4DataCell(join.iStartCol, join.iStartRow, fmtTmp);
        fmtComponent=fmtTmp;
    }

    else {
        GetFmt4NonDataCell(pGJoinStartTblOb, eJoinStartGridComp, fmtComponent);
    }

    // include hidden cells in design view
    CTabView* pView = (CTabView*)GetParent();
    bool bDesignView = true;
    pView ? bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView() : bDesignView = true;

    // see if this cell is hidden
    if (fmtComponent.GetHidden()==HIDDEN_YES && !bDesignView) {
        // hidden, so skip this cell...
        return false;
    }

    // we shouldn't be exporting hidden rows
    ASSERT(GetRowHeight(iRow) > 0);

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblGrid::IsAreaCaptionRow(long row)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::IsAreaCaptionRow(long row)
{
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    GetComponent(0,row,eGridComp,&pGTblOb);
    return (eGridComp == FMT_ID_AREA_CAPTION);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::GetComponent(int col, long row, FMT_ID& eGridComp, CGTblOb** pGTblOb)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::GetComponent(int col, long row, FMT_ID& eGridComp, CGTblOb** pGTblOb)
{
    //if the cell is title
    *pGTblOb = nullptr;
    eGridComp = FMT_ID_INVALID;
    CUGCell cellGrid;
    bool bHasSubTitle =false;
    bool bHasPageNote = false;
    bool bHasEndNote = false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

    pTblFmt ?  bHasSubTitle = pTblFmt->HasSubTitle() : bHasSubTitle = pDefTblFmt->HasSubTitle();
    pTblFmt ?  bHasPageNote = pTblFmt->HasPageNote() : bHasPageNote = pDefTblFmt->HasPageNote();
    pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();

    int iEndNoteRow =-1;
    int iPageNoteRow =-1;
    if(bHasPageNote && bHasEndNote){
        iPageNoteRow = m_iNumGridRows-2;
        iEndNoteRow = m_iNumGridRows-1;
    }
    else if (bHasPageNote){
        iPageNoteRow = m_iNumGridRows-1;
    }
    else if (bHasEndNote){
        iEndNoteRow = m_iNumGridRows-1;
    }

    if(row==0){
        eGridComp = FMT_ID_TITLE;
        *pGTblOb= &m_GTitle;
    }
    else if(row == 1 && bHasSubTitle){
        //if the cell is subtitle
        eGridComp = FMT_ID_SUBTITLE;
        *pGTblOb= &m_GSubTitle;
    }
    else if(iPageNoteRow !=-1 && iPageNoteRow == row){
         eGridComp = FMT_ID_PAGENOTE;
        *pGTblOb= &m_GPageNote;
    }
    else if(iEndNoteRow !=-1 && iEndNoteRow == row){
         eGridComp = FMT_ID_ENDNOTE;
        *pGTblOb= &m_GEndNote;
    }
    else if (col == 0 && row < GetNumHeaderRows()){
        eGridComp = FMT_ID_STUBHEAD;
        *pGTblOb = &m_GStubHead;
    }
    //if the cell is colhead
    else if (col >0 && row ==GetNumHeaderRows()-1){
        eGridComp = FMT_ID_COLHEAD;
        GetCell(col,row,&cellGrid);
        CGTblOb** pTblOb = (CGTblOb**)cellGrid.GetExtraMemPtr();
        if(pTblOb){
            *pGTblOb = *pTblOb;
        }
    }
    //if the cell is spanner unless special case
    else if (col > 0 && row > 0 && row < GetNumHeaderRows()){
        eGridComp = FMT_ID_SPANNER;
        int iStartCol = col;
        long iStartRow = row;
        GetJoinStartCell(&iStartCol, &iStartRow);
        GetCell(iStartCol,iStartRow,&cellGrid);
        CGTblOb** pOb = (CGTblOb**)cellGrid.GetExtraMemPtr();
        if(pOb){
            *pGTblOb = *pOb;
            CGTblCol* pGTblCol = DYNAMIC_DOWNCAST(CGTblCol,*pGTblOb);
            if(pGTblCol && pGTblCol->GetTabVar()){//Special case
                if(row == GetNumHeaderRows() -2 && IsAStarBPlusCMode(pGTblCol->GetTabVar())){
                    eGridComp = FMT_ID_COLHEAD;
                }
            }
        }


    }
    else if (col ==0 && row > GetNumHeaderRows()-1){
        GetCell(col,row,&cellGrid);
        CGTblOb** pOb = (CGTblOb**)cellGrid.GetExtraMemPtr();
        if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblRow)) ){
            CGTblRow* pGridRow = (CGTblRow*)(*pOb);
            if ( pGridRow->GetNumChildren() > 0 ){//side head
                eGridComp = FMT_ID_CAPTION;
                *pGTblOb = *pOb;
            }
            else if(pGridRow->GetTabVal() == nullptr && pGridRow->GetTabVar() == nullptr && pGridRow->GetTblOb() == m_pTable->GetAreaCaption()){
                eGridComp = FMT_ID_AREA_CAPTION;
                *pGTblOb = *pOb;
            }
            else {//row stub
                eGridComp = FMT_ID_STUB;
                *pGTblOb = *pOb;
            }
        }
    }
    //if  the cell is data cell
    else if (col > 0 && row > GetNumHeaderRows()-1){
        GetCell(0,row,&cellGrid);
        CGTblOb** pOb = (CGTblOb**)cellGrid.GetExtraMemPtr();
        if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblRow)) ){
            CGTblRow* pGridRow = (CGTblRow*)(*pOb);
            if(pGridRow->GetTabVal() == nullptr && pGridRow->GetTabVar() == nullptr){
                //eGridComp = FMT_ID_AREA_CAPTION;
                eGridComp = FMT_ID_INVALID; //this is area caption cell
                *pGTblOb = *pOb;
                return;
            }
        }
        GetCell(col,row,&cellGrid);
        pOb = (CGTblOb**)cellGrid.GetExtraMemPtr();
        eGridComp = FMT_ID_DATACELL;
        if(pOb){
            *pGTblOb = *pOb;
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ProcessFmtDlg(int iCol , long lRow)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ProcessFmtDlg(int iCol , long lRow)
{
    //FMT_ID eGridComp = FMT_ID_INVALID;
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    CTblBase* pTblBase= nullptr;
#ifdef __HOOKEDUP
    GetComponent(iCol,lRow,eGridComp,&pGTblOb);
    CFmt* pFmtChanged = nullptr;
    CFmt* pFmt= nullptr;
    if(pGTblOb && eGridComp != FMT_ID_DATACELL){
        CTabView* pView = (CTabView*)GetParent();
        CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
        const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();
        CGFmtPropDlg dlg;
        dlg.m_eComp = eGridComp;
        CFmt* pFmt = nullptr;
        pTblBase = pGTblOb->GetTblOb();
        switch(eGridComp){
        case FMT_ID_TITLE:
            if(pGTblOb->GetTblOb()->GetFmt()){
                pFmt = DYNAMIC_DOWNCAST(CFmt,pGTblOb->GetTblOb()->GetFmt());
                pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
            }
            else {
                pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_TITLE));
                pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
            }
            break;
        case FMT_ID_SUBTITLE:
            if(pGTblOb->GetTblOb()->GetFmt()){
                pFmt = DYNAMIC_DOWNCAST(CFmt,pGTblOb->GetTblOb()->GetFmt());
                pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
            }
            else {
                pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SUBTITLE));
                pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
            }
            break;
        case FMT_ID_STUBHEAD:
            if(pGTblOb->GetTblOb()->GetFmt()){
                pFmt = DYNAMIC_DOWNCAST(CFmt,pGTblOb->GetTblOb()->GetFmt());
                pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
            }
            else {
                pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_STUBHEAD));
                pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
            }
            break;
        case FMT_ID_SPANNER:
            {
                CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                ASSERT(pCol);
                CTabVar* pTabVar = pCol->GetTabVar();
                pTblBase = pTabVar;
                if(pTabVar && pTabVar->GetFmt()){
                    pFmt = DYNAMIC_DOWNCAST(CFmt,pTabVar->GetFmt());
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SPANNER));
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                }
            }
            break;
        case FMT_ID_COLHEAD:
            {
                CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                ASSERT(pCol);
                CTabValue* pTabVal = pCol->GetTabVal();
                pTblBase = pTabVal;
                if(pTabVal && pTabVal->GetFmt()){
                    pFmt = DYNAMIC_DOWNCAST(CFmt,pTabVal->GetFmt());
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                }

            }
            break;
        case FMT_ID_CAPTION:
            {
                CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                ASSERT(pRow);
                CTabVar* pTabVar = pRow->GetTabVar();
                pTblBase = pTabVar;
                if(pTabVar && pTabVar->GetFmt()){
                    pFmt =pTabVar->GetFmt();
                    pFmtChanged = new CVarFmt(*((CVarFmt*)pFmt));   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION));
                    pFmtChanged = new CVarFmt(*((CVarFmt*)pFmt));   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                }

            }
            break;
        case FMT_ID_STUB:
            {
                CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                ASSERT(pRow);
                CTabValue* pTabVal = pRow->GetTabVal();
                pTblBase = pTabVal;
                if(pTabVal && pTabVal->GetFmt()){
                    pFmt =pTabVal->GetFmt();
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_STUB));
                    pFmtChanged = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
                }
            }
            break;
        case FMT_ID_DATACELL:
            AfxMessageBox(_T("Modifying Cells"));
            break;
        case FMT_ID_PAGENOTE:
            AfxMessageBox(_T("Modifying PageNote"));
            break;
        /*case FMT_ID_EndNote:
            AfxMessageBox("Modifying EndNote");
            break;*/
        default:
            break;
        }
        bool bKeepObj=false;  // true if we are retaining the dynamically allocated CVarFmt

        ASSERT(pTblBase);
        dlg.m_bCustomTxt= pTblBase->IsTextCustom();
        dlg.m_pFmt = pFmtChanged;
        if(dlg.DoModal() ==IDOK){
            if((dlg.m_bCustomTxt != FALSE)!= pTblBase->IsTextCustom()){
                pTblOb->SetCustom(dlg.m_bCustomTxt != FALSE);
                pDoc->SetModifiedFlag();
            }
            if (*pFmt!=*pFmtChanged) {
                // give the changed CFmt its own unique ID...
                CFmtReg* pFmtReg = pDoc->GetTableSpec()->GetFmtRegPtr();

                pFmtChanged->SetID(pFmtReg->GetCustomFmtID(*pFmtChanged));

                // add the changed CFmt to the fmt registry
                pFmtReg->AddFmt(pFmtChanged);

                // assign the new format to the pg ob
                pTblOb->SetFmt(pFmtChanged);   // this will cause it get persisted
                // let doc know that we've changed
                pDoc->SetModifiedFlag();
                bKeepObj=true;
            }
        }
        if (!bKeepObj) {
            // nuke the memory, since we aren't keeping the fmt
            delete pFmtChanged;
        }
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CFmt* CTblGrid::GetFmtFromRowCol(int iCol , long lRow)
//
/////////////////////////////////////////////////////////////////////////////////
CFmt* CTblGrid::GetFmtFromRowCol(int iCol , long lRow)
{
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    CTblBase* pTblOb = nullptr;
    GetComponent(iCol,lRow,eGridComp,&pGTblOb);
    CFmt* pFmt = nullptr;
    if(pGTblOb){
        CTabView* pView = (CTabView*)GetParent();
        CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
        const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();
        pTblOb = pGTblOb->GetTblOb();
        switch(eGridComp){
        case FMT_ID_TITLE:
            if(pTblOb &&  pTblOb->GetFmt()){
                pFmt =DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
            }
            else {
                pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_TITLE));
            }
            break;
        case FMT_ID_SUBTITLE:
            if(pTblOb &&  pTblOb->GetFmt()){
                pFmt =DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
            }
            else {
                pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SUBTITLE));
            }
            break;
        case FMT_ID_STUBHEAD:
            if(pTblOb &&  pTblOb->GetFmt()){
                pFmt =DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
            }
            else {
                pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_STUBHEAD));
            }
            break;
        case FMT_ID_SPANNER:
            {
                CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                ASSERT(pCol);
                CTabVar* pTabVar = pCol->GetTabVar();
                pTblOb = pTabVar;
                if(pTblOb &&  pTblOb->GetFmt()){
                    pFmt =DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SPANNER));
                }
            }
            break;
        case FMT_ID_COLHEAD:
            {
                CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                ASSERT(pCol);
                CTabValue* pTabVal = pCol->GetTabVal();
                pTblOb = pTabVal;
                if(pTblOb &&  pTblOb->GetFmt()){
                    pFmt =DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
                }

            }
            break;
        case FMT_ID_CAPTION:
            {
                CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                ASSERT(pRow);
                CTabVar* pTabVar = pRow->GetTabVar();
                pTblOb = pTabVar;
                if(pTblOb &&  pTblOb->GetFmt()){
                    pFmt =DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION));
                }

            }
            break;
        case FMT_ID_STUB:
            {
                CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                ASSERT(pRow);
                CTabValue* pTabVal = pRow->GetTabVal();
                pTblOb = pTabVal;
                if(pTblOb &&  pTblOb->GetFmt()){
                    pFmt =DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
                }
                else {
                    pFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_STUB));
                }
            }
            break;
        case FMT_ID_DATACELL:
            AfxMessageBox(_T("Modifying Cells"));
            break;
        case FMT_ID_PAGENOTE:
            AfxMessageBox(_T("Modifying PageNote"));
            break;
        /*case FMT_ID_EndNote:
            AfxMessageBox("Modifying EndNote");*/
            break;
        default:
            break;
        }

    }
    return pFmt;
}


int CTblGrid::StyleLineToGridMask (const CFmt* const pFmt)
{
    ASSERT(pFmt);
    int iReturn = 0;

    switch (pFmt->GetLineLeft())  {
    case LINE_NONE:
    case LINE_NOT_APPL:
        break;
    case LINE_THIN:
        iReturn |= UG_BDR_LTHIN;
        break;
    case LINE_THICK:
        iReturn |= UG_BDR_LTHICK;
        break;
    case LINE_DEFAULT:
    default:
        ASSERT(FALSE);
    }
    switch (pFmt->GetLineRight())  {
    case LINE_NONE:
    case LINE_NOT_APPL:
        break;
    case LINE_THIN:
        iReturn |= UG_BDR_RTHIN;
        break;
    case LINE_THICK:
        iReturn |= UG_BDR_RTHICK;
        break;
    case LINE_DEFAULT:
    default:
        ASSERT(FALSE);
    }
    switch (pFmt->GetLineRight())  {
    case LINE_NONE:
    case LINE_NOT_APPL:
        break;
    case LINE_THIN:
        iReturn |= UG_BDR_RTHIN;
        break;
    case LINE_THICK:
        iReturn |= UG_BDR_RTHICK;
        break;
    case LINE_DEFAULT:
    default:
        ASSERT(FALSE);
    }
    switch (pFmt->GetLineTop())  {
    case LINE_NONE:
    case LINE_NOT_APPL:
        break;
    case LINE_THIN:
        iReturn |= UG_BDR_TTHIN;
        break;
    case LINE_THICK:
        iReturn |= UG_BDR_TTHICK;
        break;
    case LINE_DEFAULT:
    default:
        ASSERT(FALSE);
    }
    switch (pFmt->GetLineBottom())  {
    case LINE_NONE:
    case LINE_NOT_APPL:
        break;
    case LINE_THIN:
        iReturn |= UG_BDR_BTHIN;
        break;
    case LINE_THICK:
        iReturn |= UG_BDR_BTHICK;
        break;
    case LINE_DEFAULT:
    default:
        ASSERT(FALSE);
    }

    return iReturn;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  short CTblGrid::GetAlignmentFlags(const CFmt* const pFmt)
//
/////////////////////////////////////////////////////////////////////////////////
short CTblGrid::GetAlignmentFlags(const CFmt* const pFmt)
{
    short iAlign=0 ;
    if(pFmt){
        switch(pFmt->GetHorzAlign()) {
        case HALIGN_LEFT:
            iAlign |= UG_ALIGNLEFT;
            break;
        case HALIGN_CENTER:
            iAlign |= UG_ALIGNCENTER;
            break;
        case HALIGN_RIGHT:
            iAlign |= UG_ALIGNRIGHT;
            break;
        default:
            iAlign |= UG_ALIGNRIGHT;
        }
        switch(pFmt->GetVertAlign()) {
        case VALIGN_TOP:
            iAlign |= UG_ALIGNTOP;
            break;
        case VALIGN_MID:
            iAlign |= UG_ALIGNVCENTER;
            break;
        case VALIGN_BOTTOM:
            iAlign |= UG_ALIGNBOTTOM;
            break;
        default:
            iAlign |= UG_ALIGNVCENTER;
        }
    }
    return iAlign;
}

/*void  CTblGrid::DoRender()
{
    RenderTitle();
    RenderSubTitle();
    RenderHeaders();
    RenderStubs();
    RenderDataCells();
}*/


void CTblGrid::ApplyFormat2Cell(CUGCell& gridCell , const CFmt* const pFmt)
{
    ASSERT(pFmt);
    const CFmtFont& fmtFont = pFmt->GetFmtFont();
    CFont* pFont = fmtFont.GetFont();

    //Do the alignment
    short iAlign;
    iAlign = GetAlignmentFlags(pFmt);
    gridCell.SetAlignment(iAlign);

    //Do the font;
    ASSERT(pFont);
    gridCell.SetFont(pFont);

    //Do the  lines . //First set them appropriately and do Fixlines later
    int iLineMask = StyleLineToGridMask(pFmt);
    gridCell.SetBorder(iLineMask);
    gridCell.SetBorderColor(&(m_blackPen)); //for now border color is black

    gridCell.SetTextColor(pFmt->GetTextColor().m_rgb);
    gridCell.SetBackColor(pFmt->GetFillColor().m_rgb);
    CIMSAString csText = gridCell.GetText();
    if(pFmt->IsTextCustom()){
        csText=pFmt->GetCustom().m_sCustomText;
    }
    int iIndent = pFmt->GetIndent(LEFT);
    if(iIndent>0){
        //For now . Later on convert inches/twips --> Spaces
        iIndent = Twips2NumSpaces(iIndent, pFont);
    }
    csText = csText.AdjustLenLeft(csText.GetLength() + iIndent);
    iIndent = pFmt->GetIndent(RIGHT);
    if(iIndent>0){
        //For now . Later on convert inches/twips --> Spaces
      iIndent = Twips2NumSpaces(iIndent, pFont);
    }
    csText = csText.AdjustLenRight(csText.GetLength() + iIndent);
    gridCell.SetText(csText);

    //Check if hidden
    if(pFmt->GetHidden() == HIDDEN_YES){
       // gridCell.SetText(" - " ); //Hidden mask for now . later on use  "" and
        gridCell.SetBackColor(rgbVLtGray);
        //some fill color
    }
    gridCell.SetCellTypeEx(UGCT_NORMALMULTILINE);
    //To Do other stuff //Like hide etc
}

/////////////////////////////////////////////////////////////////////////////////
//
//  std::unique_ptr<CFmt> CTblGrid::GetFmt4Cell(int iCol, int iRow)
//
/////////////////////////////////////////////////////////////////////////////////
std::unique_ptr<CFmt> CTblGrid::GetFmt4Cell(int iCol, int iRow)
{
    CGTblOb* pGTblOb = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;
    GetComponent(iCol, iRow, eGridComp, &pGTblOb);
    if (eGridComp == FMT_ID_DATACELL) {
        CDataCellFmt* pDataCellFmt = new CDataCellFmt;
        GetFmt4DataCell(iCol, iRow, *pDataCellFmt);
        return std::unique_ptr<CFmt>(pDataCellFmt);
    } else {
        if(eGridComp == FMT_ID_INVALID){
            return std::unique_ptr<CFmt>();
        }
        CFmt* pFmt = new CFmt;
        GetFmt4NonDataCell(pGTblOb, eGridComp, *pFmt);
        return std::unique_ptr<CFmt>(pFmt);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::GetFmt4NonDataCell(CGTblOb* pGTblOb ,FMT_ID eGridComp, CFmt& retFmt)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::GetFmt4NonDataCell(CGTblOb* pGTblOb ,FMT_ID eGridComp, CFmt& retFmt)
{
    ASSERT(pGTblOb);
    CFmt* pFmt = nullptr;
    if(eGridComp==FMT_ID_TITLE || eGridComp==FMT_ID_SUBTITLE || eGridComp==FMT_ID_PAGENOTE || eGridComp== FMT_ID_ENDNOTE || eGridComp== FMT_ID_STUBHEAD ||eGridComp== FMT_ID_STUBHEAD_SEC
        || eGridComp==FMT_ID_AREA_CAPTION) {
        CTblOb* pTblOb = pGTblOb->GetTblOb();
        if(eGridComp==FMT_ID_AREA_CAPTION){
            pTblOb = m_pTable->GetAreaCaption();
        }
        ASSERT(pTblOb);
        pFmt = pTblOb->GetDerFmt();
    }
    else if(eGridComp==FMT_ID_CAPTION || eGridComp==FMT_ID_STUB){
        ASSERT(pGTblOb);
        CGTblRow* pGTblRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
        ASSERT(pGTblRow);
         //second test in the following is for A*B where you have tabval of A as a caption
        if(!pGTblRow->GetTabVal()) {
            ASSERT(pGTblRow->GetTabVar());
            pFmt = pGTblRow->GetTabVar()->GetDerFmt();
        }
        else {
            ASSERT(pGTblRow->GetTabVal());
            pFmt = pGTblRow->GetTabVal()->GetDerFmt();
        }
    }
     else if(eGridComp==FMT_ID_SPANNER || eGridComp==FMT_ID_COLHEAD){
        ASSERT(pGTblOb);
        CGTblCol* pGTblCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
        ASSERT(pGTblCol);
        //second test in the following is for A*B where you have tabval of A as a spanner
        if(pGTblCol->GetNumChildren() > 0 &&  !pGTblCol->GetTabVal()) {
            ASSERT(pGTblCol->GetTabVar());
            pFmt = pGTblCol->GetTabVar()->GetDerFmt();
        }
        else {
            ASSERT(pGTblCol->GetTabVal());
            pFmt = pGTblCol->GetTabVal()->GetDerFmt();
        }
    }
    ASSERT(m_pTable->GetFmtRegPtr());
    CFmt* pDefFmt  = DYNAMIC_DOWNCAST(CFmt,m_pTable->GetFmtRegPtr()->GetFmt(eGridComp));
    ASSERT(pDefFmt);
    if(pFmt){
        retFmt = *pFmt;
    }
    else {
        retFmt=*pDefFmt;
    }
    switch(eGridComp){
        case FMT_ID_TITLE:
        case FMT_ID_SUBTITLE:
        case FMT_ID_PAGENOTE:
        case FMT_ID_ENDNOTE:
        case FMT_ID_STUBHEAD:
        case FMT_ID_STUBHEAD_SEC:
            if(!pFmt){
                return;
            }
            else {
                //CopyNonDefVals(retFmt,*pDefFmt);
                retFmt.CopyNonDefaultValues(pDefFmt);
            }
            break;
        case FMT_ID_AREA_CAPTION:
            if(!pFmt){
                if(ForceHideAreaCaptionInOneRowTable()){
                    retFmt.SetHidden(HIDDEN_YES);
                }
                return;
            }
            else {
                //CopyNonDefVals(retFmt,*pDefFmt);
                retFmt.CopyNonDefaultValues(pDefFmt);
                if(ForceHideAreaCaptionInOneRowTable()){
                    retFmt.SetHidden(HIDDEN_YES);
                }
            }
            break;
        case FMT_ID_CAPTION:
        case FMT_ID_SPANNER:
        case FMT_ID_STUB:
        case FMT_ID_COLHEAD:
            {
                CGRowColOb* pRowColOb = DYNAMIC_DOWNCAST(CGRowColOb,pGTblOb);
                //CopyNonDefVals(retFmt,*pDefFmt);
                bool bTxtColorCustom = retFmt.IsTextColorCustom();
                bool bFillColorCustom = retFmt.IsFillColorCustom();
                retFmt.CopyNonDefaultValues(pDefFmt);
                if(!bTxtColorCustom || pFmt == nullptr){
                    FMT_COLOR fmtColor = retFmt.GetTextColor();
                    fmtColor.m_bUseDefault = true;
                    retFmt.SetTextColor(fmtColor);
                }
                if(!bFillColorCustom || pFmt == nullptr){
                    FMT_COLOR fmtColor = retFmt.GetFillColor();
                    fmtColor.m_bUseDefault = true;
                    retFmt.SetFillColor(fmtColor);
                }
                // Spanner /stubs/colheads using the hierarchy for lines
                GetLineFmt4NonDataCell(pRowColOb,eGridComp,retFmt);
            }
            break;
        default:
            break;
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::CopyNonDefVals(CFmt& toFmt,const CFmt& frmFmt)
//
/////////////////////////////////////////////////////////////////////////////////
/*void CTblGrid::CopyNonDefVals(CFmt& toFmt,const CFmt& frmFmt)
{
    if(toFmt.GetFmtFont().GetFont() == nullptr){
        toFmt.SetFont(frmFmt.GetFmtFont().GetFont());
    }
    if(toFmt.GetHorzAlign() == HALIGN_DEFAULT){
        toFmt.SetHorzAlign(frmFmt.GetHorzAlign());
    }
    if(toFmt.GetVertAlign() == VALIGN_DEFAULT){
        toFmt.SetVertAlign(frmFmt.GetVertAlign());
    }
    if(!toFmt.IsTextColorCustom()){
        toFmt.SetTextColor(frmFmt.GetTextColor());
    }
    if(!toFmt.IsFillColorCustom()){
        toFmt.SetFillColor(frmFmt.GetFillColor());
    }
    if(!toFmt.IsIndentCustom(LEFT)){
        toFmt.SetIndent(LEFT,frmFmt.GetIndent(LEFT));
    }
    if(!toFmt.IsIndentCustom(RIGHT)){
        toFmt.SetIndent(RIGHT,frmFmt.GetIndent(RIGHT));
    }
    if(toFmt.GetLineLeft()== LINE_DEFAULT){
        toFmt.SetLineLeft(frmFmt.GetLineLeft());
    }
    if(toFmt.GetLineRight()== LINE_DEFAULT){
        toFmt.SetLineRight(frmFmt.GetLineRight());
    }
    if(toFmt.GetLineTop()== LINE_DEFAULT){
        toFmt.SetLineTop(frmFmt.GetLineTop());
    }
    if(toFmt.GetLineBottom()== LINE_DEFAULT){
        toFmt.SetLineBottom(frmFmt.GetLineBottom());
    }
    if(toFmt.GetHidden()== HIDDEN_DEFAULT){
        toFmt.SetHidden(frmFmt.GetHidden());
    }
    if(toFmt.IsTextCustom()== HIDDEN_DEFAULT){
        toFmt.SetHidden(frmFmt.GetHidden());
    }

}*/

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ApplyFormat2StubCol()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ApplyFormat2StubCol()
{
    int iRow  = GetNumHeaderRows();
    int iNumRows  = GetNumberRows();

    bool bHasPageNote =false;
    bool bHasEndNote = false;
    int iPageNoteRow = -1;
    int iEndNoteRow = -1;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

    pTblFmt ?  bHasPageNote = pTblFmt->HasPageNote() : bHasPageNote = pDefTblFmt->HasPageNote();
    pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();
    bHasPageNote ? iNumRows-- : iNumRows;
    bHasEndNote ? iNumRows-- : iNumRows;

    CFmt fmt;
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    for(int iIndex = iRow; iIndex < iNumRows;iIndex++){
        CUGCell cellGrid;
        GetCell(0,iIndex,&cellGrid);
        GetComponent(0,iIndex,eGridComp,&pGTblOb);

        GetFmt4NonDataCell(pGTblOb ,eGridComp, fmt);
        ApplyFormat2Cell(cellGrid,&fmt);
        SetCell(0, iIndex ,  &cellGrid);

         //Join Caption
        if(eGridComp == FMT_ID_CAPTION){
            bool bSpanCells = true;
            int iCols = GetNumberCols();
            bSpanCells = fmt.GetSpanCells() == SPAN_CELLS_YES;
            CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
            if(bSpanCells && pRow) {
                JoinCells(0,pRow->GetStartRow(),iCols-1,pRow->GetStartRow());
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ApplyFormat2SpannersNColHeads()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ApplyFormat2SpannersNColHeads()
{
    int iNumHeaderRows  = GetNumHeaderRows();
    int iNumCols  = GetNumberCols();
    CFmt fmt;
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;

    int iStartRow = GetStartRow4SpannerNColHeadProcessing();

    for(int iRow = iStartRow; iRow < iNumHeaderRows;iRow++){
        for(int iCol =1; iCol < iNumCols ; iCol++){
            CUGCell cellGrid;
            int iStartCol = iCol;
            long iStartRow = iRow;
            GetJoinStartCell(&iStartCol, &iStartRow);
            if(iStartCol != iCol || iStartRow != iRow)
                continue;
            GetCell(iStartCol,iStartRow,&cellGrid);
           // GetComponent(iCol,iRow,eGridComp,&pGTblOb);
             GetComponent(iStartCol,iStartRow,eGridComp,&pGTblOb);
            ASSERT(pGTblOb);
            GetFmt4NonDataCell(pGTblOb ,eGridComp, fmt);
            ApplyFormat2Cell(cellGrid,&fmt);
            SetCell(iStartCol, iStartRow ,  &cellGrid);
        }
    }
}



////////////////////////////////////////////////////////////////////////////////
//   ATTRIB     COLHEAD  (EXTENDS)    DATACELL(AVAILABLE)       SPANNER/CAPTION(EXTENDS)
//-----------------------------------------------------------------------------------------
//   ALIGN      NEVER                   YES                      NO
//   LINES      YES/NO                  NO                       YES/NO
//   INDENT     YES/NO                  YES                      NO
//   HIDDEN     ALWAYS                  NO                       NO
//   FONT       YES/NO                  YES                      NO
//   COLOR      YES/NO                  YES                      NO
//   NUMDECIMALS  ALWAYS                YES                      NO
//   CUSTOM-TEXT NEVER                  YES                      NO
//
//
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::GetFmt4DataCell(int iCol, int  iRow, CDataCellFmt& retFmt)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::GetFmt4DataCell(int iCol, int  iRow, CDataCellFmt& retFmt)
{
    CGTblOb* pGTblOb = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;
    GetComponent(iCol,iRow,eGridComp,&pGTblOb);
    ASSERT(eGridComp == FMT_ID_DATACELL);

    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();
    CDataCellFmt* pDataCellFmt = nullptr;
    pDataCellFmt  = DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_DATACELL));
    CGTblCell* pGTblCell = GetGTblCell(iCol,iRow);
    ASSERT(pGTblCell);

//Get GridTblCol
    CGTblCol* pGTblCol = pGTblCell->GetTblCol();
    ASSERT(pGTblCol);

//Get GridTblRow
    CGTblRow* pGTblRow = pGTblCell->GetTblRow();
    ASSERT(pGTblRow);
    ASSERT(pGTblRow->GetTabVar());
    CFmt* pRowFmt  =  nullptr;

//Get Special Cell
    CSpecialCell* pSpecialCell = nullptr;
    if(pGTblRow->GetTabVal()){
       int iPanel = GetRowPanel(iRow);
       if(iPanel > 0 ){
           pSpecialCell = pGTblRow->GetTabVal()->FindSpecialCell(iPanel,iCol);
       }
    }
//Call the new Fmt4DataCell2.
    GetFmt4DataCell2(pGTblRow ,pGTblCol ,retFmt,pSpecialCell);
#ifdef _OLD_
    CFmt* pDefRowFmt = nullptr;
    CFmt* pDefColFmt = nullptr;
    ///Get the Special cell here once we handle the special cells
    CSpecialCell* pSpecialCell = nullptr;
    if(pGTblRow->GetTabVal()){
       int iPanel = GetRowPanel(iRow);
       if(iPanel > 0 ){
           pSpecialCell = pGTblRow->GetTabVal()->FindSpecialCell(iPanel,iCol);
       }
    }
    if(!pSpecialCell){
        retFmt = *pDataCellFmt;
        retFmt.CopyNonDefaultValues(pDataCellFmt);
    }
    else {
        retFmt = *(pSpecialCell->GetDerFmt());
        retFmt.CopyNonDefaultValues(pDataCellFmt);
    }

    CFmt evaldRowFmt;
    if(pGTblRow->GetTabVal()){
        pRowFmt =pGTblRow->GetTabVal()->GetDerFmt();

        FMT_ID eGridComp4StubOrCaption = FMT_ID_INVALID;
        CGTblOb* pGTblOb4StubOrCaption = nullptr;
        GetComponent(0,iRow,eGridComp4StubOrCaption,&pGTblOb4StubOrCaption);

        if(!pRowFmt){
            pDefRowFmt = pRowFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(eGridComp4StubOrCaption));
            GetFmt4NonDataCell(pGTblRow,eGridComp4StubOrCaption,evaldRowFmt);
        }
        else {
            pDefRowFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(eGridComp4StubOrCaption));
            GetFmt4NonDataCell(pGTblRow,eGridComp4StubOrCaption,evaldRowFmt);
        }
    }
    else if(pGTblRow->GetTabVar()){
        pRowFmt =pGTblRow->GetTabVar()->GetDerFmt();
        if(!pRowFmt){
            pDefRowFmt =pRowFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION));
            GetFmt4NonDataCell(pGTblRow,FMT_ID_CAPTION,evaldRowFmt);
        }
        else {
            /*evaldRowFmt = *pRowFmt;
            evaldRowFmt.CopyNonDefaultValues(fmtReg.GetFmt(FMT_ID_CAPTION));*/
            pDefRowFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_STUB));
            GetFmt4NonDataCell(pGTblRow,FMT_ID_CAPTION,evaldRowFmt);
        }
    }
    else {
        ASSERT(FALSE);
    }




    //Get ColFmt
    CGTblCol* pGTblCol = pGTblCell->GetTblCol();
    ASSERT(pGTblCol);
    ASSERT(pGTblCol->GetTabVar());

    CFmt* pColFmt  =  nullptr;
    CFmt evaldColFmt;
    if(pGTblCol->GetTabVal()){
        pColFmt =pGTblCol->GetTabVal()->GetDerFmt();
        if(!pColFmt){
            pDefColFmt = pColFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
            GetFmt4NonDataCell(pGTblCol,FMT_ID_COLHEAD,evaldColFmt);
        }
        else {
            /*evaldColFmt = *pColFmt;
            evaldColFmt.CopyNonDefaultValues(fmtReg.GetFmt(FMT_ID_COLHEAD));*/
            pDefColFmt= DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
            GetFmt4NonDataCell(pGTblCol,FMT_ID_COLHEAD,evaldColFmt);
        }
    }
    else if(pGTblCol->GetTabVar()){
        pColFmt =pGTblCol->GetTabVar()->GetDerFmt();
        if(!pColFmt){
            pDefColFmt = pColFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SPANNER));
            GetFmt4NonDataCell(pGTblCol,FMT_ID_SPANNER,evaldColFmt);
        }
        else {
            /*evaldColFmt = *pColFmt;
            evaldColFmt.CopyNonDefaultValues(fmtReg.GetFmt(FMT_ID_SPANNER));*/
            pDefColFmt= DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SPANNER));
            GetFmt4NonDataCell(pGTblCol,FMT_ID_SPANNER,evaldColFmt);
        }
    }


    //For Alignment And CustomText -- this attribute is ready and nothing else need to be done
    //--cos this one never extends and is only for the cell .Now it would
    //have come from either special cell / ID_FMT_DATACELL copy from above


    //TextColor
    bool bRowCust = evaldRowFmt.IsTextColorCustom();
    bool bColCust = evaldColFmt.IsTextColorCustom();

    bool bRowExtends = evaldRowFmt.GetTextColorExtends();
    bool bColExtends = evaldColFmt.GetTextColorExtends();

    if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
        if(bRowCust && bColCust && evaldRowFmt.GetTextColor().m_rgb != evaldColFmt.GetTextColor().m_rgb){
            ASSERT(pSpecialCell);
        }
        if(pSpecialCell && pSpecialCell->GetDerFmt()->IsTextColorCustom() ){
            //Do nothing.It is already copied into the retFmt
        }
        else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
            retFmt.SetTextColor(evaldRowFmt.GetTextColor());
        }
        else if(!bRowExtends && bColExtends){//use col val if col extends and not row
            retFmt.SetTextColor(evaldColFmt.GetTextColor());
        }
        else if(bRowCust ){//use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetTextColor(evaldRowFmt.GetTextColor());
        }
        else if(bColCust ){//use Columns val  if  col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            retFmt.SetTextColor(evaldColFmt.GetTextColor());
        }
        else {//none of them are  custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetTextColor(evaldRowFmt.GetTextColor());
        }
    }
    //FillColor
    bRowCust = evaldRowFmt.IsFillColorCustom();
    bColCust = evaldColFmt.IsFillColorCustom();

    bRowExtends = evaldRowFmt.GetFillColorExtends();
    bColExtends = evaldColFmt.GetFillColorExtends();

    if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
        if(bRowCust && bColCust && evaldRowFmt.GetFillColor().m_rgb != evaldColFmt.GetFillColor().m_rgb){
            ASSERT(pSpecialCell);
        }
        if(pSpecialCell && pSpecialCell->GetDerFmt()->IsFillColorCustom() ){
            //Do nothing.It is already copied into the retFmt
        }
        else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
            retFmt.SetFillColor(evaldRowFmt.GetFillColor());
        }
        else if(!bRowExtends && bColExtends){//use col val if col extends and not row
            retFmt.SetFillColor(evaldColFmt.GetFillColor());
        }
        else if(bRowCust ){//use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetFillColor(evaldRowFmt.GetFillColor());
        }
        else if(bColCust ){//use Columns val  if  col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            retFmt.SetFillColor(evaldColFmt.GetFillColor());
        }
        else {//none of them are  custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetFillColor(evaldRowFmt.GetFillColor());
        }
    }

    //Font
    bRowCust = evaldRowFmt.GetFont() != nullptr && pDefRowFmt->GetFont() !=evaldRowFmt.GetFont();
    bColCust = evaldColFmt.GetFont() != nullptr && pDefColFmt->GetFont() !=evaldColFmt.GetFont() ;

    bRowExtends = evaldRowFmt.GetFontExtends();
    bColExtends = evaldColFmt.GetFontExtends();

    if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
        LOGFONT lfRow ,lfCol;
        evaldRowFmt.GetFont()->GetLogFont(&lfRow);
        evaldColFmt.GetFont()->GetLogFont(&lfCol);
        int iRet = memcmp(&lfRow,&lfCol,sizeof(LOGFONT));
        if(bRowCust && bColCust &&  iRet!=0){
            ASSERT(pSpecialCell);
        }
        if(pSpecialCell && pSpecialCell->GetDerFmt()->IsFontCustom() ){
            //Do nothing.It is already copied into the retFmt
        }
        else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
            retFmt.SetFont(evaldRowFmt.GetFont());
        }
        else if(!bRowExtends && bColExtends){//use col val if col extends and not row
            retFmt.SetFont(evaldColFmt.GetFont());
        }
        else if(bRowCust ){//use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetFont(evaldRowFmt.GetFont());
        }
        else if(bColCust ){//use Columns val  if  col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            retFmt.SetFont(evaldColFmt.GetFont());
        }
        else {//none of them are  custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetFont(evaldRowFmt.GetFont());
        }
    }
     //Decimals
    //second test in the following is for A*B where you have tabval of A as a caption
    if(pGTblRow->GetTabVal() && pGTblRow->GetNumChildren() ==0){//only rows with stubs have decimals
        CDataCellFmt* pDRowFmt =  DYNAMIC_DOWNCAST(CDataCellFmt,pRowFmt);
        ASSERT(pDRowFmt);
        CDataCellFmt* pDColFmt =  DYNAMIC_DOWNCAST(CDataCellFmt,pColFmt);
        ASSERT(pDColFmt);

        CDataCellFmt evaldRowDataFmt = *pDRowFmt;
        CDataCellFmt evaldColDataFmt = *pDColFmt;

        evaldRowDataFmt.CopyNonDefaultValues(fmtReg.GetFmt(FMT_ID_STUB));
        evaldColDataFmt.CopyNonDefaultValues(fmtReg.GetFmt(FMT_ID_COLHEAD));

        bRowCust = pDRowFmt->GetNumDecimals() != NUM_DECIMALS_DEFAULT && pDRowFmt->GetIndex() !=0;
        bColCust = pDColFmt->GetNumDecimals() != NUM_DECIMALS_DEFAULT && pDColFmt->GetIndex() !=0;

        //Decimals always extend
        bRowExtends = true;
        bColExtends = true;

        if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
            if(bRowCust && bColCust &&  evaldRowDataFmt.GetNumDecimals() != evaldColDataFmt.GetNumDecimals()){
                ASSERT(pSpecialCell);
            }
            if(pSpecialCell && pSpecialCell->GetDerFmt()->GetNumDecimals() != NUM_DECIMALS_DEFAULT ){
                //Do nothing.It is already copied into the retFmt
            }
            else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
                retFmt.SetNumDecimals(evaldRowDataFmt.GetNumDecimals());
            }
            else if(!bRowExtends && bColExtends){//use col val if col extends and not row
                retFmt.SetNumDecimals(evaldColDataFmt.GetNumDecimals());
            }
            else if(bRowCust ){//use rows val if  row is custom and if both extend
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetNumDecimals(evaldRowDataFmt.GetNumDecimals());
            }
            else if(bColCust ){//use Columns val  if  col is custom and if both extend
                ASSERT(bColExtends);
                ASSERT(bRowExtends);
                retFmt.SetNumDecimals(evaldColDataFmt.GetNumDecimals());
            }
            else {//none of them are  custom .both are default. Let's use row's default
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetNumDecimals(evaldRowDataFmt.GetNumDecimals());
            }
        }
    }

    //Hidden
    if(pRowFmt && pColFmt ){//hidden for sutb/captions/ spanners/colheads
        //Hidden always extend
        bRowExtends = true;
        bColExtends = true;
        bool bRowCust = pRowFmt->GetHidden() != HIDDEN_DEFAULT && pRowFmt->GetIndex() !=0;
        bool bColCust = pColFmt->GetHidden() != HIDDEN_DEFAULT && pColFmt->GetIndex() !=0;

        if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
            if(bRowCust && bColCust &&  evaldRowFmt.GetHidden() != evaldColFmt.GetHidden()){
                ASSERT(pSpecialCell);
            }
            if(pSpecialCell){ //hidden is not appl for data cells . the attributes is infered from stub/colhead
                if(evaldColFmt.GetHidden() == HIDDEN_YES || evaldRowFmt.GetHidden() == HIDDEN_YES){
                    retFmt.SetHidden(HIDDEN_YES);
                }
            }
            else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
                retFmt.SetHidden(evaldRowFmt.GetHidden());
            }
            else if(!bRowExtends && bColExtends){//use col val if col extends and not row
                retFmt.SetHidden(evaldColFmt.GetHidden());
            }
            else if(bRowCust ){//use rows val if  row is custom and if both extend
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetHidden(evaldRowFmt.GetHidden());
            }
            else if(bColCust ){//use Columns val  if  col is custom and if both extend
                ASSERT(bColExtends);
                ASSERT(bRowExtends);
                retFmt.SetHidden(evaldColFmt.GetHidden());
            }
            else {//none of them are  custom .both are default. Let's use row's default
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetHidden(evaldRowFmt.GetHidden());
            }
        }
    }
    //Left Indent
    //second test in the following is for A*B where you have tabval of A as a caption
    if(pGTblRow->GetTabVal() && pGTblRow->GetNumChildren()==0){//only rows with stubs have indent extends
        bRowCust = pRowFmt->GetIndent(LEFT) !=pDefRowFmt->GetIndent(LEFT) ;
        bColCust = pColFmt->GetIndent(LEFT) !=pDefColFmt->GetIndent(LEFT) ;

        bRowExtends = evaldRowFmt.GetIndentationExtends();
        bColExtends = evaldColFmt.GetIndentationExtends();

        if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
            if(bRowCust && bColCust && evaldRowFmt.GetIndent(LEFT) != evaldColFmt.GetIndent(LEFT)){
                ASSERT(pSpecialCell);
            }
            if(pSpecialCell && pSpecialCell->GetDerFmt()->IsIndentCustom(LEFT)){
                //Do nothing.It is already copied into the retFmt
            }
            else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
                retFmt.SetIndent(LEFT,evaldRowFmt.GetIndent(LEFT));
            }
            else if(!bRowExtends && bColExtends){//use col val if col extends and not row
                retFmt.SetIndent(LEFT,evaldColFmt.GetIndent(LEFT));
            }
            else if(bRowCust ){//use rows val if  row is custom and if both extend
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetIndent(LEFT,evaldRowFmt.GetIndent(LEFT));
            }
            else if(bColCust ){//use Columns val  if  col is custom and if both extend
                ASSERT(bColExtends);
                ASSERT(bRowExtends);
                retFmt.SetIndent(LEFT,evaldColFmt.GetIndent(LEFT));
            }
            else {//none of them are  custom .both are default. Let's use row's default
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetIndent(LEFT,evaldRowFmt.GetIndent(LEFT));
            }
        }
    }
    //Right Indent
   //second test in the following is for A*B where you have tabval of A as a caption
    if(pGTblRow->GetTabVal() && pGTblRow->GetNumChildren() ==0){//only rows with stubs have indent extends
        bRowCust = pRowFmt->GetIndent(RIGHT) !=pDefRowFmt->GetIndent(RIGHT) ;
        bColCust = pColFmt->GetIndent(RIGHT) !=pDefColFmt->GetIndent(RIGHT) ;

        bRowExtends = evaldRowFmt.GetIndentationExtends();
        bColExtends = evaldColFmt.GetIndentationExtends();

        if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
            if(bRowCust && bColCust && evaldRowFmt.GetIndent(RIGHT) != evaldColFmt.GetIndent(RIGHT)){
                ASSERT(pSpecialCell);
            }
            if(pSpecialCell && pSpecialCell->GetDerFmt()->IsIndentCustom(RIGHT)){
                //Do nothing.It is already copied into the retFmt
            }
            else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
                retFmt.SetIndent(RIGHT,evaldRowFmt.GetIndent(RIGHT));
            }
            else if(!bRowExtends && bColExtends){//use col val if col extends and not row
                retFmt.SetIndent(RIGHT,evaldColFmt.GetIndent(RIGHT));
            }
            else if(bRowCust ){//use rows val if  row is custom and if both extend
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetIndent(RIGHT,evaldRowFmt.GetIndent(RIGHT));
            }
            else if(bColCust ){//use Columns val  if  col is custom and if both extend
                ASSERT(bColExtends);
                ASSERT(bRowExtends);
                retFmt.SetIndent(RIGHT,evaldColFmt.GetIndent(RIGHT));
            }
            else {//none of them are  custom .both are default. Let's use row's default
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetIndent(RIGHT,evaldRowFmt.GetIndent(RIGHT));
            }
        }
    }


    //Do Lines
    bRowCust = false; //Lines are independent of col (top/bottom)
    bColCust = false; //lines are independent of row (left/right)
    bRowExtends = evaldRowFmt.GetLinesExtend();
    bColExtends = evaldColFmt.GetLinesExtend();

    if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
        //Lines cant be applied on an individual cell  .So special cells line attribute
        //need not be considered
        if(bRowExtends ){
            bool bTop = true;
            bool bBottom = true;
            if(!pRowFmt->GetLinesExtend()){
                bTop = false;
                bBottom = false;
                CGRowColOb* pParent = DYNAMIC_DOWNCAST(CGRowColOb,pGTblRow->GetParent());
                ASSERT(pParent);
                int iNumChildren = pParent->GetNumChildren();
                if(pGTblRow == pParent->GetChild(0)){
                    if(pGTblRow->GetTabVal() && m_pTable->GetRowRoot()->GetNumChildren()>0){
                        //this case covers row items more than one A+B and first stub shld not
                        //have the top line  because The spanner covers it //Other wise you
                        //will get top lines for both spanner and stub
                        bTop =false;
                    }
                    else {
                        bTop = true;
                    }
                }
                if(pGTblRow == pParent->GetChild(iNumChildren-1)){
                    bBottom = true;
                }
            }
            if(bTop){
                retFmt.SetLineTop(evaldRowFmt.GetLineTop());
            }
            if(bBottom){
                retFmt.SetLineBottom(evaldRowFmt.GetLineBottom());
            }
        }
        if(bColExtends){//use col val if col extends and not row
            bool bLeft = true;
            bool bRight = true;
            if(!pColFmt->GetLinesExtend()){
                bLeft = false;
                bRight = false;
                CGRowColOb* pParent = DYNAMIC_DOWNCAST(CGRowColOb,pGTblCol->GetParent());
                ASSERT(pParent);
                int iNumChildren = pParent->GetNumChildren();
                if(pGTblCol == pParent->GetChild(0)){
                    bLeft = true;
                }
                if(pGTblCol == pParent->GetChild(iNumChildren-1)){
                    bRight = true;
                }
            }
            if(bLeft){
               retFmt.SetLineLeft(evaldColFmt.GetLineLeft());
            }
            if(bRight){
                retFmt.SetLineRight(evaldColFmt.GetLineRight());
            }

        }
    }
#endif //End of OLD STUFF ..//DELETE THIS ONCE TESTED

}

//New Optimized stuff
void CTblGrid::GetFmt4DataCell2(CGTblRow* pGTblRow , CGTblCol* pGTblCol ,CDataCellFmt& retFmt,CSpecialCell* pSpecialCell /*=nullptr*/)
{

    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();
    CDataCellFmt* pDataCellFmt = nullptr;
    pDataCellFmt  = DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_DATACELL));
    bool bDeleteEvalDRowFmt = false;
    bool bDeleteEvalDColFmt = false;
    //Get RowFmt
    ASSERT(pGTblRow);
    ASSERT(pGTblRow->GetTabVar());
    CFmt* pRowFmt  =  nullptr;

    CFmt* pDefRowFmt = nullptr;
    CFmt* pDefColFmt = nullptr;
    ///Get the Special cell here once we handle the special cells
    if(!pSpecialCell){
        retFmt = *pDataCellFmt;
        retFmt.CopyNonDefaultValues(pDataCellFmt);
    }
    else {
        retFmt = *(pSpecialCell->GetDerFmt());
        retFmt.CopyNonDefaultValues(pDataCellFmt);
    }

    CDataCellFmt* pEvaldRowFmt = nullptr;
    CTabValue* pRowTabVal = pGTblRow->GetTabVal();
    CTabVar*   pRowTabVar = pGTblRow->GetTabVar();
    if(pRowTabVal){
        CArray<CDataCellFmt,CDataCellFmt&>& arrRowDataCellFmts =  pRowTabVal->m_arrRunTimeDataCellFmts;
        arrRowDataCellFmts.GetSize() > 0 ? pEvaldRowFmt = &arrRowDataCellFmts[0] : pEvaldRowFmt = nullptr;
        if(!pEvaldRowFmt){
            pRowFmt =pRowTabVal->GetDerFmt();
            pEvaldRowFmt = new CDataCellFmt();

            bDeleteEvalDRowFmt =true;
            FMT_ID eGridComp4StubOrCaption = FMT_ID_STUB;
            if(pGTblRow->GetNumChildren() >0){//if a tabval has children then it is a caption
                eGridComp4StubOrCaption = FMT_ID_CAPTION;
            }

            if(!pRowFmt){
                pDefRowFmt = pRowFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(eGridComp4StubOrCaption));
                GetFmt4NonDataCell(pGTblRow,eGridComp4StubOrCaption,*pEvaldRowFmt);
            }
            else {
                pDefRowFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(eGridComp4StubOrCaption));
                GetFmt4NonDataCell(pGTblRow,eGridComp4StubOrCaption,*pEvaldRowFmt);
            }
            arrRowDataCellFmts.Add(*pEvaldRowFmt);
            //delete pEvaldRowFmt;
        }
        else {
            pRowFmt =pRowTabVal->GetDerFmt();

            FMT_ID eGridComp4StubOrCaption = FMT_ID_STUB;
            if(pGTblRow->GetNumChildren() >0){//if a tabval has children then it is a caption
                eGridComp4StubOrCaption = FMT_ID_CAPTION;
            }
            if(!pRowFmt){
                pDefRowFmt = pRowFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(eGridComp4StubOrCaption));
            }
            else {
                pDefRowFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(eGridComp4StubOrCaption));
            }
        }
    }
    else if(pRowTabVar){
        CArray<CDataCellFmt,CDataCellFmt&>& arrRowDataCellFmts =  pRowTabVar->m_arrRunTimeDataCellFmts;
        arrRowDataCellFmts.GetSize() > 0 ? pEvaldRowFmt = &arrRowDataCellFmts[0] : pEvaldRowFmt = nullptr;

        if(!pEvaldRowFmt){
            pRowFmt =pRowTabVar->GetDerFmt();
            pEvaldRowFmt = new CDataCellFmt();
            bDeleteEvalDRowFmt = true;

            if(!pRowFmt){
                pDefRowFmt =pRowFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION));
                GetFmt4NonDataCell(pGTblRow,FMT_ID_CAPTION,*pEvaldRowFmt);
            }
            else {
                pDefRowFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION));
                GetFmt4NonDataCell(pGTblRow,FMT_ID_CAPTION,*pEvaldRowFmt);
            }
            arrRowDataCellFmts.Add(*pEvaldRowFmt);
            //delete pEvaldRowFmt;
        }
        else {
            pRowFmt =pRowTabVar->GetDerFmt();
            if(!pRowFmt){
                pDefRowFmt =pRowFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION));
            }
            else {
                pDefRowFmt = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION));
            }
        }
    }
    else {
        ASSERT(FALSE);
    }




    //Get ColFmt
    ASSERT(pGTblCol->GetTabVar());
    CDataCellFmt* pColFmt  =  nullptr;
    CDataCellFmt* pEvaldColFmt = nullptr;

    CTabValue* pColTabVal = pGTblCol->GetTabVal();
    CTabVar* pColTabVar = pGTblCol->GetTabVar();

    if(pColTabVal ){
        CArray<CDataCellFmt,CDataCellFmt&>& arrColDataCellFmts =  pColTabVal->m_arrRunTimeDataCellFmts;
        arrColDataCellFmts.GetSize() > 0 ? pEvaldColFmt = &arrColDataCellFmts[0] : pEvaldColFmt = nullptr;

        if(!pEvaldColFmt){
            pColFmt =pColTabVal->GetDerFmt();
            pEvaldColFmt = new CDataCellFmt();
            bDeleteEvalDColFmt = true;
            if(!pColFmt){
                pDefColFmt = pColFmt  = DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
                GetFmt4NonDataCell(pGTblCol,FMT_ID_COLHEAD,*pEvaldColFmt);
            }
            else {
                pDefColFmt= DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
                GetFmt4NonDataCell(pGTblCol,FMT_ID_COLHEAD,*pEvaldColFmt);
            }
            arrColDataCellFmts.Add(*pEvaldColFmt);
           // delete pEvaldColFmt;
        }
        else {
            pColFmt =pColTabVal->GetDerFmt();
            if(!pColFmt){
                pDefColFmt = pColFmt  = DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
            }
            else {
                pDefColFmt= DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
            }
        }
    }
    else{
       ASSERT(FALSE);
    }


    //For Alignment And CustomText -- this attribute is ready and nothing else need to be done
    //--cos this one never extends and is only for the cell .Now it would
    //have come from either special cell / ID_FMT_DATACELL copy from above


    //TextColor
    bool bRowCust = pEvaldRowFmt->IsTextColorCustom();
    bool bColCust = pEvaldColFmt->IsTextColorCustom();

    bool bRowExtends = pEvaldRowFmt->GetTextColorExtends();
    bool bColExtends = pEvaldColFmt->GetTextColorExtends();

    if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
        if(bRowCust && bColCust && pEvaldRowFmt->GetTextColor().m_rgb != pEvaldColFmt->GetTextColor().m_rgb){
            ASSERT(pSpecialCell);
        }
        if(pSpecialCell && pSpecialCell->GetDerFmt()->IsTextColorCustom() ){
            //Do nothing.It is already copied into the retFmt
        }
        else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
            retFmt.SetTextColor(pEvaldRowFmt->GetTextColor());
        }
        else if(!bRowExtends && bColExtends){//use col val if col extends and not row
            retFmt.SetTextColor(pEvaldColFmt->GetTextColor());
        }
        else if(bRowCust ){//use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetTextColor(pEvaldRowFmt->GetTextColor());
        }
        else if(bColCust ){//use Columns val  if  col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            retFmt.SetTextColor(pEvaldColFmt->GetTextColor());
        }
        else {//none of them are  custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetTextColor(pEvaldRowFmt->GetTextColor());
        }
    }
    //FillColor
    bRowCust = pEvaldRowFmt->IsFillColorCustom();
    bColCust = pEvaldColFmt->IsFillColorCustom();

    bRowExtends = pEvaldRowFmt->GetFillColorExtends();
    bColExtends = pEvaldColFmt->GetFillColorExtends();

    if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
        if(bRowCust && bColCust && pEvaldRowFmt->GetFillColor().m_rgb != pEvaldColFmt->GetFillColor().m_rgb){
            ASSERT(pSpecialCell);
        }
        if(pSpecialCell && pSpecialCell->GetDerFmt()->IsFillColorCustom() ){
            //Do nothing.It is already copied into the retFmt
        }
        else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
            retFmt.SetFillColor(pEvaldRowFmt->GetFillColor());
        }
        else if(!bRowExtends && bColExtends){//use col val if col extends and not row
            retFmt.SetFillColor(pEvaldColFmt->GetFillColor());
        }
        else if(bRowCust ){//use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetFillColor(pEvaldRowFmt->GetFillColor());
        }
        else if(bColCust ){//use Columns val  if  col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            retFmt.SetFillColor(pEvaldColFmt->GetFillColor());
        }
        else {//none of them are  custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetFillColor(pEvaldRowFmt->GetFillColor());
        }
    }

    //Font
    bRowCust = pEvaldRowFmt->GetFont() != nullptr && pDefRowFmt->GetFont() !=pEvaldRowFmt->GetFont();
    bColCust = pEvaldColFmt->GetFont() != nullptr && pDefColFmt->GetFont() !=pEvaldColFmt->GetFont() ;

    bRowExtends = pEvaldRowFmt->GetFontExtends();
    bColExtends = pEvaldColFmt->GetFontExtends();

    if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
        LOGFONT lfRow ,lfCol;
        pEvaldRowFmt->GetFont()->GetLogFont(&lfRow);
        pEvaldColFmt->GetFont()->GetLogFont(&lfCol);
        int iRet = memcmp(&lfRow,&lfCol,sizeof(LOGFONT));
        if(bRowCust && bColCust &&  iRet!=0){
            ASSERT(pSpecialCell);
        }
        if(pSpecialCell && pSpecialCell->GetDerFmt()->IsFontCustom() ){
            //Do nothing.It is already copied into the retFmt
        }
        else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
            retFmt.SetFont(pEvaldRowFmt->GetFont());
        }
        else if(!bRowExtends && bColExtends){//use col val if col extends and not row
            retFmt.SetFont(pEvaldColFmt->GetFont());
        }
        else if(bRowCust ){//use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetFont(pEvaldRowFmt->GetFont());
        }
        else if(bColCust ){//use Columns val  if  col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            retFmt.SetFont(pEvaldColFmt->GetFont());
        }
        else {//none of them are  custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            retFmt.SetFont(pEvaldRowFmt->GetFont());
        }
    }
     //Decimals
    //second test in the following is for A*B where you have tabval of A as a caption
    if(pGTblRow->GetTabVal() && pGTblRow->GetNumChildren() ==0){//only rows with stubs have decimals
        CDataCellFmt* pDRowFmt =  DYNAMIC_DOWNCAST(CDataCellFmt,pRowFmt);
        ASSERT(pDRowFmt);
        CDataCellFmt* pDColFmt =  DYNAMIC_DOWNCAST(CDataCellFmt,pColFmt);
        ASSERT(pDColFmt);

        CDataCellFmt evaldRowDataFmt = *pDRowFmt;
        CDataCellFmt evaldColDataFmt = *pDColFmt;

        evaldRowDataFmt.CopyNonDefaultValues(fmtReg.GetFmt(FMT_ID_STUB));
        evaldColDataFmt.CopyNonDefaultValues(fmtReg.GetFmt(FMT_ID_COLHEAD));

        bRowCust = pDRowFmt->GetNumDecimals() != NUM_DECIMALS_DEFAULT && pDRowFmt->GetIndex() !=0;
        bColCust = pDColFmt->GetNumDecimals() != NUM_DECIMALS_DEFAULT && pDColFmt->GetIndex() !=0;

        //Decimals always extend
        bRowExtends = true;
        bColExtends = true;

        if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
            if(bRowCust && bColCust &&  evaldRowDataFmt.GetNumDecimals() != evaldColDataFmt.GetNumDecimals()){
                //ASSERT(pSpecialCell);
            }
            if(pSpecialCell && pSpecialCell->GetDerFmt()->GetNumDecimals() != NUM_DECIMALS_DEFAULT ){
                //Do nothing.It is already copied into the retFmt
            }
            else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
                retFmt.SetNumDecimals(evaldRowDataFmt.GetNumDecimals());
            }
            else if(!bRowExtends && bColExtends){//use col val if col extends and not row
                retFmt.SetNumDecimals(evaldColDataFmt.GetNumDecimals());
            }
            else if(bRowCust ){//use rows val if  row is custom and if both extend
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetNumDecimals(evaldRowDataFmt.GetNumDecimals());
            }
            else if(bColCust ){//use Columns val  if  col is custom and if both extend
                ASSERT(bColExtends);
                ASSERT(bRowExtends);
                retFmt.SetNumDecimals(evaldColDataFmt.GetNumDecimals());
            }
            else {//none of them are  custom .both are default. Let's use row's default
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetNumDecimals(evaldRowDataFmt.GetNumDecimals());
            }
        }
    }

    //Hidden
    if(pRowFmt && pColFmt ){//hidden for sutb/captions/ spanners/colheads
        //Hidden always extend
        bRowExtends = true;
        bColExtends = true;
        bool bRowCust = pRowFmt->GetHidden() != HIDDEN_DEFAULT && pRowFmt->GetIndex() !=0;
        bool bColCust = pColFmt->GetHidden() != HIDDEN_DEFAULT && pColFmt->GetIndex() !=0;

        if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
            if(bRowCust && bColCust &&  pEvaldRowFmt->GetHidden() != pEvaldColFmt->GetHidden()){
              //  ASSERT(pSpecialCell); //captions and spanners do not have special cells
            }
            if(pSpecialCell){ //hidden is not appl for data cells . the attributes is infered from stub/colhead
                if(pEvaldColFmt->GetHidden() == HIDDEN_YES || pEvaldRowFmt->GetHidden() == HIDDEN_YES){
                    retFmt.SetHidden(HIDDEN_YES);
                }
            }
            else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
                retFmt.SetHidden(pEvaldRowFmt->GetHidden());
            }
            else if(!bRowExtends && bColExtends){//use col val if col extends and not row
                retFmt.SetHidden(pEvaldColFmt->GetHidden());
            }
            else if(bRowCust ){//use rows val if  row is custom and if both extend
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetHidden(pEvaldRowFmt->GetHidden());
            }
            else if(bColCust ){//use Columns val  if  col is custom and if both extend
                ASSERT(bColExtends);
                ASSERT(bRowExtends);
                retFmt.SetHidden(pEvaldColFmt->GetHidden());
            }
            else {//none of them are  custom .both are default. Let's use row's default
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetHidden(pEvaldRowFmt->GetHidden());
            }
        }
    }
    //Left Indent
    //second test in the following is for A*B where you have tabval of A as a caption
    if(pGTblRow->GetTabVal() && pGTblRow->GetNumChildren()==0){//only rows with stubs have indent extends
        bRowCust = pRowFmt->GetIndent(LEFT) !=pDefRowFmt->GetIndent(LEFT) ;
        bColCust = pColFmt->GetIndent(LEFT) !=pDefColFmt->GetIndent(LEFT) ;

        bRowExtends = pEvaldRowFmt->GetIndentationExtends();
        bColExtends = pEvaldColFmt->GetIndentationExtends();

        if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
            if(bRowCust && bColCust && pEvaldRowFmt->GetIndent(LEFT) != pEvaldColFmt->GetIndent(LEFT)){
                ASSERT(pSpecialCell);
            }
            if(pSpecialCell && pSpecialCell->GetDerFmt()->IsIndentCustom(LEFT)){
                //Do nothing.It is already copied into the retFmt
            }
            else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
                retFmt.SetIndent(LEFT,pEvaldRowFmt->GetIndent(LEFT));
            }
            else if(!bRowExtends && bColExtends){//use col val if col extends and not row
                retFmt.SetIndent(LEFT,pEvaldColFmt->GetIndent(LEFT));
            }
            else if(bRowCust ){//use rows val if  row is custom and if both extend
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetIndent(LEFT,pEvaldRowFmt->GetIndent(LEFT));
            }
            else if(bColCust ){//use Columns val  if  col is custom and if both extend
                ASSERT(bColExtends);
                ASSERT(bRowExtends);
                retFmt.SetIndent(LEFT,pEvaldColFmt->GetIndent(LEFT));
            }
            else {//none of them are  custom .both are default. Let's use row's default
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetIndent(LEFT,pEvaldRowFmt->GetIndent(LEFT));
            }
        }
    }
    //Right Indent
   //second test in the following is for A*B where you have tabval of A as a caption
    if(pGTblRow->GetTabVal() && pGTblRow->GetNumChildren() ==0){//only rows with stubs have indent extends
        bRowCust = pRowFmt->GetIndent(RIGHT) !=pDefRowFmt->GetIndent(RIGHT) ;
        bColCust = pColFmt->GetIndent(RIGHT) !=pDefColFmt->GetIndent(RIGHT) ;

        bRowExtends = pEvaldRowFmt->GetIndentationExtends();
        bColExtends = pEvaldColFmt->GetIndentationExtends();

        if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
            if(bRowCust && bColCust && pEvaldRowFmt->GetIndent(RIGHT) != pEvaldColFmt->GetIndent(RIGHT)){
                ASSERT(pSpecialCell);
            }
            if(pSpecialCell && pSpecialCell->GetDerFmt()->IsIndentCustom(RIGHT)){
                //Do nothing.It is already copied into the retFmt
            }
            else if(bRowExtends && !bColExtends){//use rows val  if row extends and not col
                retFmt.SetIndent(RIGHT,pEvaldRowFmt->GetIndent(RIGHT));
            }
            else if(!bRowExtends && bColExtends){//use col val if col extends and not row
                retFmt.SetIndent(RIGHT,pEvaldColFmt->GetIndent(RIGHT));
            }
            else if(bRowCust ){//use rows val if  row is custom and if both extend
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetIndent(RIGHT,pEvaldRowFmt->GetIndent(RIGHT));
            }
            else if(bColCust ){//use Columns val  if  col is custom and if both extend
                ASSERT(bColExtends);
                ASSERT(bRowExtends);
                retFmt.SetIndent(RIGHT,pEvaldColFmt->GetIndent(RIGHT));
            }
            else {//none of them are  custom .both are default. Let's use row's default
                ASSERT(bRowExtends);
                ASSERT(bColExtends);
                retFmt.SetIndent(RIGHT,pEvaldRowFmt->GetIndent(RIGHT));
            }
        }
    }


    //Do Lines
    bRowCust = false; //Lines are independent of col (top/bottom)
    bColCust = false; //lines are independent of row (left/right)
    bRowExtends = pEvaldRowFmt->GetLinesExtend();
    bColExtends = pEvaldColFmt->GetLinesExtend();

    if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
        //Lines cant be applied on an individual cell  .So special cells line attribute
        //need not be considered
        if(bRowExtends ){
            bool bTop = true;
            bool bBottom = true;
            if(!pRowFmt->GetLinesExtend()){
                bTop = false;
                bBottom = false;
                CGRowColOb* pParent = DYNAMIC_DOWNCAST(CGRowColOb,pGTblRow->GetParent());
                ASSERT(pParent);
                int iNumChildren = pParent->GetNumChildren();
                if(pGTblRow == pParent->GetChild(0)){
                    if(pGTblRow->GetTabVal() && m_pTable->GetRowRoot()->GetNumChildren()>0){
                        //this case covers row items more than one A+B and first stub shld not
                        //have the top line  because The spanner covers it //Other wise you
                        //will get top lines for both spanner and stub
                        bTop =false;
                    }
                    else {
                        bTop = true;
                    }
                }
                if(pGTblRow == pParent->GetChild(iNumChildren-1)){
                    bBottom = true;
                }
            }
            if(bTop){
                retFmt.SetLineTop(pEvaldRowFmt->GetLineTop());
            }
            if(bBottom){
                retFmt.SetLineBottom(pEvaldRowFmt->GetLineBottom());
            }
        }
        if(bColExtends){//use col val if col extends and not row
            bool bLeft = true;
            bool bRight = true;
            if(!pColFmt->GetLinesExtend()){
                bLeft = false;
                bRight = false;
                CGRowColOb* pParent = DYNAMIC_DOWNCAST(CGRowColOb,pGTblCol->GetParent());
                ASSERT(pParent);
                int iNumChildren = pParent->GetNumChildren();
                if(pGTblCol == pParent->GetChild(0)){
                    bLeft = true;
                }
                if(pGTblCol == pParent->GetChild(iNumChildren-1)){
                    bRight = true;
                }
            }
            if(bLeft){
               retFmt.SetLineLeft(pEvaldColFmt->GetLineLeft());
            }
            if(bRight){
                retFmt.SetLineRight(pEvaldColFmt->GetLineRight());
            }

        }
    }
    bDeleteEvalDRowFmt? delete pEvaldRowFmt : pEvaldRowFmt = pEvaldRowFmt;
    bDeleteEvalDColFmt? delete pEvaldColFmt : pEvaldColFmt = pEvaldColFmt;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::GetLineFmt4NonDataCell(CGRowColOb* pGTblOb ,FMT_ID eGridComp, CFmt& retFmt)
// Do not use the original fmt of the CTblOb use a copy of the fmt as the extends attribute
// is changed in the retfmt by looking at the heirarchy
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::GetLineFmt4NonDataCell(CGRowColOb* pGTblOb ,FMT_ID eGridComp, CFmt& retFmt)
{
    //ASSERT if it is not spanner / caption / stub / colhead
    bool bComponent = (eGridComp ==  FMT_ID_CAPTION || eGridComp == FMT_ID_SPANNER || eGridComp == FMT_ID_STUB
        || eGridComp ==  FMT_ID_COLHEAD );
    bool bLeft =false; //using for left/right (or top/bottom lines)
    bool bRight  = false;

    ASSERT(bComponent);
    ASSERT(pGTblOb);
    CGTblCol* pGTblCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
    CGTblRow* pGTblRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
    bool bASSERT =  pGTblCol || pGTblRow;
    if(pGTblCol == nullptr && pGTblRow == nullptr){
        ASSERT(FALSE); //It has to be a Row / col grid object and not a cell
    }
    CFmt* pFmtSource = nullptr;
    CFmt fmtSource;
    CFmt fmtStubHeadCopy;
    if(eGridComp==FMT_ID_CAPTION || eGridComp==FMT_ID_STUB){
        ASSERT(pGTblRow);
        //second test in the following is for A*B where you have tabval of A as a caption
        if(!pGTblRow->GetTabVal()) {
            ASSERT(pGTblRow->GetTabVar());
            pFmtSource = pGTblRow->GetTabVar()->GetDerFmt();
        }
        else {
            ASSERT(pGTblRow->GetTabVal());
            pFmtSource = pGTblRow->GetTabVal()->GetDerFmt();
        }
        if (nullptr!=m_pTable->GetStubhead(0)->GetDerFmt()) {
            const CFmt* pFmtStubHead = nullptr;
            pFmtStubHead = m_pTable->GetStubhead(0)->GetDerFmt();
            fmtStubHeadCopy.Assign(*pFmtStubHead);
            CFmt* pefFmtStubHead=DYNAMIC_DOWNCAST(CFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_STUBHEAD,0));
            fmtStubHeadCopy.CopyNonDefaultValues(pefFmtStubHead);
        }
        else {
            const CFmt* pFmtStubHead = nullptr;
            pFmtStubHead=DYNAMIC_DOWNCAST(CFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_STUBHEAD,0));
            fmtStubHeadCopy.Assign(*pFmtStubHead);

        }
        if(fmtStubHeadCopy.GetLinesExtend()){
           // retFmt.SetLinesExtend(true);
            bLeft = true;
            bRight =true;
        }
    }
    else if(eGridComp==FMT_ID_SPANNER || eGridComp==FMT_ID_COLHEAD){
        //second test in the following is for A*B where you have tabval of A as a Spanner
        if(pGTblCol->GetNumChildren() > 0 && !pGTblCol->GetTabVal() ) {
            ASSERT(pGTblCol->GetTabVar());
            pFmtSource = pGTblCol->GetTabVar()->GetDerFmt();
        }
        else {
            ASSERT(pGTblCol->GetTabVal());
            pFmtSource = pGTblCol->GetTabVal()->GetDerFmt();
        }
    }
    ASSERT(m_pTable->GetFmtRegPtr());
    CFmt* pDefFmt  = DYNAMIC_DOWNCAST(CFmt,m_pTable->GetFmtRegPtr()->GetFmt(eGridComp));
    ASSERT(pDefFmt);
    if(pFmtSource){
        fmtSource = *pFmtSource;
    }
    else {
        fmtSource=*pDefFmt;
    }
    //CopyNonDefVals(fmtSource,*pDefFmt);
    fmtSource.CopyNonDefaultValues(pDefFmt);
    //Copy just the line related stuff to the retFmt;
    retFmt.SetLineTop(fmtSource.GetLineTop()) ;
    retFmt.SetLineBottom(fmtSource.GetLineBottom()) ;
    retFmt.SetLineLeft(fmtSource.GetLineLeft()) ;
    retFmt.SetLineRight(fmtSource.GetLineRight()) ;

    CGRowColOb* pRoot = pGTblRow ? (CGRowColOb*)GetRowRoot()  : (CGRowColOb*)GetColRoot();
    //If the object has lines and it extends . //Set the lines to thin/ thick  using  precendence
    if(fmtSource.GetLinesExtend()){
        retFmt.SetLinesExtend(true);
        bLeft = true;
        bRight = true;
    }
    else {
        //see if "this object" is first / last in the "group"
        if(pGTblOb->GetParent() && pGTblOb->GetParent() != pRoot){
            CGRowColOb* pParent = DYNAMIC_DOWNCAST(CGRowColOb,pGTblOb->GetParent());
            ASSERT(pParent);
            int iNumChildren = pParent->GetNumChildren();
            ASSERT(iNumChildren !=0);
            if(pParent->GetChild(0) == pGTblOb){
                bLeft = true;
            }
            else if(pParent->GetChild(iNumChildren-1) == pGTblOb){
                bRight = true;
            }
        }
    }
    if(!bLeft && !bRight){//nothing to do
        return;
    }

    //Get the precedence rule thin/thick  line types if parent extends
    CGRowColOb* pParentOb = DYNAMIC_DOWNCAST(CGRowColOb,pGTblOb->GetParent());
    bool bLoop  = (pParentOb && pParentOb->GetParent()&& pParentOb !=pRoot);
    while(bLoop){
        FMT_ID eParentFmtID  = FMT_ID_INVALID;
        CFmt retParentFmt;
        if(pGTblRow) {
            eParentFmtID =  pParentOb->HasChildren()? FMT_ID_CAPTION : FMT_ID_STUB;
        }
        else if(pGTblCol){
            eParentFmtID =  pParentOb->HasChildren()? FMT_ID_SPANNER : FMT_ID_COLHEAD;
        }
        GetLineFmt4NonDataCell(pParentOb ,eParentFmtID,retParentFmt);
        if(retParentFmt.GetLinesExtend()){
            retFmt.SetLinesExtend(true); //by the virtue of the parent
            switch(eGridComp){
                case FMT_ID_STUB:
                case FMT_ID_CAPTION:
                    //top and bottom lines for caption need not be processed .They come from themselves
                    //left and right are derived from the stubhead
                    if (bLeft && fmtStubHeadCopy.GetLineLeft()==LINE_THICK) {
                        retFmt.SetLineLeft(LINE_THICK);
                    }
                    if (bLeft &&  fmtStubHeadCopy.GetLineLeft()==LINE_THIN && fmtSource.GetLineLeft()!=LINE_THICK) {
                        retFmt.SetLineLeft(LINE_THIN);
                    }
                    if (bRight&&  fmtStubHeadCopy.GetLineRight()==LINE_THICK ) {
                        retFmt.SetLineRight(LINE_THICK);
                    }
                    if (bRight &&  fmtStubHeadCopy.GetLineRight()==LINE_THIN && fmtSource.GetLineRight()!=LINE_THICK) {
                        retFmt.SetLineRight(LINE_THIN);
                    }
                    break;
                case FMT_ID_COLHEAD:
                case FMT_ID_SPANNER:
                    if (bLeft && retParentFmt.GetLineLeft()==LINE_THICK ) {
                        retFmt.SetLineLeft(LINE_THICK);
                    }
                    if (bLeft && retParentFmt.GetLineLeft()==LINE_THIN && fmtSource.GetLineLeft()!=LINE_THICK) {
                        retFmt.SetLineLeft(LINE_THIN);
                    }
                    if (bRight && retParentFmt.GetLineRight()==LINE_THICK) {
                        retFmt.SetLineRight(LINE_THICK);
                    }
                    if (bRight && retParentFmt.GetLineRight()==LINE_THIN && fmtSource.GetLineRight()!=LINE_THICK) {
                        retFmt.SetLineRight(LINE_THIN);
                    }
                    break;
                default:
                    ASSERT(FALSE);
            }
        }
        pParentOb = DYNAMIC_DOWNCAST(CGRowColOb,pParentOb->GetParent());
        bLoop  = (pParentOb && pParentOb->GetParent()&& pParentOb !=pRoot);
    }

    //along the chain and return
}

/////////////////////////////////////////////////////////////////////////////////
//
//  int  CTblGrid::GetRowPanel(int iRow)
//
/////////////////////////////////////////////////////////////////////////////////
//Call this only for data cells . Panel number is 1 based
int  CTblGrid::GetRowPanel(int iRow)
{
    int iRet = 0;
    CGTblOb* pGTblOb = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;

    int iStartRow = m_iGridHeaderRows;
    int iTotalRows = GetNumberRows();
    int iNumSlices = m_pTable->GetTabDataArray().GetSize();

    iNumSlices == 0? iNumSlices = 1 : iNumSlices = iNumSlices;
    int iNumRowsPerSlice = ((iTotalRows -iStartRow +1) / iNumSlices) ;
    int iCurrentRow = iRow;
    HasArea() && m_iCurrSelArea !=0 ? iNumRowsPerSlice =iTotalRows:iNumSlices = iNumSlices;
    if(HasArea() && iNumRowsPerSlice !=0){
        m_iCurrSelArea !=0 ? iNumSlices =1:iNumSlices = iNumSlices;
        iNumRowsPerSlice = ((iTotalRows -iStartRow +1) / iNumSlices) ;
        while (iCurrentRow > iStartRow + iNumRowsPerSlice){
            iCurrentRow = iCurrentRow -iNumRowsPerSlice;
        }
    }
    GetComponent(0,iCurrentRow,eGridComp,&pGTblOb);
    if(eGridComp == FMT_ID_INVALID || eGridComp == FMT_ID_PAGENOTE
        || eGridComp == FMT_ID_ENDNOTE){
        return iRet;
    }

    CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
    ASSERT(pRow);
    CTabValue* pCompareVal = pRow->GetTabVal();
    if(!pCompareVal){
        return iRet;
    }
    int iMaxPanelSearchRow = iTotalRows;
    HasArea() ? iMaxPanelSearchRow = iStartRow+ iNumRowsPerSlice :iMaxPanelSearchRow =iMaxPanelSearchRow ;
    for(int iRowIndex = iStartRow; iRowIndex < iMaxPanelSearchRow; iRowIndex++){
        CGTblOb* pGTblOb = nullptr;
        GetComponent(0,iRowIndex,eGridComp,&pGTblOb);
        CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
        ASSERT(pRow);
        CTabValue* pTabVal = pRow->GetTabVal();
        if(pRow->GetTabVal() == pCompareVal){
            iRet++;
        }
        if(iRowIndex == iCurrentRow){
            break;
        }
    }
   return iRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  int CTblGrid::GetNextPanel(int iCurPanelRow, int& iNextPanelRow)
//
/////////////////////////////////////////////////////////////////////////////////
int CTblGrid::GetNextPanel(int iCurPanelRow, int& iNextPanelRow)
{
    int iRet = 0;
    iNextPanelRow = 0;
    CGTblOb* pGTblOb = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;
    GetComponent(0,iCurPanelRow,eGridComp,&pGTblOb);

    CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
    ASSERT(pRow);
    CTabValue* pCompareVal = pRow->GetTabVal();
    if(!pCompareVal){
        return iRet;
    }
    int iStartRow = m_iGridHeaderRows;
    int iTotalRows = GetNumberRows();
    for(int iRowIndex = iStartRow; iRowIndex < iTotalRows; iRowIndex++){
        CGTblOb* pGTblOb = nullptr;
        GetComponent(0,iRowIndex,eGridComp,&pGTblOb);
        CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
        if(!pRow){
            continue;
        }
        CTabValue* pTabVal = pRow->GetTabVal();
        if(pRow->GetTabVal() == pCompareVal){
            iRet++;
            if(iRowIndex > iCurPanelRow){
                iNextPanelRow = iRowIndex;
                break;
            }
        }
    }
    if(iNextPanelRow ==0){
        iRet = 0;
    }
    return iRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  int CTblGrid::GetNextColOffSet(int iColOffSet)
//
/////////////////////////////////////////////////////////////////////////////////
int CTblGrid::GetNextColOffSet(int iCurColOffSet)
{
    int iRet = 0;
    CGTblOb* pGTblOb = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;

    int iRow = GetNumHeaderRows() -1;
    ASSERT(iRow > 0);
    GetComponent(iCurColOffSet,iRow,eGridComp,&pGTblOb);


   CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
    ASSERT(pCol);
    CTabValue* pCompareVal = pCol->GetTabVal();
    if(!pCompareVal){
        return iRet;
    }
    int iNumCols = GetNumberCols();

    for(int iColIndex = iCurColOffSet+1; iColIndex < iNumCols; iColIndex++){
        CGTblOb* pGTblOb = nullptr;
        GetComponent(iColIndex,iRow,eGridComp,&pGTblOb);

        CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
        ASSERT(pCol);
        CTabValue* pTabVal = pCol->GetTabVal();
        if(pCol->GetTabVal() == pCompareVal){
            iRet = iColIndex;
            break;
        }
    }
    return iRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  int CTblGrid::GetColNumForFirstPanel(int iCurColOffSet)
//
/////////////////////////////////////////////////////////////////////////////////
int  CTblGrid::GetColNumForFirstPanel(int iCurColOffSet)
{
    int iRet = 0;
    CGTblOb* pGTblOb = nullptr;
    FMT_ID eGridComp = FMT_ID_INVALID;

    int iRow = GetNumHeaderRows() -1;
    ASSERT(iRow > 0);
    GetComponent(iCurColOffSet,iRow,eGridComp,&pGTblOb);


   CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
    ASSERT(pCol);
    CTabValue* pCompareVal = pCol->GetTabVal();
    if(!pCompareVal){
        return iRet;
    }
    int iNumCols = GetNumberCols();

    for(int iColIndex = 1; iColIndex < iNumCols; iColIndex++){
        CGTblOb* pGTblOb = nullptr;
        GetComponent(iColIndex,iRow,eGridComp,&pGTblOb);

        CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
        ASSERT(pCol);
        CTabValue* pTabVal = pCol->GetTabVal();
        if(pCol->GetTabVal() == pCompareVal){
            iRet = iColIndex;
            break;
        }
    }
    return iRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  CIMSAString CTblGrid::GetComponentString(int col, long row)
//
/////////////////////////////////////////////////////////////////////////////////
CIMSAString CTblGrid::GetComponentString(int col, long row)
{
    CIMSAString sRet;
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    GetComponent(col,row,eGridComp,&pGTblOb);
     switch(eGridComp){
        case FMT_ID_TITLE:
            sRet = _T("Title");
            break;
        case FMT_ID_SUBTITLE:
            sRet = _T("Sub Title");
            break;
        case FMT_ID_STUBHEAD:
            sRet = _T("Stub Head");
            break;
        case FMT_ID_SPANNER:
           sRet = _T("Spanner");
           break;
        case FMT_ID_COLHEAD:
            sRet = _T("Column Head");
            break;
        case FMT_ID_CAPTION:
             sRet = _T("Caption");
            break;
        case FMT_ID_STUB:
            sRet = _T("Stub");
            break;
        case FMT_ID_DATACELL:
           sRet = _T("Data Cell");
            break;
        case FMT_ID_PAGENOTE:
            sRet = _T("Page Note");
            break;
        case FMT_ID_ENDNOTE:
             sRet = _T("End Note");
            break;
        case FMT_ID_AREA_CAPTION:
            sRet = _T("Area Caption");
            break;
        default:
            break;
    }
    return sRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void  CTblGrid::DrawTblBorders()
//
/////////////////////////////////////////////////////////////////////////////////
void  CTblGrid::DrawTblBorders()
{
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));
    int iNumRows = GetNumberRows();
    int iNumCols = GetNumberCols();

    CTblFmt evaldTblFmt;
    pTblFmt ?evaldTblFmt = *pTblFmt : evaldTblFmt = *pDefTblFmt;
    evaldTblFmt.CopyNonDefaultValues(pDefTblFmt);
    //Draw Top line for the first row
    if(evaldTblFmt.GetBorderTop() != LINE_NONE){
        UINT iMask =0;
        LINE eLine = evaldTblFmt.GetBorderTop();
        switch(eLine){
        case LINE_THIN:
            iMask = UG_BDR_TTHIN;
            break;
        case LINE_THICK:
            iMask = UG_BDR_TTHICK;
            break;
        default:
            break;
        }
        if(iMask){
            for(int iCol =0;iCol<iNumCols;iCol++){
                CUGCell gridCell;
                GetCell(iCol,0,&gridCell);
                UINT iBorder = gridCell.GetBorder();
                gridCell.SetBorder(iBorder|iMask);
                SetCell(iCol,0,&gridCell);
            }
        }
    }

    //Draw right line for the last column
    if(evaldTblFmt.GetBorderRight() != LINE_NONE){
        UINT iMask =0;
        LINE eLine = evaldTblFmt.GetBorderRight();
        switch(eLine){
        case LINE_THIN:
            iMask = UG_BDR_RTHIN;
            break;
        case LINE_THICK:
            iMask = UG_BDR_RTHICK;
            break;
        default:
            break;
        }
        if(iMask){
            for(int iRow =0;iRow<iNumRows;iRow++){
                CUGCell gridCell;
                int iStartCol = iNumCols-1;
                long iStartRow = iRow;
                GetJoinStartCell(&iStartCol, &iStartRow);
                GetCell(iStartCol,iStartRow,&gridCell);
                UINT iBorder = gridCell.GetBorder();
                gridCell.SetBorder(iBorder|iMask);
                SetCell(iStartCol,iStartRow,&gridCell);
            }
        }
    }

    //Draw Bottom line for the last row
   if(evaldTblFmt.GetBorderBottom() != LINE_NONE){
        UINT iMask =0;
        LINE eLine = evaldTblFmt.GetBorderBottom();
        switch(eLine){
        case LINE_THIN:
            iMask = UG_BDR_BTHIN;
            break;
        case LINE_THICK:
            iMask = UG_BDR_BTHICK;
            break;
        default:
            break;
        }
        if(iMask){
            for(int iCol =0;iCol<iNumCols;iCol++){
                CUGCell gridCell;
                GetCell(iCol,iNumRows-1,&gridCell);
                UINT iBorder = gridCell.GetBorder();
                gridCell.SetBorder(iBorder|iMask);
                SetCell(iCol,iNumRows-1,&gridCell);
            }
        }
    }

    //Draw left line for the  first column
      //Draw Bottom line for the last row
   if(evaldTblFmt.GetBorderLeft() != LINE_NONE){
        UINT iMask =0;
        LINE eLine = evaldTblFmt.GetBorderLeft();
        switch(eLine){
        case LINE_THIN:
            iMask = UG_BDR_LTHIN;
            break;
        case LINE_THICK:
            iMask = UG_BDR_LTHICK;
            break;
        default:
            break;
        }
        if(iMask){
            for(int iRow =0;iRow<iNumRows;iRow++){
                CUGCell gridCell;
                GetCell(0,iRow,&gridCell);
                UINT iBorder = gridCell.GetBorder();
                gridCell.SetBorder(iBorder|iMask);
                SetCell(0,iRow,&gridCell);
            }
        }
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//  int CTblGrid::Twips2NumSpaces(int iTwips)
//
/////////////////////////////////////////////////////////////////////////////////
int CTblGrid::Twips2NumSpaces(int iTwips, CFont* pFont)
{
    CDC* pDC = GetDC();
    CFont* pOldFont = pDC->SelectObject(pFont);
    int iPixelsXPerInch = pDC->GetDeviceCaps(LOGPIXELSX);
    int iNumPixels = (iPixelsXPerInch * iTwips)/TWIPS_PER_INCH;
    CSize size = pDC->GetOutputTextExtent(_T(" "));
    int iNumSpaces = iNumPixels /size.cx ;
    pDC->SelectObject(pOldFont);
    ReleaseDC(pDC);
    return iNumSpaces;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool  CTblGrid::IsAStarBPlusCMode(CTabVar* pTabVar)
//
//Use only for the columns processing to generate A*B+C kind of header properly in the grid
//If "C" is passed in the pTabVar it results in "true" for the A*B+C config
//If A/B is passed in the pTabVar it results in "false" for the A*B+C" config
/////////////////////////////////////////////////////////////////////////////////
bool  CTblGrid::IsAStarBPlusCMode(CTabVar* pTabVar)
{
    bool bRet = false;
    ASSERT(m_pTable);
    CTabVar* pColRoot = m_pTable->GetColRoot();

    if(pTabVar->GetParent() != pColRoot){// This makes sure for "B" in A*B it returns false
        return bRet;
    }
    //Check if the current var has no children
    else if(pTabVar->GetNumChildren() != 0) {//This makes sure for "A" in A*B
        return bRet;
    }
    else {//now check if we have A*B + C kind of configuration in the spec
        //The pTabVar  is now of similar to "C" has no children and has a parent which is the col root
        //now look for other siblings of this C which have children ==> look if we have any "A"'s in the spec .
        for(int iIndex =0; iIndex < pColRoot->GetNumChildren(); iIndex++){
            if(pTabVar == pColRoot->GetChild(iIndex)){
                continue;
            }
            else {
                if(pColRoot->GetChild(iIndex)->GetNumChildren() > 0){
                    bRet = true;
                    break;
                }
            }
        }
    }
    return bRet;
}

void  CTblGrid::HandleAStarBPlusCModeDisplay()
{
  //Go through the top level spanners (box heads ? )
    int iSpannerStartRow =1;
    bool bHasSubTitle =false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));
    pTblFmt ?  bHasSubTitle = pTblFmt->HasSubTitle() : bHasSubTitle = pDefTblFmt->HasSubTitle();
    if(bHasSubTitle){
        iSpannerStartRow++;
    }
    //int iNumCols = GetNumberCols();
    //for(int iCol = 1; iCol < iNumCols; iCol++){
        int iNumSpanners = m_pTblColRoot->GetNumChildren();
        if(iNumSpanners ==0) {
            return ;
        }
        else {
        int iChild = 0;
        CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,m_pTblColRoot->GetChild(iChild));
        while(pCol){
            //Get the spanner TabVar;
            CTabVar* pTabVar = pCol->GetTabVar();
            ASSERT(pTabVar);
            if(IsAStarBPlusCMode(pTabVar)){
                //join this row and the next one
                UnJoinCells(pCol->GetStartCol(),iSpannerStartRow);
                UnJoinCells(pCol->GetStartCol(),iSpannerStartRow+1);
                JoinCells(pCol->GetStartCol(),iSpannerStartRow,pCol->GetStartCol()+pTabVar->GetNumValues()-1,iSpannerStartRow+1);

                UnJoinCells(pCol->GetStartCol(),iSpannerStartRow+2);
                for(int iCol =pCol->GetStartCol(); iCol < pCol->GetStartCol()+pTabVar->GetNumValues(); iCol++){
                    CUGCell gridCell;
                    GetCell(iCol,iSpannerStartRow+3,&gridCell);
                    CTblOb** pOTblOb = (CTblOb**)gridCell.GetExtraMemPtr();
                    CGTblCol* pGTblCol = nullptr;
                     if(pOTblOb && (*pOTblOb) && (*pOTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
                        pGTblCol= (CGTblCol*)*pOTblOb;
                    }

                    JoinCells(iCol,iSpannerStartRow+2,iCol,iSpannerStartRow+3);
                    GetCell(iCol,iSpannerStartRow+2,&gridCell);
                    void** pTblOb = (void**)gridCell.AllocExtraMem(sizeof(LPVOID));
                    *pTblOb = pGTblCol;
                    gridCell.SetText(pGTblCol->GetText());
                    SetCell(iCol,iSpannerStartRow+2,&gridCell);
                }
            }
            iChild++;
            iChild<iNumSpanners ? pCol = DYNAMIC_DOWNCAST(CGTblCol,m_pTblColRoot->GetChild(iChild)): pCol = nullptr;
        }

    }

 // check if the var associated to the spanner is in IsAStarBPlusCMode
 //if so we have to process this
 // join this row and the next one to get "The spanner"
 //Unjoin the the row after this one
 // and join the vals in the colheads
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::CheckHideCaptionRow(int iRow)
//
// Return true if the row containing a table caption should be hidden.
// When you have 1 var in the row . And you have areas . and you do not want the
// the var label to appear for the area breaks . 'cos this is already in the stub head
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::CheckHideCaptionRow(int iRow)
{
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    GetComponent(0, iRow, eGridComp, &pGTblOb);
    if (eGridComp == FMT_ID_CAPTION) {
        CGTblRow* pTblRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
        if(pTblRow && !pTblRow->GetTabVal()){
            CFmt retFmt;
            GetFmt4NonDataCell(pGTblOb,eGridComp, retFmt);
            bool bHide = (IsOneRowVarTable()) ;
            if(retFmt.GetHidden() == HIDDEN_YES || bHide){
                return true;
            }
        }
    }
    else if (eGridComp == FMT_ID_AREA_CAPTION) {
        CFmt retFmt;
        CTabView* pView = (CTabView*)GetParent();
        bool bDesignView = true;
        pView ? bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView() : bDesignView = true;


        GetFmt4NonDataCell(pGTblOb,eGridComp, retFmt);
        bool bForceHide = ForceHideAreaCaptionInOneRowTable();
        if((retFmt.GetHidden() == HIDDEN_YES ||  bForceHide) && !bDesignView){
            return true;
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::CheckHideAllZeroRow(int iRow)
//
// Return true if a row contains all zero valued cells and
// if the fmt for the corresponding stub has hide if all zero flag
// set
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::CheckHideAllZeroRow(int iRow)
{
    bool bDataAvailable = (m_pTable->GetTabDataArray().GetSize()>0);
	if (!bDataAvailable) {
		return false;
	}

	// don't hide them in design view
	CTabView* pView = (CTabView*)GetParent();
    bool bDesignView = true;
    pView ? bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView() : bDesignView = true;
	if (bDesignView) {
		return false;
	}

	bool bHide = false;
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    GetComponent(0, iRow, eGridComp, &pGTblOb);
    CGTblRow* pTblRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
    if(pTblRow && pTblRow->GetTabVal()){
		CDataCellFmt* pFmt = pTblRow->GetTabVal()->GetDerFmt();
		CDataCellFmt* pDefStubFmt=DYNAMIC_DOWNCAST(CDataCellFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_STUB));
		bool bProcess = false;
		if(pFmt){
			bProcess = pFmt->GetZeroHidden();
		}
		else {
			bProcess = pDefStubFmt->GetZeroHidden();
		}
		if(bProcess){//check if the cells are all zero
			bHide = true; // assume we hide unless we find a non-zero cell
			for(int iCol =1; iCol < GetNumberCols() ; iCol++){
				GetComponent(iCol,iRow,eGridComp,&pGTblOb);
				if(!IsDataCell(iCol,iRow) || eGridComp == FMT_ID_INVALID){
					continue;//We are not concerned with non-data cells
				}

				// ignore hidden cols
				const int iColHeadRow=GetNumHeaderRows()-1;
				std::unique_ptr<CFmt> pFmt = GetFmt4Cell(iCol, iColHeadRow);
				ASSERT(pFmt.get());
				ASSERT(pFmt->GetID() == FMT_ID_COLHEAD);
				if (pFmt->GetHidden() == HIDDEN_YES) {
					continue;
				}

				CUGCell cellGrid;
				CGTblCell* pGTblCell = GetGTblCell(iCol,iRow);
				if(pGTblCell){
					double dValue = pGTblCell->GetData();
					if(dValue ==0  || dValue >= 1.0e50 ) {
						continue;
					}
					else {
						bHide =false;
						break;
					}
				}
			}
		}
	}
	return bHide;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::IsSystemTotal
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::IsSystemTotal(CGTblOb* pGTblOb)
{
    CGTblCol* pTblCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
    if(pTblCol && !pTblCol->GetTabVal()){
        return pTblCol->GetTabVar() &&  pTblCol->GetTabVar()->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0;
    }

    return false;
}
//Savy (R) sampling app 20081209
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::IsSystemStat
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::IsSystemStat(CGTblOb* pGTblOb)
{
    CGTblCol* pTblCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
    if(pTblCol && !pTblCol->GetTabVal()){
        return pTblCol->GetTabVar() &&  pTblCol->GetTabVar()->GetName().CompareNoCase(WORKVAR_STAT_NAME) ==0;
    }

    return false;
}

void CTblGrid::ProcessHideCaptions()
{
    //go through all the rows
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    for(int iRow = 0; iRow < GetNumberRows(); iRow++){
        if (CheckHideCaptionRow(iRow)) {
            SetRowHeight(iRow,0);
        }
    }
}

void CTblGrid::ProcessHideSpanners(bool bOnlySystemTotalSpanner /*=false*/)
{
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    //For each column
    //For each row until the numheader -1
    //check if spanner . If hidden . get row and make the rowwidth zero
    int iNumCols = GetNumberCols();
    int iNumHeaders = GetNumHeaderRows();
    int iStartCol;
    long iStartRow;
    CFmt retFmt;
    bool bHide =false;
    bool bIsSystemTotal = false;
    for(int iCol =1; iCol < iNumCols ; iCol++){
        for(int iRow = 1; iRow < iNumHeaders-1; iRow++){
            eGridComp = FMT_ID_INVALID;
            pGTblOb = nullptr;
            iStartCol = iCol;
            iStartRow = iRow;
            GetJoinStartCell(&iStartCol, &iStartRow);
            if(iStartCol != iCol && iStartRow != iRow)
                continue;

            GetComponent(iCol,iRow,eGridComp,&pGTblOb);
            if(eGridComp != FMT_ID_SPANNER){
                continue;
            }
            else {
                bIsSystemTotal = IsSystemTotal(pGTblOb);
                GetFmt4NonDataCell(pGTblOb,eGridComp, retFmt);
                if(bOnlySystemTotalSpanner){
                    bHide = bIsSystemTotal;
                }
                else {
                    bHide = retFmt.GetHidden() == HIDDEN_YES || bIsSystemTotal;
                }
                if(bHide){
                    SetRowHeight(iRow,0);
                }
            }
        }
    }
}

void CTblGrid::ProcessHideStubs()
{
     //go through all the rows
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    int iStartRow = GetNumHeaderRows()-1;
	CTabSetFmt* pTabSetFmt=DYNAMIC_DOWNCAST(CTabSetFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABSET));
	CIMSAString sZeroFill =pTabSetFmt->GetZeroMask();
	CIMSAString sZRound =pTabSetFmt->GetZRoundMask();

    for(int iRow = iStartRow; iRow < GetNumberRows(); iRow++){
        eGridComp = FMT_ID_INVALID;
        pGTblOb = nullptr;
        GetComponent(0,iRow,eGridComp,&pGTblOb);
        if(eGridComp != FMT_ID_STUB){
            continue;
        }
        else {//Hide the row
            CGTblRow* pTblRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
            if(pTblRow && pTblRow->GetTabVal()){
                CFmt retFmt;
                GetFmt4NonDataCell(pGTblOb,eGridComp, retFmt);
                if(retFmt.GetHidden() == HIDDEN_YES){
                    SetRowHeight(iRow,0);
                }
				else if (CheckHideAllZeroRow(iRow)) {
						SetRowHeight(iRow,0);
				}
			}
		}
	}
}

void CTblGrid::BuildAllRunTimeFmts()
{
    //go through all the rows
    FMT_ID eGridComp = FMT_ID_INVALID;
    int iPanel =0;
    CUGCell cellGrid;
    CDataCellFmt fmt,eValDFmt;
    CArray<CGTblCol*,CGTblCol*> arrTblCols;
    int iNumCols = GetNumberCols();
    CGTblCol* pGTblCol = nullptr;
    CGTblOb** pGTblOb = nullptr;
    for(int iRow = m_iGridHeaderRows; iRow < GetNumberRows(); iRow++){
        iPanel = GetRowPanel(iRow);
        //iPanel =0;
        if(m_arrGTblRows.GetSize() <= iRow-m_iGridHeaderRows){
            //This happens in case of headers /footers / stats in frq
            break;
        }
        CGTblRow* pGTblRow =m_arrGTblRows[iRow-m_iGridHeaderRows];
        for(int iCol =1; iCol < iNumCols ; iCol++){
            if(!IsDataCell(iCol,iRow)){
                continue;
            }

            ASSERT(pGTblRow);
            if(iRow == m_iGridHeaderRows){
                GetCell(iCol,m_iGridHeaderRows-1,&cellGrid);
                CTblOb** pOb = (CTblOb**)cellGrid.GetExtraMemPtr();
                ASSERT(pOb);
                pGTblCol = DYNAMIC_DOWNCAST(CGTblCol,*pOb);
                ASSERT(pGTblCol);
                arrTblCols.Add(pGTblCol);
            }
            pGTblCol = arrTblCols[iCol-1];
            ASSERT(pGTblCol);

            CSpecialCell* pSpecialCell = nullptr;

            CTabValue* pRowTabVal =  pGTblRow->GetTabVal();
            CTabVar* pRowTabVar =  pGTblRow->GetTabVar();

            if(pRowTabVal){
                if(pRowTabVal->m_arrRunTimeDataCellFmts.GetSize() != iNumCols){
                    pSpecialCell =pRowTabVal->FindSpecialCell(iPanel,iCol);
                    GetFmt4DataCell2(pGTblRow, pGTblCol,fmt,pSpecialCell);
                    pRowTabVal->m_arrRunTimeDataCellFmts.Add(fmt);
                }
                eValDFmt = pRowTabVal->m_arrRunTimeDataCellFmts[iCol];
            }
            else if(pRowTabVar){
                if(pRowTabVar->m_arrRunTimeDataCellFmts.GetSize() != iNumCols){
                    pSpecialCell =nullptr;
                    GetFmt4DataCell2(pGTblRow, pGTblCol,fmt,pSpecialCell);
                    pRowTabVar->m_arrRunTimeDataCellFmts.Add(fmt);
                }
                eValDFmt = pRowTabVar->m_arrRunTimeDataCellFmts[iCol];
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ApplyFormat2DataCells2(int iCol,long lRow, CUGCell* cell)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ApplyFormat2DataCells2(int iCellCol ,long lCellRow , CUGCell* cell)
{
    if(!m_bGridUpdate || lCellRow >= GetLastDataRow())
        return;

    int iStartRow = m_iGridHeaderRows;
    int iMaxRows = GetNumberRows();
    int iNumCols = GetNumberCols();

    CTabView* pView = (CTabView*)GetParent();
    bool bDesignView = true;
    pView ? bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView() : bDesignView = true;

    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    CDataCellFmt eValDFmt;
    CDataCellFmt fmt;
    CIMSAString sVal;
    double dValue =0;
    double dAbs =0;
    int iPanel = 0;
    CTabSetFmt* pTabSetFmt=DYNAMIC_DOWNCAST(CTabSetFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABSET));
    CIMSAString sZeroFill =pTabSetFmt->GetZeroMask();

	// 20100215 possible optimization
	if( lCellRow < iStartRow || lCellRow >= iMaxRows || iCellCol < 1 || iCellCol >= iNumCols )
		return;

    //for(long iRow = iStartRow; iRow < iMaxRows;iRow++){
    for(long iRow = lCellRow; iRow <= lCellRow;iRow++){
        iPanel = GetRowPanel(iRow);
        for(int iCol =iCellCol; iCol <= iCellCol ; iCol++){

/*    for(long iRow = iStartRow; iRow < iMaxRows;iRow++){
        if(iRow != lCellRow){
            continue;
        }
        iPanel = GetRowPanel(iRow);
        for(int iCol =1; iCol < iNumCols ; iCol++){
            if(iCellCol != iCol){
                continue;
            }*/
            GetComponent(iCol,iRow,eGridComp,&pGTblOb);
            if(!IsDataCell(iCellCol,lCellRow) || eGridComp == FMT_ID_INVALID){
                continue;//We are not concerned with non-data cells
            }
            dValue =0;
            sVal =_T("");
            CTblOb** pOb = (CTblOb**)cell->GetExtraMemPtr();
            {
                CGTblCell* pGTblCell = DYNAMIC_DOWNCAST(CGTblCell,*pOb);
                CSpecialCell* pSpecialCell = nullptr;
                CTabValue* pRowTabVal =  pGTblCell->GetTblRow()->GetTabVal();
                CTabVar* pRowTabVar =  pGTblCell->GetTblRow()->GetTabVar();
                bool bNonDataCell = pGTblCell->GetTblRow()->HasChildren()  || (pRowTabVal &&  pRowTabVal->GetTabValType() == RDRBRK_TABVAL);
                bool bNoData = (m_pTable && m_pTable->GetTabDataArray().GetSize()==0);

                if(pRowTabVal){
                    if(pRowTabVal->m_arrRunTimeDataCellFmts.GetSize() != iNumCols){
                        ASSERT(FALSE); //all the formats shld be ready by now
                    }
                    eValDFmt = pRowTabVal->m_arrRunTimeDataCellFmts[iCol];
                }
                else if(pRowTabVar){
                    if(pRowTabVar->m_arrRunTimeDataCellFmts.GetSize() != iNumCols){
                        ASSERT(FALSE); //all the formats shld be ready by now
                    }
                    eValDFmt = pRowTabVar->m_arrRunTimeDataCellFmts[iCol];
                }
                if(bNonDataCell || bNoData){
                    sVal = _T("");
                }
                else {
                    if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblCell))){
                        dValue = ((CGTblCell*)(*pOb))->GetData();
                    }
                    dValue < 0  ? dAbs = -dValue : dAbs = dValue;
                    if(dAbs <= 1.0e50){
                        sVal = m_pTable->FormatDataCell(dValue, m_pSpec->GetTabSetFmt(),&eValDFmt);
                    }
                    else {
                      //  sVal = "";
                        sVal = sZeroFill;
                    }
                }
                cell->SetText(sVal);
                ApplyFormat2Cell(*cell,&eValDFmt);
            }
            SetCell(iCol, iRow ,cell);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ClearAllRunTimeFmts(CGTblRow* pGTblRow)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ClearAllRunTimeFmts(CGTblRow* pGTblRow)
{
    //go through all the rows
    if(pGTblRow){
        CTabValue* pTabVal = pGTblRow->GetTabVal();
        CTabVar* pTabVar = pGTblRow->GetTabVar();

        if(pTabVal){
            pTabVal->m_arrRunTimeDataCellFmts.RemoveAll();
        }
        else if(pTabVar){
            pTabVar->m_arrRunTimeDataCellFmts.RemoveAll();
        }
        CGTblRow* pChild = nullptr;
        for(int iIndex =0; iIndex < pGTblRow->GetNumChildren(); iIndex++){
            pChild = DYNAMIC_DOWNCAST(CGTblRow,pGTblRow->GetChild(iIndex));
            ClearAllRunTimeFmts(pChild);
        }
    }
}

void CTblGrid::ClearAllRunTimeFmts()
{
    ClearAllRunTimeFmts(m_pTblRowRoot);
    //Clear ColHead Fmts
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    //For each column
    //For each row until the numheader -1
    //check if spanner . If hidden . get row and make the rowwidth zero
    int iNumCols = GetNumberCols();
    int iNumHeaders = GetNumHeaderRows();
    int iStartCol;
    long iStartRow;
    for(int iCol =1; iCol < iNumCols ; iCol++){
        for(int iRow = 1; iRow <= iNumHeaders-1; iRow++){
            eGridComp = FMT_ID_INVALID;
            pGTblOb = nullptr;
            iStartCol = iCol;
            iStartRow = iRow;
            GetJoinStartCell(&iStartCol, &iStartRow);
            if(iStartCol != iCol && iStartRow != iRow)
                continue;

            GetComponent(iCol,iRow,eGridComp,&pGTblOb);
            if(eGridComp != FMT_ID_COLHEAD){
                continue;
            }
            else {
                CGTblCol* pTblCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                if(pTblCol && pTblCol->GetTabVal()){
                    pTblCol->GetTabVal()->m_arrRunTimeDataCellFmts.RemoveAll();
                }
            }
        }
    }
}

void CTblGrid::ProcessHideColHeads()
{
     FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    //For each column
    //For each row until the numheader -1
    //check if spanner . If hidden . get row and make the rowwidth zero
    int iNumCols = GetNumberCols();
    int iNumHeaders = GetNumHeaderRows();
    int iStartCol;
    long iStartRow;
    for(int iCol =1; iCol < iNumCols ; iCol++){
        for(int iRow = 1; iRow <= iNumHeaders-1; iRow++){
            eGridComp = FMT_ID_INVALID;
            pGTblOb = nullptr;
            iStartCol = iCol;
            iStartRow = iRow;
            GetJoinStartCell(&iStartCol, &iStartRow);
            if(iStartCol != iCol && iStartRow != iRow)
                continue;

            GetComponent(iCol,iRow,eGridComp,&pGTblOb);
            if(eGridComp != FMT_ID_COLHEAD){
                continue;
            }
            else {
                CGTblCol* pTblCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                if(pTblCol && pTblCol->GetTabVal()){
                    CFmt retFmt;
                    GetFmt4NonDataCell(pGTblOb,eGridComp, retFmt);
                    if(retFmt.GetHidden() == HIDDEN_YES){
                        SetColWidth(iCol,0);
                    }
                }
            }
        }
    }
 }

/////////////////////////////////////////////////////////////////////////////////
//
//CTabVar* CTblGrid::GetColHead(int iCurrentCol)
//
/////////////////////////////////////////////////////////////////////////////////
CTabVar* CTblGrid::GetColHead(int iCurrentCol)
{
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    CTabVar* pRetVar  = nullptr;
    //For each column
    //For each row until the numheader -1
    //check if spanner . If hidden . get row and make the rowwidth zero
    int iNumCols = GetNumberCols();
    int iNumHeaders = GetNumHeaderRows();
    int iStartCol;
    long iStartRow;
    for(int iCol =1; iCol < iNumCols ; iCol++){
        if( iCol>iCurrentCol){
            break;
        }
        else if (iCol != iCurrentCol){
            continue;
        }
        for(int iRow = 1; iRow <= iNumHeaders-1; iRow++){
            eGridComp = FMT_ID_INVALID;
            pGTblOb = nullptr;
            iStartCol = iCol;
            iStartRow = iRow;

            GetJoinStartCell(&iStartCol, &iStartRow);
            GetComponent(iCol,iRow,eGridComp,&pGTblOb);
            if(eGridComp != FMT_ID_COLHEAD){
                continue;
            }
            else {
                CGTblCol* pTblCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                pRetVar = pTblCol->GetTabVar();

            }
        }
    }
    return pRetVar;
}

long CTblGrid::GetLastDataRow()
{
    bool bHasPageNote =false;
    bool bHasEndNote = false;
    long  iLastDataRow = m_iNumGridRows;
    int iPageNoteRow = -1;
    int iEndNoteRow = -1;
    if(!m_pTable){
        return m_iNumGridRows;
    }
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

    pTblFmt ?  bHasPageNote = pTblFmt->HasPageNote() : bHasPageNote = pDefTblFmt->HasPageNote();
    pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();

    bHasPageNote ? iLastDataRow-- : iLastDataRow;
    bHasEndNote ? iLastDataRow-- : iLastDataRow;

    return iLastDataRow;
}
void CTblGrid::SetAreaLabels4AllTables()
{
    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    CTabSet* pTabSet = pDoc->GetTableSpec();
    CIMSAString sBreakKey,sAreaLabel;
    bool bIsViewer = ((CTableChildWnd*)pView->GetParentFrame())->IsViewer();
    CMapStringToString& areaLabelLookup = pDoc->GetAreaLabelLookup();
    if(HasArea() && !bIsViewer){
        for (int iTable =0 ; iTable <pTabSet->GetNumTables(); iTable++){
            CArray<CTabData*, CTabData*>&arrTabData =pTabSet->GetTable(iTable)->GetTabDataArray();
            if(arrTabData.GetSize()  < 1){
                continue;
            }

            for(int iTbdSlice =0; iTbdSlice < arrTabData.GetSize(); iTbdSlice++){

                CTabData* pTabData = arrTabData[iTbdSlice]; //zero for now when areas come in do the rest
                sBreakKey = pTabData->GetBreakKey();
                sBreakKey.Remove(';');
                sBreakKey.Replace(_T("-"),_T(" "));
                sBreakKey.MakeUpper();
                sAreaLabel = _T("");
                if (bIsViewer) {
                    // in viewer we don't have areaLabelLookup so use
                    // area label in CTabData if there is one
                    if (!pTabData->GetAreaLabel().IsEmpty()) {
                        sBreakKey = pTabData->GetAreaLabel();
                    }
                }
                else if(areaLabelLookup.Lookup(sBreakKey,sAreaLabel)){

                    CIMSAString sSeperatedBreakKey;
                    CTabSet* pTabSet = pDoc->GetTableSpec();
                    int iNumAreaLevels = pTabSet->GetConsolidate()->GetNumAreas();
                    CArray<int,int> arrAreaLen;
                    const CDataDict* pDict = pTabSet->GetDict();
                    int iStart=0;
                    for(int iArea =0; iArea < iNumAreaLevels ; iArea++){
                        const DictLevel* pDictLevel = nullptr;
                        const CDictRecord* pDictRecord = nullptr;
                        const CDictItem* pDictItem = nullptr;
                        const DictValueSet* pDictVSet = nullptr;

                        CIMSAString sAreaName = pTabSet->GetConsolidate()->GetArea(iArea);
                        const CDataDict* pDict= pTabSet->LookupName(sAreaName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
                        ASSERT(pDict);
                        ASSERT((int)(iStart+pDictItem->GetLen()) <= sBreakKey.GetLength());
                        if(iArea ==iNumAreaLevels -1){
                            sSeperatedBreakKey += sBreakKey.Mid(iStart,pDictItem->GetLen());
                        }
                        else {
                            sSeperatedBreakKey += sBreakKey.Mid(iStart,pDictItem->GetLen())+_T(";");
                        }
                        iStart += pDictItem->GetLen();
                    }
                    pTabData->SetBreakKey(sSeperatedBreakKey);
					if(IsOneRowVarTable()){
						CTabVar* pTabVar = m_pTable->GetRowRoot()->GetChild(0);
						if(m_pTable->GetRowRoot()->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) != 0){
							sAreaLabel.Trim();//remove indent
						}
					}
                    pTabData->SetAreaLabel(sAreaLabel);
                }
            }
        }
    }
}

void CTblGrid::ProcessAreaTokensinRows()
{
    //Go through the CTable data area and format the text to put into the cells
    int iIndex =0;
    int iNumcols = GetNumberCols();

    if(!m_pTable)
        return;

    CArray<CTabData*, CTabData*>&arrTabData =  m_pTable->GetTabDataArray();
    if(arrTabData.GetSize()  < 1)
        return;
    long iLastDataRow = GetLastDataRow();
    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();

    CMapStringToString& areaLabelLookup = pDoc->GetAreaLabelLookup();

    int iStartRow = m_iGridHeaderRows;
    int iEndRow = iLastDataRow;
    int iRow =-1;
    for(int iTbdSlice =0; iTbdSlice < arrTabData.GetSize(); iTbdSlice++){
        if(m_iCurrSelArea !=0 && iTbdSlice != m_iCurrSelArea -1){
            continue;
        }
        CTabData* pTabData = arrTabData[iTbdSlice]; //zero for now when areas come in do the rest
        CIMSAString sBreakKey = pTabData->GetBreakKey();
        sBreakKey.Remove(';');
        sBreakKey.Replace(_T("-"),_T(" "));
        sBreakKey.MakeUpper();
        CIMSAString sAreaLabel;
        if (((CTableChildWnd*)pView->GetParentFrame())->IsViewer()|| HasArea()) {
            // in viewer we don't have areaLabelLookup so use
            // area label in CTabData if there is one
            if (!pTabData->GetAreaLabel().IsEmpty()) {
				CIMSAString sAreaLabel = pTabData->GetAreaLabel();
				if(IsOneRowVarTable()){
					CTabVar* pTabVar = m_pTable->GetRowRoot()->GetChild(0);
					if(m_pTable->GetRowRoot()->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) != 0){
						sAreaLabel.Trim();//remove indent
					}
				}
                sBreakKey = sAreaLabel;
            }
        }
        /*else if(HasArea()){
            CTabView* pView = (CTabView*)GetParent();
            CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();

            CIMSAString sSeperatedBreakKey;
            CTabSet* pTabSet = pDoc->GetTableSpec();
            int iNumAreaLevels = pTabSet->GetConsolidate()->GetNumAreas();
            CArray<int,int> arrAreaLen;
            CDataDict* pDict = pTabSet->GetDict();
            int iStart=0;
            for(int iArea =0; iArea < iNumAreaLevels ; iArea++){
                DictLevel* pDictLevel = nullptr;
                CDictRecord* pDictRecord = nullptr;
                CDictItem* pDictItem = nullptr;
                DictValueSet* pDictVSet = nullptr;

                CIMSAString sAreaName = pTabSet->GetConsolidate()->GetArea(iArea);
                CDataDict* pDict= pTabSet->LookupName(sAreaName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
                ASSERT(pDict);
                ASSERT((int)(iStart+pDictItem->GetLen()) <= sBreakKey.GetLength());
                if(iArea ==iNumAreaLevels -1){
                    sSeperatedBreakKey += sBreakKey.Mid(iStart,pDictItem->GetLen());
                }
                else {
                    sSeperatedBreakKey += sBreakKey.Mid(iStart,pDictItem->GetLen())+";";
                }
                iStart += pDictItem->GetLen();
            }
            pTabData->SetBreakKey(sSeperatedBreakKey);
            if (areaLabelLookup.Lookup(sBreakKey,sAreaLabel)) {
                pTabData->SetAreaLabel(sAreaLabel);
                sBreakKey = sAreaLabel;
            }
        }*/


        CArray<double, double&>& arrCells = pTabData->GetCellArray();
        iIndex =0;
        int iNumCells = arrCells.GetSize();
		bool bHideAreaGroupSection = false;
        for(iRow = iStartRow; iRow < iLastDataRow; iRow++) {
            if(iIndex == iNumCells){
                break; //We are done with the slice
            }
            //if it is row group continue;
            CUGCell cellGrid;
            GetCell(0,iRow,&cellGrid);
            CIMSAString sStubOrCaption;
            QuickGetText(0,iRow,&sStubOrCaption);
            CIMSAString sTemp = sStubOrCaption;
            CIMSAString sWord;
            while (sTemp.GetLength() > 0) {
                sWord = sTemp.GetToken();
                if (sWord.CompareNoCase(AREA_TOKEN) == 0) {
					bHideAreaGroupSection =false;
                    sStubOrCaption.Replace(sWord,sBreakKey);
                }
            }
            QuickSetText(0,iRow,sStubOrCaption);
			CIMSAString sSuppressCaption = sStubOrCaption;
			sSuppressCaption.Trim();
			if(!sSuppressCaption.IsEmpty() && sSuppressCaption[0] == _T('~')){
				bHideAreaGroupSection = true;
				SetRowHeight(iRow,0);
			}
			if(bHideAreaGroupSection){//Hide the rows under the area group if the current group is to be set hidden 'cos of ~
				SetRowHeight(iRow,0);
			}
            CTblOb** pOb = (CTblOb**)cellGrid.GetExtraMemPtr();
            if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblRow)) ){
                CTabValue* pTabVal = ((CGTblRow*)(*pOb))->GetTabVal();
                CTabVar* pTabVar = ((CGTblRow*)(*pOb))->GetTabVar();
                if(!pTabVal && !pTabVar){//this is area caption
                    continue;
                }
                if(pTabVal && pTabVal->GetTabValType() == RDRBRK_TABVAL){//reader break no data
                    continue;
                }
                else if ( ((CGTblRow*)(*pOb))->GetNumChildren() > 0 ){
                    continue; //row group no data
                }
            }
            for (int iCol = 1; iCol < iNumcols ; iCol++) {
                CUGCell cellGrid;
                CIMSAString sVal;
                GetCell(iCol,iRow,&cellGrid);
                //Get the cell

                if(iIndex > iNumCells ){
                    ASSERT(FALSE); //grid cellls exceeds data cells
                }
                double dValue =0;
                if(iIndex> iNumCells){
                    ASSERT(FALSE); //SAVY To add stuff for Notapplicable/Default/missing  etc into the
                    //crosstab command .
                }
                iIndex++;

            }

        }
        iStartRow = iRow;
    }

}

bool CTblGrid::HasArea()
{
    bool bHasArea = false;
    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();

    int iNumAreaLevels = pDoc->GetTableSpec()->GetConsolidate()->GetNumAreas();
    iNumAreaLevels > 0 ? bHasArea = true : bHasArea = false;

    return bHasArea;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblGrid::IsOneRowVarTable()
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::IsOneRowVarTable()
{
    bool bOneRow = m_pTable->GetRowRoot() && m_pTable->GetRowRoot()->GetNumChildren() == 1 && m_pTable->GetRowRoot()->GetChild(0)->GetNumChildren() ==0  &&  m_pTable->GetColRoot()->GetNumChildren() != 0;
    return bOneRow;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ReconcileSpecialCells()
//
//If the use drops a var on column we have a problem withe specials
//cos the current system has no information about reconciling the offset
//information in the pRowVal's specials array.Talked to chris about it
//and we decided for now we delete all the specials of the rows when the
//user drops /drags out  a var from the col .And recreate if specials are required
//based on the rowvar;s format. so If the col is custom and row is custom and extends
//we create the special for the cell based on the rowvar.
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ReconcileSpecialCells()
{
    //Remove all the special cells from row
    for(int iRow = m_iGridHeaderRows; iRow < m_iNumGridRows; iRow++) {
        //if it is row group continue;
        CUGCell cellGrid;
        GetCell(0,iRow,&cellGrid);
        CTblOb** pOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblRow)) ){
            CTabValue* pTabVal = ((CGTblRow*)(*pOb))->GetTabVal();
            if(pTabVal){
                if(pTabVal->GetTabValType() == RDRBRK_TABVAL){//reader break no data
                    continue;
                }
                else {
                    pTabVal->GetSpecialCellArr().RemoveAll();
                }
            }
            else if ( ((CGTblRow*)(*pOb))->GetNumChildren() > 0 ){
                continue; //row group no data
            }

        }
    }
    //Recreate specials if needed
    for(int iRow = m_iGridHeaderRows; iRow < m_iNumGridRows; iRow++) {
        //if it is row group continue;
        CUGCell cellGrid;
        GetCell(0,iRow,&cellGrid);
        CTblOb** pOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblRow)) ){
            CTabValue* pTabVal = ((CGTblRow*)(*pOb))->GetTabVal();
            if(!pTabVal){
                continue;
            }
            else if(pTabVal->GetTabValType() == RDRBRK_TABVAL){//reader break no data
                continue;
            }
            else if ( ((CGTblRow*)(*pOb))->GetNumChildren() > 0 ){
                continue; //row group no data
            }
            else {//Add Specials
                CFmtReg* pFmtReg = m_pTable->GetFmtRegPtr();
                CTabView* pTabView = (CTabView*)GetParent();
                for(int iCol = 1; iCol < GetNumberCols();iCol++){
                    //do we need special for iCol,iRow if so add it
                    bool bProcessSpecial =false;
                    bool bRowFmtCustom = pTabVal->GetDerFmt() && pTabVal->GetDerFmt()->GetIndex()!=0;
                    bRowFmtCustom ? bProcessSpecial = true:bProcessSpecial =false;
                    if(bProcessSpecial){
                        pTabView->m_iColRClick =0;
                        pTabView->m_lRowRClick=iRow;
                        pTabView->ProcessSpecials(pTabVal);
                    }
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
// void CTblGrid::ReconcileTabValFmts()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ReconcileTabValFmts()
{
    //Reconcile Rows
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    CFmtReg* pFmtReg = m_pTable->GetFmtRegPtr();
    for(int iRow = m_iGridHeaderRows; iRow < m_iNumGridRows; iRow++) {
        //if it is row group continue;
        CUGCell cellGrid;
        GetCell(0,iRow,&cellGrid);
        CTblOb** pOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblRow)) ){
            CTabValue* pTabVal = ((CGTblRow*)(*pOb))->GetTabVal();
            CTabVar* pTabVar = ((CGTblRow*)(*pOb))->GetTabVar();

            if(pTabVal && pTabVal->GetTabValType() == RDRBRK_TABVAL){//reader break no data
                continue;
            }
            else if(pTabVar && pTabVal){
                eGridComp = FMT_ID_INVALID;
                pGTblOb = nullptr;
                GetComponent(0,iRow,eGridComp,&pGTblOb);
                //We have a TabVal .Check if this row is a caption. If the format is not of
                // the "caption type" .delete this tabVal's format.
                CFmt* pFmt = pTabVar->GetDerFmt();

                if(eGridComp == FMT_ID_CAPTION){
                    if(pTabVal->GetDerFmt() && pTabVal->GetDerFmt()->GetID()!=FMT_ID_CAPTION){
                        ASSERT(pTabVal->GetDerFmt()->GetIndex()!=0);//cannot be the default fmt
                        pFmtReg->Remove(pTabVal->GetDerFmt()->GetID(),pTabVal->GetDerFmt()->GetIndex());
                        pTabVal->SetFmt(nullptr);
                    }

                }
                else if(eGridComp == FMT_ID_STUB){
                    if(pTabVal->GetDerFmt() && pTabVal->GetDerFmt()->GetID()!=FMT_ID_STUB){
                        ASSERT(pTabVal->GetDerFmt()->GetIndex()!=0);//cannot be the default fmt
                        pFmtReg->Remove(pTabVal->GetDerFmt()->GetID(),pTabVal->GetDerFmt()->GetIndex());
                        pTabVal->SetFmt(nullptr);
                    }
                }

                //We have a TabVal .Check if this row is a stub. If the format is not of
                // the "stub type" .delete this tabVal's format.
                //ReconcileSpecialCells will take care of recreating specials
/*

                CTabView* pTabView = (CTabView*)GetParent();
                for(int iCol = 1; iCol < GetNumberCols();iCol++){
                    //do we need special for iCol,iRow if so add it
                    bool bProcessSpecial =false;
                    bool bRowFmtCustom = pTabVal->GetDerFmt() && pTabVal->GetDerFmt()->GetIndex()!=0;
                    bRowFmtCustom ? bProcessSpecial = true:bProcessSpecial =false;
                    if(bProcessSpecial){
                        pTabView->m_iColRClick =0;
                        pTabView->m_lRowRClick=iRow;
                        pTabView->ProcessSpecials(pTabVal);
                    }
                }*/
            }
        }
    }
    //Reconcile columns
    int iStartRow = GetStartRow4SpannerNColHeadProcessing();
    int iNumHeaderRows  = GetNumHeaderRows();
    int iNumCols  = GetNumberCols();
    int iRow = -1;
    for(iRow = iStartRow; iRow < iNumHeaderRows;iRow++){
        for(int iCol =1; iCol < iNumCols ; iCol++){
            eGridComp = FMT_ID_INVALID;
            pGTblOb = nullptr;
            CUGCell cellGrid;
            int iStartCol = iCol;
            long iStartRow = iRow;
            GetJoinStartCell(&iStartCol, &iStartRow);
            if(iStartCol != iCol || iStartRow != iRow)
                continue;
            GetCell(iStartCol,iStartRow,&cellGrid);
            // GetComponent(iCol,iRow,eGridComp,&pGTblOb);
            GetComponent(iStartCol,iStartRow,eGridComp,&pGTblOb);
            ASSERT(pGTblOb);
            if(pGTblOb && (pGTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol)) ){
                CTabVar* pTabVar = ((CGTblCol*)pGTblOb)->GetTabVar();
                CTabValue* pTabVal = ((CGTblCol*)(pGTblOb))->GetTabVal();
                if(pTabVar && pTabVal){
                    if(eGridComp == FMT_ID_SPANNER){
                        if(pTabVal->GetDerFmt() && pTabVal->GetDerFmt()->GetID()!=FMT_ID_SPANNER){
                            ASSERT(pTabVal->GetDerFmt()->GetIndex()!=0);//cannot be the default fmt
                            pFmtReg->Remove(pTabVal->GetDerFmt()->GetID(),pTabVal->GetDerFmt()->GetIndex());
                            pTabVal->SetFmt(nullptr);
                        }

                    }
                    else if(eGridComp == FMT_ID_COLHEAD){
                        if(pTabVal->GetDerFmt() && pTabVal->GetDerFmt()->GetID()!=FMT_ID_COLHEAD){
                            ASSERT(pTabVal->GetDerFmt()->GetIndex()!=0);//cannot be the default fmt
                            pFmtReg->Remove(pTabVal->GetDerFmt()->GetID(),pTabVal->GetDerFmt()->GetIndex());
                            pTabVal->SetFmt(nullptr);
                        }
                    }
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  int CTblGrid::GetStartRow4SpannerNColHeadProcessing()
//
/////////////////////////////////////////////////////////////////////////////////
int CTblGrid::GetStartRow4SpannerNColHeadProcessing()
{
    int iStartRow = 1;
    bool bHasSubTitle =false;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));
    pTblFmt ?  bHasSubTitle = pTblFmt->HasSubTitle() : bHasSubTitle = pDefTblFmt->HasSubTitle();
    bHasSubTitle  ? iStartRow =2 : iStartRow =1;
    return iStartRow;
}

LRESULT CTblGrid::OnUpdateTable(WPARAM /*wParam*/,LPARAM/* lParam*/)
{
    Update();
    return 0L;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::CleanupDataCells()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::CleanupDataCells()
{
    if(m_pTable){
        m_pTable->RemoveAllData();
        DestroyCells();
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ForceUpdate()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ForceUpdate()
{
    CWaitCursor wait;
    //for all data cells .
    //check if the alloc exits other wise create and set it
    int iStartRow = m_iGridHeaderRows;
    int iNumRows = GetNumberRows();
    int iNumCols = GetNumberCols();
    CUGCell cell;
    for(long lRow =0; lRow < iNumRows; lRow++){
        for(int iCol = 1; iCol < iNumCols; iCol++){
            if(!IsDataCell(iCol,lRow)){
                continue;
            }
            GetCell(iCol,lRow,&cell);
            CGTblCell* pGTblCell = GetGTblCell(iCol,lRow);
            ASSERT(pGTblCell);
            void** pOb = (void**)cell.GetExtraMemPtr();
            //Is cell data cell
            //If so get the fmt and apply to cell
            //set it in the cellgrid
            //Get the CGTblCell and set it
            if(!pOb){
                pOb = (void**)cell.AllocExtraMem(sizeof(LPVOID));
                *pOb = pGTblCell;
            }
            else {
                continue;
            }
            //Apply format to data cell
            ApplyFormat2DataCells2(iCol,lRow,&cell);
            SetCell(iCol,lRow,&cell);

            pGTblCell->SetDirty(false);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblGrid::CanDrawSubTblBoxes()
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::CanDrawSubTblBoxes()
{
    bool bRet = true;
    CTabView* pView = (CTabView*)GetParent();
    ASSERT(pView);
    bool bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView();
    if(!bDesignView || !m_pTable){
        return false;
    }

    CTabVar* pRowRootVar = m_pTable->GetRowRoot();
    int iNumChildren = pRowRootVar->GetNumChildren();
    if(iNumChildren <=1){
        if(iNumChildren ==0){
            bRet =false;
        }
        else {
            CTabVar* pTabVar = pRowRootVar->GetChild(0);
            if(pTabVar->GetNumChildren() <=1){
                CTabVar* pColRootVar = m_pTable->GetColRoot();
                if(pColRootVar->GetNumChildren() == 1 ){
                    bRet = false;
                    if(pColRootVar->GetChild(0)->GetNumChildren() >1){
                        bRet =true;
                    }
                }
            }
        }
    }
    return bRet;
}

void CTblGrid::ComputeSubTableRects()
{
    m_arrSubTableBoxes.RemoveAll();
    if(!CanDrawSubTblBoxes()){
        return;
    }
    int iColorIndex =0;
    const COLORREF BOX_RED = RGB(255,0,0);
    const COLORREF BOX_BLUE = RGB(0,0,255);
    const COLORREF BOX_GREEN = RGB(0,0,255);

    // Go through all the rows . If only one var first value rectangle dims
    bool bDone = false;
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    int iStartRow = GetNumHeaderRows()-1;
    int iNumCols = GetNumberCols();

    if(m_pTable && m_pTable->GetRowRoot()->GetNumChildren() ==1){
        CTabVar* pTabVar = m_pTable->GetRowRoot()->GetChild(0);
        if(pTabVar && pTabVar->GetNumChildren() ==0){ //Not of type A*B
            //Get the first CGRowColOb stub which has the same tabval  as the first
            //TabVal of this var
            bDone =true;
            if(pTabVar->GetNumValues() >0) {
                CTabValue* pTabVal = pTabVar->GetValue(0);
                for(int iRow = iStartRow; iRow < GetNumberRows(); iRow++){
                    eGridComp = FMT_ID_INVALID;
                    GetComponent(0,iRow,eGridComp,&pGTblOb);
                    if(eGridComp != FMT_ID_STUB){
                        continue;
                    }
                    if(pGTblOb){
                        CGTblRow* pSubTblRowVar = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                        if(pSubTblRowVar){
                            ComputeSubTableRects(pSubTblRowVar,iColorIndex);
                            break; //done after first call only one var
                        }
                    }
                }
            }
        }
    }
    if(!bDone){
        CMap<CTabVar*,CTabVar*,int,int&> mapSubTables;
        for(int iRow = iStartRow; iRow < GetNumberRows(); iRow++){
            eGridComp = FMT_ID_INVALID;
            pGTblOb = nullptr;
            GetComponent(0,iRow,eGridComp,&pGTblOb);
            if(eGridComp != FMT_ID_CAPTION){
                continue;
            }
            else if(pGTblOb){
                CGTblRow* pSubTblRowVar = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                //ignore tabvars "A" of type A*B .Just look for vars without any children
                if(pSubTblRowVar && pSubTblRowVar->GetTabVar() && pSubTblRowVar->GetTabVar()->GetNumChildren() ==0){
                    int iVal =0;
                    if(mapSubTables.Lookup(pSubTblRowVar->GetTabVar(),iVal)){
                        //Already processed this subtable
                        continue;
                    }
                    ComputeSubTableRects(pSubTblRowVar,iColorIndex);
                    iVal =1;
                    mapSubTables.SetAt(pSubTblRowVar->GetTabVar(),iVal);
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ComputeSubTableRects(CGTblRow* pSubTblRowVar,int& iColorIndex)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ComputeSubTableRects(CGTblRow* pSubTblRowVar,int& iColorIndex)
{
    bool bRowSubPiecesDone = false;
    CMap<CTabVar*,CTabVar*,int,int&> mapSubTables2Color;
    RECT* pRect = nullptr;
    while(!bRowSubPiecesDone){
        //Get top point  for this stub/caption
        if(pRect){
            delete pRect;
            pRect = nullptr;
        }
        pRect = new RECT;
        RECT& rect = *pRect;
        rect.left = rect.top = rect.bottom =rect.right =0;
        RECT stubOrCaptionRect;
        GetCellRect(0,pSubTblRowVar->GetStartRow(),&stubOrCaptionRect);
        rect.top=stubOrCaptionRect.top;
        //Get bottom point this stub
        int iBottomRow =pSubTblRowVar->GetStartRow()+pSubTblRowVar->GetNumGridRows();
        if(IsOneRowVarTable()){
            iBottomRow =pSubTblRowVar->GetStartRow()+pSubTblRowVar->GetTabVar()->GetNumValues();
            iBottomRow--;
        }
        GetCellRect(0,iBottomRow,&stubOrCaptionRect);
        rect.bottom=stubOrCaptionRect.bottom;
        //For each subtable along the column get the remaining coordinates
        //along the columns check for C*(D+E)case
        int iNumHeaders = GetNumHeaderRows();
        int iNumCols = GetNumberCols();
        FMT_ID eGridComp = FMT_ID_INVALID;
        CGTblOb* pGTblOb = nullptr;
        for(int iCol =1; iCol < iNumCols ; iCol++){
            for(int iRow =1; iRow < iNumHeaders-1; iRow++){
                eGridComp = FMT_ID_INVALID;
                pGTblOb = nullptr;
                int iStartCol = iCol;
                long iStartRow = iRow;
                GetJoinStartCell(&iStartCol, &iStartRow);
                if(iStartCol != iCol || iStartRow != iRow)
                    continue;

                GetComponent(iCol,iRow,eGridComp,&pGTblOb);
                if(!pGTblOb || eGridComp != FMT_ID_SPANNER){
                    continue;
                }
                else {
                    CGRowColOb* pSpannerOb = DYNAMIC_DOWNCAST(CGRowColOb,pGTblOb);
                    //Check if it has a child which is a tabvar. if so ignore.
                    //cos we need the lowest spanner
                    if(pSpannerOb->GetTabVar() && pSpannerOb->GetTabVar()->GetNumChildren() ==0) {
                        int iEndCol = iStartCol;
                        long iEndRow = iStartRow;
                        GetJoinRange(&iStartCol, &iStartRow, &iEndCol, &iEndRow);

                        CRect spannerRect;
                        //Get the rectangle
                        GetRangeRect(iStartCol,iStartRow,iEndCol,iEndRow,spannerRect);
                        spannerRect.left >0 ? rect.left =spannerRect.left : rect.left = rect.left;
                        spannerRect.right >0 ? rect.right =spannerRect.right : rect.right =rect.right;

                        //GetRangeRect gives incorrect rectangle when both the  start ,endcol is not visible
                        //which give extra lines extending beyond gridwidth this is workaround fix for the problem
                        if(rect.right > m_GI->m_gridWidth){
                            CRect cellRect;
                            GetCellRect(GetNumberCols()-1,iStartRow,&cellRect);
                            rect.right = cellRect.right;
                        }

                        int iSubTableColor =iColorIndex;
                        if(mapSubTables2Color.Lookup(pSpannerOb->GetTabVar(),iSubTableColor)){
                        }
                        else {
                            mapSubTables2Color.SetAt(pSpannerOb->GetTabVar(),iSubTableColor);
                            iColorIndex++;//increment the colorindex for a new subtable
                        }
                        //Fill the rect. //Set the box strucure with rect and color //add this to the array of boxes
                        SUBTBL_BOX subTblBox;
                        subTblBox.m_iColorIndex = iSubTableColor;
                        subTblBox.m_rect = rect;
                        m_arrSubTableBoxes.Add(subTblBox);
                    }
                }
            }
        }
        //Get the next subpiece  //if cant find break
        bRowSubPiecesDone = true;
        for(int iRow = pSubTblRowVar->GetStartRow()+1; iRow < GetNumberRows(); iRow++){
            eGridComp = FMT_ID_INVALID;
            pGTblOb = nullptr;
            GetComponent(0,iRow,eGridComp,&pGTblOb);
            if(eGridComp != FMT_ID_CAPTION){
                continue;
            }
            else if(pGTblOb){
                CGTblRow* pCurrentRowVar = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                if(pCurrentRowVar && pCurrentRowVar->GetTabVar() == pSubTblRowVar->GetTabVar()){
                    pSubTblRowVar=pCurrentRowVar;
                    bRowSubPiecesDone = false;
                    break;
                }
            }
        }
        //End while loop
        if(pRect){
            delete pRect ;
            pRect = nullptr;
        }
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::DrawSubTableRects(bool bComputeRects /*=false*/)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::DrawSubTableRects(bool bComputeRects /*=false*/)
{
    if(!CanDrawSubTblBoxes()){
        return;
    }
    if(bComputeRects){
        ComputeSubTableRects();
    }
    if(m_arrSubTableBoxes.GetSize() ==0) {
        //ASSERT(false);
        return;
    }
    //Get the "y" coordinate for the the stubhead bottom
    int stubBottom = -1;
    if(m_GI->m_topRow != m_GI->m_numLockRows){
        //now get the bottom coordinate for the stubhead
        CRect stubRect(0,0,0,0);
        int iStartCol = 0;
        long iStartRow = GetNumHeaderRows()-1;
        int iEndCol = iStartCol;
        long iEndRow = iStartRow;
        GetJoinRange(&iStartCol, &iStartRow, &iEndCol, &iEndRow);
        GetRangeRect(iStartCol,iStartRow,iEndCol,iEndRow,&stubRect);
        stubBottom = stubRect.bottom;
    }
    CDC *pDC = m_CUGGrid->GetDC();
    for(int iIndex = 0; iIndex <m_arrSubTableBoxes.GetSize(); iIndex++){
        int iSavedDC = pDC->SaveDC();
        CRect rect(m_arrSubTableBoxes[iIndex].m_rect);
        int iColor =  m_arrSubTableBoxes[iIndex].m_iColorIndex % 3;
        //Get the rectangle
        COLORREF colorRef;
        switch(iColor){
            case 0:
                colorRef = RGB(0,0,192);
                break;
            case 1:
                colorRef = RGB(192,0,0);
                break;
            case 2:
                colorRef = RGB(0,192,0);
                break;
            default :
                ASSERT(FALSE);
        }
        CPen penThickLine(PS_SOLID, 3, colorRef);
        pDC->SelectObject(&penThickLine);
        if(stubBottom != -1 && (rect.top == stubBottom || rect.bottom ==stubBottom)){
            //Draw lines instead of rect and ignore the topline
            if(rect.bottom == stubBottom){
            }
            else if(rect.top == stubBottom) { //ignore the topline
                rect.DeflateRect(1,1);
                pDC->MoveTo(rect.left,rect.top);
                pDC->LineTo(rect.left,rect.bottom);
                pDC->LineTo(rect.right,rect.bottom);
                pDC->LineTo(rect.right,rect.top);
            }
        }
        else {
            rect.DeflateRect(1,1);
            pDC->MoveTo(rect.left,rect.top);
            pDC->LineTo(rect.left,rect.bottom);
            pDC->LineTo(rect.right,rect.bottom);
            pDC->LineTo(rect.right,rect.top);
            pDC->LineTo(rect.left,rect.top);
        }
        pDC->RestoreDC(iSavedDC);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::FixAreaTokensInOneRowTable()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::FixAreaTokensInOneRowTable()
{
    if(IsOneRowVarTable()){
        if(HasArea()){
            CTabVar* pTabVar = m_pTable->GetRowRoot()->GetChild(0);
            if(pTabVar && pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                pTabVar->GetValue(0)->SetText(AREA_TOKEN);
            }
        }
        else {
            CTabVar* pTabVar = m_pTable->GetRowRoot()->GetChild(0);
            ASSERT(pTabVar);
            if(pTabVar && pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                pTabVar->GetValue(0)->SetText(TOTAL_LABEL);
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblGrid::ForceHideAreaCaptionInOneRowTable()
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::ForceHideAreaCaptionInOneRowTable()
{
    bool bRet = false;
    if(IsOneRowVarTable()){
        if(HasArea()){
            CTblOb* pAreaCaption = m_pTable->GetAreaCaption();
            CFmt* pAreaCaptionFmt = pAreaCaption->GetDerFmt();
            CFmtReg* pFmtReg = m_pTable->GetFmtRegPtr();
            CFmt* pDefAreaCaptionFmt = DYNAMIC_DOWNCAST(CFmt,pFmtReg->GetFmt(FMT_ID_AREA_CAPTION));
            if(pAreaCaptionFmt && pAreaCaptionFmt->GetIndex() !=0){ //do nothing
            }
            else{
                CTabVar* pTabVar = m_pTable->GetRowRoot()->GetChild(0);
                ASSERT(pTabVar);
                if(pTabVar && pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                    bRet = true;
                }
            }
        }
    }
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblGrid::GetFmt4AreaCaptionCells(CGTblCol* pGTblCol ,CFmt& retFmt)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblGrid::GetFmt4AreaCaptionCells(CGTblCol* pGTblCol ,CFmt& retFmt)
{
    bool bRet = false;
    CTblOb* pTblOb = m_pTable->GetAreaCaption();

    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();
    CFmt* pAreaCaptionFmt = nullptr;

    pAreaCaptionFmt = pTblOb->GetDerFmt();
    if(!pAreaCaptionFmt){
        pAreaCaptionFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_AREA_CAPTION));
    }
    if(pAreaCaptionFmt->GetSpanCells() != SPAN_CELLS_NO){
        return bRet;
    }

    bool bDeleteEvalDRowFmt = false;
    bool bDeleteEvalDColFmt = false;
    //Get RowFmt

    CFmt* pDefColFmt = nullptr;

    retFmt = *pAreaCaptionFmt;

    //Get ColFmt
    ASSERT(pGTblCol->GetTabVar());
    CDataCellFmt* pColFmt  =  nullptr;
    CDataCellFmt* pEvaldColFmt = nullptr;

    CTabValue* pColTabVal = pGTblCol->GetTabVal();
    CTabVar* pColTabVar = pGTblCol->GetTabVar();

    if(pColTabVal ){
        CArray<CDataCellFmt,CDataCellFmt&>& arrColDataCellFmts =  pColTabVal->m_arrRunTimeDataCellFmts;
        arrColDataCellFmts.GetSize() > 0 ? pEvaldColFmt = &arrColDataCellFmts[0] : pEvaldColFmt = nullptr;

        if(!pEvaldColFmt){
            pColFmt =pColTabVal->GetDerFmt();
            pEvaldColFmt = new CDataCellFmt();
            bDeleteEvalDColFmt = true;
            if(!pColFmt){
                pDefColFmt = pColFmt  = DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
                GetFmt4NonDataCell(pGTblCol,FMT_ID_COLHEAD,*pEvaldColFmt);
            }
            else {
                pDefColFmt= DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
                GetFmt4NonDataCell(pGTblCol,FMT_ID_COLHEAD,*pEvaldColFmt);
            }
            arrColDataCellFmts.Add(*pEvaldColFmt);
           // delete pEvaldColFmt;
        }
        else {
            pColFmt =pColTabVal->GetDerFmt();
            if(!pColFmt){
                pDefColFmt = pColFmt  = DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
            }
            else {
                pDefColFmt= DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_COLHEAD));
            }
        }
    }
    else{
       ASSERT(FALSE);
    }


    //Do Lines
    bool bRowCust = false; //Lines are independent of col (top/bottom)
    bool bColCust = false; //lines are independent of row (left/right)
    bool bRowExtends = pAreaCaptionFmt->GetLinesExtend();
    bool bColExtends = pEvaldColFmt->GetLinesExtend();
    retFmt.SetLineLeft(LINE_NONE);
    retFmt.SetLineRight(LINE_NONE);
    retFmt.SetLineBottom(LINE_NONE);
    retFmt.SetLineTop(LINE_NONE);
    if(bRowExtends || bColExtends){//if it does not extend you dont need to do anything
        //Lines cant be applied on an individual cell  .So special cells line attribute
        //need not be considered
        if(bRowExtends ){
            bool bTop = true;
            bool bBottom = true;
            if(!pAreaCaptionFmt->GetLinesExtend()){
                bTop = false;
                bBottom = false;
            }
            if(bTop){
                retFmt.SetLineTop(pAreaCaptionFmt->GetLineTop());
            }
            if(bBottom){
                retFmt.SetLineBottom(pAreaCaptionFmt->GetLineBottom());
            }
        }
        if(bColExtends){//use col val if col extends and not row
            bool bLeft = true;
            bool bRight = true;
            if(!pColFmt->GetLinesExtend()){
                bLeft = false;
                bRight = false;
                CGRowColOb* pParent = DYNAMIC_DOWNCAST(CGRowColOb,pGTblCol->GetParent());
                ASSERT(pParent);
                int iNumChildren = pParent->GetNumChildren();
                if(pGTblCol == pParent->GetChild(0)){
                    bLeft = true;
                }
                if(pGTblCol == pParent->GetChild(iNumChildren-1)){
                    bRight = true;
                }
            }
            if(bLeft){
               retFmt.SetLineLeft(pEvaldColFmt->GetLineLeft());
            }
            if(bRight){
                retFmt.SetLineRight(pEvaldColFmt->GetLineRight());
            }

        }
    }
    retFmt.GetLineBottom() == LINE_DEFAULT? retFmt.SetLineBottom(LINE_NONE) :_T("") ;
    retFmt.GetLineTop() ==    LINE_DEFAULT? retFmt.SetLineTop(LINE_NONE) :_T("") ;
    retFmt.GetLineRight() == LINE_DEFAULT? retFmt.SetLineRight(LINE_NONE) :_T("") ;
    retFmt.GetLineLeft() == LINE_DEFAULT? retFmt.SetLineLeft(LINE_NONE) :_T("") ;

    //Hidden
    retFmt.SetHidden(pAreaCaptionFmt->GetHidden() == HIDDEN_YES || pEvaldColFmt->GetHidden() == HIDDEN_YES ?
                     HIDDEN_YES :
                     HIDDEN_NO);

    bDeleteEvalDColFmt? delete pEvaldColFmt : pEvaldColFmt = pEvaldColFmt;

    return true;
}

void CTblGrid::DrawLines4AreaCaptionRows()
{
    //for each row starting from header rows
    //look for area caption .
    //get the cells line format .
    //Draw lines using the format information
    //go through all the rows
    CTblOb* pTblOb = m_pTable->GetAreaCaption();

    CTabView* pView = (CTabView*)GetParent();
    CTabulateDoc* pDoc = (CTabulateDoc*)pView->GetDocument();
    const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();
    CFmt* pAreaCaptionFmt = nullptr;

    pAreaCaptionFmt = pTblOb->GetDerFmt();
    if(!pAreaCaptionFmt){
        pAreaCaptionFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_AREA_CAPTION));
    }
    if(pAreaCaptionFmt->GetSpanCells() != SPAN_CELLS_NO){
        return ;
    }
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    int iStartRow = GetNumHeaderRows()-1;
    int iNumCols = GetNumberCols();
    CArray<CFmt,CFmt&> arrLineFmts4AreaCaptionCells;
    bool bComputedOnce = false;

    for(int iRow = iStartRow; iRow < GetNumberRows(); iRow++){
        eGridComp = FMT_ID_INVALID;
        pGTblOb = nullptr;
        GetComponent(0,iRow,eGridComp,&pGTblOb);
        if(eGridComp != FMT_ID_AREA_CAPTION){
            continue;
        }
        if(!bComputedOnce){
            for(int iCol =1; iCol < iNumCols ; iCol++){
                for(int iColheadRow = 1; iColheadRow <= GetNumHeaderRows()-1; iColheadRow++){
                    eGridComp = FMT_ID_INVALID;
                    pGTblOb = nullptr;
                    int iStartCol = iCol;
                    long iStartRow = iColheadRow;
                    GetJoinStartCell(&iStartCol, &iStartRow);
                    if(iStartCol != iCol && iStartRow != iColheadRow)
                        continue;

                    GetComponent(iCol,iColheadRow,eGridComp,&pGTblOb);
                    if(eGridComp != FMT_ID_COLHEAD){
                        continue;
                    }
                    CFmt fmt;
                    CGTblCol* pGTblCol  = DYNAMIC_DOWNCAST(CGTblCol ,pGTblOb);
                    ASSERT(pGTblCol);
                    GetFmt4AreaCaptionCells(pGTblCol ,fmt);
                    arrLineFmts4AreaCaptionCells.Add(fmt);
                }
            }
            bComputedOnce = true;
        }
        ASSERT(arrLineFmts4AreaCaptionCells.GetSize() == GetNumberCols()-1);
        //ApplyFmts for these cells
        for(int iCol =1; iCol < iNumCols ; iCol++){
            //Do the  lines . //First set them appropriately and do Fixlines later
            int iLineMask = StyleLineToGridMask(&arrLineFmts4AreaCaptionCells[iCol-1]);
            CUGCell gridCell;
            GetCell(iCol,iRow,&gridCell);
            gridCell.SetBorder(iLineMask);
            gridCell.SetBorderColor(&(m_blackPen)); //for now border color is black
            SetCell(iCol, iRow ,&gridCell);
        }

    }
}

void CTblGrid::SavePageAndEndNoteStateInfo()
{
    bool bHasPageNote =false;
    bool bHasEndNote = false;
    int iPageNoteRow = -1;
    int iEndNoteRow = -1;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

    pTblFmt ?  bHasPageNote = pTblFmt->HasPageNote() : bHasPageNote = pDefTblFmt->HasPageNote();
    pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();


    if(bHasPageNote && bHasEndNote){
        iPageNoteRow = m_iNumGridRows-2;
        iEndNoteRow = m_iNumGridRows-1;

    }
    else if (bHasPageNote){
        iPageNoteRow = m_iNumGridRows-1;
    }
    else if (bHasEndNote){
        iEndNoteRow = m_iNumGridRows-1;
    }

    int iCols = GetNumberCols();

  //Do PageNote if it has one
    if(iPageNoteRow != -1){
		CGrdViewInfo gridViewInfo;
        CTblOb* pTblOb = m_GPageNote.GetTblOb();
		if(pTblOb){
			pTblOb->RemoveAllGrdViewInfo();
			gridViewInfo.SetCurrSize(CSize(0,GetRowHeight(iPageNoteRow)));
			pTblOb->AddGrdViewInfo(gridViewInfo);
		}
    }

    //Do EndNote if it has one
    if(iEndNoteRow !=-1){
		CGrdViewInfo gridViewInfo;
        CTblOb* pTblOb = m_GEndNote.GetTblOb();
		if(pTblOb){
			pTblOb->RemoveAllGrdViewInfo();
			gridViewInfo.SetCurrSize(CSize(0,GetRowHeight(iEndNoteRow)));
			pTblOb->AddGrdViewInfo(gridViewInfo);
		}
    }

}
void CTblGrid::ApplyPageAndEndNoteStateInfo()
{
    bool bHasPageNote =false;
    bool bHasEndNote = false;
    int iPageNoteRow = -1;
    int iEndNoteRow = -1;
    CTblFmt* pTblFmt = m_pTable->GetDerFmt();
    CTblFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABLE));

    pTblFmt ?  bHasPageNote = pTblFmt->HasPageNote() : bHasPageNote = pDefTblFmt->HasPageNote();
    pTblFmt ?  bHasEndNote = pTblFmt->HasEndNote() : bHasEndNote = pDefTblFmt->HasEndNote();


    if(bHasPageNote && bHasEndNote){
        iPageNoteRow = m_iNumGridRows-2;
        iEndNoteRow = m_iNumGridRows-1;

    }
    else if (bHasPageNote){
        iPageNoteRow = m_iNumGridRows-1;
    }
    else if (bHasEndNote){
        iEndNoteRow = m_iNumGridRows-1;
    }

    int iCols = GetNumberCols();

  //Do PageNote if it has one
    if(iPageNoteRow != -1){
        CTblOb* pTblOb = m_GPageNote.GetTblOb();
		if(pTblOb->GetGrdViewInfoSize() > 0){
			SetRowHeight(iPageNoteRow,pTblOb->GetGrdViewInfo(0).GetCurrSize().cy);
		}
    }

    //Do EndNote if it has one
    if(iEndNoteRow !=-1){
        CTblOb* pTblOb = m_GEndNote.GetTblOb();
		if(pTblOb->GetGrdViewInfoSize() > 0){
			SetRowHeight(iEndNoteRow,pTblOb->GetGrdViewInfo(0).GetCurrSize().cy);
		}
    }

}
#ifdef _DELME
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblGrid::ApplyFormat2DataCells()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblGrid::ApplyFormat2DataCells()
{
    int iStartRow = m_iGridHeaderRows;
    int iMaxRows = GetNumberRows();
    int iNumCols = GetNumberCols();

    CTabView* pView = (CTabView*)GetParent();
    bool bDesignView = true;
    pView ? bDesignView = ((CTableChildWnd*)pView->GetParentFrame())->IsDesignView() : bDesignView = true;

    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    CUGCell cellGrid;
    CDataCellFmt eValDFmt;
    CDataCellFmt fmt;
    CIMSAString sVal;
    double dValue =0;
    int iPanel = 0;
    CTabSetFmt* pTabSetFmt=DYNAMIC_DOWNCAST(CTabSetFmt,m_pTable->GetFmtRegPtr()->GetFmt(FMT_ID_TABSET));
    CIMSAString sZeroFill =pTabSetFmt->GetZeroMask();

    for(int iRow = iStartRow; iRow < iMaxRows;iRow++){
        iPanel = GetRowPanel(iRow);
        //iPanel =0;
        for(int iCol =1; iCol < iNumCols ; iCol++){
            GetComponent(iCol,iRow,eGridComp,&pGTblOb);
            if(eGridComp != FMT_ID_DATACELL){
                continue;
            }
            GetCell(iCol,iRow,&cellGrid);
            dValue =0;
            sVal =_T("");
            CTblOb** pOb = (CTblOb**)cellGrid.GetExtraMemPtr();
            if(bDesignView){
                GetFmt4DataCell(iCol,iRow, fmt);
                ApplyFormat2Cell(cellGrid,&fmt);

            }
            else{
                CGTblCell* pGTblCell = DYNAMIC_DOWNCAST(CGTblCell,pGTblOb);
                CSpecialCell* pSpecialCell = nullptr;
                CTabValue* pRowTabVal =  pGTblCell->GetTblRow()->GetTabVal();
                CTabVar* pRowTabVar =  pGTblCell->GetTblRow()->GetTabVar();

                if(pRowTabVal){
                    if(pRowTabVal->m_arrRunTimeDataCellFmts.GetSize() != iNumCols){
                        pSpecialCell =pRowTabVal->FindSpecialCell(iPanel,iCol);
                        GetFmt4DataCell2(pGTblCell->GetTblRow(), pGTblCell->GetTblCol(),fmt,pSpecialCell);
                        pRowTabVal->m_arrRunTimeDataCellFmts.Add(fmt);
                    }
                    eValDFmt = pRowTabVal->m_arrRunTimeDataCellFmts[iCol];
                }
                else if(pRowTabVar){
                    if(pRowTabVar->m_arrRunTimeDataCellFmts.GetSize() != iNumCols){
                        pSpecialCell =nullptr;
                        GetFmt4DataCell2(pGTblCell->GetTblRow(), pGTblCell->GetTblCol(),fmt,pSpecialCell);
                        pRowTabVar->m_arrRunTimeDataCellFmts.Add(fmt);
                    }
                    eValDFmt = pRowTabVar->m_arrRunTimeDataCellFmts[iCol];
                }
                if(pOb && (*pOb)->IsKindOf(RUNTIME_CLASS(CGTblCell))){
                    dValue = ((CGTblCell*)(*pOb))->GetData();
                }
                if(dValue <= 1.0e50){
                    sVal = m_pTable->FormatDataCell(dValue, m_pSpec->GetTabSetFmt(),&eValDFmt);
                }
                else {
                    //sVal = "";
                    sVal = sZeroFill;
                }
                cellGrid.SetText(sVal);
                ApplyFormat2Cell(cellGrid,&eValDFmt);
            }
            SetCell(iCol, iRow ,&cellGrid);
        }
    }
}

#endif


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnShiftF10
//
/////////////////////////////////////////////////////////////////////////////
void CTblGrid::OnShiftF10() {

    int col = 0;
    long row = 0;
    int updn = 0;
    RECT rect = RECT();
    POINT point = POINT();
    int processed = 0;
    OnRClicked(col, row, updn, &rect, &point, processed);

}
