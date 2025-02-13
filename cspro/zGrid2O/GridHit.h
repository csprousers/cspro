#pragma once

#include <zGrid2O/zGrid2O.h>


// for hit testing in the grid

// for hit testing
class CLASS_DECL_ZGRID2O CHitOb
{
public:
    CHitOb() : m_ptCell(NONE,NONE) { m_iField=m_iBox=m_iTxt=NONE; }
    CHitOb(const CHitOb& h) {
        m_ptCell = h.GetCell();
        m_iField = h.GetField();
        m_iBox = h.GetBox();
        m_iTxt = h.GetText();
    }

public:
    CPoint& GetCell(void) { return m_ptCell; }
    CPoint GetCell(void) const { return m_ptCell; }
    int GetField(void) const { return m_iField; }
    int GetBox(void) const { return m_iBox; }
    int GetText(void) const { return m_iTxt; }
    void SetCell(const CPoint& ptCell) { m_ptCell = ptCell; }
    void SetField(int iField) { m_iField = iField; }
    void SetBox(int iBox) { m_iBox = iBox; }
    void SetText(int iTxt) { m_iTxt = iTxt; }

    const CHitOb& operator=(const CHitOb& h) {
        m_ptCell = h.GetCell();
        m_iField = h.GetField();
        m_iBox = h.GetBox();
        m_iTxt = h.GetText();
        return *this;
    }
    bool operator==(const CHitOb& h)  {
        return (m_ptCell==h.GetCell() && m_iField==h.GetField() && m_iBox==h.GetBox() && m_iTxt==h.GetText());
    }

private:
    CPoint  m_ptCell;   // row, col for hit object, (NONE,NONE) otherwise
    int     m_iField;   // field (0-based index) hit, NONE otherwise
    int     m_iBox;     // box (0-based index) hit, NONE otherwise
    int     m_iTxt;     // text object hit, NONE otherwise
};
