#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/Definitions.h>
#include <zToolsO/SharedPointerHelpers.h>

class CSpecFile;


/***************************************************************************
*
*       CDEFormBase
*
***************************************************************************/

class CLASS_DECL_ZFORMO CDEFormBase : public CObject
{
    DECLARE_DYNAMIC(CDEFormBase)

public:
	enum eFormType { Single, Multiple };
	enum eItemType { UnknownItem = 0, Group = 1, Field = 2, Text = 3, Roster = 5, Level = 6, Block = 7 };
	enum eRIType { UnknownRI, Record, Item, SubItem };  // used in [Roster] --still nd???
	enum eRCType { UnknownRC, Row, Col };               // the [Roster]'s Orientation; either by row or col
	enum class TextLayout { Left, Right };              // says where the data entry txt will be in relation to it's box
	enum class TextUse { Label, Name };                 // says wether the form will use names when I tems are draged on the form

    CDEFormBase(const CString& name = CString(), const CString& label = CString());
    CDEFormBase(const CDEFormBase& rhs);

    CDEFormBase& operator=(const CDEFormBase& rhs);

    virtual eItemType GetFormItemType() const { return UnknownItem; }

    const CString& GetName() const    { return m_name; }
    void SetName(const CString& name) { m_name = name; }

    const CString& GetLabel() const     { return m_label; }
    void SetLabel(const CString& label) { m_label = label; }

    // link to symbol table   //SERPRO 27/12/99
    int GetSymbol() const       { return m_iSymbol; }
    void SetSymbol(int iSymbol) { m_iSymbol = iSymbol; }

    int  GetFormFileNumber() const      { return m_iFormFileNumber; }
    void SetFormFileNumber(int iNumber) { m_iFormFileNumber = iNumber; }

    void WriteDimsToStr(CString& cs, CRect rect=CRect(0,0,0,0)) const;        // m_cDims will be written to the string as x1,y1,x2,y2 unless a rect passed in
    void SetDims (CString& cs);

    void SetDims(const CRect& rect) { m_cDims = rect; }
    void SetDims(int x1, int y1, int x2, int y2) { m_cDims.SetRect (x1,y1,x2,y2); }

    CString GetUpperDimsStr() const;
    void SetUpperDims(CString cs);

    const CRect& GetDims() const { return m_cDims; }
    CRect& GetDims()             { return m_cDims; }

    void SetUsed(bool bUsed) { m_bUsed = bUsed;}
    bool GetUsed() const     { return m_bUsed; }


    // serialization
    // --------------------------------------------------
    virtual bool Build(CSpecFile& /*frmFile*/, bool /*bSilent*/ = false) { return true; }
    virtual void Save(CSpecFile& /*frmFile*/) const = 0;

    void serialize(Serializer& ar);


protected:
    bool m_bUsed;          // used for checking if the dict item corresponding to this exists in the dict

private:
    CString m_name;        // unique name, usu. (WILL?) match DCF's
    CString m_label;       // user-determined, not unique
    CRect m_cDims;         // the box defining this obj; x1=LC, y1=TR, x2=RC, y2=BR
    int m_iSymbol;         // symbol index          //SERPRO
    int m_iFormFileNumber; // Used only @ runtime
};
