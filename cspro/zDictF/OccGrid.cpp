// OccGrid.cpp: implementation of the COccGrid class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "OccGrid.h"

const int GRID_DEFAULT_COLWIDTH = 30;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


BEGIN_MESSAGE_MAP(COccGrid,CUGCtrl)
    //{{AFX_MSG_MAP(COccGrid)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

COccGrid::COccGrid()
{
    m_bCanEdit = false;
    m_pDoc = NULL;
}


/***************************************************
OnSetup
This function is called just after the grid window
is created or attached to a dialog item.
It can be used to initially setup the grid
****************************************************/
void COccGrid::OnSetup()
{
    EnableExcelBorders(TRUE);

    SetMultiSelectMode(TRUE);
    SetHighlightRow(TRUE);
    m_LabelEdit.m_hWnd = NULL;

//  m_iEllipsisIndex =AddCellType(&m_Ellipsis);
/*      m_LabelEdit.SetRowCol(0, 0);
        GetCellRect(REL_NAME, row, &rect);
        m_LabelEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 101);
        //m_LabelEdit.SetFont(&m_font);
        GetCell(0, row, &cell);
        CString cs = "";
        cell.GetText(&cs);
        m_LabelEdit.SetWindowText(cs);
*/

    SetNumberCols(m_iCols);
    SetNumberRows(m_iRows);
    SetColWidth(-1, GRID_DEFAULT_COLWIDTH);
    CRect wrect;
    GetWindowRect(&wrect);
    for (int iCol =0 ; iCol < m_iCols ; iCol++){
        SetColWidth(iCol, wrect.Width() - GRID_DEFAULT_COLWIDTH - 3);
    }
    CUGCell cell, cell1;
    if (m_Labels.GetSize() ==0 )
    {
        m_Labels.SetSize(m_iRows);
    }
    for (int row = 0; row < m_iRows; row++)
    {
        QuickSetText(0,row,m_Labels[row]);
        QuickSetText(-1,row,IntToString(row+1));
        QuickSetAlignment (-1,row, UG_ALIGNBOTTOM | UG_ALIGNCENTER);
    }

    m_font.CreateFont(15,0,0,0,400,0,0,0,0,0,0,0,0,_T("MS Shell Dlg")); // 20111219 for unicode
    SetDefFont(&m_font);

    QuickSetFont(-1,-1,&m_font);

    QuickSetText(-1,-1,_T("Occ"));
    QuickSetText(0,-1,_T("Label"));

    QuickSetAlignment (-1,-1, UG_ALIGNBOTTOM | UG_ALIGNLEFT);
    QuickSetAlignment (0,-1, UG_ALIGNBOTTOM | UG_ALIGNLEFT);

    AdjustComponentSizes();


}

/***************************************************
OnSheetSetup
****************************************************/
void COccGrid::OnSheetSetup(int /*sheetNumber*/){
    SetMultiSelectMode(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::OnRowChange
//
/////////////////////////////////////////////////////////////////////////////

void COccGrid::OnRowChange(long /*oldrow*/, long /*newrow*/){
    m_bCanEdit = false;
}

void COccGrid::OnLClicked(int col,long row,int updn,RECT */*rect*/,POINT */*point*/,int /*processed*/){
    if (updn) {
        int r = GetCurrentRow();
        if (row != r) {
            m_bCanEdit = false;
        }
    }
    else {
        int r  = GetCurrentRow();
        if (row < 0 || col < 0) {
                m_bCanEdit = false;
                if(GetEditClass() != NULL && GetEditClass()->IsWindowVisible()){//if the edit window is visible and not null
                    //set the window text to the cell and hide the window
                    CIMSAString sText;
                    GetEditClass()->GetWindowText(sText);
                    OnEditFinish(GetCurrentCol(), GetCurrentRow(),GetEditClass(),sText,FALSE);
                }
        }
        else if ( r != row){
            m_bCanEdit = false;
        }
        else if(m_bCanEdit){
            StartEdit(col,row,0);
        }
        else{
            if(GetEditClass() != NULL && GetEditClass()->IsWindowVisible()){//if the edit window is visible and not null
                //set the window text to the cell and hide the window
                CIMSAString sText;
                GetEditClass()->GetWindowText(sText);
                OnEditFinish(GetCurrentCol(), GetCurrentRow(),GetEditClass(),sText,FALSE);
            }//end the current edit??
            m_bCanEdit = true;
        }
    }
}

void COccGrid::OnRClicked(int /*col*/,long row,int updn,RECT */*rect*/,POINT* point,int /*processed*/)
{
     if (updn) {
       if(GetEditClass() != NULL && GetEditClass()->IsWindowVisible()){//if is editing
        return;
       }
    }


    //TODO: enable if copy if any rows are selected ? Enable paste if valid fmt in clipboard
    BCMenu popMenu;    // BMD 29 Sep 2003
    popMenu.CreatePopupMenu();
    if (GetNumberRows() == 0) {
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_CUT, _T("Cu&t\tCtrl+X"));
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_COPY, _T("&Copy\tCtrl+C"));
    }
    else {
        popMenu.AppendMenu(MF_STRING, ID_EDIT_CUT, _T("Cu&t\tCtrl+X"));
        popMenu.AppendMenu(MF_STRING, ID_EDIT_COPY, _T("&Copy\tCtrl+C"));
    }
    if (IsClipboardFormatAvailable((IsClipboardFormatAvailable(_tCF_TEXT) && IsClipValidForPaste()))){
        popMenu.AppendMenu(MF_STRING, ID_EDIT_PASTE, _T("&Paste\tCtrl+V"));
    }
    else {
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_PASTE, _T("&Paste\tCtrl+V"));
    }
    popMenu.LoadToolbar(IDR_DICT_FRAME);   // BMD 29 Sep 2003

    CRect rectWin;
    GetWindowRect(rectWin);
    if (point->x == 0) {
        CRect rectCell;
        GetCellRect(1, GetCurrentRow(), &rectCell);
        point->x = (rectCell.left + rectCell.right) / 2;
        point->y = (rectCell.top + rectCell.bottom) / 2;
    }
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, rectWin.left + point->x, rectWin.top + point->y + GetRowHeight(row), this);
}

