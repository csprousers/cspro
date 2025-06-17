#pragma once

//---------------------------------------------------------------------------
//  File name: TableAc.h
//
//  Description:
//          Header for CTableAcum class
//          This class is intended for keep a slice data and it's operations.
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Jul 00  DVB      Created
//
//
//---------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>
#include <ZTBDO/defslot.h>
#include <ZTBDO/basetab2.h>
#include <zToolsO/acum.h>

// Manipulate data information in memory.
// To be used by TBD and interpreter


class CLASS_DECL_ZTBDO CTableAcum : public CAcum
{
private:
    CBaseTable2*            m_pBaseTable;

    bool Assign( CTableAcum* rAcum, csprochar cOper);

public:
    enum CTableAcum_Condition  { CTableAcum_GT,
                                 CTableAcum_LT,
                                 CTableAcum_GE,
                                 CTableAcum_LE,
                                 CTableAcum_EQ,
                                 CTableAcum_NE };

    // If pTable is a sub-table pData must be != NULL.
    // If pTable is a table,  pData can be == NULL or  != NULL.
    CTableAcum( CBaseTable2* pTable );
    ~CTableAcum();

    // base table
    CBaseTable2*     GetBaseTable(){ return m_pBaseTable; }

    // Methods to be used only if pTable->m_bSubTable is false and pData == NULL
    bool        Alloc( byte* pInitValue=NULL ); // Default is 1.0e52
    void        Free();


    // Basic operations
    // Apply to Normal Slots, Special Slots and Stat Slots
    bool Assign(CTableAcum* rAcum);

    // Apply to Normal Slots, Special Slots and Stat Slots
    bool Compare(CTableAcum* rAcum);

    // Apply to Normal Slots, Special Slots and Stat Slots
    bool Addition(CTableAcum* rAcum);

    // Apply to Normal Slots, Special Slots and Stat Slots
    bool Min(CTableAcum* rAcum);

        // Apply to Normal Slots, Special Slots and Stat Slots
    bool Max(CTableAcum* rAcum);

    // The iteration is done by iDimension.
    // aIndex contains the indexes of the other dimensions.
    // aIndex(n)=-1 if the iteration needs to be done for the full dimension (n).
    static int GetCodeSize(int iCellSize, int iNumDim);

private:
    bool AddOper(byte* pcResult, byte *pcVal1, byte *pcVal2);
    bool MaxOper(byte* pcResult, byte *pcVal1, byte *pcVal2);
    bool MinOper(byte* pcResult, byte *pcVal1, byte *pcVal2);
    bool Merge(CTableAcum* pAcumParam);
    bool EqualFreq(byte* pbVal1, int iValLen1, byte* pbVal2, int iValLen2, CTableDef::ETableType eTableType);
    bool CopyFreqValue(byte* pbVal1, int iValLen1, byte* pbVal2, int iValLen2, CTableDef::ETableType eTableType);
    bool AddFreqValue(byte* pbRes, int iResLen,
                      byte* pbVal1, int iValLen1,
                      byte* pbVal2, int iValLen2, CTableDef::ETableType eTableType);
};
