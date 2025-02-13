#pragma once

#include <zGrid2O/zGrid2O.h>
#include <zGrid2O/GridCell.h>
#include <zGrid2O/GridTrak.h>
#include <zFormO/Roster.h>


enum GRID_HITTEST { NO_HIT,
                    HIT_CELL,
                    HIT_CELLFIELD,
                    HIT_CELLBOX,
                    HIT_CELLTEXT,
                    HIT_STUB,
                    HIT_HEADER,
                    HIT_CORNER,
                    BORDER_STUBH,
                    BORDER_STUBV,
                    BORDER_HEADERH,
                    BORDER_HEADERV,
                    BORDER_CELLH,
                    BORDER_CELLV,
                    BORDER_CORNERH,
                    BORDER_CORNERV,
                    TRACK_FIELD,
                    TRACK_BOX,
                    TRACK_TEXT,
};

const int GRIDCELL_BORDER = 4;
const int HITTEST_BORDER = 8;

/////////////////////////////////////////////////////////////////////////////
// CGridWnd window


class CLASS_DECL_ZGRID2O CGridWnd : public CWnd
{
// Construction
public:
    CGridWnd();

    BOOL Create(const CRect& rcPage, CWnd* pParentWnd, CDERoster* pRoster, bool bShrinkToFit=false);
    BOOL Create(const CPoint& ptOrigin, int iNumRows, int iNumCols, CWnd* pParentWnd, CDERoster* pRoster);

// Layout interface
public:
    void Resize(const CRect& rcNewPage);    // rectangle gives new coords: left,top,right,bottom
    void Resize(int iRows, int iCols, bool bTestOnly = false, CRect* pTestRect=NULL);      // number of rows/cols to display; use NONE to leave unchanged

    void Move(const CPoint& ptNewOrigin);   // move to a new position, keep same size
    CSize GetMinSize(bool bExcludeScrollBars=false) const;

    void SelectColumn(int iCol, bool bMulti=false, bool bRange=false, bool bRedraw=true, COLORREF rgbBSel=rgbBSelDefault, COLORREF rgbFSel=rgbFSelDefault);
    void SelectRow(int iCol, bool bMulti=false, bool bRange=false, bool bRedraw=true, COLORREF rgbBSel=rgbBSelDefault, COLORREF rgbFSel=rgbFSelDefault);
    void SelectCell(int iRow, int iCol, bool bMulti=false, bool bRange=false, bool bRedraw=true, COLORREF rgbBSel=rgbBSelDefault, COLORREF rgbFSel=rgbFSelDefault);
    void SelectCell(const CPoint& ptCell, bool bMulti=false, bool bRange=false, bool bRedraw=true, COLORREF rgbBSel=rgbBSelDefault, COLORREF rgbFSel=rgbFSelDefault);
    void SelectFldTxtBox(int iRow, int iCol, int iOb, CGridRectTracker::TrackObject trackOb, bool bMulti=false, bool bNoTrack=false);
    bool IsColSelected(int iCol) const;
    bool IsRowSelected(int iRow) const;
    bool IsCellSelected(int iRow, int iCol) const;
    bool IsCellSelected(const CPoint& ptCell) const;
    bool IsFieldSelected(int iRow, int iCol, int iFld) const;
    bool IsBoxSelected(int iRow, int iCol, int iBox) const;
    bool IsTextSelected(int iRow, int iCol, int iTxt) const;

    int GetCurCol() const     { return m_iActiveCol; }      // returns NONE if no col selected
    int GetCurRow() const     { return m_iActiveRow; }  // returns NONE if no row selected
    CPoint GetCurCell() const { return m_ptActiveCell; } // returns (NONE,NONE) if no cell selected
    int GetNumSelCols() const { return m_aiSelCol.GetSize(); }
    int GetNumSelRows() const { return m_aiSelRow.GetSize(); }
    int GetCurField() const   { return m_iActiveField; }

    virtual void DrawBox(int iRow, int iCol, const CPoint& pt);

    void DeselectColumns();
    void DeselectRows();
    void DeselectCells();
    void DeselectTracker(int i);
    void DeselectTrackers();
    void Deselect();

