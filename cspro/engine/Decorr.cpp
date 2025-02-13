//-----------------------------------------------------------------------
//
//  deCORR  : data entry Corrector module
//
//-----------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "EXENTRY.H"

void CEntryDriver::corr_init() {
    // corr_init: initialize corrector
    DICT*   pDicT = DIP(0);             // input dictionary

    int     iMaxIdLen = 0;

    for( int iLevel = 0; iLevel < (int)MaxNumberLevels; iLevel++ ) {
        if( iMaxIdLen < pDicT->qlen[iLevel] )
            iMaxIdLen = pDicT->qlen[iLevel];
    }
}
