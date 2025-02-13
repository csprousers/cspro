#include "StandardSystemIncludes.h"
#include "INTERPRE.H"

//---------------------------------------------------------------------------
//  File name: tabfuncs.cpp
//
//  Description:
//          Implementation xtab driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              03 Jul 01   RHF     Redo for Multi tally support
//              11 Jul 02   RHF     Redo for Unit,SubTables,Overlapping categories
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// return position of 'value' in a var-node 'iCtNode' (-1 if fail)
//------------------------------------------------------------------------
// position of value in a var-node
int CIntDriver::ctvarpos( int iCtNode, double dValue, int iSeq ) {
    ASSERT( iSeq >= 1 );
    double   rValue = ct_ivalue( iCtNode, dValue );      // see HINT below

    CTNODE*     pNode = (CTNODE *) ( CtNodebase + iCtNode );
    CTRANGE*    pRange;

    int     iFirstRange = iCtNode + CTNODE_SLOTS;
    int     iLastRange = iFirstRange + CTRANGE_SLOTS * ( pNode->m_iCtNumRanges );
    int     relpos = 0;
    bool    bFitRange = false;
    int     iSeqNumber=0;

    /* RHF COM INIC Jul 11, 2002
    for( int i = iFirstRange; i < iLastRange && !bFitRange; i += CTRANGE_SLOTS ) {
        pRange = (CTRANGE *) ( CtNodebase + i );
        bFitRange = ( iValue >= pRange->m_iRangeLow && iValue <= pRange->m_iRangeHigh );
        if( bFitRange )
            relpos += iValue - pRange->m_iRangeLow;
        else
            relpos += pRange->m_iRangeHigh - pRange->m_iRangeLow + 1;
    }
    RHF COM END Jul 11, 2002 */

    // RHF INIC Jul 11, 2002
    int     iVector;
    int     lastrelpos=0;
    for( int i = iFirstRange; i < iLastRange && !bFitRange; i += CTRANGE_SLOTS ) { // Variable Mapping
        pRange = (CTRANGE *) ( CtNodebase + i );
        bFitRange = pRange->fitInside(rValue);
        if( bFitRange ) {
            if( pRange->m_iRangeCollapsed == 0 ) {// No collapse
                lastrelpos = relpos;
                relpos += rValue - pRange->m_iRangeLow;
            }
            else if( pRange->m_iRangeCollapsed == 1 ) // Only the first collapsed range count a cell
                relpos++;
            else // Do nothing with the rest collapsed ranges
                ;

            if( pRange->m_iRangeCollapsed >= 1 ) {
                ASSERT( relpos >= 1 );
                iVector = relpos-1;
            }
            else
                iVector = relpos;

            iSeqNumber++;
            if( iSeq == iSeqNumber ) {
                relpos = iVector;
                continue;
            }

            bFitRange = false;

            // Advance the full range if no collapsed
            if( pRange->m_iRangeCollapsed == 0 ) {
                relpos = lastrelpos + pRange->getNumCells();
            }
        }
        else {
            relpos += pRange->getNumCells();
        }
    }
    // RHF END Jul 11, 2002

    if( !bFitRange )
        relpos = -1;

    return( relpos );
}

//------------------------------------------------------------------------
// Return an 'iValue' that fits in ranges (for out-of-range: CTDEFAULT)
//------------------------------------------------------------------------
double CIntDriver::ct_ivalue( int  iCtNode, double  dValue ) {
    /*RHF COM Sep 12, 2005
    if( dValue == NOTAPPL )
        return( VAL_CTNOTAPPL );

    if( dValue == MISSING )
        return( VAL_CTMISSING );

        */

    // RHF INIT Sep 12, 2005
    CTNODE*     pNode = (CTNODE *) ( CtNodebase + iCtNode );
    if( dValue == NOTAPPL ) {
        bool bInValueSet  = ( pNode->m_iFlags & ct_NOTAPPL ) != 0;
        dValue = VAL_CTNOTAPPL;
        if( bInValueSet )
            return dValue;
    }
    else if( dValue == MISSING ){
        bool bInValueSet  = ( pNode->m_iFlags & ct_MISSING ) != 0;
        dValue = VAL_CTMISSING;
        if( bInValueSet )
            return dValue;
    }
    else if( dValue == REFUSED ){
        bool bInValueSet  = ( pNode->m_iFlags & ct_REFUSED ) != 0;
        dValue = VAL_CTREFUSED;
        if( bInValueSet )
            return dValue;
    }
    else if( dValue == DEFAULT ) {
        bool bInValueSet  = ( pNode->m_iFlags & ct_DEFAULT ) != 0;
        dValue = VAL_CTDEFAULT;
        if( bInValueSet )
            return dValue;
    }
    else {
    // RHF END Sep 12, 2005
        if( dValue <= -VAL_CTUNDEFINED || dValue >= VAL_CTUNDEFINED )
            return VAL_CTDEFAULT;
    }

    double rValue = (double) dValue;

    //--------------------------------------------------------
    // HINT: the calculation made by 'ct_ivalue' to setup the
    //       'iValue' should be changed in the next future,
    //       when no explicit range was specified (when an
    //       explicit range is present, the 'value' must be
    //       tested to fit in a range and, if it does not fit,
    //       it should be set to an impossible value, i.e. the
    //       value 20000). This modification will require to
    //       add to CTNODE the nature of the range used
    //       (it could be a copy of the full source range, or
    //       a true explicit range specified by the user).
    //---------------------------------------------------------

    //CTNODE*     pNode = (CTNODE *) ( CtNodebase + iCtNode );
    CTRANGE*    pRange;

    int iFirstRange = iCtNode + CTNODE_SLOTS;
    int iLastRange = iFirstRange + CTRANGE_SLOTS * pNode->m_iCtNumRanges;

    bool    bFitRange = false;
    for( int i = iFirstRange; i < iLastRange && !bFitRange; i += CTRANGE_SLOTS ) {
        pRange = (CTRANGE *) ( CtNodebase + i );
        bFitRange = pRange->fitInside(rValue);
    }

    if( !bFitRange )
        rValue = VAL_CTUNDEFINED;

    return( rValue );
}

