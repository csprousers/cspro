#pragma once

//***************************************************************************
//  File name: ItemGrid.h
//
//  Description:
//       Header for Data Dictionary item properties grid application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <zGridO/Ugctelps.h>

class CFromToEdit;

struct VALUEKEY
{
    int vset;
    int value;
    int vpair;
};


class CItemGrid : public CDDGrid
{
    friend class CDictPropertyGridCtrl;

public:
    CItemGrid();

    int GetLevel()  const        { return m_iLevel; }
    int GetRecord() const        { return m_iRec; }
    int GetItem() const          { return m_iItem; }
    int GetVSet(long row)  const { return m_aValue[row].vset; }
    int GetValue(long row) const { return m_aValue[row].value; }
    int GetVPair(long row) const { return m_aValue[row].vpair; }
    int GetRow(int iVSet, int iValue);
    bool HasVSets() const        { return !m_aValue.IsEmpty(); }

    void Size(CRect rect);
    void Resize(CRect rect) override;
    void Update(CDataDict* pDict, int level, int rec, int item, int vset);
    void Update();

    static bool IsClipboardValidForValueSetPaste(const CDDDoc& dictionary_doc);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnEditUndo();
    afx_msg void OnEditCut();
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    afx_msg void OnEditAdd();
    afx_msg void OnEditInsert();
    afx_msg void OnEditDelete();
    afx_msg void OnEditModify();
    afx_msg void OnEditNotes();
    afx_msg void OnShiftF10();
    afx_msg void OnPasteValueSetLink();
    afx_msg void OnRemoveValueSetLink(UINT nID);
    afx_msg void OnMergeValues();
    afx_msg void OnFormatValueLabels(UINT nID);
    afx_msg void OnValuesReplaceValueLabels();
    afx_msg void OnEditGenValueSet();
    afx_msg void OnMakeFirstValueSet();

    //***** Over-ridable Notify Functions *****
    void OnSetup() override;

    //movement and sizing
    int OnCanMove(int oldcol, long oldrow, int newcol, long newrow) override;
    void OnColSized(int col, int* width) override;

    //mouse and key strokes
    void OnLClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed) override;
    void OnRClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed) override;
    void OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed = 0) override;
    void OnCB_RClicked(int updn, RECT* rect, POINT* point, BOOL processed = 0) override;
    void OnKeyDown(UINT* vcKey, BOOL processed) override;
    void OnCharDown(UINT* vcKey, BOOL processed) override;

    //cell type notifications
    int OnCellTypeNotify(long ID, int col, long row, long msg, long param) override;

    //focus rect setup
    void OnSetFocus(int section) override;
    void OnKillFocus(int section) override;

    //***** Row edits (derived from CDDGrid)
    void EditBegin(int col, long row, UINT vcKey) override;
    bool EditEnd(bool bSilent = false) override;
    void EditQuit() override;

private:
    void EditCopy(bool bCut);
    void EditDelete();

private:
    int m_iLevel;
    int m_iRec;
    int m_iItem;
    int m_iVSet;

    CLabelEdit* m_pSetLabelEdit;
    CNameEdit* m_pSetNameEdit;
    CLabelEdit* m_pLabelEdit;
    CFromToEdit* m_pFromEdit;
    CFromToEdit* m_pToEdit;
    CDDComboBox* m_pSpecialEdit;

    CArray<VALUEKEY,VALUEKEY> m_aValue;
};
