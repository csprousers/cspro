#pragma once

// OccGrid.h: interface for the COccGrid class.
//
//////////////////////////////////////////////////////////////////////

#include <zGridO/Ugmedit.h>

class CDDDoc;


class COccGrid : public CUGCtrl
{
private:
    CFont m_font;
    bool m_bCanEdit;

public:
    int m_iRows;
    int m_iCols;
    CUGMaskedEdit m_LabelEdit;
    CStringArray m_Labels;
    CDDDoc* m_pDoc;

public:
    COccGrid();
    virtual ~COccGrid() { }

    void ResetGrid();

    //***** Over-ridable Notify Functions *****
    void OnSetup() override;
    void OnSheetSetup(int sheetNumber) override;

    //mouse and key strokes
    void OnLClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed) override;
    void OnRClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed) override;

    void OnCharDown(UINT* vcKey,BOOL processed) override;

    //cell type notifications
    int OnCellTypeNotify(long ID, int col, long row, long msg, long param) override;

    //editing
    int OnEditStart(int col, long row,CWnd **edit) override;
    int OnEditVerify(int col,long row,CWnd *edit,UINT* vcKey) override;
    int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag) override;
    int OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow) override;

    //hints
    int OnHint(int col,long row,int section,CString *string) override;
    int OnVScrollHint(long row,CString *string) override;
    int OnHScrollHint(int col,CString *string) override;

    void EditCopy(bool bCut);
    bool IsClipValidForPaste() const;

    //focus rect setup
    void OnDrawFocusRect(CDC *dc,RECT* rect) override;
    void OnRowChange(long oldrow,long newrow) override;

    //{{AFX_MSG(CItemGrid)
    afx_msg void OnEditCut();
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};
