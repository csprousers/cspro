// BreakItem.cpp: implementation of the CBreakItem class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BreakIt.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

// ------------------------------
// Method: Constructor
// ------------------------------
CBreakItem::CBreakItem() {
    m_csName = _T("");
    m_iLen = 0;
    m_cOtherInfo = 0;
}

// ------------------------------
// Method: Constructor
// ------------------------------
CBreakItem::CBreakItem(CString csName, int iLen, csprochar cOtherInfo) {
    m_csName = CString(csName);
    m_iLen = iLen;
    m_cOtherInfo = cOtherInfo;
}

// ------------------------------
// Method: Constructor
// ------------------------------
CBreakItem::CBreakItem(const CBreakItem& cbItem) {
    m_csName = cbItem.m_csName;
    m_iLen = cbItem.m_iLen;
    m_cOtherInfo = cbItem.m_cOtherInfo;
}

// ------------------------------
// Method: Destructor
// ------------------------------
CBreakItem::~CBreakItem() {
}

// ------------------------------
// Method: GetName
// ------------------------------
CString CBreakItem::GetName() {
    return m_csName;
}

// ------------------------------
// Method: GetLen
// ------------------------------
int CBreakItem::GetLen() {
    return m_iLen;
}

// ------------------------------
// Method: GetOtherInfo
// ------------------------------
csprochar CBreakItem::GetOtherInfo() {
    return m_cOtherInfo;
}