// RHF COM Jul 15, 2002//------------------------------------------------------------------------
// RHF COM Jul 15, 2002// Return the highest value of the node. Ignore special values
// RHF COM Jul 15, 2002//------------------------------------------------------------------------
// RHF COM Jul 15, 2002double CEngineArea::cthighnumvalue( int iCtNode ) {
// RHF COM Jul 15, 2002    // MaxValue is initialized with the lower negative.
// RHF COM Jul 15, 2002    int         iFirstRange, iLastRange, i, MaxValue=-CTTOTAL; // -10003
// RHF COM Jul 15, 2002    CTNODE*     pNode;
// RHF COM Jul 15, 2002    CTRANGE*    pRange;
// RHF COM Jul 15, 2002
// RHF COM Jul 15, 2002    pNode = (CTNODE *) ( CtNodebase + iCtNode );
// RHF COM Jul 15, 2002
// RHF COM Jul 15, 2002    iFirstRange = iCtNode + CTNODE_SLOTS;
// RHF COM Jul 15, 2002    iLastRange = iFirstRange + CTRANGE_SLOTS * ( pNode->m_iCtNumRanges );
// RHF COM Jul 15, 2002    for( i = iFirstRange; i < iLastRange; i += CTRANGE_SLOTS ) {
// RHF COM Jul 15, 2002        pRange = (CTRANGE *) ( CtNodebase + i );
// RHF COM Jul 15, 2002
// RHF COM Jul 15, 2002        // Special range
// RHF COM Jul 15, 2002        if( pRange->m_iRangeLow >= CTMISSING )
// RHF COM Jul 15, 2002            continue;
// RHF COM Jul 15, 2002
// RHF COM Jul 15, 2002        // Contains a special value
// RHF COM Jul 15, 2002        if( pRange->m_iRangeLow < CTMISSING && pRange->m_iRangeHigh >= CTMISSING ) {
// RHF COM Jul 15, 2002            MaxValue = CTMISSING - 1;  // 9999
// RHF COM Jul 15, 2002            break;
// RHF COM Jul 15, 2002        }
// RHF COM Jul 15, 2002
// RHF COM Jul 15, 2002        if( pRange->m_iRangeHigh > MaxValue )
// RHF COM Jul 15, 2002            MaxValue = pRange->m_iRangeHigh;
// RHF COM Jul 15, 2002    }
// RHF COM Jul 15, 2002
// RHF COM Jul 15, 2002    return( MaxValue );
// RHF COM Jul 15, 2002}

// RHF INIC Jul 15, 2002
//------------------------------------------------------------------------
// Return the LAST value of the node. Ignore special values
//------------------------------------------------------------------------
double CEngineArea::cthighnumvalue( int iCtNode ) {
    // MaxValue is initialized with the lower negative.
    double       rLastValidValue=-VAL_CTTOTAL; // used to be -10003
    CTRANGE*    pRange;

    CTNODE*     pNode = (CTNODE *) ( CtNodebase + iCtNode );

    int   iFirstRange = iCtNode + CTNODE_SLOTS;
    int   iLastRange = iFirstRange + CTRANGE_SLOTS * ( pNode->m_iCtNumRanges );
    for( int i = iFirstRange; i < iLastRange; i += CTRANGE_SLOTS ) {
        pRange = (CTRANGE *) ( CtNodebase + i );

        // Special range
        if( pRange->m_iRangeLow >= VAL_CTMISSING )
            continue;

        // Contains a special value
        if( pRange->m_iRangeLow < VAL_CTMISSING && pRange->m_iRangeHigh >= VAL_CTMISSING ) {
            rLastValidValue = VAL_CTMISSING - 1;  // 9999
            continue;
        }

        rLastValidValue = pRange->m_iRangeHigh;
    }

    return( rLastValidValue );
}
// RHF END Jul 15, 2002