    void Transpose(RosterOrientation orientation, bool bResetPositions=false);

// Entry interface:
public:
    void ScrollTo(int iRow, int iCol);          // puts the cell (iRow,iCol) into upper left corner (if possible)
    void EnsureVisible(int iRow, int iCol);     // ensures that the cell (iRow, iCol) is visible; scrolls minimally
    void GoToField(CDEField* pFld, int iOcc);

    CWnd* GetEdit() const     { return m_pEdit; }//SAVY&&& 10/04/00
    void SetEdit(CWnd* pEdit) { m_pEdit = pEdit; } //SAVY&&& 10/04/00

    virtual void StartEdit(CDEField* pFld, int iOcc);

    void StartQueue();  // starts storing update regions
    void StopQueue();   // invalidates queued update regions, redraws, flushes queue

    void SetFieldData(CDEField* pFld, int iOcc, const CString& csVal, bool bRedraw=false);
    const CString& GetFieldData(CDEField* pFld, int iOcc);

    void SetFieldBColor(CDEField* pFld, int iOcc, const COLORREF& c, bool bRedraw=false);
    COLORREF GetFieldBColor(CDEField* pFld, int iOcc);


// Common interface
public:
    // keyboard
    virtual void OnKeyDown(UINT* piKey, bool bProcessed);

    // column sizing and manipulation
    virtual bool OnCanSizeCol(int iCol) const;
    virtual bool OnCanSizeRow(int iRow) const;
    virtual bool OnCanSwapCol() const;
    virtual bool OnCanSwapRow() const;
    virtual bool OnCanDeleteCol(int iCol) const;
    virtual bool OnCanDeleteRow(int iRow) const;

    virtual void OnSwappedCol(const std::vector<int>& fromIndices, int toIndex);
    virtual void OnSwappedRow(const std::vector<int>& fromIndices, int toIndex);
    virtual void OnResizedCol(int iCol);
    virtual void OnResizedRow(int iRow);

    // selection
    virtual bool OnCanSelectCol(int iCol) const;
    virtual bool OnCanSelectRow(int iRow) const;
    virtual bool OnCanSelectCell(int iRow, int iCol) const;
    virtual bool OnCanMultiSelect() const;
    virtual bool OnCanSelectField(int iRow, int iCol, int iFld) const;
    virtual bool OnCanSelectBox(int iRow, int iCol, int iBox) const;
    virtual bool OnCanDrawBox(int iRow, int iCol);
    virtual bool OnCanSelectText(int irow, int iCol, int iTxt) const;
    virtual bool OnCanDrawText(int iRow, int iCol);

    // modification
    virtual void OnAddBox(int iRow, int iCol, const CRect& rc);
    virtual void OnAddText(int iCol, const CPoint& pt, const CString& csTxt, bool bAddToAll);
    virtual void OnBoxMoved(int iRow, int iCol, int iBox);
    virtual void OnTextMoved(int iRow, int iCol, int iTxt);
    virtual void OnFieldMoved(int iRow, int iCol, int iFld);

    // mouse stuff
    virtual void OnCB_LClicked (const CPoint& pt);
    virtual void OnCB_RClicked (const CPoint& pt);
    virtual void OnTH_LClicked (int iCol);
    virtual void OnTH_RClicked (int iCol);
    virtual void OnSH_LClicked (int iRow);
    virtual void OnSH_RClicked (int iRow);

    virtual void OnCell_LClicked(const CPoint& ptCell);
    virtual void OnCell_LClicked (int iRow, int iCol);
    virtual void OnCell_RClicked (int iRow, int iCol);
    virtual void OnCell_RClicked(const CPoint& ptCell);

    virtual void OnCellField_LClicked(CDEField* pFld, int iOcc);
    virtual void OnCellField_RClicked(CDEField* pFld, int iOcc);
    virtual void OnCellBox_LClicked(CHitOb& hitOb);
    virtual void OnCellBox_RClicked(CHitOb& hitOb);
    virtual void OnCellText_LClicked(CHitOb& hitOb);
    virtual void OnCellText_RClicked(CHitOb& hitOb);

