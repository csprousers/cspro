/***************************************************
Ultimate Grid 3.0 for MFC
Dundas Software Ltd. 1994-1996

    Cell Type Class
****************************************************/

#include "StdAfx.h"
#include "Ugctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


#define UG_SETDROPLIST 100
#define UG_CLEARDROPLIST 101

/***************************************************
****************************************************/
CUGDropListType::CUGDropListType(){

    //set up the variables
    m_pen.CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNFACE));
    m_brush.CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    m_btnWidth = GetSystemMetrics(SM_CXVSCROLL);

    m_btnDown = FALSE;

    m_btnCol = -1;
    m_btnRow = -1;

    m_listBox = new CUGLstBox;

    m_canOverLap = FALSE;

}

/***************************************************
****************************************************/
CUGDropListType::~CUGDropListType(){

    //clean up
    delete m_listBox;
}
/***************************************************
****************************************************/
LPCTSTR CUGDropListType::GetName(){
    return _T("Standard DropList");
}

/***************************************************
****************************************************/
LPCUGID CUGDropListType::GetUGID(){

    static const UGID ugid = { 0x1667a6a0, 0xf746,
                            0x11d0, 0x9c, 0x7f,
                            0x0, 0x80, 0xc8, 0x3f,
                            0x71, 0x2f };

    return &ugid;
}

/***************************************************
****************************************************/
int CUGDropListType::GetEditArea(RECT *rect){

    rect->right -=(m_btnWidth+1);

    return TRUE;
}
/***************************************************
****************************************************/
BOOL CUGDropListType::OnDClicked(int col,long row,RECT *rect,POINT *point){

    if(point->x > (rect->right - m_btnWidth))
        return OnLClicked(col,row,1,rect,point);

    return FALSE;
}
/***************************************************
****************************************************/
BOOL CUGDropListType::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point){

    if(updn){
        if(point->x > (rect->right - m_btnWidth)){
            if(col == m_btnCol && row == m_btnRow || m_btnCol ==-2){
                m_btnCol = -1;
                m_btnRow = -1;
                if(m_listBox->GetSafeHwnd() != NULL)
                    m_listBox->DestroyWindow();
            }
            else{
                //copy the droplist button co-ords
                CopyRect(&m_btnRect,rect);
                m_btnRect.left = rect->right - m_btnWidth;

                //redraw the button
                m_btnDown = TRUE;
                m_btnCol = col;
                m_btnRow = row;
                m_ctrl->RedrawCell(m_btnCol,m_btnRow);

                //start the drop list
                StartDropList();
                return TRUE;
            }
        }
        else if(m_btnCol ==-2){
            m_btnCol = -1;
            m_btnRow = -1;
            return FALSE;
        }
    }
    else if(m_btnDown){
        m_btnDown = FALSE;
        m_ctrl->RedrawCell(col,row);
        return TRUE;
    }

    return FALSE;
}
/***************************************************
****************************************************/
BOOL CUGDropListType::OnMouseMove(int col,long row,POINT *point,UINT flags){

    if((flags&MK_LBUTTON) == FALSE)
        return TRUE;

    if(point->x >= m_btnRect.left && point->x <= m_btnRect.right){
        if(point->y >= m_btnRect.top && point->y <= m_btnRect.bottom){
            if(!m_btnDown){
                m_btnDown = TRUE;
                m_ctrl->RedrawCell(m_btnCol,m_btnRow);
            }
        }
        else if(m_btnDown){
            m_btnDown = FALSE;
            m_ctrl->RedrawCell(m_btnCol,m_btnRow);
        }
    }
    else if(m_btnDown){
        m_btnDown = FALSE;
        m_ctrl->RedrawCell(m_btnCol,m_btnRow);
    }

    return FALSE;
}
/***************************************************
****************************************************/
void CUGDropListType::OnKillFocus(int col,long row,CUGCell *cell){

    m_btnCol = -1;
    m_btnRow = -1;
}
/***************************************************
****************************************************/
BOOL CUGDropListType::OnKeyDown(int col,long row,UINT *vcKey){

    if(*vcKey==VK_RETURN){
        StartDropList();
        *vcKey =0;
        return TRUE;
    }
    if(*vcKey==VK_DOWN){
        if(GetKeyState(VK_CONTROL) <0){
            StartDropList();
            *vcKey =0;
            return TRUE;
        }
    }
    return FALSE;
}
/***************************************************
****************************************************/
BOOL CUGDropListType::OnCharDown(int col,long row,UINT *vcKey){

    if(*vcKey==VK_RETURN)
        return TRUE;
    if(*vcKey==VK_DOWN)
        return TRUE;
    return FALSE;
}

