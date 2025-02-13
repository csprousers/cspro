#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/Box.h>
#include <zFormO/Text.h>


// --------------------------------------------------------------------------
//
// CDECell
//
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMO CDECell : public CObject
{
    DECLARE_DYNAMIC(CDECell)

public:
    CDECell();
    CDECell(const CDECell& rhs);

    CDECell& operator=(const CDECell& rhs);

    // moves the content of rhs into this object
    void MoveContent(CDECell& rhs);

    bool HasContent() const { return ( m_boxSet.GetNumBoxes() > 0 || m_textSet.GetNumTexts() > 0 ); }


    // boxes
    // --------------------------------------------------
    const CDEBoxSet& GetBoxSet() const { return m_boxSet; }
    CDEBoxSet& GetBoxSet()             { return m_boxSet; }


    // texts
    // --------------------------------------------------
    const CDETextSet& GetTextSet() const { return m_textSet; }
    CDETextSet& GetTextSet()             { return m_textSet; }


    // serialization
    // --------------------------------------------------
    void Save(CSpecFile& frmFile) const;

    void serialize(Serializer& ar);


private:
    CDEBoxSet m_boxSet;
    CDETextSet m_textSet;
};



// --------------------------------------------------------------------------
//
// CDEFreeCell
//
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMO CDEFreeCell : public CDECell
{
    DECLARE_DYNAMIC(CDEFreeCell)

public:
    CDEFreeCell(int row = 0, int column = 0);

    int GetRow() const   { return m_row; }
    void SetRow(int row) { m_row = row; }

    int GetColumn() const      { return m_column; }
    void SetColumn(int column) { m_column = column; }


    // serialization
    // --------------------------------------------------
    bool Build(CSpecFile& frmFile, bool bSilent = false);
    void Save(CSpecFile& frmFile, bool bWriteHeadingInfo = true) const;

    void serialize(Serializer& ar);


private:
    int m_row;    // what row is this cell in?
    int m_column; // what column is this cell in?
};
