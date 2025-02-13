/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGCellType
    Purpose
        standard text cell type class. This class is
        the default cell type for all cells in the grid
        and is the most common one to use. Also this
        class is the base class for any other cell type
        that the grid uses.
    Instructions
        -this cell types ID number never changes so there
        is a define for it's ID number:
            UGCT_NORMAL
        -There are several extended styles for this type
            UGCT_NORMALMULTILINE
                - this style is used when word wrapping
                and/or multiple lines are needed within
                one cell
            UGCT_NORMALELLIPSIS
                - this style shows three periods at the
                right of the cell if the text is too wide.
            UGCT_NORMALLABELTEXT
                - this style causes the cells label property
                to be drawn instead of its text property
************************************************/

#include "StdAfx.h"
#include "Ugctrl.h"

#include <zToolsO/Tools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/***************************************************
Constructor
    Setup the default properties for the cell type
    Cell types contain several member variables
    that modify how the grid works with the cell type
    NOTE: New cell types must be registered with the
    grid before use.
****************************************************/
CUGCellType::CUGCellType(){

    m_canTextEdit = TRUE;
    m_drawLabelText = FALSE;
    m_canOverLap = TRUE;
}
/***************************************************
Destructor
    Perform clean-up
****************************************************/
CUGCellType::~CUGCellType(){
}
/***************************************************
****************************************************/
int CUGCellType::DrawTextOrLabel(int mode){
    m_drawLabelText = mode;
    return UG_SUCCESS;
}
/***************************************************
SetID
    Purpose
        This function is called by the grid framework
        and assigns an ID number to be used by this
        cell type for the lifetime of 'this' instance
        of the grid. The ID number set by the grid
        for additional cell types depends on the order
        that the cells type were added.
    Params
        ID - ID number for the cell type to use
    Return
        none
****************************************************/
void CUGCellType::SetID(int ID){
    m_ID =ID;
}
/***************************************************
GetID
    Purpose
        Returns the ID number of this cell type. This
        number will not change during the lifetime of
        the grid's instance, but it may be different
        the next time the grid is created.
    Params
        none
    Return
        ID number of the cell
****************************************************/
int CUGCellType::GetID(){
    return m_ID;
}
/***************************************************
GetName
    Purpose
        Returns a readaable name for the cell type.
        This value is used to help end-users see what
        cell type are available.
    Params
        none
    Return
        cell type name
****************************************************/
LPCTSTR CUGCellType::GetName(){
    return _T("Normal Cell Type (default)");
}

/***************************************************
GetUGID
    Prupose
        Returns a GUID for the cell type, this number
        is unique for each cell type and never changes
        (unlike the standard celltype ID number).
        This number can be used to find the cell types
        standard ID number.
    Params
        UGID (which is actually a GUID)
****************************************************/
LPCUGID CUGCellType::GetUGID(){

    static const UGID ugid = {0x78a705c1, 0xf3e9,
                            0x11d0, 0x9c, 0x7b,
                            0x0, 0x80, 0xc8, 0x3f,
                            0x71, 0x2f};

    return &ugid;
}

/***************************************************
GetEditArea
    Purpose
        Returns the editable area of a cell type using
        a given rectangle which represents the whole
        celltype region.
        Droplists for example do not use its whole area
        for editing, as the droplist button on the right
        is not covered by the edit control. Therefor a
        droplist will modify the righthand side of the
        given rect.
    Params
        rect - rectangle to modify to give back the edit
            area
    Return
        UG_SUCCESS - success
        UG_ERROR - error
****************************************************/
int CUGCellType::GetEditArea(RECT *rect){
    return UG_SUCCESS;
}
/***************************************************
CenTextEdit
    Purpose
        Returns TRUE if the cell type allows standard
        text editing, or returns FALSE if it does not.
        The member variable m_canTextEdit is used for
        the return value.
    Params
        none
    Return
        TRUE - if the cell type allows editing
        FALSE - if the cell type does not allow editing
****************************************************/
BOOL CUGCellType::CanTextEdit(){
    return m_canTextEdit;
}
/***************************************************
CanOverLap
    Purpose
        returns TRUE if this cell type can overlap
        other cells to the right if its text is too
        wide. Generally plain cell types allow overlapping

    Params
        none
    Return
        TRUE - if overlapping may occur
        FALSE - if overlapping is not allowed

****************************************************/
BOOL CUGCellType::CanOverLap(CUGCell *cell){
    if(cell->GetCellTypeEx() == 0 && cell->IsPropertySet(UGCELL_JOIN_SET) == FALSE)
        return  m_canOverLap;
    return FALSE;
}
/***************************************************
SetTextEditMode
    Purpose
        Used to change the cell types default editing
        mode. Text editing for a cell type can be on
        or off.
    Params
        mode - TRUE(on) or FALSE(off)
    Return
        UG_SUCCESS - success
        UG_ERROR - error
****************************************************/
int CUGCellType::SetTextEditMode(int mode){

    if(mode == FALSE)
        m_canTextEdit = FALSE;
    else
        m_canTextEdit = TRUE;

    return UG_SUCCESS;
}

