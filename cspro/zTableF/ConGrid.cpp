// ConGrid.cpp : implementation file
//
#include "StdAfx.h"
#include "ConGrid.h"
#include <math.h>

// CConSpecGrid

IMPLEMENT_DYNAMIC(CConSpecGrid, CUGCtrl)

CConSpecGrid::CConSpecGrid()
{
}

CConSpecGrid::~CConSpecGrid()
{
}

/***************************************************
OnSetup
This function is called just after the grid window
is created or attached to a dialog item.
It can be used to initially setup the grid
****************************************************/
void CConSpecGrid::OnSetup(){

    EnableExcelBorders(TRUE);
    VScrollAlwaysPresent(TRUE);
    HScrollAlwaysPresent(TRUE);
    SetMultiSelectMode(TRUE);

    // Set number of rows and columns
    m_iCols = m_paLevels->GetSize() + 1;
    m_iRows = m_paConSpecs->GetSize();
    SetNumberCols(m_iCols, FALSE);
    SetNumberRows(m_iRows, FALSE);
    LockColumns(1);

    // Set font
    m_font.CreateFont(15,0,0,0,400,0,0,0,0,0,0,0,0,_T("Microsoft Sans Serif"));
    SetDefFont(&m_font);
    QuickSetFont(-1,-1,&m_font);

    // Set column headings
    SetSH_Width(0);
    SetColWidth(0,120);
    QuickSetText(0,-1,_T("Area Level Name"));
    for (int i = 0 ; i < m_paLevels->GetSize() ; i++) {
        SetColWidth(i + 1,120);
        QuickSetText(i + 1,-1,m_paLevels->GetAt(i));
    }
    for (int j = 0 ; j < m_paConSpecs->GetSize() ; j++) {
        CConSpec* pConSpec = m_paConSpecs->GetAt(j);
        QuickSetText(0, j, pConSpec->GetAreaLevel());
        for (int k = 0 ; k < pConSpec->GetNumActions() ; k++) {
            CONITEM cItem = pConSpec->GetAction(k);
            QuickSetText(k + 1, j, FormatConSpec(cItem));
        }
    }
    AdjustComponentSizes();
}

/***************************************************
OnSheetSetup
****************************************************/
void CConSpecGrid::OnSheetSetup(int sheetNumber){
    SetMultiSelectMode(TRUE);
}

