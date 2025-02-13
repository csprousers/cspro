#pragma once

#include <zGrid2O/GridHit.h>
#include <zGrid2O/GridWnd.h>


/////////////////////////////////////////////////////////////////////////////
// CFormGrid window

class CFormGrid : public CGridWnd
{
public:
    CFormGrid(CDERoster* pRoster);

    CFormScrollView* GetFormView();
    CFormDoc* GetFormDoc();

    CDERoster* GetRoster() { return m_pRoster; }

    void SetHitObj(CHitOb ho) { m_hitOb = ho; }

    void OnCell_LClicked(int iRow, int iCol) override;
    void OnCell_LClicked(const CPoint& pt) override;
    void OnTH_LClicked(int iCol) override;
    void OnSH_LClicked(int iRow) override;
    void OnCB_LClicked(const CPoint& pt) override;

    void OnTH_RClicked(int iCol) override;
    void OnSH_RClicked(int iRow) override;
    void OnCB_RClicked(const CPoint& pt) override;

    void OnCellField_LClicked(CDEField* pFld, int iOcc) override;
    void OnCellField_RClicked(CDEField* pFld, int iOcc) override;
    void OnCellText_LClicked(CHitOb& hitOb) override;
    void OnCellText_RClicked(CHitOb& hitOb) override;
    void OnCellBox_LClicked(CHitOb& hitOb) override;
    void OnCellBox_RClicked(CHitOb& hitOb) override;
    void OnCell_RClicked(int iRow, int iCol) override;

    bool OnCanSelectCol(int iCol) const override;
    bool OnCanSelectRow(int iRow) const override;
    bool OnCanSelectCell(int iRow, int iCol) const override;
    bool OnCanDrawBox(int iRow, int iCol) override;
    bool OnCanSwapCol() const override;
    bool OnCanSwapRow() const override;
    bool OnCanSizeCol(int iCol) const override;
    bool OnCanSizeRow(int iCol) const override;

    void OnSwappedCol(const std::vector<int>& fromIndices, int toIndex) override;
    void OnSwappedRow(const std::vector<int>& fromIndices, int toIndex) override;
    void OnResizedCol(int iCol) override;
    void OnResizedRow(int iRow) override;

    void OnAddBox(int iRow, int iCol, const CRect& rc) override;
    void DrawBox(int iRow, int iCol, const CPoint& pt) override;
    void OnBoxMoved(int iRow, int iCol, int iBox) override;
    void OnTextMoved(int iRow, int iCol, int iTxt) override;
    void OnFieldMoved(int iRow, int iCol, int iFld) override;

    void OnKeyDown(UINT* piKey, bool bProcessed) override;

    bool OnEditColStubProperties(CArray<CDEText*, CDEText*>& apTxt);

    bool CanAlign(const std::variant<HorizontalAlignment, VerticalAlignment>& alignment) const;
    void AlignFields(const std::variant<HorizontalAlignment, VerticalAlignment>& alignment);

    void OnEditMultipleFieldProperties();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnEditTextProp();
    afx_msg void OnAddText();
    afx_msg void OnEditBoxProp();
    afx_msg void OnEditFieldProp();
    afx_msg void OnEditColumnProp();
    afx_msg void OnEditStubProp();
    afx_msg void OnAddForm();
    afx_msg void OnEditGridProp();
    afx_msg void OnEditGridAutoFit();
    afx_msg void OnViewLogic();
    afx_msg void OnQSFEditor();
    afx_msg void OnDeleteItem();
    afx_msg void OnDeleteColumn();
    afx_msg void OnShowBoxToolbar();
    afx_msg void OnEditJoin();
    afx_msg void OnEditSplit();
    afx_msg void OnLayoutAlign(UINT nID);
    afx_msg void OnEditPaste();
    afx_msg void OnEditCopy();

private:
    void DeselectScrollViewItems();
    void OnSwappedRowOrCol(const std::vector<int>& fromIndices, int toIndex);
    void AlignCell(int row, int column, const std::variant<HorizontalAlignment, VerticalAlignment>& alignment);


private:
    CDERoster* m_pRoster;  // The roster object associated to the grid
    CHitOb m_hitOb;        // for coordinating action on clicked objects (boxes, text, fields...)
};
