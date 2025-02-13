/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGCheckBoxType
    Purpose
        checkbox cell type class
    Instructions
        - The checkbox changes state when clicked
          with the left mouse button (over the check
          box) or the space bar
        -The checkbox type always uses the same
         id number since it is a built in type.
            ID Number: UGCT_CHECKBOX
        -There are several extended styles for this type
            UGCT_CHECKBOXFLAT
            UGCT_CHECKBOXCROSS
            UGCT_CHECKBOX3DRECESS
            UGCT_CHECKBOX3DRAISED
            UGCT_CHECKBOXCHECKMARK
************************************************/
#include "StdAfx.h"
#include "Ugcbtype.h"
#include "Ugctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/***************************************************
****************************************************/
CUGCheckBoxType::CUGCheckBoxType(){

    m_canTextEdit   = FALSE;
    m_canOverLap    = FALSE;

    Lightpen.CreatePen(PS_SOLID,1,RGB(225,225,225));
    Darkpen.CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNSHADOW));
    Facepen.CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNFACE));
}
/***************************************************
****************************************************/
CUGCheckBoxType::~CUGCheckBoxType(){
}
/***************************************************
****************************************************/
LPCTSTR CUGCheckBoxType::GetName(){
    return _T("Standard CheckBox Type");
}

/***************************************************
****************************************************/
LPCUGID CUGCheckBoxType::GetUGID(){

    static const UGID ugid = { 0x93aab8d0, 0xf749, 0x11d0,
                            { 0x9c, 0x7f, 0x0, 0x80, 0xc8,
                            0x3f, 0x71, 0x2f } };

    return &ugid;
}
/***************************************************
****************************************************/
BOOL CUGCheckBoxType::OnDClicked(int col,long row,RECT *rect,POINT *point){

    return OnLClicked(col,row,1,rect,point);
}
/***************************************************
****************************************************/
BOOL CUGCheckBoxType::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point){

    int top,height;
    height = rect->bottom - rect->top - 6;
    if(height >12)
        height = 12;
    top = (rect->bottom - rect->top - height) /2;

    if(point->x < rect->left+3 || point->x > rect->left+3+height)
        return FALSE;
    if(point->y < rect->top+top || point->y > rect->top+top+height)
        return FALSE;

    if(!updn){

        m_ctrl->GetCellIndirect(col,row,&m_cell);

        if (m_cell.IsPropertySet(UGCELL_READONLY_SET))
            if (m_cell.GetReadOnly())
                return FALSE;

        if(m_cell.GetNumber() == FALSE)
            m_cell.SetNumber(TRUE);
        else
            m_cell.SetNumber(FALSE);

        m_ctrl->SetCell(col,row,&m_cell);
        m_ctrl->RedrawCell(col,row);


        m_ctrl->OnCellTypeNotify(m_ID,col,row,UGCT_CHECKBOXSET,(long)m_cell.GetNumber());

        return TRUE;
    }
    else
        return FALSE;
}
/***************************************************
****************************************************/
BOOL CUGCheckBoxType::OnCharDown(int col,long row,UINT *vcKey) {

    if( *vcKey == VK_SPACE ) {

        col = m_ctrl->GetCurrentCol();
        row = m_ctrl->GetCurrentRow();

        m_ctrl->GetCell(col,row,&m_cell);

        if (m_cell.IsPropertySet(UGCELL_READONLY_SET))
            if (m_cell.GetReadOnly())
                return FALSE;

        BOOL val;
        val = m_cell.GetBool();

        if(val == FALSE){
            m_cell.SetNumber(TRUE);
            val = TRUE;
        }
        else{
            m_cell.SetNumber(FALSE);
            val = FALSE;
        }
        m_ctrl->SetCell(col,row,&m_cell);
        m_ctrl->RedrawCell(col,row);

        //notify the user that the checkbox was checked
        m_ctrl->OnCellTypeNotify(m_ID,col,row,UGCT_CHECKBOXSET,(long)val);

        return TRUE;
    }

    return FALSE;
}
/***************************************************

  current bit:  0-do nothing
                1-highlight

****************************************************/
void CUGCheckBoxType::OnDraw(CDC *dc,RECT *rect,int col,long row,CUGCell *cell,
                            int selected,int current){


    DrawBorder(dc,rect,rect,cell);

    int right = rect->right;
    int left = rect->left;
    int top,height;
    RECT checkrect;
    CPen * oldpen;

    int style = 0;
    if(cell->IsPropertySet(UGCELL_CELLTYPEEX_SET))
        style = cell->GetCellTypeEx();

    //find the checkbox co-ords
    height = rect->bottom - rect->top - 6;
    if(height >12)
        height = 12;
    top = (rect->bottom - rect->top - height) /2;

    checkrect.left = rect->left+3;
    checkrect.top = rect->top+top;
    checkrect.right = rect->left+3+height;
    checkrect.bottom = rect->top+top+height;

    //adjust the rectangle
    rect->right = rect->left+6+height;

    //*** draw the background ***
    if(selected || (current && m_ctrl->m_GI->m_currentCellMode&2) )
        dc->SetBkColor(cell->GetHBackColor());
    else
        dc->SetBkColor(cell->GetBackColor());
    dc->ExtTextOut(left,top,ETO_OPAQUE,rect,_T(""),0,NULL);

    //*** draw the checkbox ***
    if(height >=6){

    //draw a 3D Recessed check box
    if(style&UGCT_CHECKBOX3DRECESS){

        oldpen = (CPen*)dc->SelectObject((CPen*)&Darkpen);
        dc->MoveTo(checkrect.left,checkrect.bottom);
        dc->LineTo(checkrect.left,checkrect.top);
        dc->LineTo(checkrect.right,checkrect.top);
        dc->SelectObject(&Lightpen);
        dc->LineTo(checkrect.right,checkrect.bottom);
        dc->LineTo(checkrect.left,checkrect.bottom);
        checkrect.top++;
        checkrect.left++;
        checkrect.right--;
        checkrect.bottom--;
        dc->SelectObject(&Facepen);
        dc->MoveTo(checkrect.left,checkrect.bottom);
        dc->LineTo(checkrect.right,checkrect.bottom);
        dc->LineTo(checkrect.right,checkrect.top);
        dc->SelectObject(GetStockObject(BLACK_PEN));
        dc->LineTo(checkrect.left,checkrect.top);
        dc->LineTo(checkrect.left,checkrect.bottom);
        dc->SelectObject(oldpen);

        checkrect.top++;
        checkrect.left++;
        dc->SetBkColor(GetSysColor(COLOR_WINDOW));
        dc->ExtTextOut(left,top,ETO_OPAQUE,&checkrect,_T(""),0,NULL);
    }
    //draw a 3D Raised check box
    else if(style&UGCT_CHECKBOX3DRAISED){

        oldpen = (CPen*)dc->SelectObject((CPen*)&Lightpen);
        dc->MoveTo(checkrect.left,checkrect.bottom);
        dc->LineTo(checkrect.left,checkrect.top);
        dc->LineTo(checkrect.right,checkrect.top);
        dc->SelectObject(GetStockObject(BLACK_PEN));
        dc->LineTo(checkrect.right,checkrect.bottom);
        dc->LineTo(checkrect.left,checkrect.bottom);
        checkrect.top++;
        checkrect.left++;
        checkrect.right--;
        checkrect.bottom--;
        dc->SelectObject(&Darkpen);
        dc->MoveTo(checkrect.left,checkrect.bottom);
        dc->LineTo(checkrect.right,checkrect.bottom);
        dc->LineTo(checkrect.right,checkrect.top);
        dc->SelectObject(&Facepen);
        dc->LineTo(checkrect.left,checkrect.top);
        dc->LineTo(checkrect.left,checkrect.bottom);
        dc->SelectObject(oldpen);

        checkrect.top++;
        checkrect.left++;
        dc->SetBkColor(GetSysColor(COLOR_WINDOW));
        dc->ExtTextOut(left,top,ETO_OPAQUE,&checkrect,_T(""),0,NULL);
    }
    //draw a plain check box
    else{

        oldpen = (CPen*)dc->SelectObject((CPen*)CPen::FromHandle((HPEN)GetStockObject(BLACK_PEN)));
        dc->MoveTo(checkrect.left,checkrect.top);
        dc->LineTo(checkrect.right,checkrect.top);
        dc->LineTo(checkrect.right,checkrect.bottom);
        dc->LineTo(checkrect.left,checkrect.bottom);
        dc->LineTo(checkrect.left,checkrect.top);
        dc->SelectObject(oldpen);

        dc->SetBkColor(GetSysColor(COLOR_WINDOW));
        checkrect.left++;
        checkrect.top++;
        dc->ExtTextOut(left,top,ETO_OPAQUE,&checkrect,_T(""),0,NULL);
    }

    //draw the check
    if(cell->GetNumber() >0 ){
        //draw a check mark
        if(style&UGCT_CHECKBOXCHECKMARK){
            oldpen = (CPen*)dc->SelectObject((CPen*)CPen::FromHandle((HPEN)GetStockObject(BLACK_PEN)));
            dc->MoveTo(checkrect.left+2,checkrect.bottom-4);
            dc->LineTo(checkrect.left+4,checkrect.bottom-2);
            dc->LineTo(checkrect.right+3,checkrect.top-1);
            if(height >9){
                dc->MoveTo(checkrect.left+2,checkrect.bottom-5);
                dc->LineTo(checkrect.left+4,checkrect.bottom-3);
                dc->LineTo(checkrect.right+3,checkrect.top-2);
                dc->MoveTo(checkrect.left+5,checkrect.bottom-2);
                dc->LineTo(checkrect.right+4,checkrect.top-1);
                dc->MoveTo(checkrect.left+2,checkrect.bottom-6);
                dc->LineTo(checkrect.left+5,checkrect.bottom-3);
            }
            dc->SelectObject(oldpen);
        }
        //draw an X
        else{
            oldpen = (CPen*)dc->SelectObject((CPen*)CPen::FromHandle((HPEN)GetStockObject(BLACK_PEN)));
            checkrect.left++;
            checkrect.top++;
            checkrect.right-=2;
            checkrect.bottom-=2;
            dc->MoveTo(checkrect.left,checkrect.top);
            dc->LineTo(checkrect.right+1,checkrect.bottom+1);
            dc->MoveTo(checkrect.left,checkrect.bottom);
            dc->LineTo(checkrect.right+1,checkrect.top-1);
            if(height >9){
                dc->MoveTo(checkrect.left+1,checkrect.top);
                dc->LineTo(checkrect.right+1,checkrect.bottom);
                dc->MoveTo(checkrect.left,checkrect.bottom-1);
                dc->LineTo(checkrect.right,checkrect.top-1);
                dc->MoveTo(checkrect.left,checkrect.top+1);
                dc->LineTo(checkrect.right,checkrect.bottom+1);
                dc->MoveTo(checkrect.left+1,checkrect.bottom);
                dc->LineTo(checkrect.right+1,checkrect.top);
            }
            dc->SelectObject(oldpen);
        }
    }
    }

    //draw the text using the default drawing routine
    rect->left += (6+height);
    rect->right = right;
    m_drawLabelText = TRUE;

    CUGCellType::DrawText(dc,rect,0,col,row,cell,selected,current);

    rect->left = left;
}
