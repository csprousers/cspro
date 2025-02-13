//---------------------------------------------------------------------------
//  File name: SecT.cpp
//
//  Description:
//          Implementation for engine-Sec class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              04 Jul 00   RHF     Adding IsCommon, etc
//              04 Jul 00   vc      Adding ptrs to related objets (improve execution speed)
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include "GroupVisitor.h"
#include <engine/Tables.h>
#include <engine/Engdrv.h>
#include <zJson/Json.h>


const Logic::SymbolTable& CSymbolSection::GetSymbolTable() const
{
    return m_pEngineArea->GetSymbolTable();
}


/////////////////////////////////////////////////////////////////////////////
//
// --- construction/destruction/initialization
//
/////////////////////////////////////////////////////////////////////////////

CSymbolSection::CSymbolSection(std::wstring name, CEngineDriver* pEngineDriver)
    :   ChainedSymbol(std::move(name), SymbolType::Section)
{
    m_pEngineArea = pEngineDriver->getEngineAreaPtr();

    // --- related objects
    m_pDicT           = NULL;                           // victor Jul 04, 00
    m_pSecX           = NULL;                           // victor Jul 04, 00

    // --- basic data
    SYMTfvar          = -1;
    SYMTlvar          = -1;
    m_iLevel          = -1;
    _tmemset( code, BLANK, MAX_RECTYPECODE );

    // --- structural limits on occurrences
    m_iMinOccs        =  1;
    m_iMaxOccs        =  1;

    // --- other record' data
    m_bOccGenerator   = false;
    m_bIsCommon       = false;                          // RHF Jul 04, 2000
    m_bSpecialSection = false;
    m_iLastLoc        =  0;

    // --- linked IMSA Item
    m_pDictRecord      = NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// --- other record' data
//
/////////////////////////////////////////////////////////////////////////////


void CSymbolSection::UpdateLastLoc() {                            // victor Oct 27, 99
    // UpdateLastLoc: recalculate the last-location
    int     iLastLoc = GetLastLoc();    // starts with former last location
    int     iSymVar = SYMTfvar;

    while( iSymVar > 0 ) {
        VART* pVarT = VPT(iSymVar);
        const CDictItem* pItem = pVarT->GetDictItem();
        int iMaxOccs = ( pVarT->GetClass() == CL_MULT ) ? pItem->GetOccurs() : 1;
        int iLastItemLoc = pVarT->GetLocation() + iMaxOccs * pVarT->GetLength() - 1;

        if( iLastLoc < iLastItemLoc )
            iLastLoc = iLastItemLoc;

        iSymVar = pVarT->SYMTfwd;
    }

    SetLastLocTo( iLastLoc );
}

/////////////////////////////////////////////////////////////////////////////
//
// --- other methods
//
/////////////////////////////////////////////////////////////////////////////

void CSymbolSection::AddVariable( VART* pVarT ) {                 // victor Oct 27, 99
    // AddVariable: add a given Var to this Sect
    int     iSymLastVar = ( SYMTfvar > 0 ) ? SYMTlvar : -1;

    // chain next to previous variable
    m_pEngineArea->ChainSymbol(iSymLastVar, pVarT);

    // updates last location in this section
    UpdateLastLoc();
}

// RHF INIC Feb 19, 2002
void CSymbolSection::CreateGroupArray() {
    ASSERT( m_aGroups.empty() );

    int     iSymVar = SYMTfvar;

    while( iSymVar > 0 ) {
        VART*       pVarT = VPT(iSymVar);

        ASSERT( pVarT );
        GROUPT*     pGroupT=pVarT->GetOwnerGPT();

        //ASSERT(pGroupT);

        if( pGroupT != NULL && FindGroup( pGroupT ) < 0 ) // Not found
            AddGroup(pGroupT);

        iSymVar = pVarT->SYMTfwd;
    }
}
// RHF END Feb 19, 2002


void CSymbolSection::accept( GroupVisitor* visitor ) // rcl, Sept 2005
{
    visitor->visit( this );
}


Symbol* CSymbolSection::FindChildSymbol(const std::wstring& symbol_name) const
{
    // use the dictionary's method
    return GetDicT()->FindChildSymbol(symbol_name);
}


void CSymbolSection::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    ASSERT(m_pDictRecord != nullptr);

    json_writer.Write(JK::record, *m_pDictRecord);
}