/***************************************************

  current bit:  0-do nothing
                1-highlight

****************************************************/
void CUGDropListType::OnDraw(CDC *dc,RECT *rect,int col,long row,CUGCell *cell,
                            int selected,int current){

    int left = rect->left;
    RECT rectout;
    CPen * oldpen;


    int style = 0;
    if(cell->IsPropertySet(UGCELL_CELLTYPEEX_SET))
        style = cell->GetCellTypeEx();

    //if the cell is not current and hide button is on
    //then dont draw the button
    if(style&UGCT_DROPLISTHIDEBUTTON && !current){
        CUGCellType::OnDraw(dc,rect,col,row,cell,selected,current);
        return;
    }

    DrawBorder(dc,rect,rect,cell);

    //draw the button side of the the cell
    rect->left = rect->right - m_btnWidth;

    //draw the 3D border
    if(m_btnDown && current){
        cell->SetBorder(UG_BDR_RECESSED);
        DrawBorder(dc,rect,&rectout,cell);
    }
    else{
        cell->SetBorder(UG_BDR_RAISED);
        DrawBorder(dc,rect,&rectout,cell);
    }

    //fill the border in
    dc->FillRect(&rectout,&m_brush);

    //make a line to separate the border from the rest ofthe cell
    oldpen = (CPen *)dc->SelectObject((CPen *)&m_pen);
    dc->MoveTo(rect->left-1,rect->top);
    dc->LineTo(rect->left-1,rect->bottom);

    //draw the down arrow
    int x= ((m_btnWidth-5)/2) + rect->left;
    int y = ((rect->bottom - rect->top -3)/2) + rect->top;
    dc->SelectObject((CPen*)CPen::FromHandle((HPEN)GetStockObject(BLACK_PEN)));
    dc->MoveTo(x,y);
    dc->LineTo(x+5,y);
    dc->MoveTo(x+1,y+1);
    dc->LineTo(x+4,y+1);
    dc->MoveTo(x+2,y+2);
    dc->LineTo(x+2,y+1);


    dc->SelectObject(oldpen);

    //draw the text in using the default drawing routine
    rect->left = left;
    rect->right -=(m_btnWidth+1);

    CUGCellType::DrawText(dc,rect,0,col,row,cell,selected,current);
}
/***************************************************
****************************************************/
int CUGDropListType::StartDropList(){

    RECT        rect;
    RECT        clientRect;
    int         dif,len,pos,startpos;
    CFont *     font = NULL;
    LOGFONT     lf;
    CFont *     cfont;
    CStringList list;     //lists of strings for the list are assembed here
    CString *   items;    //points to the string used to assemble the list

    //return false if it is already up
    if( m_listBox->m_HasFocus)
        return 1;

    //get the current row and col
    m_btnCol = m_ctrl->GetCurrentCol();
    m_btnRow = m_ctrl->GetCurrentRow();

    //add the strings to the CStringList
    CUGCell cell;
    m_ctrl->GetCellIndirect(m_btnCol,m_btnRow,&cell);
    CString string;
    cell.GetLabelText(&string);
    items = &string;
    len = items->GetLength();
    pos = 0;
    startpos =0;
    while(pos <len){
        if(items->GetAt(pos)== _T('\n')){
            list.AddTail(items->Mid(startpos,pos-startpos));
            startpos = pos+1;
        }
        pos++;
    }

    //notify the user of the list, so it can be modified if needed
    m_ctrl->OnCellTypeNotify(m_ID,m_btnCol,m_btnRow,UGCT_DROPLISTSTART,(long)&list);

    //get the font
    if(cell.IsPropertySet(UGCELL_FONT_SET))
        font = cell.GetFont();
    else if(m_ctrl->m_GI->m_defFont != NULL)
        font = m_ctrl->m_GI->m_defFont;

    if(font != NULL){
        //find the height of the font
        cfont = font;
        #ifdef WIN32
        cfont->GetLogFont(&lf);     // lf.lfHeight
        #else
        lf.lfHeight = 12;
        #endif
    }
    else
        lf.lfHeight = 12;

    if(lf.lfHeight < 0)
        lf.lfHeight *= -1;

    //get the rectangle for the listbox
    m_ctrl->GetCellRect(m_btnCol,m_btnRow,&rect);
    rect.top = rect.bottom;
    rect.left+=10;
    rect.right+=10;
    len = list.GetCount();
    if(len >15)
        len = 15;
    rect.bottom += lf.lfHeight * len + 2 + lf.lfHeight;

    m_ctrl->m_CUGGrid->GetClientRect(&clientRect);
    if(rect.bottom > clientRect.bottom){
        dif = rect.bottom - clientRect.bottom;
        rect.bottom -= dif;
        rect.top -= dif;
        if(rect.top <0)
            rect.top = 0;
    }
    if(rect.right > clientRect.right){
        dif = rect.right - clientRect.right;
        rect.right -= dif;
        rect.left -= dif;
        if(rect.left <0)
            rect.left = 0;
    }

    //create the CListBox
    m_listBox->m_ctrl = m_ctrl;
    m_listBox->m_cellTypeId = m_ID;
    m_listBox->Create(WS_CHILD|WS_BORDER|WS_VSCROLL,rect,m_ctrl->m_CUGGrid,123);

    //set up the font
    if(font != NULL)
        m_listBox->SetFont(font);

    //resize the window again since a new font is being used
    m_listBox->MoveWindow(&rect,FALSE);

    //add the items to the list
    len = list.GetCount();
    POSITION position = list.GetHeadPosition();
    pos =0;
    while(pos < len){
        m_listBox->AddString(list.GetAt(position));
        pos++;
        if(pos < len)
            list.GetNext(position);
    }

    //give the list box pointers to the cell
    m_listBox->m_col = &m_btnCol;
    m_listBox->m_row = &m_btnRow;

    m_listBox->ShowWindow(SW_SHOW);
    m_listBox->SetFocus();

    m_btnDown = FALSE;
    m_ctrl->RedrawCell(m_btnCol,m_btnRow);

    return UG_SUCCESS;
}
