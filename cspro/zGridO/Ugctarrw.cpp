/***************************************************
Ultimate Grid 3.0 for MFC
Dundas Software Ltd. 1994-1996

  Arrow Cell Type

    CellTypeNumber:     UGCT_ARROW
    CellTypeExNumbers:  UGCT_ARROWLEFT      left pointing arrow
                        UGCT_ARROWRIGHT     right pointing arrow
                        UGCT_ARROWDLEFT     double left pointing arrow
                        UGCT_ARROWDRIGHT    double right pointing arrow

    Text color determines the color of the arrow
    back color determines the background color
    alignment is always centered

****************************************************/

#include "StdAfx.h"
#include "UGCTarrw.h"
#include "Ugcell.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/***************************************************
****************************************************/
CUGArrowType::CUGArrowType(){

    m_canTextEdit = FALSE;

}
/***************************************************
****************************************************/
CUGArrowType::~CUGArrowType(){
}
/***************************************************
****************************************************/
void CUGArrowType::OnDraw(CDC *dc,RECT *rect,int col,long row,CUGCell *cell,
                         int selected,int current){

    DrawBorder(dc,rect,rect,cell);

    CPen pen;
    CPen * oldpen;

    int style = UGCT_ARROWRIGHT;
    if(cell->IsPropertySet(UGCELL_CELLTYPEEX_SET))
        style = cell->GetCellTypeEx();

    //*** draw the background ***
    if(selected || current)
        dc->SetBkColor(cell->GetHBackColor());
    else
        dc->SetBkColor(cell->GetBackColor());

    dc->ExtTextOut(0,0,ETO_OPAQUE,rect,_T(""),0,NULL);

    //check the width of the drawing area
    if((rect->right - rect->left) <4)
        return;


    //set up the pen color to draw with
    pen.CreatePen(PS_SOLID,1,cell->GetTextColor());
    oldpen = (CPen *)dc->SelectObject(&pen);

    //get the arrow center co-ords
    int vcenter,hcenter;
    vcenter = rect->top + (rect->bottom - rect->top)/2;
    hcenter = rect->left + (rect->right - rect->left)/2;

    if(style&UGCT_ARROWRIGHT || style == 0){      // right arrow (default)
        dc->MoveTo(hcenter-1,vcenter-3);
        dc->LineTo(hcenter-1,vcenter+4);
        dc->MoveTo(hcenter,vcenter-2);
        dc->LineTo(hcenter,vcenter+2);
        dc->LineTo(hcenter+2,vcenter);
        dc->LineTo(hcenter+1,vcenter-1);
        dc->LineTo(hcenter+1,vcenter+1);
    }
    else if(style&UGCT_ARROWLEFT){  //left arrow
        dc->MoveTo(hcenter+1,vcenter-3);
        dc->LineTo(hcenter+1,vcenter+4);
        dc->MoveTo(hcenter,vcenter-2);
        dc->LineTo(hcenter,vcenter+2);
        dc->LineTo(hcenter-2,vcenter);
        dc->LineTo(hcenter-1,vcenter-1);
        dc->LineTo(hcenter-1,vcenter+1);
    }
    else if(style&UGCT_ARROWDRIGHT){      // double right arrow
        dc->MoveTo(hcenter+2,vcenter-3);
        dc->LineTo(hcenter+2,vcenter+4);
        dc->MoveTo(hcenter+3,vcenter-2);
        dc->LineTo(hcenter+3,vcenter+2);
        dc->LineTo(hcenter+5,vcenter);
        dc->LineTo(hcenter+4,vcenter-1);
        dc->LineTo(hcenter+4,vcenter+1);

        dc->MoveTo(hcenter-4,vcenter-3);
        dc->LineTo(hcenter-4,vcenter+4);
        dc->MoveTo(hcenter-3,vcenter-2);
        dc->LineTo(hcenter-3,vcenter+2);
        dc->LineTo(hcenter-1,vcenter);
        dc->LineTo(hcenter-2,vcenter-1);
        dc->LineTo(hcenter-2,vcenter+1);
    }
    else if(style&UGCT_ARROWDLEFT){  //double left arrow
        dc->MoveTo(hcenter+4,vcenter-3);
        dc->LineTo(hcenter+4,vcenter+4);
        dc->MoveTo(hcenter+3,vcenter-2);
        dc->LineTo(hcenter+3,vcenter+2);
        dc->LineTo(hcenter+1,vcenter);
        dc->LineTo(hcenter+2,vcenter-1);
        dc->LineTo(hcenter+2,vcenter+1);

        dc->MoveTo(hcenter-2,vcenter-3);
        dc->LineTo(hcenter-2,vcenter+4);
        dc->MoveTo(hcenter-3,vcenter-2);
        dc->LineTo(hcenter-3,vcenter+2);
        dc->LineTo(hcenter-5,vcenter);
        dc->LineTo(hcenter-4,vcenter-1);
        dc->LineTo(hcenter-4,vcenter+1);
    }

    dc->SelectObject(oldpen);
}
