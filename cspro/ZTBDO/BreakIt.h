#pragma once
//---------------------------------------------------------------------------
//  File name: BreakItem.h
//
//  Description:
//          Header for CBreakItem class
//          This class define a break item
//
//  History:    Date       Author   Comment
//              ---------------------------
//              2 Jul 01   DVB      Created
//
//---------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>

// ----------------------
// Defines
// ----------------------
#define TBD_BI_NAMELEN 256

// --------------------------
// Binary BreakItem Structure
// --------------------------
typedef struct {
    csprochar cName[TBD_BI_NAMELEN];
    csprochar cItemLen;
    csprochar cOtherInfo;
} TBD_BI;


// ----------------------
// Class Definition
// ----------------------
class CLASS_DECL_ZTBDO CBreakItem {
protected:
    CString m_csName;
    int     m_iLen;
    csprochar    m_cOtherInfo;

public:
    CBreakItem();
    CBreakItem(CString csName, int iLen, csprochar m_cOtherInfo);
    CBreakItem(const CBreakItem& cbItem);
    virtual ~CBreakItem();

    CString     GetName();
    int         GetLen();
    csprochar       GetOtherInfo();

};
