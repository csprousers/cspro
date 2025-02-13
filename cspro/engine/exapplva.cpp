//---------------------------------------------------------------------------
//  File name: ExApplVa.cpp
//
//  Description:
//          Vars' fundamental functions for interpreter-driver class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   RHF     Basic conversion
//              31 Mar 00   RCH     Tailoring symbols management
//              12 May 00   vc      Basic customization
//              18 May 00   vc      Enhancing convert of iOccur coming from dOccur
//              10 Jul 00   vc      Adding hard-access for new approach
//              04 Ago 00   RHF     Customize for 3 dimensions
//              27 Oct 00   vc      Adding reaction to "occ-generator" variables
//              10 May 04   RCL     Decrease redundancy
//              25 Jun 04   rcl     index array -> index class
//              25 Jun 04   rcl     1st const correctness effort
//
//---------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Engine.h"
#include "Exappl.h"
#include <zEngineO/WorkVariable.h>
#include <zUtilO/TraceMsg.h>
#include <zAppO/FieldStatus.h>
#include <zCaseO/CaseItemReference.h>
#include <Zissalib/CsDriver.h>

#define RTRACE TRACE


// TODO: check if use of 'entermode' is compatible with recent implementation of 'enter' command
// TODO: check the SYMTowner's are properly used

// === described single-vars


template<typename... IT>
bool UseVisualValue(const CIntDriver* pIntDriver, VARX* pVarX, const IT... index)
{
    VART* pVarT = pVarX->GetVarT();

    // the second condition is for fields not on a form, which always use the visual value
    if( pVarT->IsAlwaysVisualValue() || pVarT->SYMTfrm <= 0 )
        return true;

    // rules from: RHF+vc Oct 13, 00
    csprochar* pFlag;

    if constexpr(sizeof...(index) == 0)
    {
        ASSERT(pVarT->GetFullNumOccs(true) == 1);
        pFlag = pIntDriver->GetSingVarFlagsAddr(pVarX);
    }

    else
    {
        static_assert(sizeof...(index) == 1);
        ASSERT(pVarT->GetFullNumOccs(true) > 1);
        pFlag = pIntDriver->GetMultVarFlagsAddr(pVarT, *std::get<0>(std::tuple<IT...>(index...)));
    }

    int iColor = pIntDriver->GetFlagColor(pFlag);

    // operator-controlled: must be entered or skipped
    if( pIntDriver->m_pEngineSettings->IsPathOff() )
        return ( iColor != FLAG_NOLIGHT );

    // system-controlled: must be entered
    else
        return ( iColor == FLAG_HIGHLIGHT );
}


double CIntDriver::svarvalue(VARX* pVarX) const
{
    // svarvalue: returns the value of a single-var
    // --> tailored for NEW_VERSION                // victor Jul 17, 00
    ASSERT( !pVarX->GetVarT()->IsArray() );        // PROTECTION!  // victor Jul 17, 00
    ASSERT( pVarX->GetVarT()->IsNumeric() );       // PROTECTION!  // victor Jul 17, 00

    return UseVisualValue(this, pVarX) ? GetSingVarFloatValue(pVarX) :
                                         NOTAPPL;
}


double* CIntDriver::svaraddr( VARX* pVarX ) const {
    // svaraddr: gives the float' address of a single-var
    // --> tailored for NEW_VERSION                     // victor Jul 17, 00
    // ... since 'xvaraddr' are always invoked to set a value, the occs of the owner must be affected
    VART*   pVarT=pVarX->GetVarT();

    ASSERT( !pVarT->IsArray() );        // PROTECTION!  // victor Jul 17, 00
    GROUPT* pGroupT    = pVarT->GetOwnerGPT();          // victor May 25, 00
    int     iTotOccs   = pGroupT->GetTotalOccurrences();// victor May 25, 00
    SECT*   pSecT      = pVarX->GetSecT();
    bool    bLikeBatch = ( Issamod != ModuleType::Entry || !DPX(pSecT->SYMTowner)->entermode );

    if( bLikeBatch && iTotOccs < 1 )
        m_pEngineArea->GroupSetCurOccurrence( pGroupT, 1 );

    if( pVarT->IsNumeric() ) {
        // verify if conversion ascii-to-float is needed

        // get the float-address and return
        double* pDouble = GetSingVarFloatAddr( pVarX );

        return pDouble;
    }
    else {
        csprochar*   pVarAsciiAddr = GetSingVarAsciiAddr( pVarX );

        return( (double*) pVarAsciiAddr );
    }
}


// RHF INIC Aug 04, 2000
//TODO Check ranges limit with occurrence tree

double CIntDriver::mvarvalue(VARX* pVarX, double* dOccur) const
{
    ASSERT(pVarX->GetVarT()->IsNumeric()); // PROTECTION!  // rcl, May 12, 04

    try
    {
        int aIndex[DIM_MAXDIM];

        if( pVarX->RemapIndexes( aIndex, dOccur, CHECK_TOTAL, GENERATE_EXCEPTIONS ) )
        {
            CNDIndexes theIndex(ZERO_BASED, aIndex);

            return UseVisualValue(this, pVarX, &theIndex) ? GetMultVarFloatValue(pVarX, theIndex) :
                                                            NOTAPPL;
        }
    }

    catch( const IndexUsedException& e )
    {
        if( e.getCode() != IndexUsedException::ERR_SPECIAL_VALUE_USED )
        {
            const VART* pVarT = pVarX->GetVarT();

            switch( e.getCode() )
            {
                case IndexUsedException::ERR_OVERFLOW_INDEX:
                    issaerror(MessageType::Warning, 1008, pVarT->GetName().c_str(), DoubleToString(e.getIndexUsed()).c_str());
                    break;

                case IndexUsedException::ERR_BAD_LIMITS_INDEX:
                    issaerror(MessageType::Warning, 1008, pVarT->GetName().c_str(), DoubleToString((int)e.getIndexUsed()).c_str());
                    break;

                case IndexUsedException::ERR_BAD_LIMITS_ALMOST_GOOD:
                    issaerror(MessageType::Warning, 1007, pVarT->GetName().c_str(), (int)e.getIndexUsed());
                    break;

                case IndexUsedException::ERR_SPECIAL_VALUE_USED:
                    // will never happen
                    issaerror(MessageType::Warning, 1088, pVarT->GetName().c_str(), e.getIndexUsed());
                    break;
            }

            return DEFAULT;
        }
    }

    return NOTAPPL;
}
// RHF END Aug 04, 2000

