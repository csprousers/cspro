// TbdTable.cpp: implementation of the CTbdTable class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TbdTable.h"
//SAVY 05/05/2004 no more old C libs .use C++ standard library
#include <iostream>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


//---------------------------------
// Method: Constructor
//---------------------------------
CTbdTable::CTbdTable(CString csName, CString csNextName, CArray<int,int>& aDim, CTableDef::ETableType eTableType, int iCellSize, csprochar cOtherInfo, int iTbdFileBreakKeyLen )
 : CBaseTable2(aDim, eTableType, iCellSize, cOtherInfo, NULL) {
    m_csName = csName;
    m_csNextName = csNextName;
    m_iTbdFileBreakKeyLen = iTbdFileBreakKeyLen;
}

//---------------------------------
// Method: copy Constructor
//---------------------------------
CTbdTable::CTbdTable( CTbdTable& ctTable) : CBaseTable2(ctTable.m_aDim, ctTable.m_eTableType, ctTable.m_iCellSize, ctTable.m_cOtherInfo, NULL) {
    m_csName = ctTable.m_csName;
    m_csNextName = ctTable.m_csNextName;
    m_iTbdFileBreakKeyLen = ctTable.m_iTbdFileBreakKeyLen;

//  m_aDim.RemoveAll();
//  for ( int i = 0; i < ctTable.m_aDim.GetSize(); i++ ) {   // Is defined in CBaseTable2
//      m_aDim.Add(ctTable.m_aDim.GetAt(i));
//  }

    for ( int i = 0; i < ctTable.m_aTableBreak.GetSize(); i++ )
        m_aTableBreak.Add(ctTable.m_aTableBreak.GetAt(i));
}

//---------------------------------
// Method: Constructor
//---------------------------------
CTbdTable::CTbdTable() {
    m_iTbdFileBreakKeyLen = -1;
}

//---------------------------------
// Method: Destructor
//---------------------------------
CTbdTable::~CTbdTable() {
}

//---------------------------------
// Method: AddBreak
//---------------------------------
bool CTbdTable::AddBreak( CBreakItem* pBreak ) {
    m_aTableBreak.Add(pBreak);

    return true;
}

//---------------------------------
// Method: GetBreak
//---------------------------------
CBreakItem* CTbdTable::GetBreak( int iBreakNum ) {
    return m_aTableBreak.GetAt(iBreakNum);
}

//---------------------------------
// Method: GetNumBreak
//---------------------------------
int CTbdTable::GetNumBreak() {
    return m_aTableBreak.GetSize();
}

//---------------------------------
// Method: GetNumBreak
//---------------------------------
int CTbdTable::GetDimSize(int iDim) {
    return m_aDim.GetAt(iDim);
}

//---------------------------------
// Method: GetDimSize
//---------------------------------
int CTbdTable::GetDimSize( int* aIndex ) {
    for (int i = 0; i < m_aDim.GetSize(); i++ )
        aIndex[i] = m_aDim.GetAt(i);

    return m_aDim.GetSize();
}

int CTbdTable::GetDimSize( CArray<int,int>& rDim ) {
    for (int i = 0; i < m_aDim.GetSize(); i++ )
        rDim.Add(  m_aDim.GetAt(i) );

    return  m_aDim.GetSize();
}


//---------------------------------
// Method: GetTableName
//---------------------------------
CString CTbdTable::GetTableName() {
    return m_csName;
}

//---------------------------------
// Method: SetTableName
//---------------------------------
void CTbdTable::SetTableName( CString csName ) {
    m_csName = csName;
}

//---------------------------------
// Method: GetNextTableName
//---------------------------------
CString CTbdTable::GetNextTableName() {
    return m_csNextName;
}

//---------------------------------
// Operator: assign (operator=)
//---------------------------------
const CTbdTable& CTbdTable::operator= (const CTbdTable& ctTable)  {

    if (this != &ctTable) {
        m_csName = ctTable.m_csName;
        m_csNextName = ctTable.m_csNextName;
        m_eTableType = ctTable.m_eTableType;
        m_iCellSize = ctTable.m_iCellSize;
        m_iTbdFileBreakKeyLen = ctTable.m_iTbdFileBreakKeyLen;

        m_aDim.RemoveAll();
        m_aDim.Append( ctTable.m_aDim );

        m_cOtherInfo = ctTable.m_cOtherInfo;
        for ( int i = 0; i < ctTable.m_aTableBreak.GetSize(); i++ )
            m_aTableBreak.Add(ctTable.m_aTableBreak.GetAt(i));
    }

    return *this;
}

// RHF INIT Sep 28, 2005
void CTbdTable::SetTbdFileBreakKeyLen( int iTbdFileBreakKeyLen ) {
    m_iTbdFileBreakKeyLen = iTbdFileBreakKeyLen;
}

int CTbdTable::GetTbdFileBreakKeyLen() {
    return m_iTbdFileBreakKeyLen;
}
// RHF END Sep 28, 2005
