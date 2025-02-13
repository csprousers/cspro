/***************************************************
Ultimate Grid 3.0 for MFC
Dundas Software Ltd. 1994-1996

    Cell Type Class
****************************************************/

#include "StdAfx.h"
#include "UGCTbutn.h"
#include "Ugctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//toggle button not currently implemented
#define  UGCT_BUTTONTOGGLE  32

/***************************************************
****************************************************/
CUGButtonType::CUGButtonType(){

    m_btnDown       = FALSE;
    m_mode          = -1;
    m_btnDownCol    = -1;
    m_btnDownRow    = -1;

    m_canOverLap = FALSE;
}

/***************************************************
****************************************************/
CUGButtonType::~CUGButtonType(){

}
/***************************************************
****************************************************/
BOOL CUGButtonType::OnDClicked(int col,long row,RECT *rect,POINT *point){
    return OnLClicked(col,row,1,rect,point);
}
/***************************************************
OnLClicked
    - When the mouse button goes down, setup the
    button down variables
    - When the mouse button goes up and the mouse was
      over the button then send a click notification
    - redraw the cell
****************************************************/
BOOL CUGButtonType::OnLClicked(int col,long row,int updn,RECT *rect,POINT */*point*/){

    if(updn){
        m_ctrl->OnCellTypeNotify(m_ID,col,row,UGCT_BUTTONDOWN,0);
        m_btnDown       = TRUE;
        m_btnDownCol    = col;
        m_btnDownRow    = row;
        CopyRect(&m_btnDownRect,rect);
    }
    else{

        m_ctrl->OnCellTypeNotify(m_ID,col,row,UGCT_BUTTONUP,0);
        if(m_mode != UGCT_BUTTONTOGGLE){
            if(m_btnDownCol == col && m_btnDownRow == row && m_btnDown == TRUE){
                m_ctrl->OnCellTypeNotify(m_ID,col,row,UGCT_BUTTONCLICK,0);
            }
        }
        m_btnDown = FALSE;
    }

    m_ctrl->RedrawCell(col,row);

    return TRUE;
}
/***************************************************
OnMouseMove
    - if the mouse button is not down then just return
    - if the mouse button is down then check to see
      if it is over button or not, if it is then
      recess the button, if it is not then raise the
      button
****************************************************/
BOOL CUGButtonType::OnMouseMove(int /*col*/,long /*row*/,POINT *point,UINT flags){

    //return if the button is up
    if((flags&MK_LBUTTON) == FALSE)
        return FALSE;

    if(point->x >= m_btnDownRect.left && point->x <= m_btnDownRect.right){
        if(point->y >= m_btnDownRect.top && point->y <= m_btnDownRect.bottom){
            if(m_btnDown == FALSE){
                m_btnDown = TRUE;
                m_ctrl->RedrawCell(m_btnDownCol,m_btnDownRow);
            }
            return TRUE;
        }
    }

    if(m_btnDown){
        m_btnDown = FALSE;
        m_ctrl->RedrawCell(m_btnDownCol,m_btnDownRow);
    }
    return FALSE;
}

/***************************************************
OnKeyDown
    - Send a click notification if the enter key
      was pressed
****************************************************/
BOOL CUGButtonType::OnKeyDown(int /*col*/,long /*row*/,UINT *vcKey){

    if(*vcKey == VK_RETURN){
        m_ctrl->OnCellTypeNotify(m_ID,m_ctrl->m_GI->m_currentCol,
            m_ctrl->m_GI->m_currentRow,UGCT_BUTTONCLICK,0);
        return TRUE;
    }

    return FALSE;
}

/***************************************************
OnKillFocus
    - Reset the button state
****************************************************/
void CUGButtonType::OnKillFocus(int col,long row,CUGCell *cell){

    CUGCellType::OnKillFocus(col,row,cell);

    m_btnDown       = FALSE;
}

/***************************************************
OnDraw
    - Draw a raised or resecced border
    - Call the base class drawing routine to
      draw the text
    - resize the rect to 0 wide and high if no
      focus rect is to be displayed
****************************************************/
void CUGButtonType::OnDraw(CDC *dc,RECT *rect,int col,long row,
                           CUGCell *cell,int selected,int current){

    RECT rectout;

    //draw the button
    if(m_btnDown && current){
        cell->SetBorder(UG_BDR_RECESSED);
        DrawBorder(dc,rect,&rectout,cell);
    }
    else{
        cell->SetBorder(UG_BDR_RAISED);
        DrawBorder(dc,rect,&rectout,cell);
    }

    //draw the text in using the default drawing routine
    DrawText(dc,&rectout,0,col,row,cell,selected,current);

    if(cell->IsPropertySet(UGCELL_CELLTYPEEX_SET)){
        if(cell->GetCellTypeEx()&UGCT_BUTTONNOFOCUS){
            rect->right = rect->left;
            rect->bottom = rect->top;
        }
    }

    return;
}
