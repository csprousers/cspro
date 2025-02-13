/***************************************************
****************************************************
Skeleton Class for a Derived CAplFileAssociationsGrid v3.5
****************************************************
****************************************************/
#include "StdAfx.h"
#include "AplFileAssociationsGrid.h"
#include <zUtilO/FileUtil.h>

const int GRID_DEFAULT_ROWHEIGHT =  5;
const int GRID_DEFAULT_COLWIDTH  = 75;
const int GRID_DEFAULT_COLS      =  0;
const int GRID_DEFAULT_ROWS      =  0;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CAplFileAssociationsGrid,CUGCtrl)
//{{AFX_MSG_MAP(CAplFileAssociationsGrid)
// NOTE - the ClassWizard will add and remove mapping macros here.
//    DO NOT EDIT what you see in these blocks of generated code !
//}}AFX_MSG_MAP

END_MESSAGE_MAP()


/***************************************************
****************************************************/
CAplFileAssociationsGrid::CAplFileAssociationsGrid()
{
}
/***************************************************
****************************************************/
CAplFileAssociationsGrid::~CAplFileAssociationsGrid()
{
}


void CAplFileAssociationsGrid::MyClear(void){

    EnableExcelBorders(FALSE);

    SetNumberCols(GRID_DEFAULT_COLS);
    SetNumberRows(GRID_DEFAULT_ROWS);

    for (int iCol=0; iCol<GRID_DEFAULT_COLS; iCol++) {
        for (int iRow=0; iRow<GRID_DEFAULT_ROWS; iRow++) {
            QuickSetText(iCol, iRow, _T(""));
            QuickSetBorder (iCol, iRow, 0);
        }
        SetColWidth(iCol, GRID_DEFAULT_COLWIDTH);
    }
}

/***************************************************
OnSetup
This function is called just after the grid window
is created or attached to a dialog item.
It can be used to initially setup the grid
****************************************************/
void CAplFileAssociationsGrid::OnSetup(){

    RECT rect = {0,0,0,0};
    m_pPifEdit.Create(WS_VISIBLE,rect,this,125);

    VScrollAlwaysPresent(TRUE);
    EnableExcelBorders(TRUE);
    SetMultiSelectMode(TRUE);

    m_iEllipsisIndex =AddCellType(&m_Ellipsis);
    SetNumberCols(m_iCols);
    SetNumberRows(m_iRows);
    for (int iCol =0 ; iCol < m_iCols ; iCol++){
        SetColWidth(iCol, GRID_DEFAULT_COLWIDTH);
    }

    //m_font.CreateFont(15,0,0,0,400,0,0,0,0,0,0,0,0,_T("MS Sans Serif"));
    m_font.CreateFont(15,0,0,0,400,0,0,0,0,0,0,0,0,_T("MS Shell Dlg")); // GHM 20111228 changed for unicode
    SetDefFont(&m_font);

    QuickSetFont(-1,-1,&m_font);
    QuickSetText(-1,-1,_T(""));
    QuickSetText(0,-1,_T("Data File Name"));

    QuickSetAlignment (-1,-1, UG_ALIGNBOTTOM | UG_ALIGNLEFT);
    QuickSetAlignment (0,-1, UG_ALIGNBOTTOM | UG_ALIGNLEFT);

    AdjustComponentSizes();


}

/***************************************************
OnSheetSetup
****************************************************/
void CAplFileAssociationsGrid::OnSheetSetup(int sheetNumber){
    SetMultiSelectMode(TRUE);
}