double CIntDriver::mvarvalue( VARX* pVarX, double dOccur ) const
{
    if( dOccur == INVALIDINDEX ) // RHF Sep 21, 2001
        return NOTAPPL;// RHF Sep 21, 2001

    // mvarvalue: returns the value of a mult-var for a given occurrence
    // --> tailored for NEW_VERSION                     // victor Jul 17, 00
    VART*   pVarT      = pVarX->GetVarT();
    ASSERT( pVarT->GetNumDim() == 1 );  // PROTECTION!  // victor Jul 17, 00
    ASSERT( pVarT->IsNumeric() );       // PROTECTION!  // victor Jul 17, 00
    GROUPT* pGroupT    = pVarT->GetOwnerGPT();          // victor May 25, 00
    int     iMaxOccs   = pGroupT->GetMaxOccs();         // victor May 25, 00
    int     iTotOccs   = pGroupT->GetTotalOccurrences();// victor May 25, 00
    bool    bGoodIndex = ( dOccur >= INT_MIN && dOccur <= INT_MAX );
    int     iOccur     = ( bGoodIndex ) ? (int) dOccur : -1;

    if( iOccur > 0 && iOccur <= iTotOccs )  {
        // normal case: returns a float value

        // set THE proper index (for 1-dimension only)          // TRANSITION
        CNDIndexes theIndex( ZERO_BASED );
        pVarX->PassTheOnlyIndex( theIndex, iOccur );            // TRANSITION

        return UseVisualValue(this, pVarX, &theIndex) ? GetMultVarFloatValue(pVarX, theIndex) :
                                                        NOTAPPL;
    }
    else if( iOccur > 0 && iOccur <= iMaxOccs ) {
        issaerror( MessageType::Warning, 1007, pVarT->GetName().c_str(), iOccur );
    }
    else if( !bGoodIndex ) {                                        // victor May 18, 00
        issaerror( MessageType::Warning, 1088, pVarT->GetName().c_str(), dOccur ); // victor May 18, 00
    }
    else {
        issaerror( MessageType::Warning, 1008, pVarT->GetName().c_str(), DoubleToString(iOccur).c_str() );
    }

    return DEFAULT;
}

// RHF INIC Aug 04, 2000
double* CIntDriver::mvaraddr( VARX* pVarX, double dOccur[] ) const {
    VART*   pVarT   = pVarX->GetVarT();
    double* pDouble = 0;

    int     aIndex[DIM_MAXDIM];

    try
    {
        if( pVarX->RemapIndexes( aIndex, dOccur, CHECK_TOTAL, GENERATE_EXCEPTIONS ) )
        {
            CNDIndexes theIndex( ZERO_BASED, aIndex );

            // get the Float or the Ascii address based on the indexes-array
            if( pVarT->IsNumeric() )
                pDouble = GetMultVarFloatAddr( pVarX, theIndex );
            else
                pDouble = (double*) GetMultVarAsciiAddr( pVarX, theIndex );
        }

    }
    catch( IndexUsedException& e )
    {
        switch( e.getCode() )
        {
            case IndexUsedException::ERR_OVERFLOW_INDEX:
                issaerror(MessageType::Warning, 1008, pVarT->GetName().c_str(), DoubleToString(e.getIndexUsed()).c_str());
                break;

            case IndexUsedException::ERR_BAD_LIMITS_INDEX:
                issaerror(MessageType::Warning, 1008, pVarT->GetName().c_str(), DoubleToString((int)e.getIndexUsed()).c_str());
                break;

            case IndexUsedException::ERR_BAD_LIMITS_ALMOST_GOOD:
                issaerror(MessageType::Warning, 1007, pVarT->GetName().c_str(), (int)e.getIndexUsed());
                break;

            case IndexUsedException::ERR_SPECIAL_VALUE_USED:
                issaerror(MessageType::Warning, 1088, pVarT->GetName().c_str(), e.getIndexUsed());
                break;
        }

        pDouble = 0;
    }

    return pDouble;
}
// RHF END Aug 04, 2000

double* CIntDriver::mvaraddr( VARX* pVarX, double dOccur ) const {
    // mvaraddr: returns the float' address of a mult-var for a given occurrence
    // ... since 'xvaraddr' are always invoked to set a value, the occs of the owner must be affected
    // --> tailored for NEW_VERSION                     // victor Jul 17, 00
    VART*   pVarT=pVarX->GetVarT();

    ASSERT( pVarT->GetNumDim() == 1 );  // PROTECTION!  // victor Jul 17, 00
    GROUPT* pGroupT    = pVarT->GetOwnerGPT();          // victor May 25, 00
    int     iMaxOccs   = pGroupT->GetMaxOccs();         // victor May 25, 00
    int     iTotOccs   = pGroupT->GetTotalOccurrences();// victor May 25, 00
    bool    bGoodIndex = ( dOccur >= INT_MIN && dOccur <= INT_MAX );
    int     iOccur     = ( bGoodIndex ) ? (int) dOccur : -1;
    double* pDouble    = NULL;

    // BUCEN
    SECT*   pSecT      = pVarX->GetSecT();
    bool    bIsInputDict = (pVarT->GetSubType() == SymbolSubType::Input);
    bool    bLikeBatch = ( Issamod != ModuleType::Entry || !DPX(pSecT->SYMTowner)->entermode );
    if(bLikeBatch && bIsInputDict) {
        bool bValidRange = ( iOccur > 0 && iOccur <= iTotOccs );
        if(!bValidRange) {
            if( iOccur > 0 && iOccur <= iMaxOccs ){
                issaerror( MessageType::Warning, 1007, pVarT->GetName().c_str(), iOccur );
            }
            else if( !bGoodIndex ) { // invalid occurrrence requested
                // victor May 18, 00
                issaerror( MessageType::Warning, 1088, pVarT->GetName().c_str(), dOccur );
            }
            else {
                issaerror( MessageType::Warning, 1008, pVarT->GetName().c_str(), DoubleToString(iOccur).c_str() );
            }

            if( pVarT->IsNumeric() ){
                static double dInvalidValue = 0;
                pDouble = &dInvalidValue;
            }
            return pDouble;
        }
    }
    //BUCEN

    if( iOccur > 0 && iOccur <= iMaxOccs ) {
        SECT*   pSecT      = pVarX->GetSecT();
        bool    bLikeBatch = ( Issamod != ModuleType::Entry || !DPX(pSecT->SYMTowner)->entermode );

        if( bLikeBatch && iTotOccs < iOccur )
            m_pEngineArea->GroupSetCurOccurrence( pGroupT, iOccur );

        // set THE proper index (for 1-dimension only)          // TRANSITION
        CNDIndexes theIndex( ZERO_BASED );
        pVarX->PassTheOnlyIndex( theIndex, iOccur );            // TRANSITION

        // get the Float or the Ascii address based on the indexes-array
        if( pVarT->IsNumeric() )
            pDouble = GetMultVarFloatAddr( pVarX, theIndex );
        else
            pDouble = (double*) GetMultVarAsciiAddr( pVarX, theIndex );

        return pDouble;
    }
    else {
        // invalid occurrrence requested
        if( !bGoodIndex )                               // victor May 18, 00
            issaerror( MessageType::Warning, 1088, pVarT->GetName().c_str(), dOccur );
        else
            issaerror( MessageType::Warning, 1008, pVarT->GetName().c_str(), DoubleToString(iOccur).c_str() );

        if( pVarT->IsNumeric() ) {
            static double dInvalidValue = 0;
            pDouble = &dInvalidValue;
        }
    }

    return pDouble;
}

// RHF COM Aug 04, 2000double* CIntDriver::mvaraddr( int nva, double dOccur ) {
// RHF COM Aug 04, 2000    VARX*   pVarX = VIX(nva);
// RHF COM Aug 04, 2000
// RHF COM Aug 04, 2000    return mvaraddr( pVarX, dOccur );
// RHF COM Aug 04, 2000}


