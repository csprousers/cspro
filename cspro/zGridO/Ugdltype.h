#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGDropListType
    Purpose
    Details
************************************************/

#include <zGridO/zGridO.h>
#include <zGridO/Ugceltyp.h>

class CLASS_DECL_ZGRIDO CUGDropListType: public CUGCellType{

    int     m_btnWidth;
    int     m_btnDown;
    RECT    m_btnRect;
    int     m_btnCol;
    long    m_btnRow;

protected:

    CPen        m_pen;
    CBrush      m_brush;
    CUGLstBox*  m_listBox;

    int StartDropList();

public:

    CUGDropListType();
    ~CUGDropListType();

    virtual LPCTSTR GetName();
    virtual LPCUGID GetUGID();

    virtual int GetEditArea(RECT *rect);
    virtual BOOL OnLClicked(int col,long row,int updn,RECT *rect,POINT *point);
    virtual BOOL OnDClicked(int col,long row,RECT *rect,POINT *point);
    virtual BOOL OnKeyDown(int col,long row,UINT *vcKey);
    virtual BOOL OnCharDown(int col,long row,UINT *vcKey);
    virtual BOOL OnMouseMove(int col,long row,POINT *point,UINT flags);

    virtual void OnKillFocus(int col,long row,CUGCell *cell);

    virtual void OnDraw(CDC *dc,RECT *rect,int col,long row,CUGCell *cell,
        int selected,int current);
};
