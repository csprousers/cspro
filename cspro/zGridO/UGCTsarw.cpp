/***************************************************
Ultimate Grid 3.0 for MFC
Dundas Software Ltd. 1994-1996

    Sort Arrow CellType
****************************************************/

#include "StdAfx.h"
#include "UGCTsarw.h"
#include "Ugcell.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/***************************************************
****************************************************/
CUGSortArrowType::CUGSortArrowType(){
    m_ArrowWidth = GetSystemMetrics(SM_CXVSCROLL);
}

/***************************************************
****************************************************/
CUGSortArrowType::~CUGSortArrowType(){

}
/***************************************************
****************************************************/
void CUGSortArrowType::OnDraw(CDC *dc,RECT *rect,int col,long row,
                             CUGCell *cell,int selected,int current){


    DrawBorder(dc,rect,rect,cell);


    long cellTypeEx = 0;
    if(cell->IsPropertySet(UGCELL_CELLTYPEEX_SET))
        cellTypeEx = cell->GetCellTypeEx();

    int oldleft = rect->left;
    rect->left = rect->right - m_ArrowWidth;

    dc->SetBkColor(cell->GetBackColor());
    dc->ExtTextOut(rect->left,rect->top,ETO_OPAQUE,rect,_T(""),0,NULL);

    int inc = m_ArrowWidth/4;
    if(inc >= (rect->bottom - rect->top))
        inc = rect->bottom - rect->top -1;
    int top = rect->top + (rect->bottom - rect->top)/2 - inc;



    if(cellTypeEx&UGCT_SORTARROWUP){
        dc->SelectObject((CPen*)CPen::FromHandle((HPEN)GetStockObject(WHITE_PEN)));
        dc->MoveTo(rect->left+inc,top+inc+inc);
        dc->LineTo(rect->right-inc,top+inc+inc);
        dc->LineTo(rect->left+inc+inc,top);
        dc->SelectObject(GetStockObject(BLACK_PEN));
        dc->LineTo(rect->left+inc,top+inc+inc);
    }
    else{
        dc->SelectObject((CPen*)CPen::FromHandle((HPEN)GetStockObject(BLACK_PEN)));
        dc->MoveTo(rect->right-inc,top);
        dc->LineTo(rect->left+inc,top);
        dc->LineTo(rect->left+inc+inc,top+inc+inc);
        dc->SelectObject((CPen*)CPen::FromHandle((HPEN)GetStockObject(WHITE_PEN)));
        dc->LineTo(rect->right-inc,top);
        dc->SelectObject((CPen*)CPen::FromHandle((HPEN)GetStockObject(BLACK_PEN)));
    }

    rect->right = rect->left;
    rect->left = oldleft;

    //draw the text in using the default drawing routine
    DrawText(dc,rect,0,col,row,cell,selected,current);
}