double CIntDriver::GetVarValue( int iSymVar, int iOccur, bool bVisualValue ) const {
    // ... formerly 'getvarvalue', now 'GetVarValue'    // victor May 24, 00
    // ... moved here from IntFuncs.cpp                 // victor May 24, 00
    // iSymVar SOLO puede ser una variable. NO SE ACEPTA WV
    // retorna DEFAULT (o NOTAPPL) si indice fuera de rango
    Symbol* pSymbol=NPT(iSymVar);// RHF Aug 04, 2000

    double  dValue = DEFAULT;

    if( pSymbol->IsA(SymbolType::WorkVariable) ) {       // --- work-Variable:
        dValue = ((WorkVariable*)pSymbol)->GetValue();                  // RHF May 30, 2000
    }
    else if( pSymbol->IsA(SymbolType::Variable) ) {  // --- described Variable:
        // dict' described variable:
        VART*   pVarT = (VART*)pSymbol;
        // TODO: should set dValue to DEFAULT if non-numeric
        ASSERT( pVarT->IsNumeric() );
        // RHF COM Nov 28, 2001 int     maxocc = pVarT->GetMaxOccs();
        int         maxocc = pVarT->GetFullNumOccs(true); // RHF Nov 28, 2001

        if( iOccur < 1 || iOccur > maxocc )
            dValue = ( bVisualValue ) ? DEFAULT : NOTAPPL;
        // TODO: check iOccur against 'current occurrence' and decide some dValue
        else {
            VARX*   pVarX = pVarT->GetVarX();                   // victor May 24, 00

            // set THE proper index (for 1-dimension only)      // TRANSITION
            CNDIndexes theIndex( ZERO_BASED );
            pVarX->PassTheOnlyIndex( theIndex, iOccur );            // TRANSITION

            if( bVisualValue ) {
                if( pVarT->IsArray() )                              // victor Jul 24, 00
                    dValue = GetMultVarFloatValue( pVarX, theIndex ); // victor Jul 24, 00
                else                                                // victor Jul 24, 00
                    dValue = GetSingVarFloatValue( pVarX );         // victor Jul 24, 00
            }
            else {
                if( pVarT->IsArray() )
                    dValue = mvarvalue( pVarX, (double) iOccur );
                else
                    dValue = svarvalue( pVarX );
            }
        }
    }

    return dValue;
}


TCHAR* CIntDriver::GetVarAsciiValue( double dValue, TCHAR* pAsciiVal/* = nullptr*/ ) const
{
    // if pAsciiVal is not set, it will allocate memory as it previously did in the next function
    if( pAsciiVal == nullptr )
        pAsciiVal = (csprochar*)malloc(80);

    if( IsSpecial(dValue) )
    {
        _tcscpy(pAsciiVal, SpecialValues::ValueToString(dValue));
    }

    else
    {
        _stprintf(pAsciiVal,_T("%0.6f"),dValue);

        // right-trim the zeroes
        csprochar* pStr = pAsciiVal + _tcslen(pAsciiVal) - 1;

        for( ; pStr >= pAsciiVal; pStr-- )
        {
            if( *pStr != _T('0') )
            {
                // don't display a decimal character if this is an integer
                if( *pStr == _T('.') )
                    pStr--;

                break;
            }
        }

        *( pStr + 1 ) = 0; // end the string
    }

    return pAsciiVal;
}

TCHAR* CIntDriver::GetVarAsciiValue( int iSymVar, int iOccur, bool bVisualValue ) const
{
    // Retorna BLANCOS o DEFAULT si ocurrencia esta fuera de rango.
    // Retorna NULL si no hay memoria.
    // Si iOccur es <= 0 entonces usa la ocurrencia actual
    Symbol* pSymbol = NPT(iSymVar);
    SymbolType eType = pSymbol->GetType();
    TCHAR* pAsciiVal = nullptr;

    if( eType == SymbolType::WorkVariable ) // --- work-Variable:
    {
        // build the ascii image from its double' value-holder
        WorkVariable* pWVar = (WorkVariable*)pSymbol;
        double dValue = pWVar->GetValue();
        pAsciiVal = GetVarAsciiValue(dValue);
    }

    else if( eType == SymbolType::Variable )            // --- described Variable:
    {
        // build the ascii image from its double' value-holder
        VART* pVarT = (VART*)pSymbol;
        VARX* pVarX = pVarT->GetVarX();

        if( iOccur <= 0 )               // get current occurrence if iOccur <= 0
            iOccur = pVarT->GetOwnerGPT()->GetCurrentOccurrences(); // victor May 25, 00

        // 1e+50 produce en formato %f del orden de 57 caracteres
        // El valor 11 con formato %f se expande a 11.00000
        pAsciiVal = (csprochar*)malloc( ( pVarT->GetLength() + 10 ) * sizeof(csprochar)); // GHM 20120130  * sizeof(csprochar) for unicode

        if( pAsciiVal != NULL )
        {
            if( pVarT->IsNumeric() )
            {
                double dValue = GetVarValue( iSymVar, iOccur, bVisualValue );
                GetVarAsciiValue(dValue,pAsciiVal);
            }

            else
            {
                csprochar* p = NULL; //SAVY 01/12/04 for CAPI in designer

                if( pVarX )
                {
                    if( pVarT->IsArray() )
                        p = (csprochar*) mvaraddr( pVarX, (double) iOccur );
                    else
                        p = (csprochar*) svaraddr( pVarX );
                }

                if( p != NULL )
                    _tmemcpy( pAsciiVal, p, pVarT->GetLength() );

                else
                    _tmemset( pAsciiVal, _T(' '), pVarT->GetLength() );

                pAsciiVal[pVarT->GetLength()] = 0;
            }
        }
    }

    return pAsciiVal;
}


/////////////////////////////////////////////////////////////////////////////
//
// --- field' flags management: get/set "color"
//
/////////////////////////////////////////////////////////////////////////////


int CIntDriver::GetFieldColor( int iSymVar, int iOccur ) const {
    VARX*   pVarX=VPX(iSymVar);

    // set THE proper index (for 1-dimension only)          // TRANSITION
    CNDIndexes theIndex( ZERO_BASED );
    pVarX->PassTheOnlyIndex( theIndex, iOccur );              // TRANSITION

    return( GetFieldColor( pVarX, theIndex ) );
}

void CIntDriver::SetFieldColor( int iSymVar, int iOccur, csprochar cColor ) {
    VARX*   pVarX=VPX(iSymVar);

    // set THE proper index (for 1-dimension only)          // TRANSITION
    CNDIndexes theIndex( ZERO_BASED );
    pVarX->PassTheOnlyIndex( theIndex, iOccur );            // TRANSITION

    SetFieldColor( cColor, pVarX, theIndex );
}

int CIntDriver::GetFieldColor( int iSymVar, const CNDIndexes& theIndex ) const { // rcl, Jun 25, 2004
    ASSERT( iSymVar > 0 );
    ASSERT( theIndex.isZeroBased() );
    ASSERT( theIndex.isValid() );

    return GetFieldColor( VPX(iSymVar), theIndex );
}

//////////////////////////////////////////////////////////////////////////
// rcl 'Extensions', Jun, 2004

int CIntDriver::GetFieldColor( VARX* pVarX, const CNDIndexes& theIndex ) const  // rcl Jun 25, 04
{
// ... index class version: 'theIndex' must be 0-based
    ASSERT( theIndex.isZeroBased() );
    ASSERT( pVarX != 0 );

    return GetFieldColor( pVarX->GetVarT(), theIndex );
}

