#pragma once

//***************************************************************************
//  File name: RecGrid.h
//
//  Description:
//       Header for Data Dictionary record properties grid application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   bmd     Created for CSPro 2.0
//
//***************************************************************************

#define RECTYPE -2

struct ITEMSORT
{
    int level;
    int rec;
    int item;
    int start;
};


class CRecordGrid : public CDDGrid
{
public:
    CRecordGrid();

    int GetLevel() const          { return m_iLevel; }
    int GetRecord() const         { return m_iRec; }
    int GetLevel(long row) const  { return m_aItem[row].level; }
    int GetRecord(long row) const { return m_aItem[row].rec; }
    int GetItem(long row)   const { return m_aItem[row].item; }
    int GetRow(int iItem);

    void Size(CRect rect);
    void Resize(CRect rect) override;
    void Update(CDataDict* pDict, int level, int rec);
    void Update();

    void EditChange(UINT uChar) override;
    void OnCharDown(UINT* vcKey,BOOL processed) override;

    static int GetStartColumn();
    static int GetLabelColumn();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnEditAdd();
    afx_msg void OnEditInsert();
    afx_msg void OnEditDelete();
    afx_msg void OnEditCut();
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    afx_msg void OnEditUndo();
    afx_msg void OnEditMakeSubitems();
    afx_msg void OnEditFlattenOccurrences();
    afx_msg void OnEditModify();
    afx_msg void OnEditNotes();
    afx_msg void OnEditOccLabels();
    afx_msg void OnShiftF10();

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
    void OnDClicked(int col, long row, RECT* rect, POINT* point, BOOL processed) override;
    void OnKeyDown(UINT* vcKey, BOOL processed) override;

    //cell type notifications
    int OnCellTypeNotify(long ID, int col, long row, long msg, long param) override;

    //focus rect setup
    void OnSetFocus(int section) override;
    void OnKillFocus(int section) override;

    //Row edits (derived from CDDGrid)
    void EditBegin(int col, long row, UINT vcKey) override;
    bool EditEnd(bool bSilent = false) override;
    void EditQuit() override;

private:
    void SortItems();
    void UpdateRecTypeItem(int row);
    void UpdateRecItem(const CDictItem& dict_item, int row, COLORREF rgb);
    void CreateDataTypesCombo(long row, const CString& current_data_type);
    void CreateItemTypesCombo(long row, const CString& current_item_type);
    void CreateDecEdit(long row, const CString& current_dec);
    void CreateDecCharCombo(long row, const CString& current_dec_char);
    void CreateZeroFillCombo(long row, const CString& current_zero_fill);
    void CreateOccEdit(long row, const CString& current_occ);
    void CreateLenEdit(long row, const CString& current_len);
    void CreateStartPositionEdit(long row, const CString& current_start);
    void CreateLabelEdit(long row, const CString& current_label);
    void CreateNameEdit(long row, const CString& current_name);
    void UpdateEnabledEditControls();

private:
    std::vector<size_t> GetSelectedItems();
    bool GetSelectedItemsAndQueryForSubitems(std::vector<size_t>& selected_items, bool& subitems_too, const TCHAR* action_text);

    void EditCopy(bool bCut);
    void EditDelete(const std::vector<size_t>& selected_items, bool subitems_too);

private:
    int m_iLevel;
    int m_iRec;

    int m_iOldStart;
    int m_iOldLen;

    CLabelEdit* m_pLabelEdit;
    CNameEdit* m_pNameEdit;
    CNumEdit* m_pStartEdit;
    CNumEdit* m_pLenEdit;
    CDDComboBox* m_pDataTypeEdit;
    CDDComboBox* m_pItemTypeEdit;
    CNumEdit* m_pOccEdit;
    CNumEdit* m_pDecEdit;
    CDDComboBox* m_pDecCharEdit;
    CDDComboBox* m_pZeroFillEdit;

    CArray<ITEMSORT, ITEMSORT> m_aItem;
};
