/**********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGExcelCtrl
    Purpose
        This class is derived from the grid's main
        base class CUGCtrl.
        This class is setup to give the grid an
        Excel look and feel without the need for
        programming it in.
        Derive from this class if an Excel look
        and feel is required.
    Details
        -This class uses an excel style focus rectangle,
         multiple selection, editing, tabs with multiple
         sheets and other excel like features.
        -This class still has all the functionality of
         the CUGCtrl class.
************************************************/

#include "StdAfx.h"
#include "Ugexcel.h"


BEGIN_MESSAGE_MAP(CUGExcelCtrl,CUGCtrl)
    //{{AFX_MSG_MAP(CUGExcelCtrl)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/***************************************************
****************************************************/
CUGExcelCtrl::CUGExcelCtrl()
{
}
/***************************************************
****************************************************/
CUGExcelCtrl::~CUGExcelCtrl()
{
}

/***************************************************
OnSetup
    This function is called just after the grid window
    is created or attached to a dialog item.
    It can be used to initially setup the grid
****************************************************/
void CUGExcelCtrl::OnSetup(){

    CUGCell cell;

    m_font.CreateFont(16,0,0,0,500,0,0,0,0,0,0,0,0,_T("Arial"));
    SetDefFont(&m_font);

    GetGridDefault(&cell);
    cell.SetBorder(UG_BDR_RTHIN|UG_BDR_BTHIN);
    cell.SetFont(&m_font);
    cell.SetHBackColor(RGB(0,0,0));
    SetGridDefault(&cell);

    GetHeadingDefault(&cell);
    cell.SetFont(&m_font);
    SetHeadingDefault(&cell);

    SetDefRowHeight(17);
    SetDefColWidth(70);
    SetTH_Height(17);
    SetSH_Width(30);

    SetMultiSelectMode(1);

    SetDoubleBufferMode(1);
}