int CIntDriver::GetFieldColor( VART* pVarT, const CNDIndexes& theIndex ) const  // rcl Jun 25, 04
{
// ... index class version: 'theIndex' must be 0-based
    ASSERT( theIndex.isZeroBased() );
    ASSERT( pVarT != 0 );

    csprochar* pFlag = GetVarFlagsAddr( pVarT, theIndex );
    int     iFlagColor = ( pFlag != NULL ) ? GetFlagColor( pFlag ) : FLAG_NOLIGHT;

    return iFlagColor;
}

int CIntDriver::GetFieldColor( VARX* pVarX ) const  // rcl Jun 27,, 04
{
    csprochar* pFlag = GetVarFlagsAddr( pVarX );
    int   iFlagColor = ( pFlag != NULL ) ? GetFlagColor( pFlag ) : FLAG_NOLIGHT;

    return iFlagColor;
}

int CIntDriver::GetFieldColor( int iSymVar ) const { // rcl Jun 27, 04
    ASSERT( iSymVar > 0 );

    return GetFieldColor( VPX(iSymVar) );
}

void CIntDriver::SetFieldColor( csprochar cColor, int iSymVar, const CNDIndexes& theIndex ) { // victor Jul 22, 00
    ASSERT( iSymVar > 0 );
    ASSERT( theIndex.isZeroBased() );

    SetFieldColor( cColor, VPX(iSymVar), theIndex );
}

void CIntDriver::SetFieldColor( csprochar cColor, VART* pVarT, const CNDIndexes& theIndex ) { // victor Jul 22, 00
    ASSERT( theIndex.isZeroBased() );
    SetFieldColor( cColor, pVarT->GetVarX(), theIndex );
}

void CIntDriver::SetFieldColor( csprochar cColor, VARX* pVarX, const CNDIndexes& theIndex ) { // victor Jul 22, 00
    // ... new, index-array version: 'aIndex' must be 0-based
    ASSERT( theIndex.isZeroBased() );

    csprochar*   pFlag = GetVarFlagsAddr( pVarX, theIndex );

    if( pFlag != NULL )
        SetFlagColor( pFlag, cColor );
}


/////////////////////////////////////////////////////////////////////////////
//
// --- flags' deep-management: get/set "color"
//
//              ... color: low order 4-bits
//
/////////////////////////////////////////////////////////////////////////////

int CIntDriver::GetFlagColor( csprochar* pFlag ) const {
    int     iFlagColor = ( *pFlag & FLAG_maskCOLOR );

    return iFlagColor;
}

void CIntDriver::SetFlagColor( csprochar* pFlag, csprochar cColor ) {
    csprochar    cFlagOther = ( *pFlag & FLAG_maskNOCOL );
    csprochar    cFlagColor = ( cColor ? cColor & FLAG_maskCOLOR : FLAG_NOLIGHT );

    *pFlag = cFlagOther | cFlagColor;
}


FieldStatus CIntDriver::GetFieldStatus(const CaseItem& case_item, const CaseItemIndex& index)
{
    ASSERT(assert_cast<CEntryDriver*>(m_pEngineDriver) != nullptr);

    const CDictItem& dict_item = case_item.GetDictionaryItem();

    int aIndex[3] =  { static_cast<int>(index.GetRecordOccurrence()),
                       static_cast<int>(index.GetItemOccurrence()),
                       static_cast<int>(index.GetSubitemOccurrence()) };

    CNDIndexes theIndex(ZERO_BASED, aIndex);

    VART* pVarT = VPT(dict_item.GetSymbol());
    const int field_color = GetFieldColor(pVarT, theIndex);

    if( field_color == FLAG_HIGHLIGHT )
        return FieldStatus::Visited;

    if( field_color == FLAG_MIDLIGHT )
        return FieldStatus::Skipped;

    // if not entered, check if this is the current field
    ASSERT(field_color == FLAG_NOLIGHT);

    DEFLD3* defld = assert_cast<CEntryDriver*>(m_pEngineDriver)->GetCsDriver()->GetCurDeFld();

    if( defld != nullptr && defld->GetSymbol() == pVarT->GetSymbolIndex() )
    {
        ItemIndex current_field_index;
        ConvertIndex(defld->GetIndexes(), current_field_index);

        if( current_field_index == index )
            return FieldStatus::Current;
    }

    return FieldStatus::Unvisited;
}


/////////////////////////////////////////////////////////////////////////////
//
// ---  hard-access to Flags
//
//      Remark: this family of hard-access' methods follow a new approach for
//              memory allocation of data-areas, based on separated areas
//              (including Flags/Ascii/Floats) for each Sect' occurrence, and
//              it correspond to the current implementation of 'exallocd.cpp'.
//              Both VART and VARX versions are provided for each method.
//
//              The client must take care of the following rules:
//
//      Rule 1: these methods assume no checking is needed, specifically:
//
//              ... on the nature of the Var (scalar or array): scalar' Vars
//                      should use GetSingVar' methods, array' Vars should
//                      use GetMultVar' methods
//              ... on the type of Var (numeric or alpha): a failure will
//                      arise when asking for Float values of an alpha Var
//              ... on the indexes provided: it is assumed that the indexes
//                      are 0-based.  It is also assumed the indexes are
//                      valid for the data structure (the indexes do not
//                      exceed the max-occs of each data element)
//
//              Some ASSERTs partially look for these restrictions.
//              Under _DEBUG, the quality is checked by using 'CheckIndexArray'
//
//      Rule 2: the GetMultVar... methods receive an array of occurrence'
//              indexes (see 'aIndex') with DIM_MAXDIM entries.  The indexes'
//              array MUST always contain
//
//              ... 2 indexes if requesting for an Item,
//              ... 3 indexes if requesting for a SubItem
//
//      Rule 3: client must provide every index up to the actual number of
//              indexes needed by the requested Var, and
//
//              ... unneeded, inner indexes MUST come with a 0-value
//              ... remaining, uneeded entries should be filled with -1s
//
//              For current DIM_MAXDIM = 3, the only possibilities are:
//
//                  requesting for                         expected array
//                  --------------                         --------------
//
//                  Sing-Item in Sing-Sect                   [0][0][0]
//                  Sing-Item in Mult-Sect                   [i][0][0]
//                  Mult-Item in Sing-Sect                   [0][j][0]
//                  Mult-Item in Mult-Sect                   [i][j][-1]
//                  Mult-SubItem of Sing-Item in Sing-Sect   [0][0][k]
//                  Mult-SubItem of Sing-Item in Mult-Sect   [i][0][k]
//                  Mult-SubItem of Mult-Item in Sing-Sect   [0][j][k]
//                  Mult-SubItem of Mult-Item in Mult-Sect   [i][j][k]
//
//              Remark that a failure (maybe a memory violation) will arise
//              when a negative value is found for a required index.
//
/////////////////////////////////////////////////////////////////////////////

