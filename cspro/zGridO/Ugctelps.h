#pragma once
/**********************************************************
Ultimate Grid for MFC  - Ellipsis Button Cell Type Add-on


CellTypeEx:
    none

Notifications (OnCellTypeNotify)
    UGCT_ELLIPSISBUTTONCLICK - sent when the ellipsis button
        is clicked

***********************************************************/

#include <zGridO/Ugceltyp.h>

//celltype notification
#define UGCT_ELLIPSISBUTTONCLICK    100

class CLASS_DECL_ZGRIDO CUGEllipsisType: public CUGCellType{

    int     m_btnWidth;
    int     m_btnDown;
    long    m_btnRow;
    int     m_btnCol;
    RECT    m_btnRect;

    CPen    m_pen;
    CBrush  m_brush;

public:

    CUGEllipsisType();
    ~CUGEllipsisType();

    virtual int GetEditArea(RECT *rect);

    virtual BOOL OnLClicked(int col,long row,int updn,RECT *rect,POINT *point);
    virtual BOOL OnDClicked(int col,long row,RECT *rect,POINT *point);
    virtual BOOL OnMouseMove(int col,long row,POINT *point,UINT flags);

    virtual void OnDraw(CDC *dc,RECT *rect,int col,long row,
        CUGCell *cell,int selected,int current);

};