/***************************************************
OnCanMove
    Sent when the current cell in the grid is about
    to move
    A return of TRUE allows the move, a return of
    FALSE stops the move
****************************************************/
int CUGExcelCtrl::OnCanMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}
/***************************************************
OnCanMove
    Sent when the top row or left column in the grid is about
    to move
    A return of TRUE allows the move, a return of
    FALSE stops the move
****************************************************/
int CUGExcelCtrl::OnCanViewMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}
/***************************************************
****************************************************/
void CUGExcelCtrl::OnHitBottom(long numrows,long rowspast,long rowsfound){
}
/***************************************************
****************************************************/
void CUGExcelCtrl::OnHitTop(long numrows,long rowspast){

}
/***************************************************
OnCanSizeCol
    Sent when the user is over a separation line on
    the top heading
    A return value of TRUE allows the possibiliy of
    a resize
****************************************************/
int CUGExcelCtrl::OnCanSizeCol(int col){
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
void CUGExcelCtrl::OnColSizing(int col,int *width){
}
/***************************************************
OnColSized
    This is sent when the user finished sizing the
    given column (see above for more details)
****************************************************/
void CUGExcelCtrl::OnColSized(int col,int *width){
}
/***************************************************
OnCanSizeRow
    Sent when the user is over a separation line on
    the side heading
    A return value of TRUE allows the possibiliy of
    a resize
****************************************************/
int  CUGExcelCtrl::OnCanSizeRow(long row){
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
void CUGExcelCtrl::OnRowSizing(long row,int *height){
}
/***************************************************
OnRowSized
    This is sent when the user is finished sizing hte
    given row ( see above for more details)
****************************************************/
void CUGExcelCtrl::OnRowSized(long row,int *height){
}
/***************************************************
OnCanSizeSideHdg
    This is sent when the user moves into position
    for sizing the width of the side heading
    return TRUE to allow the sizing
    or FALSE to not allow it
****************************************************/
int CUGExcelCtrl::OnCanSizeSideHdg(){
    return TRUE;
}
/***************************************************
OnCanSizeTopHdg
    This is sent when the user moves into position
    for sizing the height of the top heading
    return TRUE to allow the sizing
    or FALSE to not allow it
****************************************************/
int CUGExcelCtrl::OnCanSizeTopHdg(){
    return TRUE;
}
/***************************************************
OnSideHdgSizing
****************************************************/
int CUGExcelCtrl::OnSideHdgSizing(int *width){
    return TRUE;

}
/***************************************************
OnTopHdgSizing
****************************************************/
int CUGExcelCtrl::OnTopHdgSizing(int *height){
    return TRUE;

}
/***************************************************
OnColChange
    Sent whenever the current column changes
    The old and the new columns are given
****************************************************/
void CUGExcelCtrl::OnColChange(int oldcol,int newcol){
}
/***************************************************
OnRowChange
    Sent whenever the current row changes
    The old and the new rows are given
****************************************************/
void CUGExcelCtrl::OnRowChange(long oldrow,long newrow){
}
/***************************************************
OnCellChange
    Sent whenever the current cell changes rows or
    columns
****************************************************/
void CUGExcelCtrl::OnCellChange(int oldcol,int newcol,long oldrow,long newrow){
}
/***************************************************
OnLeftColChange
    Sent whenever the left visible column in the grid
    changes
****************************************************/
void CUGExcelCtrl::OnLeftColChange(int oldcol,int newcol){
}
/***************************************************
OnTopRowChange
    Sent whenever the top visible row in the grid changes
****************************************************/
void CUGExcelCtrl::OnTopRowChange(long oldrow,long newrow){
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
void CUGExcelCtrl::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed){
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
void CUGExcelCtrl::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed){
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
void CUGExcelCtrl::OnDClicked(int col,long row,RECT *rect,POINT *point,int processed){
    StartEdit();
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
void CUGExcelCtrl::OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){

    if(updn){
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

        Update();
    }
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
void CUGExcelCtrl::OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnTH_LClicked
    Sent whenever the user double clicks the left mouse
    button within the top heading

    'col' is negitive if the area clicked in is not valid
****************************************************/
void CUGExcelCtrl::OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
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
void CUGExcelCtrl::OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){

    if(updn){
        if(GetKeyState(VK_CONTROL) <0){
            SelectRange(0,row,GetNumberCols()-1,row);
        }
        else if(GetKeyState(VK_SHIFT) <0){
            m_GI->m_multiSelect->EndBlock(GetNumberCols()-1,row);
        }
        else{
            ClearSelections();
            SelectRange(0,row,GetNumberCols()-1,row);
        }

        Update();
    }
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
void CUGExcelCtrl::OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnSH_DClicked
    Sent whenever the user double clicks the left mouse
    button within the side heading

    'row' is negitive if the area clicked in is not valid
****************************************************/
void CUGExcelCtrl::OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
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
void CUGExcelCtrl::OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed){

    SelectRange(0,0,GetNumberCols()-1,GetNumberRows()-1);
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
void CUGExcelCtrl::OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnCB_DClicked
    Sent whenever the user double clicks the left mouse
    button within the top corner button
****************************************************/
void CUGExcelCtrl::OnCB_DClicked(RECT *rect,POINT *point,BOOL processed){

}
/***************************************************
OnKeyDown
    Sent whenever the user types when the grid has
    focus. The keystroke can be modified here as well.
    (See WM_KEYDOWN for more information)
****************************************************/
void CUGExcelCtrl::OnKeyDown(UINT *vcKey,int processed){
    if(*vcKey == VK_DELETE)
        if(DeleteCell(GetCurrentCol(),GetCurrentRow()) == UG_SUCCESS)
            RedrawAll();
}
/***************************************************
OnCharDown
    Sent whenever the user types when the grid has
    focus. The keystroke can be modified here as well.
    (See WM_CHAR for more information)
****************************************************/
void CUGExcelCtrl::OnCharDown(UINT *vcKey,int processed){
    if(!processed)
        StartEdit(*vcKey);
}

/***************************************************
OnGetCell
    This message is sent everytime the grid needs to
    draw a cell in the grid. At this point the cell
    class has been filled with the information to be
    used to draw the cell. The information can now be
    changed before it is used for drawing
****************************************************/
void CUGExcelCtrl::OnGetCell(int col,long row,CUGCell *cell){

    CString buf;

    if(col < 0){

        if(row < 0)
            return;

        buf.Format(_T("%ld"),row+1);
        cell->SetText(buf);
    }
    else if(row <0){
        int low = col%26;
        int hi  = (col/26)%26;
        if(hi >0)
            buf.Format(_T("%c%c"),(_T('A')+hi-1),(_T('A')+low));
        else
            buf.Format(_T("%c"),(_T('A')+low));
        cell->SetText(buf);
    }
}
/***************************************************
OnSetCell
    This message is sent everytime the a cell is about
    to change.
****************************************************/
void CUGExcelCtrl::OnSetCell(int col,long row,CUGCell *cell){
}
/***************************************************
OnDataSourceNotify
    This message is sent from a data source , message
    depends on the data source - check the information
    on the data source(s) being used
    - The ID of the Data source is also returned
****************************************************/
void CUGExcelCtrl::OnDataSourceNotify(int ID,long msg,long param){
}
/***************************************************
OnCellTypeNotify
    This message is sent from a cell type , message
    depends on the cell type - check the information
    on the cell type classes
    - The ID of the cell type is given
****************************************************/
int CUGExcelCtrl::OnCellTypeNotify(long ID,int col,long row,long msg,long param){
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
int CUGExcelCtrl::OnEditStart(int col, long row,CWnd **edit){
    return TRUE;
}
/***************************************************
OnEditVerify
    This is send when the editing is about to end
****************************************************/
int CUGExcelCtrl::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey){
    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CUGExcelCtrl::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag){
    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CUGExcelCtrl::OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow){
    return TRUE;
}
/***************************************************
sections - UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
            UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
****************************************************/
void CUGExcelCtrl::OnMenuCommand(int col,long row,int section,int item){

    if(section == UG_GRID && item == 1000){
        LockColumns(col);
        LockRows(row);
        RedrawAll();
    }
}
/***************************************************
return UG_SUCCESS to allow the menu to appear
return 1 to not allow the menu to appear
****************************************************/
int CUGExcelCtrl::OnMenuStart(int col,long row,int section){
    //if(section != UG_GRID)
    //  return FALSE;
    return TRUE;
}

/*********************************************
drag and drop
**********************************************/
#ifdef UG_ENABLE_DRAGDROP
/***************************************************
****************************************************/
DROPEFFECT  CUGExcelCtrl::OnDragEnter(COleDataObject* pDataObject){
    return DROPEFFECT_NONE;
}
/***************************************************
****************************************************/
DROPEFFECT  CUGExcelCtrl::OnDragOver(COleDataObject* pDataObject,int col,long row){
    return DROPEFFECT_NONE;
}
/***************************************************
****************************************************/
DROPEFFECT  CUGExcelCtrl::OnDragDrop(COleDataObject* pDataObject,int col,long row){
    return DROPEFFECT_NONE;
}
void CUGExcelCtrl::OnDragLeave(CWnd* pWnd)
{
}
DROPEFFECT CUGExcelCtrl::OnDragScroll(CWnd* pWnd, DWORD dwKeyState, CPoint point)
{
    return DROPEFFECT_NONE;
}
#endif
/***************************************************
****************************************************/
void CUGExcelCtrl::OnScreenDCSetup(CDC *dc,int section){
}
/***************************************************
****************************************************/
void CUGExcelCtrl::OnTabSelected(int ID){

    SetSheetNumber(ID);
}
/***************************************************
****************************************************/
BOOL CUGExcelCtrl::OnColSwapStart(int col){

    return TRUE;
}

/***************************************************
    Purpose
    Params
    Return
****************************************************/
void CUGExcelCtrl::OnDrawFocusRect(CDC *dc,RECT *rect){

    DrawExcelFocusRect(dc,rect);
}

