#pragma once

//***************************************************************************
//  File name: LevGrid.h
//
//  Description:
//       Header for Data Dictionary level properties grid application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

class CDDComboBox;
class CNumEdit;


class CLevelGrid : public CDDGrid
{
public:
    CLevelGrid();

    int GetLevel() const    { return m_iLevel; }
    int GetFirstRow() const { return m_iFirstRow; }

    void Size(CRect rect);
    void Resize(CRect rect) override;
    void Update(CDataDict* pDict, int level);
    void Update();

    void OnCharDown(UINT* vcKey, BOOL processed) override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnEditAdd();
    afx_msg void OnEditDelete();
    afx_msg void OnEditInsert();
    afx_msg void OnEditCut();
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    afx_msg void OnEditUndo();
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

    //***** Row edits (derived from CDDGrid)
    void EditBegin(int col, long row, UINT vcKey) override;
    bool EditEnd(bool bSilent = false) override;
    void EditQuit() override;

private:
    std::vector<size_t> GetSelectedRecords();

    void EditCopy(bool bCut);
    void EditDelete(const std::vector<size_t>& selected_records);

private:
    int m_iLevel;
    long m_iFirstRow;

    CLabelEdit* m_pLabelEdit;
    CNameEdit* m_pNameEdit;
    CLabelEdit* m_pTypeEdit;
    CDDComboBox* m_pReqEdit;
    CNumEdit* m_pMaxEdit;
};
