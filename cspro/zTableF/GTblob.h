#pragma once
// GTblob.h: interface for the CGTblOb class.
//
//////////////////////////////////////////////////////////////////////

#include <zTableF/zTableF.h>
#include <zTableO/Table.h>

class CLASS_DECL_ZTABLEF CGTblOb : public CObject
{
DECLARE_DYNAMIC(CGTblOb)
public:
    CGTblOb();
    virtual ~CGTblOb() { }

public:
    CTblOb* GetTblOb(void) { return m_pTblOb;}
    void SetTblOb(CTblOb* pTblOb) { m_pTblOb = pTblOb;}
    void SetBlocked(bool bBlocked) {m_bBlocked = bBlocked;}
    bool GetBlocked(void)const {return m_bBlocked ;}

    CGTblOb* GetParent(void) const { return m_pParent; }
    void SetParent(CGTblOb* pGTblOb) {m_pParent =pGTblOb; }

protected:
    bool m_bBlocked; // This is for selection in the grid .

private:
    CTblOb* m_pTblOb; // Used for Title/Subtitle/Footnote/Pagenote (stuff which have no tabvar/tabvals)
    CGTblOb* m_pParent;
};


class CLASS_DECL_ZTABLEF CGRowColOb : public CGTblOb
{
DECLARE_DYNAMIC(CGRowColOb)
public:
    CGRowColOb();
    virtual ~CGRowColOb();
    CSize   m_szCurr;
protected:
    CArray<CGRowColOb*,CGRowColOb*> m_arrChildren;
    CTabValue*  m_pTabValue;
    CTabVar*    m_pTabVar;
  //  CGRowColOb* m_pParent;
    int         m_iCurLevel; //hierarchy level
public:
    //Operations
    // CGRowColOb* GetParent() { return m_pParent; }
    //void SetParent(CGRowColOb* pParent) {  m_pParent = pParent ; }

    CTabVar* GetTabVar(void) { return m_pTabVar; }
    void SetTabVar(CTabVar* pTabVar) { m_pTabVar = pTabVar; }

    CTabValue* GetTabVal(void) { return m_pTabValue; }
    void SetTabVal(CTabValue* pTabValue) { m_pTabValue = pTabValue; }

    int GetCurLevel() { return m_iCurLevel; }
    void SetCurLevel(int iCurLevel) { m_iCurLevel = iCurLevel; }

    bool HasChildren() { return m_arrChildren.GetSize() >0;}
    int  GetNumChildren() {  return m_arrChildren.GetSize();}
    CGRowColOb* GetChild(int iIndex) { return m_arrChildren[iIndex];}

    void RemoveAllChildren(bool bDelete = true);
    void AddChild(CGRowColOb* pChild) { m_arrChildren.Add(pChild);}
    CIMSAString GetText() {
        if(m_pTabValue){
            return m_pTabValue->GetText();
        }
        else if (m_pTabVar) {
            return m_pTabVar->GetText();
        }
        else{
            return _T("");
        }
    }
};


class CLASS_DECL_ZTABLEF CGTblCol : public CGRowColOb
{
DECLARE_DYNAMIC(CGTblCol)
public:
    CGTblCol();
    virtual ~CGTblCol();

    bool IsColGroup() {return HasChildren(); }
private:
    int                 m_iNumGridCols;     // >1 for spanners
    int                 m_iStartGridCol;    // start joining from this cell

public:
   int  GetNumGridCols() {return m_iNumGridCols;}
   void SetNumGridCols(int iNumCols) { m_iNumGridCols  = iNumCols;}

    void SetStartCol(int iStart) { m_iStartGridCol = iStart;}
    int GetStartCol(void) {return  m_iStartGridCol ;}
};


class CLASS_DECL_ZTABLEF CGTblRow : public CGRowColOb
{
DECLARE_DYNAMIC(CGTblRow)
public:
    CGTblRow();
    virtual ~CGTblRow();

    bool IsRowGroup() {return HasChildren(); }
private:
    int                 m_iNumGridRows;     // >1 for rows containing other rows
    int                 m_iStartGridRow;    // start joining from this cell

public:
    int GetNumGridRows() {return m_iNumGridRows;}
    void SetNumGridRows(int iNumRows) { m_iNumGridRows  = iNumRows;}

    void SetStartRow(int iStart) { m_iStartGridRow = iStart;}
    int GetStartRow(void) {return  m_iStartGridRow ;}

};


class CLASS_DECL_ZTABLEF CGTblCell : public CGTblOb
{
DECLARE_DYNAMIC(CGTblCell)

public:
    CGTblCell();
    virtual ~CGTblCell();
protected:
    bool   m_bDirty;
    double m_iData;
    //sourcing info
    CGTblCol*         m_pColumn;          // the (parent) column we belong to
    CGTblRow*         m_pRow;             // the (parent) row we belong to

public :
    void SetData(const double& iData) { m_iData = iData;};
    double GetData(void) const { return m_iData; }

    void SetTblCol(CGTblCol* pCol){ m_pColumn = pCol;}
    CGTblCol* GetTblCol() { return m_pColumn;}

    void SetTblRow(CGTblRow* pRow){ m_pRow = pRow;}
    CGTblRow* GetTblRow() { return m_pRow;}

    void SetDirty(const bool& bDirty) { m_bDirty = bDirty;}
    bool IsDirty(void)const {return  m_bDirty;}
};
