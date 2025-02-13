#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGCheckBoxType
    Purpose
        checkbox cell type class
    Details
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

#include <zGridO/zGridO.h>
#include <zGridO/Ugceltyp.h>
#include <zGridO/Ugcell.h>

class CLASS_DECL_ZGRIDO CUGCheckBoxType: public CUGCellType{

    CPen    Darkpen;
    CPen    Lightpen;
    CPen    Facepen;

    CUGCell m_cell;

public:

    CUGCheckBoxType();
    ~CUGCheckBoxType();

    virtual LPCTSTR GetName();
    virtual LPCUGID GetUGID();

    virtual BOOL OnLClicked(int col,long row,int updn,RECT *rect,POINT *point);
    virtual BOOL OnDClicked(int col,long row,RECT *rect,POINT *point);
    virtual BOOL OnCharDown(int col,long row,UINT *vcKey);
    virtual void OnDraw(CDC *dc,RECT *rect,int col,long row,CUGCell *cell,
        int selected,int current);

};
