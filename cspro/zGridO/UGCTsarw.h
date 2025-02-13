#pragma once

/**********************************************************
Ultimate Grid for MFC  - Sort Arrow Cell Type Add-on


CellTypeEx:
    none

Notifications (OnCellTypeNotify)
    UGCT_SORTARROWUP - sent when the up arrow is pressed
    UGCT_SORTARROWSOWN - sent when the down arrow is pressed

***********************************************************/

#include <zGridO/zGridO.h>
#include <zGridO/Ugceltyp.h>

//celltype notifications
#define UGCT_SORTARROWUP    16
#define UGCT_SORTARROWDOWN  32

class CLASS_DECL_ZGRIDO CUGSortArrowType: public CUGCellType{

    int m_ArrowWidth;

public:

    CUGSortArrowType();
    ~CUGSortArrowType();
    virtual void OnDraw(CDC *dc,RECT *rect,int col,long row,CUGCell *cell,int selected,int current);
};