    // fonts
    void ChangeFont(const PortableFont& font);

public:
    void GetClientRect(LPRECT lpRect) const
    {
        if (m_pRoster->GetRightToLeft()) {
            lpRect->left=m_rcScroll.left; lpRect->top=m_rcScroll.top-m_szHeader.cy; lpRect->right=m_rcScroll.right+m_szHeader.cx; lpRect->bottom=m_rcScroll.bottom;
        }
        else {
            lpRect->left=m_rcScroll.left-m_szHeader.cx; lpRect->top=m_rcScroll.top-m_szHeader.cy; lpRect->right=m_rcScroll.right; lpRect->bottom=m_rcScroll.bottom;
        }
    }

    const CRect& GetClientRect() const { return m_rcScroll; }    // %%% IS THIS RIGHT?

    void GetTotalRect(LPRECT lpRect) const
    {
        if (m_pRoster->GetRightToLeft()) {
            lpRect->left=m_rcTotal.left; lpRect->top=m_rcTotal.top-m_szHeader.cy; lpRect->right=m_rcTotal.right-m_szHeader.cx; lpRect->bottom=m_rcTotal.bottom;
        }
        else {
            lpRect->left=m_rcTotal.left-m_szHeader.cx; lpRect->top=m_rcTotal.top-m_szHeader.cy; lpRect->right=m_rcTotal.right; lpRect->bottom=m_rcTotal.bottom;
        }
    }

    const CRect& GetTotalRect() const { return m_rcTotal; }

    void SetGridBkColor(COLORREF color) { m_cGridbkcolor = color; }

    //FABN Dec 12, 2002
    COLORREF GetGridBkColor();

    //FABN Dec 12, 2002
    CSize GetHeaderSize();

    //FABN Dec 12, 2002 : Moved from  private
    BOOL IsResizing() const { return m_bResizing; }

    //FABN Dec 12, 2002 : Moved from private
    BOOL IsSwapping() const { return m_bSwapping; }

    //FABN Dec 12, 2002.
    int GetResizingCol() const  { return m_iResizingCol; }
    int GetResizingRow() const  { return m_iResizingRow; }
    int GetCurResizePos() const { return m_iCurResizePos; }

    CPoint GetScrollPos() const { return CPoint(m_sbHorz.GetScrollPos(), m_sbVert.GetScrollPos()); }

    void RecalcLayout(CSize szNewPage=CSize(-1,-1), bool bRedraw=true);    // default signals no change in size, with redraw

    CDERoster* GetRoster() { return m_pRoster; }

    const CGridCell& GetCell(int iRow, int iCol) const { return m_aRow[iRow].GetCell(iCol); }
    CGridCell& GetCell(int iRow, int iCol)             { return m_aRow[iRow].GetCell(iCol); }

    int GetStubColumn() const {return m_pRoster->GetRightToLeft() ? GetNumCols()-1 : 0;}

    const CGridCell& GetHeaderCell(int col) const { return GetCell(0, col); }
    CGridCell& GetHeaderCell(int col)             { return GetCell(0, col); }

    const CGridCell& GetStubCell(int row) const { return GetCell(row, 0); }
    CGridCell& GetStubCell(int row)             { return GetCell(row, 0); }

    int GetLeftHeaderMargin() const {return m_pRoster->GetRightToLeft() ? 0 : m_szHeader.cx;}
    int GetRightHeaderMargin() const {return m_pRoster->GetRightToLeft() ? m_szHeader.cx : 0;}
    int GetNumRows() const { return m_aRow.GetSize(); }  // includes header row

    int GetNumCols() const { return (int)m_aRow.ElementAt(0).GetNumCells(); }

    // offset used to right justify cells for right to left (arabic)
    // returns 0 if right to left is off.
    int GetRightJustifyOffset() const;

    const CGridRow& operator[](int i) const { return m_aRow[i]; }
    CGridRow& operator[](int i)             { return m_aRow[i]; }

    const CGridRow& GetRow(int i) const { return m_aRow[i]; }
    CGridRow& GetRow(int i)             { return m_aRow[i]; }

// Trackers
public:
    bool IsTrackerActive() const { return ( m_aTracker.GetSize() > 0); }
    int GetNumTrackers() const   { return m_aTracker.GetSize(); }
    int GetNumTrackers(CGridRectTracker::TrackObject trackOb) const;

