//--------------------------------------------------------------------------
//  EXALLOCD.cpp : alloc and init dictionary execution data structures
//--------------------------------------------------------------------------
#include "StdAfx.h"

#include <engine/Exappl.h>
#include <engine/Engine.h>
#include <engine/VARX.h>
#include <engine/Dicx.h>


#define RTRACE TRACE

class CMemoryError {
    CString m_csWhere;
public:
    CMemoryError( CString csWhere ) : m_csWhere( csWhere ) {}
    CString Where() { return m_csWhere; }
};

#define CheckMem( what, where )  if( what == NULL ) throw CMemoryError( where )

void CIntDriver::AllocExecBase()
{
    // AllocExecBase: create basic tables to support 'compall' of app' declarations
    // ... formerly 'exalloc0', now 'AllocExecBase'
    try {
        // --- DICX/execution Dictionaries
        int iNumDicts = m_engineData->dictionaries_pre80.size();

        DIXBASE = (DICX*) calloc( iNumDicts, sizeof(DICX) );
        if( DIXBASE != NULL )
        {
            // initializing // victor Jul 04, 00
            for( int iDicSlot = 0; iDicSlot < iNumDicts; iDicSlot++ )
            {
                DICX* pDicX = DIX(iDicSlot);
                pDicX->Init();

                // tying both DICT & DICX entries together
                DICT* pDicT = DIP(iDicSlot);

                pDicX->SetDicT(pDicT);
                pDicT->SetDicX(pDicX);
            }
        }

        CheckMem( DIXBASE, _T("DICX") );                    // RCH March 27, 2000
    }
    catch( CMemoryError& e ) {
        issaerror( MessageType::Abort, 1000, e.Where().GetBuffer() );
    }
}

void CIntDriver::AllocExecTables()
{
    // AllocExecTables: create remaining executor' tables
    // ... formerly 'exallocdict', now 'AllocExecTables'

    // NOTE: any changes here should probably also be made in CIntDriver::AllocLocalAlphas (or the
    // code should be refactored to use shared functions)

    try {
        Appl.MaxLevel = DIP(0)->maxlevel;

        // --- SECX/execution Sections
        int iNumSects = m_engineData->sections.size();

        SIXBASE = (SECX*) calloc( iNumSects, sizeof(SECX) );
        if( SIXBASE != NULL ) {         // initializing // victor Jul 04, 00
            for( int iSecSlot = 0; iSecSlot < iNumSects; iSecSlot++ ) {
                SECX*   pSecX = SIX(iSecSlot);

                pSecX->Init();

                // tying both SECT & SECX entries together
                SECT*   pSecT = SIP(iSecSlot);

                pSecX->SetSecT( pSecT );
                pSecT->SetSecX( pSecX );
            }
        }

        CheckMem( SIXBASE, _T("SECX") );                    // RCH March 27, 2000

        // --- VARX/execution Variables
        if( !m_engineData->variables.empty() )
        {
            VIXBASE = (VARX*)calloc(m_engineData->variables.size(), sizeof(VARX));

            if( VIXBASE != NULL )
            {
                for( size_t iVarSlot = 0; iVarSlot < m_engineData->variables.size(); iVarSlot++ )
                {
                    VARX* pVarX = VIX(iVarSlot);

                    pVarX->Init();

                    // tying both VART & VARX entries together
                    VART* pVarT = VIP(iVarSlot);

                    pVarX->SetVarT( pVarT );
                    pVarT->SetVarX( pVarX );
                }
            }

            CheckMem( VIXBASE, _T("VARX") );                // RCH March 27, 2000
        }

        m_pEngineArea->VarxStart();
    }
    catch( CMemoryError& e ) {
        issaerror( MessageType::Abort, 1000, e.Where().GetBuffer() );
    }
}