/***************************************************
OnSystemChange
    Purpose
        This function is called for each cell type
        when system settings change, such as screen
        resolution and or colors.
    Params
        none
    Return
        UG_SUCCESS - success
        UG_ERROR - error
****************************************************/
int CUGCellType::OnSystemChange(){
    return UG_SUCCESS;
}
/***************************************************
OnLClicked
    Purpose
        This function is called when the left mouse
        button is clicked over a cell using this cell
        type. If this function does something with this
        event then return TRUE othrwise return FALSE.
    Params
        col - column that was clicked in
        row - row that was clicked in
        updn - TRUE if the mouse button just went down
             - FALSE if the mouse button just went up
        rect - rectangle of the cell that was clicked in
        point - point where the mouse was clicked
    Return
        TRUE - if the event was processed
        FALSE - if the event was not
****************************************************/
BOOL CUGCellType::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point){
    return FALSE;
}
/***************************************************
OnRClicked
    Purpose
        This function is called when the right mouse
        button is clicked over a cell using this cell
        type. If this function does something with this
        event then return TRUE othrwise return FALSE.
    Params
        col - column that was clicked in
        row - row that was clicked in
        updn - TRUE if the mouse button just went down
             - FALSE if the mouse button just went up
        rect - rectangle of the cell that was clicked in
        point - point where the mouse was clicked
    Return
        TRUE - if the event was processed
        FALSE - if the event was not
****************************************************/
BOOL CUGCellType::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point){
    return FALSE;
}
/***************************************************
OnDClicked
    Purpose
        This function is called when the left mouse
        button is double clicked over a cell using this cell
        type. If this function does something with this
        event then return TRUE othrwise return FALSE.
    Params
        col - column that was clicked in
        row - row that was clicked in
        rect - rectangle of the cell that was clicked in
        point - point where the mouse was clicked
    Return
        TRUE - if the event was processed
        FALSE - if the event was not
****************************************************/
BOOL CUGCellType::OnDClicked(int col,long row,RECT *rect,POINT *point){
    return FALSE;
}
/***************************************************
OnKeyDown
    Purpose
        This function is called when a cell of this type
        has focus and a key is pressed.(See WM_KEYDOWN)
        If this function does something with this
        event then return TRUE othrwise return FALSE.
    Params
        col - column that has focus
        row - row that has focus
        vcKey - pointer to the virtual key code, of the
            key that was pressed
    Return
        TRUE - if the event was processed
        FALSE - if the event was not
****************************************************/
BOOL CUGCellType::OnKeyDown(int col,long row,UINT *vcKey){
    return FALSE;
}
/***************************************************
OnCharDown
    Purpose
        This function is called when a cell of this type
        has focus and a printable key is pressed.(WM_CHARDOWN)
        If this function does something with this
        event then return TRUE othrwise return FALSE.
    Params
        col - column that has focus
        row - row that has focus
        vcKey - pointer to the virtual key code, of the
            key that was pressed
    Return
        TRUE - if the event was processed
        FALSE - if the event was not
****************************************************/
BOOL CUGCellType::OnCharDown(int col,long row,UINT *vcKey){
    return FALSE;
}
/***************************************************
OnMouseMove
    Purpose
        This function is called when the mouse  is over
        a cell of this celltype.
        If this function does something with this
        event then return TRUE othrwise return FALSE.
    Params
        col - column that the mouse is over
        row - row that the mouse is over
        point - point where the mouse is
        flags - mouse move flags (see WM_MOUSEMOVE)
    Return
        TRUE - if the event was processed
        FALSE - if the event was not
****************************************************/
BOOL CUGCellType::OnMouseMove(int col,long row,POINT *point,UINT flags){
    return FALSE;
}
/***************************************************
SetOption
    Purpose
        This function is a standard interface for
        setting extended options for a cell type.
        What this function does is determined on a
        celltype by celltype basis
    Params
        option - option ID number
        param - value for the specified option
    Return
        UG_NA - if this function does not perform
            anything
        UG_SUCCESS - success
        UG_ERROR - error
****************************************************/
int CUGCellType::SetOption(long option,long param){
    return UG_NA;
}
/***************************************************
GetOption
    Purpose
        This function is a standard interface for
        getting extended options for a cell type.
        What this function does is determined on a
        celltype by celltype basis
    Params
        option - option ID number
        param - value for the specified option
    Return
        UG_NA - if this function does not perform
            anything
        UG_SUCCESS - success
        UG_ERROR - error
****************************************************/
int CUGCellType::GetOption(long option,long* param){
    return UG_NA;
}
/***************************************************
OnSetFocus
    Purpose
        This function is called when a cell of this
        type receives focus.
    Params
        col - column that just received focus
        row - row that just received focus
        cell - pointer to the cell object located at col/row
    Return
        none
****************************************************/
void CUGCellType::OnSetFocus(int col,long row,CUGCell *cell){
}
/***************************************************
OnKillFocus
    Purpose
        This function is called when a cell of this
        type loses focus.
    Params
        col - column that just lost focus
        row - row that just lost focus
        cell - pointer to the cell object located at col/row
    Return
        none
****************************************************/
void CUGCellType::OnKillFocus(int col,long row,CUGCell *cell){
}

