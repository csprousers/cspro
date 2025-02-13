/***************************************************
Ultimate Grid 3.0 for MFC
Dundas Software Ltd. 1994-1996

    Cell Type Class
****************************************************/
#include "StdAfx.h"


#include <zGridO/Ugctrl.h>
#include "Ugctelps.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/********************************************
*********************************************/
CUGEllipsisType::CUGEllipsisType(){

    m_btnWidth =    GetSystemMetrics(SM_CXVSCROLL);
    m_btnDown =     FALSE;

    m_pen.CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNFACE));
    m_brush.CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    m_canOverLap = FALSE;
}
/********************************************
*********************************************/
CUGEllipsisType::~CUGEllipsisType(){
}
/********************************************
*********************************************/
BOOL CUGEllipsisType::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point){

    if(updn){
        if(point->x > (rect->right -m_btnWidth)){

            //copy the button co-ords
            CopyRect(&m_btnRect,rect);
            m_btnRect.left = rect->right - m_btnWidth;

            //redraw the button
            m_btnDown = TRUE;
            m_btnCol = col;
            m_btnRow = row;
            m_ctrl->RedrawCell(m_btnCol,m_btnRow);
            return TRUE;
        }

        m_btnCol = -1;
        m_btnRow = -1;
        return FALSE;
    }
    else if(m_btnDown){

        //notify the user that the button has been clicked
        m_ctrl->OnCellTypeNotify(m_ID,col,row,UGCT_ELLIPSISBUTTONCLICK,0);
        m_btnDown = FALSE;
        m_btnCol = -1;
        m_btnRow = -1;

        m_ctrl->RedrawCell(col,row);
        return TRUE;
    }
    return FALSE;
}
/********************************************
*********************************************/
BOOL CUGEllipsisType::OnDClicked(int col,long row,RECT *rect,POINT *point){
    UNREFERENCED_PARAMETER(col);
    UNREFERENCED_PARAMETER(row);
    UNREFERENCED_PARAMETER(rect);
    UNREFERENCED_PARAMETER(point);
    return TRUE;
}
/********************************************
*********************************************/
BOOL CUGEllipsisType::OnMouseMove(int col,long row,POINT *point,UINT flags){
    UNREFERENCED_PARAMETER(col);
    UNREFERENCED_PARAMETER(row);

    if((flags&MK_LBUTTON) == FALSE)
        return FALSE;

    if(point->x >= m_btnRect.left && point->x <= m_btnRect.right){
        if(point->y >= m_btnRect.top && point->y <= m_btnRect.bottom){
            if(!m_btnDown){
                m_btnDown = TRUE;
                m_ctrl->RedrawCell(m_btnCol,m_btnRow);
            }
            return TRUE;
        }
    }
    if(m_btnDown){
        m_btnDown = FALSE;
        m_ctrl->RedrawCell(m_btnCol,m_btnRow);
    }
    return FALSE;
}
/********************************************
*********************************************/
int CUGEllipsisType::GetEditArea(RECT *rect){
    //Savy&&
    rect->right = rect->right - m_btnWidth;
    return UG_SUCCESS;
}
/********************************************
*********************************************/
void CUGEllipsisType::OnDraw(CDC *dc,RECT *rect,int col,long row,
                             CUGCell *cell,int selected,int current){

    DrawBorder(dc,rect,rect,cell);

    int left = rect->left;
    RECT rectout;
    CPen * oldpen;

    //draw the 3D border
    rect->left = rect->right - m_btnWidth;
    if(m_btnDown && current){
        cell->SetBorder(UG_BDR_RECESSED);
        DrawBorder(dc,rect,&rectout,cell);
    }
    else{
        cell->SetBorder(UG_BDR_RAISED);
        DrawBorder(dc,rect,&rectout,cell);
    }

    //fill the border in
    dc->SetBkColor(GetSysColor(COLOR_BTNFACE));
    dc->FillRect(&rectout,&m_brush);

    //make a line to separate the border from the rest ofthe cell
    oldpen = (CPen *)dc->SelectObject((CPen *)&m_pen);
    dc->MoveTo(rect->left-1,rect->top);
    dc->LineTo(rect->left-1,rect->bottom);

    //draw the three ...
    int x= rect->left + ((m_btnWidth-7)/2);
    int y = rect->top + ((rect->bottom - rect->top)/4)*3;

    dc->SelectObject((CPen*)CPen::FromHandle((HPEN)GetStockObject(BLACK_PEN)));
    dc->MoveTo(x,y);
    dc->LineTo(x,y+2);
    dc->MoveTo(x+3,y);
    dc->LineTo(x+3,y+2);
    dc->MoveTo(x+6,y);
    dc->LineTo(x+6,y+2);

    dc->SelectObject(oldpen);

    //draw the text in using the default drawing routine
    rect->left =left;
    rect->right -=(m_btnWidth+1);
    DrawText(dc,rect,0,col,row,cell,selected,current);

    return;
}