bool CIntDriver::SetVarFloatValue( double dValue, VARX* pVarX, const CNDIndexes& theIndex ) {


    ASSERT( theIndex.isZeroBased() );

    // ... generic: for both Sing or Mult
    VART*   pVarT = pVarX->GetVarT();
    ASSERT( pVarT->IsNumeric() );
    if( !pVarT->IsArray() )
        return SetVarFloatValueSingle( dValue, pVarX );

    ASSERT( pVarT->IsArray() );

    double* pValue = GetMultVarFloatAddr( pVarX, theIndex );
    bool    bDone = false;

    if( pValue != NULL ) {
        *pValue = dValue;

//TODO: SetVarRelatedsValidFloat( false, pVarX, theIndex );
//TODO: SetVarRelatedsValidAscii( false, pVarX, theIndex );

        bDone = true;
    }

    bool    bLikeBatch = ( Issamod != ModuleType::Entry || !pVarT->GetSPT()->GetDicT()->GetDicX()->entermode ); // RHF Jun 12, 2001

    bLikeBatch = bLikeBatch || !pVarT->IsInAForm(); // RHF Feb 12, 2003

    if( dValue != NOTAPPL ) // DON'T ADD OCCURRENCES WHEN ASSIGN A NOTAPPL TO AN ITEM!!!// RHF Nov 08, 2001
    // --> check if the var is "occurrences-generator"  // victor Oct 27, 00
    if( pVarT->IsOccGenerator() || bLikeBatch ) { // RHF Jun 12, 2001 Add bLikeBatch for fix problem in CopyCase
        int     iOldOccurrence;
        int     iNewOccurrence = 1;     // enough for non-array vars

        GROUPT* pParentGroupT = pVarT->GetParentGPT();

        ASSERT( pParentGroupT != 0 );
        ASSERT( pVarT->GetParentGPT() == pVarT->GetOwnerGPT() );

        iOldOccurrence = pParentGroupT->GetTotalOccurrences();

        if( pParentGroupT->GetDimType() != CDimension::Record )
            iOldOccurrence = pParentGroupT->GetTotalOccurrences( theIndex );

        int     iDimType = pParentGroupT->GetDimType();
        ASSERT( iDimType >= 0 && iDimType <= 2 );

        iNewOccurrence = theIndex.getIndexValue(iDimType) + 1;
        ASSERT( iNewOccurrence > 0 );


        // if needed, install the new occurrence using the normalized engine' method
        if( iNewOccurrence > iOldOccurrence ) {

            int iSymVar = pVarT->GetSymbolIndex();
            // theIndex is using all dimensions, but
            // from here and on we need to specify which
            // dimensions are being used
            C3DIndexes theFixedIndex = theIndex;
            int iWhichDimsToUse = 0;
            int aWhich[DIM_MAXDIM] = { USE_DIM_1, USE_DIM_2, USE_DIM_3 };
            for( int i = 0; i < pVarT->GetNumDim(); i++ )
            {
                iWhichDimsToUse |= aWhich[ pVarT->GetDimType(i) ];
            }
            theFixedIndex.specifyIndexesUsed(iWhichDimsToUse);

            C3DObject theObject;
            theObject.SetSymbol( iSymVar );
            theObject.setIndexes( theFixedIndex );
            theObject.setIndexValue( iDimType, iNewOccurrence );

            m_pEngineArea->GroupSetCurOccurrenceUsingVar( theObject );
            // m_pEngineArea->GroupSetCurOccurrence( iSymVar, iNewOccurrence );

            pParentGroupT->SetTotalOccurrences( theIndex, iNewOccurrence );
        }
    }

    return bDone;
}

bool CIntDriver::SetVarFloatValueSingle( double dValue, VARX* pVarX ) { // rcl, Jun 22 2004

    // ... only for Single variables
    VART*   pVarT = pVarX->GetVarT();
    ASSERT( pVarT->IsNumeric() );
    ASSERT( !pVarT->IsArray() );

    double* pValue = GetSingVarFloatAddr( pVarX );
    bool    bDone = false;

    if( pValue != NULL ) {
        *pValue = dValue;

        bDone = true;
    }

    bool    bLikeBatch = ( Issamod != ModuleType::Entry || !pVarT->GetSPT()->GetDicT()->GetDicX()->entermode ); // RHF Jun 12, 2001

    bLikeBatch = bLikeBatch || !pVarT->IsInAForm(); // RHF Feb 12, 2003

    if( dValue != NOTAPPL ) { // DON'T ADD OCCURRENCES WHEN ASSIGN A NOTAPPL TO AN ITEM!!!// RHF Nov 08, 2001
        // --> check if the var is "occurrences-generator"  // victor Oct 27, 00
        if( pVarT->IsOccGenerator() || bLikeBatch ) { // RHF Jun 12, 2001 Add bLikeBatch for fix problem in CopyCase
            int     iOldOccurrence;
            int     iNewOccurrence = 1;     // enough for non-array vars

            GROUPT* pOwnerGroupT = pVarT->GetOwnerGPT();
            if( pOwnerGroupT != NULL )
                iOldOccurrence = pOwnerGroupT->GetTotalOccurrences();
            else
                iOldOccurrence = iNewOccurrence; // Do Nothing. Profile items!!

            // if needed, install the new occurrence using the normalized engine' method
            if( iNewOccurrence > iOldOccurrence ) {

                int iSymVar = pVarT->GetSymbolIndex();
                m_pEngineArea->GroupSetCurOccurrence( iSymVar, iNewOccurrence );
            }
        }
    }

    return bDone;
}

bool CIntDriver::SetVarAsciiValue( csprochar* pAscii, VARX* pVarX, int* aIndex ) {
    // ... generic: for both Sing or Mult
    VART*   pVarT = pVarX->GetVarT();
    ASSERT( !pVarT->IsNumeric() );
#ifdef  _DEBUG
    if( !pVarT->IsArray() )
    {
        CNDIndexes theIndex( ZERO_BASED, aIndex );
        if( !CheckIndexArray( pVarT, theIndex ) )
            return false;
    }
#endif//_DEBUG

    csprochar*   pAsciiAddr = ( !pVarT->IsArray() ) ?
                         GetSingVarAsciiAddr( pVarX ) : GetMultVarAsciiAddr( pVarX, aIndex );
    bool    bDone = false;

    if( pAsciiAddr != NULL ) {
        _tmemcpy( pAsciiAddr, pAscii, pVarT->GetLength() );
// RHF COM Jul 27, 2000        SetFieldValidAscii( true, pVarX, aIndex );

        // this Set...Relateds below affect only subItems (not VSets)
//TODO: SetVarRelatedsValidAscii( true, pVarX, aIndex );

        // this Set...Relateds below affect both subItems and VSets
//TODO: SetVarRelatedsValidFloat( false, pVarX, aIndex );

        bDone = true;
    }

    bool    bLikeBatch = ( Issamod != ModuleType::Entry || !pVarT->GetSPT()->GetDicT()->GetDicX()->entermode ); // RHF Jun 12, 2001

    bLikeBatch = bLikeBatch || !pVarT->IsInAForm(); // RHF Feb 12, 2003

    // --> check if the var is "occurrences-generator"  // victor Oct 27, 00
    if( pVarT->IsOccGenerator() || bLikeBatch ) { // RHF Jun 12, 2001 Add bLikeBatch for fix problem in CopyCase
        int     iOldOccurrence;
        int     iNewOccurrence = 1;     // enough for non-array vars

        if( pVarT->IsArray() ) {
            GROUPT* pParentGroupT = pVarT->GetParentGPT();
            iOldOccurrence = pParentGroupT->GetTotalOccurrences();

            ASSERT( pVarT->GetNumDim() == 1 );   // TRANSITION
            int     iDimType = pParentGroupT->GetDimType();
            ASSERT( iDimType >= 0 && iDimType <= 2 );

            iNewOccurrence = aIndex[iDimType] + 1;
            ASSERT( iNewOccurrence > 0 );
        }
        else {
            GROUPT* pOwnerGroupT = pVarT->GetOwnerGPT();
            if( pOwnerGroupT != NULL )
                iOldOccurrence = pOwnerGroupT->GetTotalOccurrences();
            else
                iOldOccurrence = iNewOccurrence; // Do Nothing. Profile items!!
        }

        // if needed, install the new occurrence using the normalized engine' method
        if( iNewOccurrence > iOldOccurrence ) {

            int iSymVar = pVarT->GetSymbolIndex();
            m_pEngineArea->GroupSetCurOccurrence( iSymVar, iNewOccurrence );
        }
    }

    return bDone;
}

