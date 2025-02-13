#pragma once

//
//   GridCell.h
//
// definitions for classes  CCellField  and   CGridCell
//

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      class CCellField
//
///////////////////////////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZGRID2O CCellField
{
// construction
public:
    CCellField() {
        m_colorB = rgbBFieldDefault;
        m_colorF = rgbFFieldDefault;
        m_colorBSel = rgbBSelDefault;
        m_colorFSel = rgbFSelDefault;
        m_posFld = posNone;
        m_pDEField = NULL;
        m_bSelected = false;
    }
    CCellField(const CCellField& fld) : m_csVal(fld.GetVal())  {
        m_colorB = fld.GetBColor();
        m_colorF = fld.GetFColor();
        m_colorBSel = fld.GetBSelColor();
        m_colorFSel = fld.GetFSelColor();
        m_posFld = fld.GetFldPos();
        m_pDEField = fld.GetDEField();
        m_bSelected = fld.IsSelected();
    }

// operations
public:
    const CString& GetVal() const     { return m_csVal; }
    void SetVal(const CString& csVal) { m_csVal = csVal; }

    bool IsSelected() const        { return m_bSelected; }
    void Select(bool bSelect=true) { m_bSelected = bSelect; }

    COLORREF GetBColor() const { return m_colorB; }
    void SetBColor(COLORREF c) { m_colorB = c; }

    COLORREF GetFColor() const { return m_colorF; }
    void SetFColor(COLORREF c) { m_colorF = c; }

    COLORREF GetBSelColor() const { return m_colorBSel; }
    void SetBSelColor(COLORREF c) { m_colorBSel = c; }

    COLORREF GetFSelColor() const { return m_colorFSel; }
    void SetFSelColor(COLORREF c) { m_colorFSel = c; }

    CDEField* GetDEField() const        { return m_pDEField; }
    void SetDEField(CDEField* pDEField) { m_pDEField = pDEField; }

    GridObjPosition GetFldPos() const   { return m_posFld; }
    void SetFldPos(GridObjPosition pos) { m_posFld = pos; }

    const CCellField& operator=(const CCellField& fld)  {
        m_csVal = fld.GetVal();
        m_colorB = fld.GetBColor();
        m_colorF = fld.GetFColor();
        m_colorBSel = fld.GetBSelColor();
        m_colorFSel = fld.GetFSelColor();
        m_posFld = fld.GetFldPos();
        m_pDEField = fld.GetDEField();
        m_bSelected = fld.IsSelected();
        return *this;
    }

// attributes
private:
    CString         m_csVal;
    COLORREF        m_colorB;
    COLORREF        m_colorF;
    COLORREF        m_colorFSel;
    COLORREF        m_colorBSel;
    bool            m_bSelected;
    CDEField*       m_pDEField;
    GridObjPosition m_posFld;      // makes it easier to position
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      class CGridCell
//
///////////////////////////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZGRID2O CGridCell
{
// construction
public:
    CGridCell();
    CGridCell(const CGridCell& cell);

    virtual ~CGridCell() {}

    const CGridCell& operator=(const CGridCell& cell);

    size_t GetNumBoxes() const                                { return m_boxes.size(); }
    const SharedPointerVectorWrapper<CDEBox> GetBoxes() const { return SharedPointerVectorWrapper<CDEBox>(m_boxes); }
    const CDEBox& GetBox(size_t index) const                  { ASSERT(index < m_boxes.size()); return *m_boxes[index]; }
    CDEBox& GetBox(size_t index)                              { ASSERT(index < m_boxes.size()); return *m_boxes[index]; }
    void AddBoxSet(CDEBoxSet& box_set);

    size_t GetNumTexts() const                                 { return m_texts.size(); }
    const SharedPointerVectorWrapper<CDEText> GetTexts() const { return SharedPointerVectorWrapper<CDEText>(m_texts); }
    SharedPointerVectorWrapper<CDEText, true> GetTexts()       { return SharedPointerVectorWrapper<CDEText, true>(m_texts); }
    const CDEText& GetText(size_t index) const                 { ASSERT(index < m_texts.size()); return *m_texts[index]; }
    CDEText& GetText(size_t index)                             { ASSERT(index < m_texts.size()); return *m_texts[index]; }
    void AddTextSet(CDETextSet& text_set);
    void AddText(std::shared_ptr<CDEText> text)                { m_texts.emplace_back(std::move(text)); }
    void RemoveAllTexts()                                      { m_texts.clear(); }

    int GetNumFields() const { return m_aField.GetSize(); }
    CCellField& GetField(int i) { return m_aField[i]; }
    CCellField GetField(int i) const { return m_aField[i]; }
    void AddField(CCellField& field) { m_aField.Add(field); }
    void RemoveAllFields() { m_aField.RemoveAll(); } // does *not* free memory

    bool IsSelected() const        { return m_bSelected; }
    void Select(bool bSelect=true) { m_bSelected = bSelect; }

    COLORREF GetBColor() const { return m_colorB; }
    void SetBColor(COLORREF c) { m_colorB = c; }

    COLORREF GetFColor() const { return m_colorF; }
    void SetFColor(COLORREF c) { m_colorF = c; }

    COLORREF GetBSelColor() const { return m_colorBSel; }
    void SetBSelColor(COLORREF c) { m_colorBSel = c; }

    COLORREF GetFSelColor() const { return m_colorFSel; }
    void SetFSelColor(COLORREF c) { m_colorFSel = c; }

    const CRect& GetRect() const { return m_rcClient; }
    CRect& GetRect()             { return m_rcClient; }

    void SetRect(const CRect& rcClient) { m_rcClient = rcClient; }
    void SetRectEmpty()                 { m_rcClient.SetRectEmpty(); }

    const CSize& GetMinSizeH() const { return m_szMinH; }
    CSize& GetMinSizeH()             { return m_szMinH; }
    const CSize& GetMinSizeV() const { return m_szMinV; }
    CSize& GetMinSizeV()             { return m_szMinV; }

    void CalcMinSize(CDC* pDC, CSize& szFieldFontTextExt);


// attributes
protected:
    CArray<CCellField, CCellField&> m_aField;   // each cell in a column needs a copy

    bool            m_bSelected;
    CRect           m_rcClient;
    CSize           m_szMinV;           // min size if cell is squished vertically
    CSize           m_szMinH;           // min size if cell is squished horizontally

    COLORREF        m_colorB;           // background and foreground colors
    COLORREF        m_colorF;
    COLORREF        m_colorBSel;        // selected colors
    COLORREF        m_colorFSel;

private:
    std::vector<std::shared_ptr<CDEBox>> m_boxes;
    std::vector<std::shared_ptr<CDEText>> m_texts;
};


class CGridRow
{
public:
    size_t GetNumCells() const { return m_cells.size(); }

    const CGridCell& GetCell(size_t index) const { ASSERT(index < m_cells.size()); return m_cells[index]; }
    CGridCell& GetCell(size_t index)             { ASSERT(index < m_cells.size()); return m_cells[index]; }

    void AddCell(CGridCell cell) { m_cells.emplace_back(std::move(cell)); }

private:
    std::vector<CGridCell> m_cells;
};