/***************************************************
OnCanMove
Sent when the current cell in the grid is about
to move
A return of TRUE allows the move, a return of
FALSE stops the move
****************************************************/
int CAplFileAssociationsGrid::OnCanMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}
/***************************************************
OnCanMove
Sent when the top row or left column in the grid is about
to move
A return of TRUE allows the move, a return of
FALSE stops the move
****************************************************/
int CAplFileAssociationsGrid::OnCanViewMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}
/***************************************************
****************************************************/
void CAplFileAssociationsGrid::OnHitBottom(long numrows,long rowspast,long rowsfound){
}
/***************************************************
****************************************************/
void CAplFileAssociationsGrid::OnHitTop(long numrows,long rowspast){

}
/***************************************************
OnCanSizeCol
Sent when the user is over a separation line on
the top heading
A return value of TRUE allows the possibiliy of
a resize
****************************************************/
int CAplFileAssociationsGrid::OnCanSizeCol(int col){
//  if(col == -1)
//      return TRUE;
//  else
        return FALSE;
}
/***************************************************
OnColSizing
Sent when the user is sizing a column
The column that is being sized is given as
well as the width. Plus the width can be modified
at this point. This makes it easy to set min and
max widths
****************************************************/
void CAplFileAssociationsGrid::OnColSizing(int col,int *width){
}
/***************************************************
OnColSized
This is sent when the user finished sizing the
given column (see above for more details)
****************************************************/
void CAplFileAssociationsGrid::OnColSized(int col,int *width){
}
/***************************************************
OnCanSizeRow
Sent when the user is over a separation line on
the side heading
A return value of TRUE allows the possibiliy of
a resize
****************************************************/
int  CAplFileAssociationsGrid::OnCanSizeRow(long row){
    return FALSE;
}
/***************************************************
OnRowSizing
Sent when the user is sizing a row
The row that is being sized is given as
well as the height. Plus the height can be modified
at this point. This makes it easy to set min and
max heights
****************************************************/
void CAplFileAssociationsGrid::OnRowSizing(long row,int *height){
}
/***************************************************
OnRowSized
This is sent when the user is finished sizing hte
given row ( see above for more details)
****************************************************/
void CAplFileAssociationsGrid::OnRowSized(long row,int *height){
}
/***************************************************
OnCanSizeSideHdg
This is sent when the user moves into position
for sizing the width of the side heading
return TRUE to allow the sizing
or FALSE to not allow it
****************************************************/
int CAplFileAssociationsGrid::OnCanSizeSideHdg(){
    return FALSE;
}
/***************************************************
OnCanSizeTopHdg
This is sent when the user moves into position
for sizing the height of the top heading
return TRUE to allow the sizing
or FALSE to not allow it
****************************************************/
int CAplFileAssociationsGrid::OnCanSizeTopHdg(){
    return FALSE;
}
/***************************************************
OnSideHdgSizing
****************************************************/
int CAplFileAssociationsGrid::OnSideHdgSizing(int *width){
    return TRUE;

}
/***************************************************
OnSideHdgSized
****************************************************/
int CAplFileAssociationsGrid::OnSideHdgSized(int *width){
    return TRUE;

}
/***************************************************
OnTopHdgSized
****************************************************/
int CAplFileAssociationsGrid::OnTopHdgSized(int *height){
    return TRUE;

}
/***************************************************
OnTopHdgSizing
****************************************************/
int CAplFileAssociationsGrid::OnTopHdgSizing(int *height){
    return TRUE;

}
/***************************************************
OnColChange
Sent whenever the current column changes
The old and the new columns are given
****************************************************/
void CAplFileAssociationsGrid::OnColChange(int oldcol,int newcol){
}
/***************************************************
OnRowChange
Sent whenever the current row changes
The old and the new rows are given
****************************************************/
void CAplFileAssociationsGrid::OnRowChange(long oldrow,long newrow){
}
/***************************************************
OnCellChange
Sent whenever the current cell changes rows or
columns
****************************************************/
void CAplFileAssociationsGrid::OnCellChange(int oldcol,int newcol,long oldrow,long newrow){
}
/***************************************************
OnLeftColChange
Sent whenever the left visible column in the grid
changes
****************************************************/
void CAplFileAssociationsGrid::OnLeftColChange(int oldcol,int newcol){
}
/***************************************************
OnTopRowChange
Sent whenever the top visible row in the grid changes
****************************************************/
void CAplFileAssociationsGrid::OnTopRowChange(long oldrow,long newrow){
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
void CAplFileAssociationsGrid::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed){
    if(col ==0 && row != -1) {
        StartEdit();
    }

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
void CAplFileAssociationsGrid::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed){


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
void CAplFileAssociationsGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnMouseMove
****************************************************/
void CAplFileAssociationsGrid::OnMouseMove(int col,long row,POINT *point,UINT nFlags,BOOL processed){
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
void CAplFileAssociationsGrid::OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){

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
void CAplFileAssociationsGrid::OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnTH_LClicked
Sent whenever the user double clicks the left mouse
button within the top heading

  'col' is negative if the area clicked in is not valid
****************************************************/
void CAplFileAssociationsGrid::OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
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
void CAplFileAssociationsGrid::OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){


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
void CAplFileAssociationsGrid::OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnSH_DClicked
Sent whenever the user double clicks the left mouse
button within the side heading

  'row' is negative if the area clicked in is not valid
****************************************************/
void CAplFileAssociationsGrid::OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
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
void CAplFileAssociationsGrid::OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed){
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
void CAplFileAssociationsGrid::OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnCB_DClicked
Sent whenever the user double clicks the left mouse
button within the top corner button
****************************************************/
void CAplFileAssociationsGrid::OnCB_DClicked(RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnKeyDown
Sent whenever the user types when the grid has
focus. The keystroke can be modified here as well.
(See WM_KEYDOWN for more information)
****************************************************/
void CAplFileAssociationsGrid::OnKeyDown(UINT *vcKey,BOOL processed){
}
/***************************************************
OnCharDown
Sent whenever the user types when the grid has
focus. The keystroke can be modified here as well.
(See WM_CHAR for more information)
****************************************************/
void CAplFileAssociationsGrid::OnCharDown(UINT *vcKey,BOOL processed){
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
void CAplFileAssociationsGrid::OnGetCell(int col,long row,CUGCell *cell){

}
/***************************************************
OnSetCell
This message is sent everytime the a cell is about
to change.
****************************************************/
void CAplFileAssociationsGrid::OnSetCell(int col,long row,CUGCell *cell){
}
/***************************************************
OnDataSourceNotify
This message is sent from a data source , message
depends on the data source - check the information
on the data source(s) being used
- The ID of the Data source is also returned
****************************************************/
void CAplFileAssociationsGrid::OnDataSourceNotify(int ID,long msg,long param){
}
/***************************************************
OnCellTypeNotify
This message is sent from a cell type , message
depends on the cell type - check the information
on the cell type classes
- The ID of the cell type is given
****************************************************/
int CAplFileAssociationsGrid::OnCellTypeNotify(long ID,int col,long row,long msg,long param){
    if(col == 0) {
        if(msg == UGCT_ELLIPSISBUTTONCLICK){

            const FileAssociation& file_association = ((CAplFileAssociationsDlg*)GetParent())->m_fileAssociations[row];

            // selecting a folder
            if( file_association.IsFolderBased() )
            {
                CString csMessage;
                csMessage.Format(_T("Choose %s"), (LPCTSTR)file_association.GetDescription());

                std::optional<std::wstring> folder = SelectFolderDialog(GetSafeHwnd(), csMessage);

                if( folder.has_value() )
                    QuickSetText(col, row, folder->c_str());
            }

            // selecting a file
            else
            {
                CIMSAString sDatFileName = this->QuickGetText(col, row);
                CString filter;
                CString default_extension;

                std::tie(filter, default_extension) = file_association.GetFilterAndDefaultExtension();
                filter.Append(_T("All Files (*.*)|*.*||"));

                CFileDialog fileDlg(FALSE, default_extension, sDatFileName, OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_PATHMUSTEXIST, filter, NULL);
                fileDlg.m_ofn.lpstrTitle = _T("Enter or Select File");
                if(fileDlg.DoModal() == IDOK) {
                    CString sTemp = fileDlg.GetPathName();
                    file_association.FixExtension(sTemp);
                    QuickSetText(col,row,sTemp);
                }
            }
        }
    }
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
int CAplFileAssociationsGrid::OnEditStart(int col, long row,CWnd **edit){
    if(col == 0) {
        CUGCell cell;
        GetCell(col,row,&cell);
        if(cell.GetBackColor() ==RGB(192,192,192) )
            return FALSE;
        *edit =&m_pPifEdit;
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
int CAplFileAssociationsGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey){
    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CAplFileAssociationsGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag){
    CIMSAString sTemp = string;
    ((CAplFileAssociationsDlg*)GetParent())->m_fileAssociations[row].FixExtension(sTemp);
    QuickSetText(col,row,sTemp);
    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CAplFileAssociationsGrid::OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow){
    return TRUE;
}
/***************************************************
sections - UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
****************************************************/
void CAplFileAssociationsGrid::OnMenuCommand(int col,long row,int section,int item){
}
/***************************************************
return UG_SUCCESS to allow the menu to appear
return 1 to not allow the menu to appear
****************************************************/
int CAplFileAssociationsGrid::OnMenuStart(int col,long row,int section){
    return TRUE;
}
/***************************************************
OnHint
****************************************************/
int CAplFileAssociationsGrid::OnHint(int col,long row,int section,CString *string){
    string->Format(_T("Col:%d Row:%ld"),col,row);
    return TRUE;
}
/***************************************************
OnVScrollHint
****************************************************/
int CAplFileAssociationsGrid::OnVScrollHint(long row,CString *string){
    return TRUE;
}
/***************************************************
OnHScrollHint
****************************************************/
int CAplFileAssociationsGrid::OnHScrollHint(int col,CString *string){
    return TRUE;
}


void CAplFileAssociationsGrid::OnScreenDCSetup(CDC *dc,int section){
}
/***************************************************
OnSortEvaluate
return      -1  <
0   ==
1   >
****************************************************/

/***************************************************
OnAdjustComponentSizes
****************************************************/
void CAplFileAssociationsGrid::OnAdjustComponentSizes(RECT *grid,RECT *topHdg,RECT *sideHdg,
                                      RECT *cnrBtn,RECT *vScroll,RECT *hScroll,RECT *tabs){
}

/***************************************************
OnDrawFocusRect
****************************************************/
void CAplFileAssociationsGrid::OnDrawFocusRect(CDC *dc,RECT *rect){

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
OnSetFocus
****************************************************/
void CAplFileAssociationsGrid::OnSetFocus(int section){
}

/***************************************************
OnKillFocus
****************************************************/
void CAplFileAssociationsGrid::OnKillFocus(int section){
}
/***************************************************
OnColSwapStart
****************************************************/
BOOL CAplFileAssociationsGrid::OnColSwapStart(int col){

    return TRUE;
}

/***************************************************
OnCanColSwap
****************************************************/
BOOL CAplFileAssociationsGrid::OnCanColSwap(int fromCol,int toCol){

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// reset the grid
void CAplFileAssociationsGrid::ResetGrid() {

    int iNumberofCols = GetNumberCols() ;
    SetRedraw(FALSE);

    for(int iIndex = 0; iIndex < iNumberofCols ; iIndex++)
    {
        DeleteCol(0);
    }

    int iNumberofRows  = GetNumberRows() ;
    for( int iIndex = 0; iIndex <iNumberofRows; iIndex++)
    {
        DeleteRow(0);
    }
    SetRedraw(TRUE);
}

