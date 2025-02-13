//---------------------------------------------------------------------------
//
//  nextnode.cpp: DAT files access, case' nodes management
//
//---------------------------------------------------------------------------
#define IMSA
#include "StandardSystemIncludes.h"
#include "Exappl.h"
#include <zToolsO/Tools.h>
#include "Engine.h"
#include "Batdrv.h"
#include <zDictO/DDClass.h>

//////////////////////////////////////////////////////////////////////////////
//
//
// CEngineDriver::methods
//
//
//////////////////////////////////////////////////////////////////////////////
void CEngineDriver::BuildGroupOccsFromNodeRecords( DICT* pDicT, int iNodeLevel ) { // victor May 16, 00
    //return; // SPEED RHF Mar 14, 2001
    // BuildGroupOccsFromNodeRecords: transfer all Sect' occurrences in node-level to related Groups
    if( iNodeLevel > 0 ) {
        int     iSymSec = pDicT->SYMTfsec;

        while( iSymSec > 0 ) {
            SECT*   pSecT = SPT(iSymSec);

            if( pSecT->GetLevel() == iNodeLevel ) {
                // generate associated Grpup' occurrences
                SetupGroupOccsFromSect( pSecT );
            }

            iSymSec = pSecT->SYMTfwd;
        }

        // RHF INIC Aug 23, 2002 Clean Output info only when there is at least 1 output dict
        if( GetHasOutputDict() && pDicT->GetSubType() == SymbolSubType::Input )
        {
            for( DICT* pDicTOutput : m_engineData->dictionaries_pre80 )
            {
                if( pDicTOutput->GetSubType() == SymbolSubType::Output )
                    ClearLevelNode( pDicTOutput, iNodeLevel );
            }
        }
        // RHF END Aug 23, 2002
    }
}