double CIntDriver::GetVarFloatValue( VART* pVarT ) const { // rcl, Jun 25 2004

    ASSERT( pVarT != 0 );
    ASSERT( !pVarT->IsArray() );

    double dValue = GetSingVarFloatValue( pVarT );
    return dValue;
}

double CIntDriver::GetVarFloatValue( VARX* pVarX ) const { // rcl, Jun 25 2004
    ASSERT( pVarX != 0 );

    return GetVarFloatValue( pVarX->GetVarT() );
}

double CIntDriver::GetVarFloatValue( VART* pVarT, const CNDIndexes& theIndex ) const // rcl Jun 25 2004
{
    ASSERT( pVarT != 0 );

#ifdef  _DEBUG
    if( !pVarT->IsArray() )
    {
        if( !CheckIndexArray( pVarT, theIndex ) )
            return DEFAULT;
    }
#endif//_DEBUG

    double dValue;

    if( pVarT->IsArray() )
    {
        dValue = GetMultVarFloatValue( pVarT, theIndex );
    }
    else
    {
        dValue = GetSingVarFloatValue( pVarT );
    }

    return dValue;
}

double CIntDriver::GetVarFloatValue( VARX* pVarX, const CNDIndexes& theIndex ) const // rcl Jun 25 2004
{
    return GetVarFloatValue( pVarX->GetVarT(), theIndex );
}

//////////////////////////////////////////////////////////////////////////

double* CIntDriver::GetVarFloatAddr( VART* pVarT, const CNDIndexes& theIndex ) const { // victor Jul 22, 00
    // ... generic: for both Sing or Mult

#ifdef  _DEBUG
    if( !pVarT->IsArray() )
    {
        if( !CheckIndexArray( pVarT, theIndex ) )
            return NULL;
    }
#endif//_DEBUG

    double* pFloat = ( !pVarT->IsArray() ) ?
        GetSingVarFloatAddr( pVarT ) : GetMultVarFloatAddr( pVarT, theIndex );

    return pFloat;
}

double* CIntDriver::GetVarFloatAddr( VARX* pVarX, const CNDIndexes& theIndex  ) const { // victor Jul 22, 00
    // ... generic: for both Sing or Mult

    return GetVarFloatAddr( pVarX->GetVarT(), theIndex );
}

double* CIntDriver::GetVarFloatAddr( VART* pVarT ) const { // rcl, Jun 25 2004
    // ... specific: only for Sing vars
    ASSERT( !pVarT->IsArray() );

    double* pFloat = GetSingVarFloatAddr( pVarT );

    return pFloat;
}

double* CIntDriver::GetVarFloatAddr( VARX* pVarX ) const { // rcl, Jun 25 2004
    // ... specific: only for Sing vars

    return GetVarFloatAddr( pVarX->GetVarT() );
}

//////////////////////////////////////////////////////////////////////////

csprochar* CIntDriver::GetVarAsciiAddr( VARX* pVarX, const CNDIndexes& theIndex ) const
{
    ASSERT( theIndex.isZeroBased() );
    return GetVarAsciiAddr( pVarX->GetVarT(), theIndex );
}

csprochar* CIntDriver::GetVarAsciiAddr( VART* pVarT, const CNDIndexes& theIndex ) const
{
    ASSERT( theIndex.isZeroBased() );

#ifdef  _DEBUG
    if( !pVarT->IsArray() )
    {
        if( !CheckIndexArray( pVarT, theIndex ) )
            return NULL;
    }
#endif//_DEBUG

    csprochar*   pAscii = ( !pVarT->IsArray() ) ?
        GetSingVarAsciiAddr( pVarT ) : GetMultVarAsciiAddr( pVarT, theIndex );

    return pAscii;
}

// GetVarAsciiAddr for single variables

csprochar* CIntDriver::GetVarAsciiAddr( VARX* pVarX ) const { // rcl, Jun 25 2004
    // ... specific: for Sing variable

    return GetVarAsciiAddr( pVarX->GetVarT() );
}

csprochar* CIntDriver::GetVarAsciiAddr( VART* pVarT ) const { // rcl, Jun 25 2004
    // ... specific: for Sing variable
    ASSERT( !pVarT->IsArray() );

    csprochar*   pAscii = GetSingVarAsciiAddr( pVarT );

    return pAscii;
}

//////////////////////////////////////////////////////////////////////////

csprochar* CIntDriver::GetVarFlagsAddr( VARX* pVarX, const CNDIndexes& theIndex ) const
{
    ASSERT( pVarX != 0 );
    return GetVarFlagsAddr( pVarX->GetVarT(), theIndex );
}

csprochar* CIntDriver::GetVarFlagsAddr( VART* pVarT, const CNDIndexes& theIndex ) const
{
    // ... generic: for both Sing or Mult
    ASSERT( pVarT != 0 );

#ifdef  _DEBUG
    if( !pVarT->IsArray() )
    {
        if( !CheckIndexArray( pVarT, theIndex ) )
            return NULL;
    }
#endif//_DEBUG

    csprochar*   pFlag = ( !pVarT->IsArray() ) ?
        GetSingVarFlagsAddr( pVarT ) : GetMultVarFlagsAddr( pVarT, theIndex );

    return pFlag;
}

csprochar* CIntDriver::GetVarFlagsAddr( VARX* pVarX ) const   // rcl, Jun 27, 04
{
    ASSERT( pVarX != 0 );

    return GetVarFlagsAddr( pVarX->GetVarT() );
}

csprochar* CIntDriver::GetVarFlagsAddr( VART* pVarT ) const   // rcl, Jun 27, 04
{
    ASSERT( pVarT != 0 );
    ASSERT( !pVarT->IsArray() );

    csprochar*   pFlag = GetSingVarFlagsAddr( pVarT );

    return pFlag;
}

//////////////////////////////////////////////////////////////////////////

double CIntDriver::GetSingVarFloatValue( VART* pVarT ) const {    // victor Jul 10, 00
    return GetSingVarFloatValue( pVarT->GetVarX() );
}

double CIntDriver::GetSingVarFloatValue( VARX* pVarX ) const {    // victor Jul 10, 00
    VART*   pVarT = pVarX->GetVarT();
    ASSERT( pVarT->IsNumeric() );
    double* pFloatAddr  = GetSingVarFloatAddr( pVarX );
    double  pFloatValue = ( pFloatAddr != NULL ) ? *pFloatAddr : DEFAULT;

    return pFloatValue;
}

//////////////////////////////////////////////////////////////////////////

double CIntDriver::GetMultVarFloatValue( VARX* pVarX ) const // rcl, Jun 24 2004
{
    return GetMultVarFloatValue( pVarX->GetVarT() );
}

double CIntDriver::GetMultVarFloatValue( VART* pVarT ) const // rcl, Jun 24 2004
{
    ASSERT( pVarT->IsNumeric() );
    double  pFloatValue = DEFAULT;

    return pFloatValue;
}

