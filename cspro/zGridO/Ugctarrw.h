#pragma once

#include <zGridO/zGridO.h>
#include <zGridO/Ugceltyp.h>


class CLASS_DECL_ZGRIDO CUGArrowType: public CUGCellType{

public:

    CUGArrowType();
    ~CUGArrowType();
    virtual void OnDraw(CDC *dc,RECT *rect,int col,long row,CUGCell *cell,
                        int selected,int current);

};