    const CGridRectTracker& GetTracker(int i) const { return m_aTracker[i]; }
    CGridRectTracker& GetTracker(int i)             { return m_aTracker[i]; }
    void AddTracker(CGridRectTracker& t)            { m_aTracker.Add(t); }
    void RemoveTrackerAt(int i)                     { m_aTracker.RemoveAt(i); }
    void RemoveAllTrackers()                        { m_aTracker.RemoveAll(); }
    CRect UnionTrackers() const;


// Attributes
protected:
    CScrollBar              m_sbHorz;
    CScrollBar              m_sbVert;
    CWnd                    m_wndCorner;    // "dead-space" when both horz & vert scrollbars are present

    CRect                   m_rcScroll;             // scroll area (does not include sbars and header row/stub col)
    CRect                   m_rcTotal;              // total view size (not viewport)
    CPoint                  m_ptCorner;             // upper left corner cell (row, col)
    CSize                   m_szHeader;             // size of header row and stub col
    COLORREF                m_cGridbkcolor; // Color of the grid header cells

    // selection stuff
    int                             m_iActiveCol;   // currently active col (or NONE if none is active)
    int             m_iActiveRow;       // currently active row (or NONE if none is active)
    int             m_iActiveField; // currently active field w/in the cell indicated by (m_iActiveRow, m_iActiveCol)
    CPoint          m_ptActiveCell; // currently active cel (or NONE,NONE if none is active)
    CArray<int,int> m_aiSelCol;     // selected cols (when multi is allowed)
    CArray<int,int> m_aiSelRow;     // selected rows (when multi is allowed)
    CArray<CPoint, CPoint&> m_aptSelCell;     // selected cells (when multi is allowed)

private:
    static int              m_iId;          // for assigning windows IDs to ourselves and scrollbar children

    BOOL                    m_bInitialized;
    int                     m_iTextBoxHeightMin;//Text box minimum height

    // optimized redrawing stuff
    bool            m_bUpdateFlag;   //Update flag for draw.
    bool            m_bQueue;       // for update queuing
    CArray<CRect,CRect&> m_aQueue;

    // resizing/swapping stuff
    BOOL            m_bResizing;
    BOOL            m_bSwapping;
    BOOL            m_bPreSwapping; // user has clicked but not dragged to a border area
    CArray<CGridRectTracker, CGridRectTracker&>  m_aTracker;

    int             m_iResizingCol; // column that we are resizing or swapping
    int             m_iResizingRow; // row that we are resizing or swapping
    int             m_iMinResizePos;
    int             m_iMaxResizePos;
    int             m_iCurResizePos;

    // editing stuff
    CWnd*          m_pEdit;

    CDERoster*     m_pRoster;
    CArray<CGridRow, CGridRow&> m_aRow;
    TCHAR   m_decimalChar;


// misc
public:
    static __int64    m_lNumColors;   // number of colors in this display

// implementation
public:
    void BuildGrid();
    void RefreshOccLabelsStubText();
    void StartResizeCol(int iCol);
    void StartResizeRow(int iRow);
    void StartSwapCol(int iCol);
    void StartSwapRow(int iRow);

protected:
    DWORD   m_dwStyle;


protected:
    void RedrawCell(int iRow, int iCol, bool bUpdate=false);
    void Queue(CRect& rc);

    GRID_HITTEST HitTest(const CPoint& ptTest, CHitOb& hitOb) const;

private:
    BOOL IsPreSwapping() const { return m_bPreSwapping; }

    void MoveTrackerObject(int iCode);

public:
    CCellField& FindField(CDEField* pFld, int iOcc, int* piRow=NULL, int* piCol=NULL);

    void SetUpdateFlag(bool bFlag) { m_bUpdateFlag = bFlag; }
    bool GetUpdateFlag() const     { return m_bUpdateFlag; }

    void SetDecimalChar(TCHAR decimalChar) { m_decimalChar = decimalChar; }
    TCHAR GetDecimalChar() const           { return m_decimalChar; }

protected:
    CPoint GetMaxCorner() const;
    void SetColWidth(int iWidth, int iCol=NONE, bool bRedraw=true);
    void SetRowHeight(int iHeight, int iRow=NONE, bool bRedraw=true);

    bool IsQueuing() const { return m_bQueue; }

// Generated message map functions
protected:
    //{{AFX_MSG(CGridWnd)
    afx_msg void OnPaint();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
