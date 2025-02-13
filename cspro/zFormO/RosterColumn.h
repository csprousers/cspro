#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/RosterCells.h>


// --------------------------------------------------------------------------
//
// CDECol
//
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMO CDECol : public CObject
{
    DECLARE_DYNAMIC(CDECol)

public:
    CDECol();
    CDECol(const CDECol& rhs);
    ~CDECol();

    int GetWidth() const { return m_iWidth; }
    void SetWidth(int i) { m_iWidth = i; }

    const CPoint& GetOffset() const  { return m_Offset; }
    void SetOffset(const CPoint& cp) { m_Offset = cp; }
    void SetOffset(CString cs);

    const CDECell& GetColumnCell() const { return m_columnCell; }
    CDECell& GetColumnCell()             { return m_columnCell; }

    const CDEText& GetHeaderText() const           { return *m_headerText; }
    CDEText& GetHeaderText()                       { return *m_headerText; }
    std::shared_ptr<CDEText> GetSharedHeaderText() { return m_headerText; }
    void SetHeaderText(CDEText header_text)        { *m_headerText = std::move(header_text); }


    // methods for m_aField

    int GetNumFields        ()  const     { return (int)m_aField.size(); }
    CDEField* GetField      (int i) const { return m_aField [i]; }
    int GetFieldIndex(CDEField* pField) const;

    void AddField           (CDEField* pField)              { m_aField.emplace_back(pField); }
    void SetFieldAt         (CDEField* pField, int i)       { m_aField[i] = pField; }
    void InsertFieldAt      (CDEField* pField, int i)       { m_aField.insert(m_aField.begin() + i, pField); }

    // Removes field from column but not from owning roster, to do that
    // use CDEGroup::RemoveField
    void RemoveFieldAt      (int i);
    void RemoveAllFields    ();

    void ResetSizeAndHeader();
    

    // serialization
    // --------------------------------------------------
    bool Build(CSpecFile& frmFile, CDEGroup* pParentGroup, bool bSilent = false);
    void Save(CSpecFile& frmFile) const;

    void serialize(Serializer& ar, CDEGroup* pParentGroup);
    

private:
    int m_iWidth;         // width of the column, unique to each
    CPoint m_Offset;      // x,y offset from the grid's origin; y always 0
    CDECell m_columnCell;
    std::shared_ptr<CDEText> m_headerText;

    // These are ptrs to fields in parent rosters CDEGroup, group owns them so
    // column should not delete them. CDEGroup RemoveField routines should
    // be used to remove fields from roster.
    std::vector<CDEField*> m_aField;
};
