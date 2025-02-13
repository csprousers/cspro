#pragma once
//***************************************************************************
//  File name: AplFileAssociationsGrid.h
//
//  Description:
//       Definition table grid
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1998        gsf       created from MyCug.h
//
//***************************************************************************

#include <zGridO/Ugctelps.h>


/////////////////////////////////////////////////////////////////////////////
//
//                             CAplFileAssociationsGrid
//
/////////////////////////////////////////////////////////////////////////////
class CAplFileAssociationsGrid : public CUGCtrl
{
private:
    CUGEllipsisType m_Ellipsis;
    CFont           m_font;
public:
    CAplFileAssociationsGrid();
    ~CAplFileAssociationsGrid();

    int             m_iRows;
    int             m_iCols;
    int             m_iEllipsisIndex;
    CUGMaskedEdit   m_pPifEdit;


    void MyClear(void);
    void ResetGrid(void);

    void ForceButtonClick(int rowNum) { OnCellTypeNotify(0,0,rowNum,UGCT_ELLIPSISBUTTONCLICK,0); } // GHM 20110805

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
    //}}AFX_VIRTUAL


    //{{AFX_MSG(CAplFileAssociationsGrid)
    // NOTE - the ClassWizard will add and remove member functions here.
    //    DO NOT EDIT what you see in these blocks of generated code!

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


    //***** Over-ridable Notify Functions *****
    virtual void OnSetup();
    virtual void OnSheetSetup(int sheetNumber);

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
    virtual int  OnTopHdgSized(int *height);
    virtual int  OnSideHdgSized(int *width);

    virtual void OnColChange(int oldcol,int newcol);
    virtual void OnRowChange(long oldrow,long newrow);
    virtual void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);
    virtual void OnLeftColChange(int oldcol,int newcol);
    virtual void OnTopRowChange(long oldrow,long newrow);

    //mouse and key strokes
    virtual void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
    virtual void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
    virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,int processed);
    virtual void OnMouseMove(int col,long row,POINT *point,UINT nFlags,BOOL processed=0);
    virtual void OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed=0);
    virtual void OnCB_DClicked(RECT *rect,POINT *point,BOOL processed=0);

    virtual void OnKeyDown(UINT *vcKey,BOOL processed);
    virtual void OnCharDown(UINT *vcKey,BOOL processed);


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
    virtual int OnHint(int col,long row,int section,CString *string);
    virtual int OnVScrollHint(long row,CString *string);
    virtual int OnHScrollHint(int col,CString *string);


    //find row/col item
    CString GetRowColString(int iRow,int iCol);


    //DC setup
    virtual void OnScreenDCSetup(CDC *dc,int section);

    virtual void OnAdjustComponentSizes(RECT *grid,RECT *topHdg,RECT *sideHdg,RECT *cnrBtn,
        RECT *vScroll,RECT *hScroll,RECT *tabs);

    //focus rect setup
    virtual void OnDrawFocusRect(CDC *dc,RECT *rect);
    virtual void OnSetFocus(int section);
    virtual void OnKillFocus(int section);

    //column swapping
    virtual BOOL OnColSwapStart(int col);
    virtual BOOL OnCanColSwap(int fromCol,int toCol);
};
