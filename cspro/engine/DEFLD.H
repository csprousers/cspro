#pragma once

//---------------------------------------------------------------------------
//  File name: VarT.h
//
//  Description:
//          Header for engine-Fields descriptors
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              18 Jul 00   RHF+vc  Adding DEFLD3
//              21 May 01   vc      Now SetIndexes allow for a NULL aIndex
//
//---------------------------------------------------------------------------
#include <Zissalib/CsKernel.h>
#include <engine/VarT.h>

//---------------------------------------------------------------------------
//
//  struct DEFLD3 - expanded engine' fields descriptor for full-indexing and interface
//
//  Description:
//      Data-entry fields expanded description (up to DIM_MAXDIM occurrence' indexes)
//
//  Construction/Destruction/Initialization
//      Init                        Initialization
//
//  Basic access
//      GetSymbol                   Return the Var' symbol whom the field descriptor belongs
//      SetSymbol                   Set the Var' symbol owning this field descriptor
//      GetIndexes                  Get the whole array of occurrence' values
//      SetIndexes                  Set the whole array of occurrence' values
//      GetIndex                    Get the occurrence' value of a given type
//      SetIndex                    Set the occurrence' value of a given type to a given value
//
//  Operator helpers
//      =                           Assignment
//
//---------------------------------------------------------------------------

typedef C3DObject DEFLD3;
// DEFLD3 specific names mapped to C3DObject names

// could have been defined like this
//
//class DEFLD3: public C3DObject
//{
//public:
//    DEFLD3() {}
//    int GetSymVar() { return GetSymbol(); }
//    void SetSymVar(int iSymvar) { SetSymbol(iSymvar); }
//};
//

//---------------------------------------------------------------------------
//
//  class DEFLD_INFO
//
//  Description:
//      ???
//
//  Methods
//      ???
//
//---------------------------------------------------------------------------

struct DEFLD_INFO
{
    CString dname;
    CString sname;
    CString fname;
    CString vname;
    int level = 0;      // Hierarchy level: 1 to MaxNumberLevels
    VART* vp = nullptr; // Internal Variable Pointer
};


typedef DEFLD3 DEFLD;
