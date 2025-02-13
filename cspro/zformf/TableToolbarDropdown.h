#pragma once
class TableToolbarButton;

class TableToolbarDropdown : public CWnd
{
public:

    TableToolbarDropdown();

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);

    bool Create(TableToolbarButton* parent);

protected:
    CSize CalcBoxesSize(int rows, int columns) const;
    void DrawBoxes(CDC& dc);
    void DrawDimensionText(CDC &dc);
    int GetMargin() const;
    CSize GetBoxSize() const;

    int m_selected_rows;
    int m_selected_columns;
    int m_text_height;
    TableToolbarButton* m_parent;

    DECLARE_MESSAGE_MAP()
};

