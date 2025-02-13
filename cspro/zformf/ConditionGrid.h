#pragma once

//***************************************************************************
//
//  Description:
//       Header for Data Dictionary record properties grid application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   bmd     Created for CSPro 2.0
//
//***************************************************************************

#include <zGridO/Ugctrl.h>

class CLabelEdit1;


class CCondGrid : public CUGCtrl
{
private:

    int                 m_iOldStart;
    int                 m_iOldLen;

    CLabelEdit1* m_pLabelEdit;

    bool                m_bCanMove;


protected:
    bool            m_bCanEdit;
    bool            m_bEditing;
    bool            m_bAdding;
    bool            m_bInserting;

    LOGFONT* m_plf;
    CFont           m_font;
public:
    int         m_iEditRow;

public:

    friend class CQSFEView;
    void Size(CRect rect);
    void Resize(CRect rect);
    void EditChange(UINT uChar, bool bSilent = false);

    void SetGridFont(LOGFONT* plf) { m_plf = plf; m_font.CreateFontIndirect(plf); }

    void OnUpdate();

    bool IsEditing(void) { return m_bEditing; }
    void EditContinue(void);
    void ResetGrid();

    int IsDuplicate(const CString& qsfCond, int iIgnoreRow = -1);

    void Cut();
    void Copy();
    void Paste();
    void Undo();

private:
    void UpdateGrid();

public:
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCondGrid)
    //}}AFX_VIRTUAL

    //{{AFX_MSG(CCondGrid)
    afx_msg void OnEditAdd();
    afx_msg void OnEditInsert();
    afx_msg void OnEditDelete();
    afx_msg void OnEditModify();
    afx_msg void OnSetFocus(CWnd* pOldWnd);

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


    //***** Over-ridable Notify Functions *****
    virtual void OnSetup();

    //movement and sizing
    virtual void OnColSized(int col, int* width);
    virtual void OnRowChange(long oldrow, long newrow);



    //mouse and key strokes
    virtual void OnLClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed);
    virtual void OnRClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed);
    virtual void OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed = 0);
    virtual void OnCB_RClicked(int updn, RECT* rect, POINT* point, BOOL processed = 0);
    virtual void OnDClicked(int col, long row, RECT* rect, POINT* point, BOOL processed);
    virtual void OnKeyDown(UINT* vcKey, BOOL processed);

    //cell type notifications
    virtual int OnCellTypeNotify(long ID, int col, long row, long msg, long param);

    //focus rect setup
    virtual void OnKillFocus(int section);

    //Row edits (derived from CDDGrid)
    virtual void EditBegin(int col, long row, UINT vcKey);
    virtual bool EditEnd(bool bSilent = false);
    virtual void EditQuit(void);

    // Movement and sizing (derived from CUGCtrl)
    virtual int  OnCanSizeTopHdg();
    virtual int  OnCanSizeSideHdg();
    virtual int  OnCanSizeCol(int col);
    virtual void OnColSizing(int col, int* width);

    virtual int  OnCanSizeRow(long row);

public:
    virtual void OnCharDown(UINT* vcKey, BOOL processed);
};