double CIntDriver::GetMultVarFloatValue( VART* pVarT, const CNDIndexes& theIndex ) const // rcl, Jun 24 2004
{
    ASSERT( pVarT != 0 );
    ASSERT( pVarT->IsNumeric() );
    VARX* pVarX = pVarT->GetVarX();
    double* pFloatAddr  = GetMultVarFloatAddr( pVarX, theIndex );
    double  pFloatValue = ( pFloatAddr != NULL ) ? *pFloatAddr : DEFAULT;

    return pFloatValue;
}

double CIntDriver::GetMultVarFloatValue( VARX* pVarX, const CNDIndexes& theIndex ) const // rcl, Jun 24 2004
{
    ASSERT( pVarX != 0 );
    return GetMultVarFloatValue( pVarX->GetVarT(), theIndex );
}

//////////////////////////////////////////////////////////////////////////

double* CIntDriver::GetSingVarFloatAddr( VART* pVarT ) const {    // victor Jul 10, 00
    return GetSingVarFloatAddr( pVarT->GetVarX() );
}

double* CIntDriver::GetSingVarFloatAddr( VARX* pVarX ) const {    // victor Jul 10, 00
    VART*   pVarT = pVarX->GetVarT();
    SECT*   pSecT = pVarX->GetSecT();
    SECX*   pSecX = pSecT->GetSecX();
    ASSERT( !pVarT->IsArray() );

    // 1. base-address of the record
    double* pFloatArea = pSecX->GetFloatAreaAtOccur( 1 );
    ASSERT( pFloatArea != NULL );

    // 2. displacement of this Var
    int     iFloatDisp = pVarX->GetIndToFloat();
    ASSERT( iFloatDisp >= 0 ); // RHF Aug 23, 2000

    // 3. final address
    double* pFloatAddr = NULL;
    //Savy: To fix the crash when an item is used as occurrence control on a roster
    // and the item is from a record that is never used on any form.
    if(pFloatArea != NULL && iFloatDisp >= 0)
        pFloatAddr = pFloatArea + iFloatDisp;

    return pFloatAddr;
}

//////////////////////////////////////////////////////////////////////////

double* CIntDriver::GetMultVarFloatAddr( VART* pVarT, const CNDIndexes& theIndex ) const { // rcl, Jun 25, 04
    return GetMultVarFloatAddr( pVarT->GetVarX(), theIndex );
}

double* CIntDriver::GetMultVarFloatAddr( VARX* pVarX, const CNDIndexes& theIndex ) const { // rcl, Jun 25, 04
    VART*   pVarT = pVarX->GetVarT();
    SECT*   pSecT = pVarX->GetSecT();
    SECX*   pSecX = pSecT->GetSecX();
    ASSERT( pVarT->IsArray() );
//#ifdef  _DEBUG
    if( !CheckIndexArray( pVarT, theIndex ) )
        return NULL;
//#endif//_DEBUG

    ASSERT( theIndex.isZeroBased() );

    // 1. base-address of the record' occurrence
    double* pFloatArea = pSecX->GetFloatAreaAtOccur( theIndex.getIndexValue(0) + 1 );
    ASSERT( pFloatArea != NULL );

    // 2. displacement of this Var
    int     iFloatDisp;
    VART*   pOwnerVarT = pVarT->GetOwnerVarT();

    if( pOwnerVarT == NULL ) {  // this Var is an Item
        iFloatDisp = pVarX->GetIndToFloat() + theIndex.getIndexValue(1);
    }
    else {                      // this Var is a SubItem
        int iSubItemOccs = pVarT->GetMaxOccs();
        int iItemOccDisp = theIndex.getIndexValue(1) * iSubItemOccs;

        iFloatDisp = pVarX->GetIndToFloat() + iItemOccDisp + theIndex.getIndexValue(2);
    }

    ASSERT( iFloatDisp >= 0 ); // RHF Aug 23, 2000

    // 3. final address
    double* pFloatAddr = pFloatArea + iFloatDisp;

    return pFloatAddr;
}

//////////////////////////////////////////////////////////////////////////

csprochar*  CIntDriver::GetSingVarAsciiAddr( VART* pVarT ) const { // victor Jul 10, 00
    return GetSingVarAsciiAddr( pVarT->GetVarX() );
}

csprochar*  CIntDriver::GetSingVarAsciiAddr( VARX* pVarX ) const { // victor Jul 10, 00
    VART*   pVarT = pVarX->GetVarT();
    SECT*   pSecT = pVarX->GetSecT();
    SECX*   pSecX = pSecT->GetSecX();
    ASSERT( !pVarT->IsArray() );

    // 1. base-address of the record
    csprochar*   pAsciiArea = pSecX->GetAsciiAreaAtOccur( 1 );
    ASSERT( pAsciiArea != NULL );

    // 2. displacement of this Var
    int     iAsciiDisp = pVarX->GetIndToAscii();

    // 3. final address
    csprochar*   pAsciiAddr = ( iAsciiDisp >= 0 ) ? pAsciiArea + iAsciiDisp : NULL;
    ASSERT( pAsciiAddr != NULL );       // can't be called for VSets

    return pAsciiAddr;
}

//////////////////////////////////////////////////////////////////////////

csprochar* CIntDriver::GetMultVarAsciiAddr( VART* pVarT, int* aIndex ) const { // victor Jul 10, 00
    return GetMultVarAsciiAddr( pVarT->GetVarX(), aIndex );
}

csprochar* CIntDriver::GetMultVarAsciiAddr( VARX* pVarX, int* aIndex ) const { // victor Jul 10, 00
    VART*   pVarT = pVarX->GetVarT();
    SECT*   pSecT = pVarX->GetSecT();
    SECX*   pSecX = pSecT->GetSecX();
    ASSERT( pVarT->IsArray() );
#ifdef  _DEBUG
    CNDIndexes theIndex( ZERO_BASED, aIndex );
    if( !CheckIndexArray( pVarT, theIndex ) )
        return NULL;
#endif//_DEBUG

    // 1. base-address of the record' occurrence
    csprochar*   pAsciiArea = pSecX->GetAsciiAreaAtOccur( aIndex[0] + 1 );
    ASSERT( pAsciiArea != NULL );

    // 2. displacement of this Var
    int     iAsciiDisp;
    VART*   pOwnerVarT = pVarT->GetOwnerVarT();

    if( pOwnerVarT == NULL ) {  // this Var is an Item
        iAsciiDisp = pVarX->GetIndToAscii() + aIndex[1] * pVarT->GetLength();
    }
    else {                      // this Var is a SubItem
        VARX*   pOwnerVarX   = pOwnerVarT->GetVarX();
        int     iItemDisp    = pOwnerVarX->GetIndToAscii() + aIndex[1] * pOwnerVarT->GetLength();
        int     iSubItemGap  = pVarT->GetLocation() - pOwnerVarT->GetLocation();
        int     iSubItemDisp = iSubItemGap + aIndex[2] * pVarT->GetLength();

        iAsciiDisp = iItemDisp + iSubItemDisp;
    }

    // 3. final address
    csprochar*   pAsciiAddr = ( iAsciiDisp >= 0 ) ? pAsciiArea + iAsciiDisp : NULL;
    ASSERT( pAsciiAddr != NULL );       // can't be called for VSets

    return pAsciiAddr;
}

