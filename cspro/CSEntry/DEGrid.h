#pragma once

// DEGrid.h: interface for the CDEGrid class.
//
//////////////////////////////////////////////////////////////////////

#include <zGrid2O/GridWnd.h>


class CDEGrid : public CGridWnd
{
    friend class CEntryrunView;
public:
    CDEGrid(CDERoster* pRoster);
    virtual ~CDEGrid();

    virtual void OnSetup();

private:
    CDERoster*  m_pRoster;             //The roster object associated to the grid
    int         m_iCurrentOccs;

public:
    int OnEditStart(int col, long row,CWnd **edit); //This may not be needed .Evaluate.
    int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);

    virtual bool OnCanSizeCol(int iCol) const;
    virtual bool OnCanSizeRow(int iRow) const;

    virtual void OnLClicked(int iRow, int iCol);

    virtual bool OnCanSelectCol(int iCol) const;
    virtual bool OnCanSelectRow(int iRow) const;
    virtual bool OnCanSelectCell(int iRow, int iCol) const;
    virtual void OnCellField_LClicked(CDEField* pFld, int iOcc);

    virtual void StartEdit(CDEField* pFld , int iOcc);
    void SetGridData(CRunAplEntry* pApl);

    virtual bool OnCanSwapCol(void) const;
    virtual bool OnCanSwapRow(void) const;
    virtual bool OnCanDeleteCol(int iCol) const;
    virtual bool OnCanDeleteRow(int iRow) const;
    virtual bool OnCanSelectField(int iRow, int iCol, int iFld) const;
    virtual bool OnCanSelectBox(int iRow, int iCol, int iBox) const;
    virtual bool OnCanDrawBox(int iRow, int iCol);
    virtual bool OnCanSelectText(int irow, int iCol, int iTxt) const;
    virtual bool OnCanDrawText(int iRow, int iCol);

    void Pad(CString& sString ,CDEField* pField);
    CString GetDecimalPart(const CString& sString , CDEField* pField);
    CString GetNonDecimalPart(const CString& sString,CDEField* pField);
    CString MakeDecNumString(const CString& sNonDecimal, const CString& sDecimal,CDEField* pField);

    BOOL ShowField(CDEField* pField , int iOcc);
    void ResetGridData(bool bForceReset = false);
    bool m_bRedraw; //for smooth redraw when "GoToField" and "Ensure visible"is called times on large grids

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};
