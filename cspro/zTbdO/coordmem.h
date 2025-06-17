#pragma once
//---------------------------------------------------------------------------
//  File name: CoordMem.h
//
//  Description:
//          Header for CCoordMember class
//          This class a coordinate
//
//  History:    Date       Author   Comment
//              ---------------------------
//              30 Jul 01   RHF     Created
//
//---------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>

namespace Logic { class SymbolTable; }

class CLASS_DECL_ZTBDO CCoordMember {
public:
    int     m_iLeft;
    int     m_iRight;
    int     m_iCellLeft;

#ifdef _DEBUG
    void Dump( CString& csMsg, int* pCtNodebase, const Logic::SymbolTable* pSymbolTable );
#endif
};
