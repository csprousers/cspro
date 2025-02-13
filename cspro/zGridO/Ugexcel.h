#pragma once
/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGExcelCtrl
    Purpose
        This class is derived from the grid's main
        base class CUGCtrl.
        This class is setup to give the grid an
        Excel look and feel without the need for
        programming it in.
        Derive from this class if an Excel look
        and feel is required.
    Details
        -This class uses an excel style focus rectangle,
         multiple selection, editing, tabs with multiple
         sheets and other excel like features.
        -This class still has all the functionality of
         the CUGCtrl class.
************************************************/
#include <zGridO/Ugctrl.h>

class CLASS_DECL_ZGRIDO CUGExcelCtrl:public CUGCtrl
{

public:
    CFont m_font;

    CUGExcelCtrl();
    ~CUGExcelCtrl();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGExcelCtrl)
    //}}AFX_VIRTUAL


    //{{AFX_MSG(CUGExcelCtrl)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


    //***** Over-ridable Notify Functions *****
    virtual void OnSetup();

    //movement and sizing
    virtual int  OnCanMove(int oldcol,long oldrow,int newcol,long newrow);
    virtual int  OnCanViewMove(int oldcol,long oldrow,int newcol,long newrow);
    virtual void OnHitBottom(long numrows,long rowspast,long rowsfound);
    virtual void OnHitTop(long numrows,long rowspast);

    virtual int  OnCanSizeCol(int col);
    virtual void OnColSizing(int col,int *width);
    virtual void OnColSized(int col,int *width);
    virtual int  OnCanSizeRow(long row);
    virtual void OnRowSizing(long row,int *height);
    virtual void OnRowSized(long row,int *height);

    virtual int  OnCanSizeTopHdg();
    virtual int  OnCanSizeSideHdg();
    virtual int  OnTopHdgSizing(int *height);
    virtual int  OnSideHdgSizing(int *width);

    virtual void OnColChange(int oldcol,int newcol);
    virtual void OnRowChange(long oldrow,long newrow);
    virtual void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);
    virtual void OnLeftColChange(int oldcol,int newcol);
    virtual void OnTopRowChange(long oldrow,long newrow);

    //mouse and key strokes
    virtual void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
    virtual void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
    virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,int processed);
    virtual void OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed);
    virtual void OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed);
    virtual void OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);
    virtual void OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed);
    virtual void OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed);
    virtual void OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);
    virtual void OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed);
    virtual void OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed);
    virtual void OnCB_DClicked(RECT *rect,POINT *point,BOOL processed);

    virtual void OnKeyDown(UINT *vcKey,int processed);
    virtual void OnCharDown(UINT *vcKey,int processed);


    //GetCellIndirect notification
    virtual void OnGetCell(int col,long row,CUGCell *cell);
    //SetCell notification
    virtual void OnSetCell(int col,long row,CUGCell *cell);

    //data source notifications
    virtual void OnDataSourceNotify(int ID,long msg,long param);

    //cell type notifications
    virtual int OnCellTypeNotify(long ID,int col,long row,long msg,long param);

    //editing
    virtual int OnEditStart(int col, long row,CWnd **edit);
    virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
    virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);
    virtual int OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow);

    //menu notifications
    virtual void OnMenuCommand(int col,long row,int section,int item);
    virtual int  OnMenuStart(int col,long row,int section);

    //hints
    //virtual int OnHint(int col,long row,CUGHint *hint);
    //virtual int OnVScrollHint(long row,int percent,CUGHint *hint);
    //virtual int OnHScrollHint(int col,int percent,CUGHint *hint);



    //drag and drop
    #ifdef UG_ENABLE_DRAGDROP
        virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject);
        virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject,int col,long row);
        virtual DROPEFFECT OnDragDrop(COleDataObject* pDataObject,int col,long row);
        virtual void OnDragLeave(CWnd* pWnd);
        virtual DROPEFFECT OnDragScroll( CWnd* pWnd, DWORD dwKeyState, CPoint point );
    #endif

    // new member functions
    virtual void OnScreenDCSetup(CDC *dc,int section);

    virtual void OnTabSelected(int ID);

    virtual BOOL OnColSwapStart(int col);

    void OnDrawFocusRect(CDC *dc,RECT *rect);
};