void CIntDriver::BuildRecordsMap( void ) {              // victor Jul 10, 00
    // calculate the record size & the number of flots and flags    // NEW
    // for each record' occurrence
    // ... formerly 'levassign', now 'BuildRecordsMap'

    // Dictionaries recognition:
    // (a) calculating lengths for records of each DIC slots
    bool    bCheckRanges = ( Issamod != ModuleType::Entry && m_pEngineSettings->HasCheckRanges() ); // victor Dec 07, 00
    bool    bSkipStruc   = ( Issamod != ModuleType::Entry && m_pEngineSettings->HasSkipStruc() );   // victor Mar 14, 01
    bool    bSetAllVarsUsed  = ( bCheckRanges || bSkipStruc );

    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        DICX*   pDicX = pDicT->GetDicX();
        bool    bIsInput  = ( pDicT->GetSubType() == SymbolSubType::Input );
        bool    bIsOutput = ( pDicT->GetSubType() == SymbolSubType::Output );

        pDicX->SetMaxRecLength( 0 );
        pDicX->SetKeyHolderLength( pDicT->GetLevelsIdLen() );

        // highest head-location in record for both Common & SecId
        int     iSymSec = pDicT->SYMTfsec;

        int     iCommonLastLoc = pDicT->Common_GetCommonLastPos( 0 );
        int     iSecIdLastLoc  = pDicT->GetSecIdLastPos();
        int     iHeadLastLoc   = ( iCommonLastLoc > iSecIdLastLoc ) ?
                                   iCommonLastLoc : iSecIdLastLoc;

        while( iSymSec > 0 ) {
            SECT*   pSecT = SPT(iSymSec);
            SECX*   pSecX = pSecT->GetSecX();

            // tying to Dictionary
            pSecT->SetDicT( pDicT );
            pSecX->SetDicX( pDicX );

            // highest location of a data-item in the record
            int     iSecLastLoc = pSecT->GetLastLoc();

            // ascii, flots, flags ... per each Record' occurrence
            int     iLenAscii  = ( iSecLastLoc > iHeadLastLoc ) ?
                                   iSecLastLoc : iHeadLastLoc;
            int     iNumFlags  = 0;
            int     iNumFlots  = 0;
            int     iSymVar = pSecT->SYMTfvar;

            while( iSymVar > 0 ) {
                VART*   pVarT = VPT(iSymVar);
                VARX*   pVarX = pVarT->GetVarX();

                // attaching owner SecT
                pVarX->SetSecT( pSecT );            // victor May 24, 00

                // installing 'IsUsed condition'
                bool    bIsField = ( Issamod == ModuleType::Entry && bIsInput && pVarT->IsInAForm() );

                if( bSetAllVarsUsed || bIsField || bIsOutput || pSecT->IsCommon() )
                    pVarT->SetUsed( true );

                // get the number of occurrences of this Var in its Sect
                int     iVarOccs = pVarT->GetFullNumOccs();
                ASSERT( iVarOccs >= 1 );

                // ascii: the natural location in record - 1 (becomes 0-based)
                pVarX->SetIndToAscii( pVarT->GetLocation() - 1 );

                // floats: as many as the occs, for used & numeric only
                bool add_float = ( ( pVarT->IsUsed() || m_pEngineDriver->GetHasSomeInsDelSortOcc() ) && pVarT->IsNumeric() );

                // all binary dictionary items, even those unused, will use the float as a way to keep track of binary data
                if( !add_float && pVarT->GetDictItem() != nullptr && IsBinary(*pVarT->GetDictItem()) )
                    add_float = true;

                if( add_float ) {
                    // ... only for used, numeric vars
                    pVarX->SetIndToFloat( iNumFlots );
                    iNumFlots += iVarOccs;
                }

                // flags: as many as the occs
                pVarX->SetIndToFlags( iNumFlags );
                iNumFlags += iVarOccs;

                iSymVar = pVarT->SYMTfwd; // RHF Jul 21, 2000
            }

            // install calculated amounts into SECX*
            pSecX->SetSecAsciiSize( iLenAscii );
            pSecX->m_iFloatSize = iNumFlots;
            pSecX->m_iFlagsSize = iNumFlags;

            // refresh the Dic' max-record-length
            if( pDicX->GetMaxRecLength() < pSecX->GetSecAsciiSize() )
                pDicX->SetMaxRecLength( pSecX->GetSecAsciiSize() );

            // next section
            iSymSec = pSecT->SYMTfwd;
        }
    }

    // (b) getting memory for management members
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        DICX* pDicX = pDicT->GetDicX();

        // a.- allocating container of 3 keys
        int     iKeySize = pDicX->GetKeyHolderLength() + 1;

// RHF COM Aug 07, 2000        pDicX->m_pKeyHolderArea = (csprochar*) calloc( 3 * iKeySize, sizeof(csprochar) );
        // RHF COM Nov 05, 2004pDicX->m_pKeyHolderArea = (csprochar*) calloc( 3 * (iKeySize+2), sizeof(csprochar) ); // RHF Aug 07, 2000
        pDicX->m_pKeyHolderArea = (csprochar*) calloc( 2 * (iKeySize+2), sizeof(csprochar) ); // RHF Nov 05, 2004

        pDicX->current_key = pDicX->m_pKeyHolderArea;
        pDicX->last_key    = pDicX->m_pKeyHolderArea + iKeySize;

        // b.- allocating container of record read
        int     iRecSize = pDicX->GetMaxRecLength() + 1;
// RHF Aug 07, 2000        csprochar*   pRecArea = (csprochar*) calloc( iRecSize, sizeof(csprochar) );
        csprochar*   pRecArea = (csprochar*) calloc( iRecSize+2, sizeof(csprochar) ); // RHF Aug 07, 2000

        pDicX->SetRecordArea( pRecArea );
    }

    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        int     iSymSec = pDicT->SYMTfsec;

        while( iSymSec > 0 ) {
            SECT*   pSecT = SPT(iSymSec);
            SECX*   pSecX = pSecT->GetSecX();

            // build the array of SecOccs for this Sect
            pSecX->BuildSecOccArray();

            // TODO: if( Issamod != ModuleType::Entry ) then set the whole Flags-area to FLAG_HIGHLIGHT
            // ... or charge 'CSecOcc::BuildFlagsArea' to do it

            // next section
            iSymSec = pSecT->SYMTfwd;
        }
    }
}

// RHF INIC Nov 08, 2000
//Add hidden groups for special section. The new ones create in compiling time
bool CIntDriver::CompleteHiddenGroup()
{
    bool bRet = true;

    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        int iSymSec = pDicT->SYMTfsec;

        while( bRet && iSymSec > 0 ) {
            SECT*   pSecT = SPT(iSymSec);

            if( pSecT->IsSpecialSection() ) {
                FLOW*      pOldFlow=m_pEngineDriver->GetFlowInProcess();

                m_pEngineDriver->SetFlowInProcess( Appl.GetFlowAt( 0 ) );
                if( !m_pEngineDriver->AddGroupTForOneSec( iSymSec ) ) {
                    bRet = false;
                    continue;
                }
                pSecT->SetSpecialSection( false );
                m_pEngineDriver->SetFlowInProcess( pOldFlow );
            }

            // next section
            iSymSec = pSecT->SYMTfwd;
        }
    }

    return bRet;
}

// RHF END Nov 08, 2000