/***************************************************
OnDraw
    Purpose
        This function is called whenever the grid needs a cell
        to be drawn. Detailed information of what needs to be
        drawn is passed in as parameters.
    Params
        dc      - device context to draw the cell with
        rect    - rectangle to draw the cell in
        col     - column that is being drawn
        row     - row that is being drawn
        cell    - cell that is being drawn
        selected- TRUE if the cell is selected, otherwise FALSE
        current - TRUE if the cell is the current cell, otherwise FALSE
    Return
        none
****************************************************/
void CUGCellType::OnDraw(CDC *dc,RECT *rect,int col,long row,CUGCell *cell,
                        int selected,int current){
    RECT rectOut;

    if(m_ctrl->m_GI->m_enableCellOverLap){
        if(CanOverLap(cell)){
            if(cell->GetTextLength() == 0){
                int overlapCol,offset;
                CUGCell overlapCell;
                offset = GetCellOverlapInfo(dc,col,row,&overlapCol,&overlapCell);
                if(offset != 0){
                    DrawBorder(dc,rect,&rectOut,cell);
                    DrawText(dc,&rectOut,offset,col,row,&overlapCell,selected,current);
                    return;
                }
            }
        }
    }


    DrawBorder(dc,rect,&rectOut,cell);
    DrawText(dc,&rectOut,0,col,row,cell,selected,current);
}