csprochar* CIntDriver::GetMultVarAsciiAddr( VART* pVarT, const CNDIndexes& theIndex ) const { // rcl, Jun 22, 2004
    ASSERT( theIndex.isZeroBased() );

    int aIndex[DIM_MAXDIM];

    for( int i = 0; i < DIM_MAXDIM; i++ )
    {
        aIndex[i] = theIndex.getIndexValue(i);
    }

    return GetMultVarAsciiAddr( pVarT, aIndex );
}

csprochar* CIntDriver::GetMultVarAsciiAddr( VARX* pVarX, const CNDIndexes& theIndex ) const { // rcl, Jun 22, 2004
    ASSERT( theIndex.isZeroBased() );
    int aIndex[DIM_MAXDIM];

    for( int i = 0; i < DIM_MAXDIM; i++ )
    {
        aIndex[i] = theIndex.getIndexValue(i);
    }

    return GetMultVarAsciiAddr( pVarX->GetVarT(), aIndex );
}

//////////////////////////////////////////////////////////////////////////

csprochar* CIntDriver::GetSingVarFlagsAddr( VART* pVarT ) const {  // victor Jul 10, 00
    return GetSingVarFlagsAddr( pVarT->GetVarX() );
}

csprochar* CIntDriver::GetSingVarFlagsAddr( VARX* pVarX ) const {  // victor Jul 10, 00
    VART*   pVarT = pVarX->GetVarT();
    SECT*   pSecT = pVarX->GetSecT();
    SECX*   pSecX = pSecT->GetSecX();
    ASSERT( !pVarT->IsArray() );

    // 1. base-address of the record
    csprochar*   pFlagsArea = pSecX->GetFlagsAreaAtOccur( 1 );
    ASSERT( pFlagsArea != NULL );

    // 2. displacement of this Var
    int     iFlagsDisp = pVarX->GetIndToFlags();
    ASSERT( iFlagsDisp >= 0 ); // RHF Aug 23, 2000

    // 3. final address
    csprochar*   pFlagsAddr = pFlagsArea + iFlagsDisp;

    return pFlagsAddr;
}

//////////////////////////////////////////////////////////////////////////

csprochar* CIntDriver::GetMultVarFlagsAddr( VART* pVarT, const CNDIndexes& theIndex ) const // rcl, Jun 23, 2004
{
    return GetMultVarFlagsAddr( pVarT->GetVarX(), theIndex );
}

csprochar* CIntDriver::GetMultVarFlagsAddr( VARX* pVarX, const CNDIndexes& theIndex ) const // rcl, Jun 23, 2004
{
    ASSERT( theIndex.isZeroBased() );

    VART*   pVarT = pVarX->GetVarT();
    SECT*   pSecT = pVarX->GetSecT();
    SECX*   pSecX = pSecT->GetSecX();
    ASSERT( pVarT->IsArray() );
#ifdef  _DEBUG
    if( !CheckIndexArray( pVarT, theIndex ) )
        return NULL;
#endif//_DEBUG

    // 1. base-address of the record' occurrence
    csprochar*   pFlagsArea = pSecX->GetFlagsAreaAtOccur( theIndex.getIndexValue(0) + 1 );
    ASSERT( pFlagsArea != NULL );

    // 2. displacement of this Var
    int     iFlagsDisp;
    VART*   pOwnerVarT = pVarT->GetOwnerVarT();

    if( pOwnerVarT == NULL ) {  // this Var is an Item
        iFlagsDisp = pVarX->GetIndToFlags() + theIndex.getIndexValue(1);
    }
    else {                      // this Var is a SubItem
        int     iSubItemOccs = pVarT->GetMaxOccs();
        int     iItemOccDisp = theIndex.getIndexValue(1) * iSubItemOccs;

        iFlagsDisp = pVarX->GetIndToFlags() + iItemOccDisp + theIndex.getIndexValue(2);
    }

    ASSERT( iFlagsDisp >= 0 ); // RHF Aug 23, 2000

    // 3. final address
    csprochar*   pFlagsAddr = pFlagsArea + iFlagsDisp;

    return pFlagsAddr;
}

bool CIntDriver::CheckIndexArray( const VART* pVarT, const CNDIndexes& theIndex ) const {  // victor Jul 22, 00
    bool    bIsValid = true;

    bIsValid = theIndex.isValid(); // checks every index value is >= 0
    if( pVarT->IsArray() && bIsValid ) {
            // checking dimension-sizes (passed index are 0-based)
          #define CHECK_DIM_OK(x)  \
          ( \
           (theIndex.isZeroBased() && theIndex.getIndexValue(x) < pVarT->GetMaxOccsInFixedDim(x)) || \
           (theIndex.getIndexValue(x) <= pVarT->GetMaxOccsInFixedDim(x)) \
          )
          bIsValid = CHECK_DIM_OK(0) &&
                     CHECK_DIM_OK(1) &&
                     CHECK_DIM_OK(2);
          // TODO: (still linked to the GetRIType2' problem?)
    }

    ASSERT( bIsValid ); // RHF Jan 23, 2001
    return bIsValid;
}



std::unique_ptr<C3DObject> CIntDriver::ConvertIndex(const CaseItem& case_item, const ItemIndex& item_index)
{
    const CDictItem& dict_item = case_item.GetDictionaryItem();

    auto the3dObject = std::make_unique<C3DObject>();
    the3dObject->SetSymbol(dict_item.GetSymbol());

    // the C3DObject index is one-based for applicable indices
    int record_occurrence = 0;
    int item_occurrence = 0;
    int subitem_occurrence = 0;

    if( dict_item.GetRecord()->GetMaxRecs() > 1 )
    {
        record_occurrence = item_index.GetRecordOccurrence() + 1;
    }

    if( dict_item.GetItemSubitemOccurs() > 1 )
    {
        if( dict_item.GetItemType() == ItemType::Item )
        {
            item_occurrence = item_index.GetItemOccurrence() + 1;
        }

        else
        {
            if( dict_item.GetOccurs() > 1 )
            {
                subitem_occurrence = item_index.GetSubitemOccurrence() + 1;
            }

            else
            {
                item_occurrence = item_index.GetItemOccurrence() + 1;
            }
        }
    }

    the3dObject->setIndexValue(0, record_occurrence);
    the3dObject->setIndexValue(1, item_occurrence);
    the3dObject->setIndexValue(2, subitem_occurrence);

    return the3dObject;
}


std::unique_ptr<C3DObject> CIntDriver::ConvertIndex(const CaseItemReference& case_item_reference)
{
    return ConvertIndex(case_item_reference.GetCaseItem(), case_item_reference);
}


void CIntDriver::ConvertIndex(const C3DObject& the3dObject, ItemIndex& item_index)
{
    item_index.SetRecordOccurrence(std::max(0, the3dObject.getIndexValue(0) - 1));
    item_index.SetItemOccurrence(std::max(0, the3dObject.getIndexValue(1) - 1));
    item_index.SetSubitemOccurrence(std::max(0, the3dObject.getIndexValue(2) - 1));
}

void CIntDriver::ConvertIndex(const C3DIndexes& the3dObject, ItemIndex& item_index)
{
    item_index.SetRecordOccurrence(std::max(0, the3dObject.getIndexValue(0) - 1));
    item_index.SetItemOccurrence(std::max(0, the3dObject.getIndexValue(1) - 1));
    item_index.SetSubitemOccurrence(std::max(0, the3dObject.getIndexValue(2) - 1));
}