/***************************************************
OnCharDown
Sent whenever the user types when the grid has
focus. The keystroke can be modified here as well.
(See WM_CHAR for more information)
****************************************************/
void COccGrid::OnCharDown(UINT* vcKey,BOOL processed){
    if(GetKeyState(VK_CONTROL)<0){
        switch(*vcKey){
        case 1: //Ctrl+A
            //Select All
            SelectRange(0,0,0,GetNumberRows());
            processed = true;
            break;
        case 3: //Ctrl+C
            //Copy selected
            OnEditCopy();
            processed = true;
            break;
        case 24://Ctrl+X
            OnEditCut();
            processed = true;
            break;
        case 22://Ctrl+V
            OnEditPaste();
            processed = true;
            break;
        default:
            break;
        }
    }
    if(!processed)
        StartEdit(*vcKey);
}

/***************************************************
OnCellTypeNotify
This message is sent from a cell type , message
depends on the cell type - check the information
on the cell type classes
- The ID of the cell type is given
****************************************************/
int COccGrid::OnCellTypeNotify(long /*ID*/,int /*col*/,long /*row*/,long /*msg*/,long /*param*/){
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
int COccGrid::OnEditStart(int col, long row,CWnd **edit){
    if(col == 0) {
        CUGCell cell;
        GetCell(col,row,&cell);
        if(cell.GetBackColor() ==RGB(192,192,192) )
            return FALSE;

        RECT rect = {0,0,0,0};
        if (m_LabelEdit.m_hWnd != NULL)
        {
            m_LabelEdit.DestroyWindow();
            m_LabelEdit.m_hWnd = NULL;
        }
//      m_LabelEdit.SetRowCol(row,col);
        GetCellRect(col, row, &rect);
        m_LabelEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 101);
        //m_LabelEdit.SetFont(&m_font);
        GetCell(col, row, &cell);
        CString cs = _T("");
        cell.GetText(&cs);
        m_LabelEdit.SetWindowText(cs);

        *edit =(CWnd*)&m_LabelEdit;
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
int COccGrid::OnEditVerify(int /*col*/, long /*row*/,CWnd */*edit*/,UINT */*vcKey*/){
    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int COccGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL /*cancelFlag*/) {
    CString cs = string;
    if (cs.SpanIncluding(_T(" ")).GetLength() == cs.GetLength()) {
        cs = _T("");
    }
    QuickSetText(col,row,cs);
    if (col == 0) {
        m_Labels[row] = cs;
    }
    edit->ShowWindow(SW_HIDE);
    m_bCanEdit  = false;
    return TRUE;
}
/***************************************************
OnEditFinish this is send when editing is finished
****************************************************/
int COccGrid::OnEditContinue(int /*oldcol*/,long /*oldrow*/,int* /*newcol*/,long* /*newrow*/){
    return TRUE;
}

/***************************************************
OnHint
****************************************************/
int COccGrid::OnHint(int col,long row,int /*section*/,CString *string){
    string->Format(_T("Col:%d Row:%ld"),col,row);
    return TRUE;
}

/***************************************************
OnVScrollHint
****************************************************/
int COccGrid::OnVScrollHint(long /*row*/,CString */*string*/){
    return TRUE;
}
/***************************************************
OnHScrollHint
****************************************************/
int COccGrid::OnHScrollHint(int /*col*/,CString */*string*/){
    return TRUE;
}

/***************************************************
OnDrawFocusRect
****************************************************/
void COccGrid::OnDrawFocusRect(CDC *dc,RECT* rect){

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

/////////////////////////////////////////////////////////////////////////////
// reset the grid
void COccGrid::ResetGrid() {

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


void COccGrid::OnEditCopy()
{
    EditCopy(false);
}

void COccGrid::OnEditCut()
{
    EditCopy(true);
}

void COccGrid::OnEditPaste()
{
    //Get the current position
    long row = GetCurrentRow();

    if( row < 0 || !IsClipboardFormatAvailable(_tCF_TEXT) )
        return;

    SO::ForeachLine(WinClipboard::GetText(), false,
        [&](CString line)
        {
            if( row < GetNumberRows() )
            {
                QuickSetText(0, row, line);
                m_Labels.SetAt(row, line);
                ++row;
                return true;
            }

            return false;
        });

    //Update Grid
    Update();
    ClearSelections();
}


bool COccGrid::IsClipValidForPaste() const
{
    bool valid_line_found = false;

    SO::ForeachLine(WinClipboard::GetText(), false,
        [&](wstring_view /*line*/)
        {
            valid_line_found = true;
            return false;
        });

    return valid_line_found;
}


void COccGrid::EditCopy(bool bCut)
{
    std::wstring labels;

    ForeachSelectedRow(
        [&](long row)
        {
            labels.append(QuickGetText(0, row));
            labels.append(_T("\n"));

            if( bCut )
            {
                QuickSetText(0, row,_T(""));
                m_Labels.SetAt(row, _T(""));
            }

            return true;
        });

    if( labels.empty() )
        return;

    // trim the last newline before copying to the clipboard
    WinClipboard::PutText(this, SO::TrimRight(labels));

    if(bCut){
        // Update grid
        Update();
        ClearSelections();
    }
}