/***************************************************
DrawText
    Purpose
        This function is the standard text drawing routine
        used by this cell type and used by many others.
    Params
        dc      - device context to draw the cell with
        rect    - rectangle to draw the cell in
        col     - column that is being drawn
        row     - row that is being drawn
        cell    - cell that is being drawn
        selected- TRUE if the cell is selected, otherwise FALSE
        current - TRUE if the cell is the current cell, otherwise FALSE
    Return
        none
****************************************************/
void CUGCellType::DrawText(CDC *dc,RECT *rect,int offset,int col,long row,CUGCell *cell,
                        int selected,int current){

    int         left,top;
    SIZE        size;
    COLORREF    backcolor;
    LPCTSTR     string;
    int         stringLen;
    long        cellTypeEx;
    short       alignment;

    int         oldleft     = rect->left;

    long        properties = cell->GetPropertyFlags();

    int         overLapCol = col;

    int         iVertOffset;                            // BMD

    //get the extended style
    if(properties&UGCELL_CELLTYPEEX_SET){
        cellTypeEx = cell->GetCellTypeEx();
    }
    else
        cellTypeEx = 0;


    //get the string to draw
    if(!m_drawLabelText && cellTypeEx != UGCT_NORMALLABELTEXT){
        if(properties&UGCELL_STRING_SET){
            string  = cell->GetText();
            stringLen = cell->GetTextLength();
        }
        else{
            string = _T("");
            stringLen = 0;
        }
    }
    else{
        if(properties&UGCELL_LABEL_SET){
            string  = cell->GetLabelText();
            stringLen = lstrlen(string);
        }
        else{
            string = _T("");
            stringLen = 0;
        }
    }

    //check for overlapping
    if(overLapCol != col){
        CUGCell olCell;
        m_ctrl->GetCellIndirect(overLapCol,row,&olCell);
        cell->SetAlignment(olCell.GetAlignment());
        string  = olCell.GetText();
        stringLen = olCell.GetTextLength();
    }


    if(properties&UGCELL_ALIGNMENT_SET)
        alignment = cell->GetAlignment();
    else
        alignment = 0;


    //select the font
    if(properties&UGCELL_FONT_SET )
        dc->SelectObject(cell->GetFont());

    //check the selected and current states
    if(selected || (current && m_ctrl->m_GI->m_currentCellMode&2)){
        dc->SetTextColor(cell->GetHTextColor());
        dc->SetBkColor(cell->GetHBackColor());
        backcolor = cell->GetHBackColor();
    }
    else{
        dc->SetTextColor(cell->GetTextColor());
        backcolor = cell->GetBackColor();
        dc->SetBkColor(backcolor);
    }

    //check for bitmaps
    if(properties&UGCELL_BITMAP_SET){
        oldleft = rect->left;
        rect->left += DrawBitmap(dc,cell->GetBitmap(),rect,backcolor);
    }

    //text alignment
    if(alignment){
        if(alignment&UG_ALIGNCENTER){
            GetTextExtentPoint(dc->m_hDC,string,stringLen,&size);
            left = rect->left + (rect->right - rect->left - size.cx) /2;
        }
        else if(alignment&UG_ALIGNRIGHT){
            GetTextExtentPoint(dc->m_hDC,string,stringLen,&size);
            left = rect->right - size.cx - m_ctrl->m_GI->m_margin;
        }
        else{
            left = rect->left + m_ctrl->m_GI->m_margin + offset;
        }

        if(alignment & UG_ALIGNVCENTER){
//          GetTextExtentPoint(dc->m_hDC,string,stringLen,&size);                           // BMD
//          top = rect->top + (rect->bottom - rect->top - size.cy) /2;
            CRect rtTemp = rect;
            iVertOffset = dc->DrawText(string, stringLen, rtTemp, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT);
            top = rect->top + (rect->bottom - rect->top - iVertOffset) / 2;
        }
        else if(alignment & UG_ALIGNBOTTOM){
//          GetTextExtentPoint(dc->m_hDC,string,stringLen,&size);                           // BMD
//          top = rect->bottom - size.cy -1;
            CRect rtTemp = rect;
            iVertOffset = dc->DrawText(string, stringLen, rtTemp, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT);
            top = rect->bottom - iVertOffset - 1;
        }
        else{
            top = rect->top + 1;
        }
    }
    else{
        left = rect->left + m_ctrl->m_GI->m_margin + offset;
        top = rect->top + 1;
    }

    if(offset < 0)
        rect->left --;

    //***draw the text***
    if(cellTypeEx&(UGCT_NORMALMULTILINE|UGCT_NORMALELLIPSIS)){

        if(cellTypeEx&UGCT_NORMALMULTILINE){ // multiline

            CRect   tempRect(rect);

            // set up a default format
            UINT format = DT_WORDBREAK | DT_NOPREFIX;

            // check alignment - multiline
            if(alignment) {
                if(alignment & UG_ALIGNCENTER) {
                    format |= DT_CENTER;
                }
                else if(alignment & UG_ALIGNRIGHT) {
                    format |= DT_RIGHT;
                    tempRect.right -= 2;
                }
                else if(alignment & UG_ALIGNLEFT) {
                    format |= DT_LEFT;
                    tempRect.left += 2;
                }
                if (alignment & UG_ALIGNVCENTER) {                      // BMD
                    tempRect.top = top;
                }
                if (alignment & UG_ALIGNBOTTOM) {                       // BMD
                    tempRect.top = top;
                }

                // note that VCENTER, TOP, and BOTTOM have no effect
                // unless combined with DT_SINGLELINE - this defeats
                // the purpose of our multiline value, so
                // should we do this?  I think not...

                //if(alignment & UG_ALIGNVCENTER) {
                //  format |= DT_VCENTER | DT_SINGLELINE;
                //}
                //else if(alignment & UG_ALIGNTOP) {
                //  format |= DT_TOP | DT_SINGLELINE;
                //}
                //else if(alignment & UG_ALIGNBOTTOM) {
                //  format |= DT_BOTTOM | DT_SINGLELINE;
                //}
            }



            dc->ExtTextOut(left,top,ETO_OPAQUE,rect,_T(""),0,NULL); //clear the rect
            dc->DrawText(string,-1,tempRect,format );           //draw the text
        }
        else{ // ellipsis
            size = dc->GetTextExtent(string,stringLen);
            if(size.cx+6 > (rect->right - rect->left)){
                size = dc->GetTextExtent(_T("..."),3);
                rect->right -= size.cx + 3;
                if(rect->right > rect->left)
                    dc->ExtTextOut(rect->left+3,top,ETO_OPAQUE|ETO_CLIPPED,rect,string,stringLen,NULL);
                rect->left = rect->right;
                if(rect->left <= oldleft)
                    rect->left = oldleft+1;
                rect->right += size.cx +3;
                dc->ExtTextOut(rect->left+1,top,ETO_OPAQUE|ETO_CLIPPED,rect,_T("..."),3,NULL);
            }
            else{
                dc->ExtTextOut(left,top,ETO_OPAQUE|ETO_CLIPPED,rect,string,stringLen,NULL);
            }
        }
    }
    else{

        if(overLapCol != col){
            //get the offset
            for(int loop = col -1;loop >= overLapCol; loop --){
                left -= m_ctrl->GetColWidth(loop);
            }
            left += 1 ;
        }

        dc->ExtTextOut(left,top,ETO_OPAQUE|ETO_CLIPPED,rect,string,stringLen,NULL);
    }

    //reset the rect
    rect->left = oldleft;
}
/***************************************************
OnPrint
    Purpose
        This function is called when the grid is going
        to be printed on a printer. This function should
        perform any setup required then call the celltypes
        OnDraw member
    Params
        dc      - device context to draw the cell with
        rect    - rectangle to draw the cell in
        bdrRect - outer retangle of the cells border
        col     - column that is being drawn
        row     - row that is being drawn
        cell    - cell that is being drawn
        overLapCol - column number of the cell to the left that
                is overlapping this cell. If this value is the same
                as 'col' then there is no overlapping
    Return
        0 - standard return value
        1 - if the text overflowed and the cell type supports
            cell overlapping
****************************************************/
void CUGCellType::OnPrint(CDC *dc,RECT *rect,int col,long row,CUGCell *cell){

    int height, numRows, col1, col2;
    long row1, row2;
    CFont * font;
    double scale;

    col1 = col;
    row1 = row;

    if(row >=0)
        height = m_ctrl->GetRowHeight(row);
    else
        height = m_ctrl->m_GI->m_topHdgHeights[((row * -1) -1)];

    scale = ((double)(rect->bottom - rect->top)) / ((double)height);

    if (cell->IsPropertySet (UGCELL_JOIN_SET)){
        m_ctrl->GetJoinRange (&col1, &row1, &col2, &row2);
        numRows = (row2 - row1) +1;

        if (numRows > 1)
            scale = (((double)(rect->bottom - rect->top))/numRows)/((double)height);
    }

    if(cell->GetFont() != NULL)
        font = cell->GetFont();
    else
        font = m_ctrl->m_GI->m_defFont;
    LOGFONT lf;
    GetObject(font->m_hObject,sizeof(lf),&lf);

    CFont scaleFont;

    lf.lfHeight = (int)((double)lf.lfHeight*scale);
    scaleFont.CreateFontIndirect(&lf);

    CFont * oldFont = dc->SelectObject(&scaleFont);
    cell->SetFont(&scaleFont);

    OnDraw(dc,rect,col,row,cell,0,0);
    dc->SelectObject(oldFont);
}
/****************************************************
BitmapDisplay
    Purpose:
        -Draws a bitmap within the given rectangle
        -The bitmap is automatically resized to fit within
        rect. The bitmap is placed on the left side of the rect
        -the with of the sized bitmap plus  a margin is returned
    Parameters:
        hdc         device context handle
        hbitmap     bitmap handle
        rect        bounding rectangle
        Margin  top and bottom margins
    return:
        returns the width of the area drawn in
*****************************************************/
int CUGCellType::DrawBitmap(CDC *dc,CBitmap * bitmap,RECT *rect,COLORREF backcolor){

    BITMAP bm;
    CBitmap * bmpOld;
    CDC dcMemory;
    long xin,yin,xout,yout;
    long ratio;
    int t;
    int resize = TRUE;
    int Margin = 2;


    //get the bitmaps co-ords
    bitmap->GetObject(sizeof(BITMAP), &bm);

    //calc the output co-ords , and maintain aspect ratio
    yout = (rect->bottom - rect->top - Margin*2)*100;
    yin =  bm.bmHeight;
    xin =  bm.bmWidth;
    ratio = yout /yin;
    xout = xin *ratio /100;
    yout /= 100;

    //if the height of the bitmap is less than the height of a row
    //then dont resize it
    if(yin <= (rect->bottom - rect->top - Margin *2) ){
        xout = xin;
        yout = yin;
        resize = FALSE;
    }
    if(xout >= rect->right - rect->left - Margin *2)
        xout = rect->right - rect->left - Margin *2;

    //create a compatible memory context
    dcMemory.CreateCompatibleDC(dc);
    bmpOld = (CBitmap *)dcMemory.SelectObject(bitmap);

    //fill in background first
    dc->SetBkColor(backcolor);
    t =rect->right;
    rect->right = rect->left+(int)xout+Margin*2;
    dc->ExtTextOut(0,0,ETO_OPAQUE,rect,_T(""),0,NULL);
    rect->right = t;

    //draw the bitmap
    if(resize){
        dc->StretchBlt(rect->left+Margin,rect->top+Margin,(int)xout,(int)yout,
            &dcMemory,0,0, bm.bmWidth, bm.bmHeight,SRCCOPY);
    }
    else{
        t= (int)((rect->bottom - rect->top - yout)/2);
        dc->BitBlt(rect->left+Margin,rect->top+t,(int)xout,(int)yout,
            &dcMemory,0,0,SRCCOPY);
    }
    //remove the compatible memory context
    dcMemory.SelectObject(bmpOld);

    return (Margin*2+(int)xout);
}

