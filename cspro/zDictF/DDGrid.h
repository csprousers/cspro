#pragma once
//***************************************************************************
//  File name: DDGrid.h
//
//  Description:
//       Header for base Data Dictionary grid application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <zGridO/UGCTbutn.h>
#include <zGridO/Ugctrl.h>

constexpr UINT DD_ONSEL = 136;


class CDDGrid : public CUGCtrl
{
public:
    CDDGrid();

public:
    // Setup
    void SetBitmaps(CBitmap* pNoteNo, CBitmap* pNoteYes, CBitmap* pNoteNoGrayed, CBitmap* pNoteYesGrayed);
    void SetGridFont(LOGFONT* plf) {m_plf = plf; m_font.CreateFontIndirect(plf);}
    void SetDict(CDataDict* pDict) {m_pDict = pDict;}

    // Settings
    void SetEditing(bool bEditing) { m_bEditing = bEditing; }
    void SetAdding(bool bAdding) { m_bAdding = bAdding; }
    void SetInserting(bool bInserting) { m_bInserting = bInserting; }

    // Status
    bool IsEditing() const   { return m_bEditing; }
    bool IsAdding() const    { return m_bAdding; }
    bool IsInserting() const { return m_bInserting; }

    // Editing
    virtual void EditBegin(int col, long row, UINT vcKey) = 0;
    virtual void EditChange(UINT uChar);
    virtual void EditContinue();
    virtual bool EditEnd(bool bSilent = false) = 0;
    virtual void EditQuit() = 0;

    // Other
    virtual void Resize(CRect rect) = 0;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

    // Movement and sizing (derived from CUGCtrl)
    int OnCanSizeTopHdg() override;
    int OnCanSizeCol(int col) override;
    void OnColSizing(int col, int* width) override;
    void OnRowChange(long oldrow, long newrow) override;

public:
    int m_iEditRow;
    int m_iEditCol;
    int m_iMaxCol;
    int m_iMinCol;

    CArray<CWnd*, CWnd*> m_aEditControl;

protected:
    bool m_bCanEdit;
    bool m_bEditing;
    bool m_bAdding;
    bool m_bInserting;
    bool m_bValueSet;
    CUGButtonType m_button;
    int m_iButton;

    CDataDict* m_pDict;

    CBitmap* m_pNoteNo;
    CBitmap* m_pNoteYes;
    CBitmap* m_pNoteNoGrayed;
    CBitmap* m_pNoteYesGrayed;
    LOGFONT* m_plf;
    CFont m_font;
};
