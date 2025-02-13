#pragma once

// RelGrid.h: interface for the CRelGrid class.
//
//////////////////////////////////////////////////////////////////////

#include <zDictF/DDGrid.h>

class CDDComboBox;
class CNameEdit;

#define REL_OCC _T("(Occ)")


class CRelGrid : public CDDGrid
{
public:
    CRelGrid();

    void Update();

    int GetCurrentRelation(int row);

    //Row edits (derived from CDDGrid)
    void EditBegin(int col, long row, UINT vcKey) override;
    void EditChange(UINT uChar) override;
    bool EditEnd(bool bSilent = false) override;
    void EditQuit() override;
    void EditContinue() override;

    void Resize(CRect /*rect*/) override { }

    afx_msg void OnEditAdd();
    afx_msg void OnEditInsert();
    afx_msg void OnEditDelete();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnEditModify();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

    //***** Over-ridable Notify Functions *****
    void OnSetup() override;

    //movement and sizing
    void OnColSized(int col, int* width) override;

    //mouse and key strokes
    void OnRClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed) override;
    void OnLClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed) override;
    void OnDClicked(int col, long row, RECT* rect, POINT* point, BOOL processed) override;
    void OnKeyDown(UINT* vcKey, BOOL processed) override;

public:
    std::vector<DictRelation> m_dictRelations;

private:
    CDDComboBox* m_pOccEdit;
    CNameEdit* m_pNameEdit;
    int m_iRel;         // Relation of the current row
    int m_iRelPart;     // Relation part of the current row
};