/***************************************************
Draw Border
    Draws a border using the given style. Defines
    for setting the style are available.
    'rect' is the area to draw the border in
    'rectout' returns the area inside the border
    'pen'  is the pen to use for drawing the first
    3 styles

Style   Thin:       bit 0-left  1-top  2-right  3-bottom
        Medium:     bit 4-left  5-top  6-right  7-bottom
        Thick:      bit 8-left  9-top  10-right 11-bottom
        3DRecess:   bit 12-left 13-top 14-right 15-bottom
        3DRaised:   bit 16-left 17-top 18-right 19-bottom
****************************************************/
void CUGCellType::DrawBorder(CDC *dc,RECT *rect,RECT *rectout,CUGCell * cell){

    long props = cell->GetPropertyFlags();

    BOOL excelBdr = m_ctrl->m_GI->m_enableExcelBorders;

    if( ( props&UGCELL_BORDERSTYLE_SET ) == 0 && !excelBdr){
        CopyRect(rectout,rect);
        return;
    }

    int style = cell->GetBorder();
    CPen *pen = cell->GetBorderColor();
    CPen *origPen = NULL;

    if(style&UG_BDR_RAISED || style&UG_BDR_RECESSED)
        excelBdr = 0;

    int left=0,top=0,right=0,bottom=0;
    if(pen != NULL)
        dc->SelectObject(pen);
    else
        dc->SelectObject(m_ctrl->m_GI->m_defBorderPen);


    if(style&15 || excelBdr){ //thin lines

        if(style & 1){ //left
            dc->MoveTo(rect->left,rect->top);
            dc->LineTo(rect->left,rect->bottom);
            left=1;
        }
        if(style & 2){ //top
            dc->MoveTo(rect->left,rect->top);
            dc->LineTo(rect->right,rect->top);
            top=1;
        }
        if(style & 4 || excelBdr){ //right
            if((style&4) ==0)
                origPen = (CPen*)dc->SelectObject(m_ctrl->m_GI->m_defBorderPen);

            dc->MoveTo(rect->right-1,rect->top);
            dc->LineTo(rect->right-1,rect->bottom);
            right=-1;

            if((style&4) ==0)
                dc->SelectObject(origPen);
        }
        if(style & 8 || excelBdr){ //bottom
            if((style&8) ==0)
                origPen = (CPen*)dc->SelectObject(m_ctrl->m_GI->m_defBorderPen);

            dc->MoveTo(rect->left,rect->bottom-1);
            dc->LineTo(rect->right,rect->bottom-1);
            bottom=-1;

            if((style&8) ==0)
                dc->SelectObject(origPen);
        }
    }
    if(style &240){ //medium lines
        if(style & 16){ //left
            dc->MoveTo(rect->left,rect->top);
            dc->LineTo(rect->left,rect->bottom);
            dc->MoveTo(rect->left+1,rect->top);
            dc->LineTo(rect->left+1,rect->bottom);
            left=2;
        }
        if(style & 32){ //top
            dc->MoveTo(rect->left,rect->top);
            dc->LineTo(rect->right,rect->top);
            dc->MoveTo(rect->left,rect->top+1);
            dc->LineTo(rect->right,rect->top+1);
            top=2;
        }
        if(style & 64){ //right
            dc->MoveTo(rect->right-1,rect->top);
            dc->LineTo(rect->right-1,rect->bottom);
            dc->MoveTo(rect->right-2,rect->top);
            dc->LineTo(rect->right-2,rect->bottom);
            right=-2;
        }
        if(style & 128){ //bottom
            dc->MoveTo(rect->left,rect->bottom-1);
            dc->LineTo(rect->right,rect->bottom-1);
            dc->MoveTo(rect->left,rect->bottom-2);
            dc->LineTo(rect->right,rect->bottom-2);
            bottom=-2;
        }
    }
    if(style &3840){ //thick lines
        if(style & 256){ //left
            dc->MoveTo(rect->left,rect->top);
            dc->LineTo(rect->left,rect->bottom);
            dc->MoveTo(rect->left+1,rect->top);
            dc->LineTo(rect->left+1,rect->bottom);
            dc->MoveTo(rect->left+2,rect->top);
            dc->LineTo(rect->left+2,rect->bottom);
            left=3;
        }
        if(style & 512){ //top
            dc->MoveTo(rect->left,rect->top);
            dc->LineTo(rect->right,rect->top);
            dc->MoveTo(rect->left,rect->top+1);
            dc->LineTo(rect->right,rect->top+1);
            dc->MoveTo(rect->left,rect->top+2);
            dc->LineTo(rect->right,rect->top+2);
            top=3;
        }
        if(style & 1024){ //right
            dc->MoveTo(rect->right-1,rect->top);
            dc->LineTo(rect->right-1,rect->bottom);
            dc->MoveTo(rect->right-2,rect->top);
            dc->LineTo(rect->right-2,rect->bottom);
            dc->MoveTo(rect->right-3,rect->top);
            dc->LineTo(rect->right-3,rect->bottom);
            right=-3;
        }
        if(style & 2048){ //bottom
            dc->MoveTo(rect->left,rect->bottom-1);
            dc->LineTo(rect->right,rect->bottom-1);
            dc->MoveTo(rect->left,rect->bottom-2);
            dc->LineTo(rect->right,rect->bottom-2);
            dc->MoveTo(rect->left,rect->bottom-3);
            dc->LineTo(rect->right,rect->bottom-3);
            bottom=-3;
        }
    }
    if(style &4096){ //3D recessed

        int loop;
        //dark color
        dc->SelectObject((CPen *)&m_ctrl->m_threeDDarkPen);
        for(loop=0;loop<m_ctrl->m_GI->m_threeDHeight;loop++){
            dc->MoveTo(rect->left+loop,rect->bottom-loop-1);
            dc->LineTo(rect->left+loop,rect->top+loop);
            dc->LineTo(rect->right-loop-1,rect->top+loop);
        }
        //light color
        dc->SelectObject((CPen *)&m_ctrl->m_threeDLightPen);
        for(loop=0;loop<m_ctrl->m_GI->m_threeDHeight;loop++){
            dc->MoveTo(rect->right-loop-1,rect->top+loop);
            dc->LineTo(rect->right-loop-1,rect->bottom-loop-1);
            dc->LineTo(rect->left+loop,rect->bottom-loop-1);
        }
        left = m_ctrl->m_GI->m_threeDHeight;
        top = m_ctrl->m_GI->m_threeDHeight;
        right = -m_ctrl->m_GI->m_threeDHeight;
        bottom = -m_ctrl->m_GI->m_threeDHeight;

    }

    if(style &8192){ //3D raised

        int loop;
        //light color
        dc->SelectObject((CPen *)&m_ctrl->m_threeDLightPen);
        for(loop=0;loop<m_ctrl->m_GI->m_threeDHeight;loop++){
            dc->MoveTo(rect->left+loop,rect->bottom-loop-1);
            dc->LineTo(rect->left+loop,rect->top+loop);
            dc->LineTo(rect->right-loop-1,rect->top+loop);
        }
        //dark color
        dc->SelectObject((CPen *)&m_ctrl->m_threeDDarkPen);
        for(loop=0;loop<m_ctrl->m_GI->m_threeDHeight;loop++){
            dc->MoveTo(rect->right-loop-1,rect->top+loop);
            dc->LineTo(rect->right-loop-1,rect->bottom-loop-1);
            dc->LineTo(rect->left+loop,rect->bottom-loop-1);
        }
        left = m_ctrl->m_GI->m_threeDHeight;
        top = m_ctrl->m_GI->m_threeDHeight;
        right = -m_ctrl->m_GI->m_threeDHeight;
        bottom = -m_ctrl->m_GI->m_threeDHeight;
    }

    rectout->left   = rect->left + left;
    rectout->top    = rect->top + top;
    rectout->right  = rect->right + right;
    rectout->bottom = rect->bottom + bottom;

    return;
}