/***************************************************
OnCanMove
Sent when the current cell in the grid is about
to move
A return of TRUE allows the move, a return of
FALSE stops the move
****************************************************/
int CConSpecGrid::OnCanMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}
/***************************************************
OnCanMove
Sent when the top row or left column in the grid is about
to move
A return of TRUE allows the move, a return of
FALSE stops the move
****************************************************/
int CConSpecGrid::OnCanViewMove(int oldcol,long oldrow,int newcol,long newrow){
    return TRUE;
}
/***************************************************
****************************************************/
void CConSpecGrid::OnHitBottom(long numrows,long rowspast,long rowsfound){
}
/***************************************************
****************************************************/
void CConSpecGrid::OnHitTop(long numrows,long rowspast){

}
/***************************************************
OnCanSizeCol
Sent when the user is over a separation line on
the top heading
A return value of TRUE allows the possibiliy of
a resize
****************************************************/
int CConSpecGrid::OnCanSizeCol(int col){
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
void CConSpecGrid::OnColSizing(int col,int *width){
}
/***************************************************
OnColSized
This is sent when the user finished sizing the
given column (see above for more details)
****************************************************/
void CConSpecGrid::OnColSized(int col,int *width){
}
/***************************************************
OnCanSizeRow
Sent when the user is over a separation line on
the side heading
A return value of TRUE allows the possibiliy of
a resize
****************************************************/
int  CConSpecGrid::OnCanSizeRow(long row){
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
void CConSpecGrid::OnRowSizing(long row,int *height){
}
/***************************************************
OnRowSized
This is sent when the user is finished sizing hte
given row ( see above for more details)
****************************************************/
void CConSpecGrid::OnRowSized(long row,int *height){
}
/***************************************************
OnCanSizeSideHdg
This is sent when the user moves into position
for sizing the width of the side heading
return TRUE to allow the sizing
or FALSE to not allow it
****************************************************/
int CConSpecGrid::OnCanSizeSideHdg(){
    return FALSE;
}
/***************************************************
OnCanSizeTopHdg
This is sent when the user moves into position
for sizing the height of the top heading
return TRUE to allow the sizing
or FALSE to not allow it
****************************************************/
int CConSpecGrid::OnCanSizeTopHdg(){
    return FALSE;
}
/***************************************************
OnSideHdgSizing
****************************************************/
int CConSpecGrid::OnSideHdgSizing(int *width){
    return TRUE;

}
/***************************************************
OnSideHdgSized
****************************************************/
int CConSpecGrid::OnSideHdgSized(int *width){
    return TRUE;

}
/***************************************************
OnTopHdgSized
****************************************************/
int CConSpecGrid::OnTopHdgSized(int *height){
    return TRUE;

}
/***************************************************
OnTopHdgSizing
****************************************************/
int CConSpecGrid::OnTopHdgSizing(int *height){
    return TRUE;

}
/***************************************************
OnColChange
Sent whenever the current column changes
The old and the new columns are given
****************************************************/
void CConSpecGrid::OnColChange(int oldcol,int newcol){
}
/***************************************************
OnRowChange
Sent whenever the current row changes
The old and the new rows are given
****************************************************/
void CConSpecGrid::OnRowChange(long oldrow,long newrow){
}
/***************************************************
OnCellChange
Sent whenever the current cell changes rows or
columns
****************************************************/
void CConSpecGrid::OnCellChange(int oldcol,int newcol,long oldrow,long newrow){

//  SetFocus();

}
/***************************************************
OnLeftColChange
Sent whenever the left visible column in the grid
changes
****************************************************/
void CConSpecGrid::OnLeftColChange(int oldcol,int newcol){
}
/***************************************************
OnTopRowChange
Sent whenever the top visible row in the grid changes
****************************************************/
void CConSpecGrid::OnTopRowChange(long oldrow,long newrow){
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
void CConSpecGrid::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed){

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
void CConSpecGrid::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed){


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
void CConSpecGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){

    if(!processed) {
        StartEdit();
    }
}
/***************************************************
OnMouseMove
****************************************************/
void CConSpecGrid::OnMouseMove(int col,long row,POINT *point,UINT nFlags,BOOL processed){
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
void CConSpecGrid::OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){

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
void CConSpecGrid::OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnTH_LClicked
Sent whenever the user double clicks the left mouse
button within the top heading

  'col' is negative if the area clicked in is not valid
****************************************************/
void CConSpecGrid::OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
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
void CConSpecGrid::OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){


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
void CConSpecGrid::OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnSH_DClicked
Sent whenever the user double clicks the left mouse
button within the side heading

  'row' is negative if the area clicked in is not valid
****************************************************/
void CConSpecGrid::OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
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
void CConSpecGrid::OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed){
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
void CConSpecGrid::OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnCB_DClicked
Sent whenever the user double clicks the left mouse
button within the top corner button
****************************************************/
void CConSpecGrid::OnCB_DClicked(RECT *rect,POINT *point,BOOL processed){
}
/***************************************************
OnKeyDown
Sent whenever the user types when the grid has
focus. The keystroke can be modified here as well.
(See WM_KEYDOWN for more information)
****************************************************/
void CConSpecGrid::OnKeyDown(UINT *vcKey,BOOL processed){
}
/***************************************************
OnCharDown
Sent whenever the user types when the grid has
focus. The keystroke can be modified here as well.
(See WM_CHAR for more information)
****************************************************/
void CConSpecGrid::OnCharDown(UINT *vcKey,BOOL processed){

    if(!processed) {
        if (*vcKey != 13) {
            StartEdit(*vcKey);
        }
        else {
            if (GetCurrentCol() + 1 < GetNumberCols()) {
                GotoCell(GetCurrentCol()+ 1,GetCurrentRow());
            }
        }
    }
}

/***************************************************
OnGetCell
This message is sent everytime the grid needs to
draw a cell in the grid. At this point the cell
class has been filled with the information to be
used to draw the cell. The information can now be
changed before it is used for drawing
****************************************************/
void CConSpecGrid::OnGetCell(int col,long row,CUGCell *cell){

}
/***************************************************
OnSetCell
This message is sent everytime the a cell is about
to change.
****************************************************/
void CConSpecGrid::OnSetCell(int col,long row,CUGCell *cell){
}
/***************************************************
OnDataSourceNotify
This message is sent from a data source , message
depends on the data source - check the information
on the data source(s) being used
- The ID of the Data source is also returned
****************************************************/
void CConSpecGrid::OnDataSourceNotify(int ID,long msg,long param){
}
/***************************************************
OnCellTypeNotify
This message is sent from a cell type , message
depends on the cell type - check the information
on the cell type classes
- The ID of the cell type is given
****************************************************/
int CConSpecGrid::OnCellTypeNotify(long ID,int col,long row,long msg,long param){
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
int CConSpecGrid::OnEditStart(int col, long row,CWnd **edit){

    return TRUE;
}
/***************************************************
OnEditVerify
This is send when the editing is about to end
****************************************************/
int CConSpecGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey){

    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CConSpecGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag){

    if (m_bClosing) {
        return TRUE;
    }
    CIMSAString sString = string;
    sString.Trim();

    if (col == 0) {
        sString.MakeUpper();
        QuickSetText(col,row,sString);
        _tcsupr((TCHAR*) string);
        if (sString.IsEmpty()) {
            return TRUE;
        }
        if (!sString.IsName()) {
            sString += _T(" is not a CSPro name.");
            AfxMessageBox(sString);
            StartEdit(col,row,0);
            return FALSE;
        }
        for (int i = 0 ; i < GetNumberRows() ; i++) {
            CUGCell cell;
            GetCell(0, i, &cell);
            CIMSAString sName;
            cell.GetText(&sName);
            if (sString.CompareNoCase(sName) == 0 && i != row) {
                AfxMessageBox(_T("Duplicate level name"));
                StartEdit(col,row,0);
                return FALSE;
            }
        }
    }
    else {
        CONITEM cItem;
        cItem.level = col - 1;
        if (ParseConSpec(sString, cItem)) {
            QuickSetText(col,row,string);
            StartEdit(col,row,0);
            return FALSE;
        }
        else {
            QuickSetText(col,row,FormatConSpec(cItem));
            _tcscpy((TCHAR*) string,FormatConSpec(cItem));
        }
    }
    return TRUE;

}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int CConSpecGrid::OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow){

    GotoCell(*newcol,*newrow);
    return FALSE;
}
/***************************************************
sections - UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
****************************************************/
void CConSpecGrid::OnMenuCommand(int col,long row,int section,int item){
}
/***************************************************
return UG_SUCCESS to allow the menu to appear
return 1 to not allow the menu to appear
****************************************************/
int CConSpecGrid::OnMenuStart(int col,long row,int section){
    return TRUE;
}
/***************************************************
OnHint
****************************************************/
int CConSpecGrid::OnHint(int col,long row,int section,CString *string){

    string->Format(_T("Col:%d Row:%ld"),col,row);
    return TRUE;
}
/***************************************************
OnVScrollHint
****************************************************/
int CConSpecGrid::OnVScrollHint(long row,CString *string){
    return TRUE;
}
/***************************************************
OnHScrollHint
****************************************************/
int CConSpecGrid::OnHScrollHint(int col,CString *string){
    return TRUE;
}


void CConSpecGrid::OnScreenDCSetup(CDC *dc,int section){
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
void CConSpecGrid::OnAdjustComponentSizes(RECT *grid,RECT *topHdg,RECT *sideHdg,
                                      RECT *cnrBtn,RECT *vScroll,RECT *hScroll,RECT *tabs){
}

/***************************************************
OnDrawFocusRect
****************************************************/
void CConSpecGrid::OnDrawFocusRect(CDC *dc,RECT *rect){

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
void CConSpecGrid::OnSetFocus(int section){
}

/***************************************************
OnKillFocus
****************************************************/
void CConSpecGrid::OnKillFocus(int section){
}
/***************************************************
OnColSwapStart
****************************************************/
BOOL CConSpecGrid::OnColSwapStart(int col){

    return TRUE;
}

/***************************************************
OnCanColSwap
****************************************************/
BOOL CConSpecGrid::OnCanColSwap(int fromCol,int toCol){

    return TRUE;
}


BEGIN_MESSAGE_MAP(CConSpecGrid, CUGCtrl)
END_MESSAGE_MAP()



// CConSpecGrid message handlers


bool CConSpecGrid::ParseConSpec(CIMSAString sValues, CONITEM& cItem)
{
    bool bRet = false;

    cItem.lower = CON_NONE;
    cItem.upper = CON_NONE;
    cItem.replace = CON_NONE;
    if (sValues.IsEmpty()) {
        cItem.level = CON_NONE;
        return bRet;
    }
    if (sValues.CompareNoCase(CON_EACH) == 0) {
        return bRet;
    }
    // Get size of item
    CUGCell cell;
    GetCell(cItem.level + 1, -1, &cell);
    CIMSAString sName;
    cell.GetText(&sName);

    const CDictItem* pItem = m_pCurrDict->LookupName<CDictItem>(sName);
    int len = pItem->GetLen();

    // Get Lower
    TCHAR cFound = _T(' ');
    CIMSAString sError;
    CIMSAString sValue = sValues.GetToken(_T(":="), &cFound);
    if (sValue.GetLength() > len) {
        sError.Format(_T("Lower value \"%s\" is longer than the item length, %d."), (LPCTSTR)sValue, len);
        AfxMessageBox(sError);
        return true;
    }
    if (!sValue.IsNumeric()) {
        sError.Format(_T("Lower value \"%s\" is not numeric"), (LPCTSTR)sValue);
        AfxMessageBox(sError);
        return true;
    }
    cItem.lower = (int) floor(sValue.fVal());
    if (cFound == ':') {
        // Get Upper
        sValue = sValues.GetToken(_T("="));
        if (sValue.GetLength() > len) {
            sError.Format(_T("Upper value \"%s\" is longer than the item length, %d."), (LPCTSTR)sValue, len);
            AfxMessageBox(sError);
            return true;
        }
        if (!sValue.IsNumeric()) {
            sError.Format(_T("Upper value \"%s\" is not numeric"), (LPCTSTR)sValue);
            AfxMessageBox(sError);
            return true;
        }
        int iVal = (int) floor(sValue.fVal());
        if (iVal < cItem.lower) {
            sError.Format(_T("Upper value \"%d\" < lower value \"%d\", Upper ignored"), iVal, cItem.lower);
            AfxMessageBox(sError);
            cItem.upper = cItem.lower;
            return true;
        }
        else {
            cItem.upper = iVal;
        }
    }
    else {
        cItem.upper = cItem.lower;
    }
    // Get Replace
    if (!sValues.IsEmpty()) {
        if (sValues.GetLength() > len) {
            sError.Format(_T("Replacement value \"%s\" is longer than the item length, %d."), (LPCTSTR)sValues, len);
            AfxMessageBox(sError);
            return true;
        }
        if (!sValues.IsNumeric()) {
            sError.Format(_T("Replacement value \"%s\" is not numeric"), (LPCTSTR)sValues);
            AfxMessageBox(sError);
            return true;
        }
        cItem.replace = (int) floor(sValues.fVal());
    }
    return bRet;
}

CIMSAString CConSpecGrid::FormatConSpec(CONITEM cItem)
{
    if (cItem.level == CON_NONE) {
        return _T("");
    }
    if (cItem.lower == CON_NONE && cItem.replace == CON_NONE) {
        return CON_EACH;
    }
    CIMSAString sSpec = _T("");
    CIMSAString sTemp = _T("");
    if (cItem.lower != CON_NONE) {
        sTemp.Str(cItem.lower);
        sSpec = sTemp;
        if (cItem.upper != cItem.lower) {
            sTemp.Str(cItem.upper);
            sSpec += _T(":") + sTemp;
        }
    }
    if (cItem.replace != CON_NONE) {
        sTemp.Str(cItem.replace);
        sSpec += _T("=") + sTemp;
    }
    return sSpec;
}