/****************************************************
GetBestSize
    Purpose
        Returns the best (nominal) size for a cell using
        this cell type, with the given cell properties.
    Params
        dc      - device context to use to calc the size on
        size    - return the best size in this param
        cell    - pointer to a cell object to use for the calc.
    Return
*****************************************************/
void CUGCellType::GetBestSize(CDC *dc,CSize *size,CUGCell *cell){

    //select the font
    CFont * oldFont = NULL;
    if(cell->IsPropertySet(UGCELL_FONT_SET))
        oldFont = (CFont *)dc->SelectObject(cell->GetFont());
    else if(m_ctrl->m_GI->m_defFont != NULL)
        oldFont = (CFont *)dc->SelectObject(m_ctrl->m_GI->m_defFont);


    //get the best size
    CSize s(3,3);
    if(cell->IsPropertySet(UGCELL_TEXT_SET))
        s = dc->GetOutputTextExtent(cell->GetText());

    //use margins
    s.cx += 6;
    s.cy += 2;

    //select back the original font
    if(oldFont != NULL)
        dc->SelectObject(oldFont);

    *size = s;

}

/****************************************************
GetCellOverlapInfo
    Purpose
        Returns the best (nominal) size for a cell using
        this cell type, with the given cell properties.
    Params
        dc      - device context to use to calc the size on
        size    - return the best size in this param
        cell    - pointer to a cell object to use for the calc.
    Return
        offset for drawing the overlapped cell
*****************************************************/
int CUGCellType::GetCellOverlapInfo(CDC* dc,int col,long row,int *overlapCol,CUGCell *cell){

    int         overLappedCol;          //used by cell-overlapping calculations
    int         loop,prevCol,width;
    CSize       size;

    //if cell overlapping is enabled check to see if the
    //cell is overlapped
    overLappedCol = col;
    if(m_ctrl->m_GI->m_enableCellOverLap){

        //check to see if there is a prev cell in the col that is not blank
        prevCol = col;
        if(m_ctrl->m_GI->m_defDataSource->GetPrevNonBlankCol(&prevCol,row) == UG_SUCCESS){

            //check to see if the text in the cell that was found
            //is wider than the column that it is in
            m_ctrl->GetCellIndirect(prevCol,row,cell);

            if(cell->IsPropertySet(UGCELL_FONT_SET))
                dc->SelectObject(cell->GetFont());
            size = dc->GetOutputTextExtent(cell->GetText());
            size.cx += m_ctrl->m_GI->m_margin;

            //if the prev cell is wider than its column width
            //and the cell allows overlapping
            if(size.cx > m_ctrl->GetColWidth(prevCol) &&
                m_ctrl->GetCellType(cell->GetCellType())->
                CanOverLap(cell) != FALSE){


                //check to see how many columns wide it is
                width = 0;
                for(loop = prevCol;loop <col;loop++){
                    width += m_ctrl->GetColWidth(loop);
                    if(width > size.cx){
                        break;
                    }
                }
                //check to see if the current cell is within the string width
                if(col == loop){
                    *overlapCol = prevCol;
                    return -(width);
                }
            }
        }
    }
    return 0;
}