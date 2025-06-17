#include "StdAfx.h"
#include "TableAc.h"
#include "Bitoper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

// -----------------------------
// Method: Constructor
// -----------------------------
CTableAcum::CTableAcum( CBaseTable2* pTable ) {
    m_pBaseTable = pTable;
}

// -----------------------------
// Method: Destructor
// -----------------------------
CTableAcum::~CTableAcum() {
    Free();
}

// -----------------------------
// Method: Alloc
// -----------------------------
bool CTableAcum::Alloc( byte* pInitValue ) {
    CArray<int,int> aDim;

    GetBaseTable()->GetDimSize( aDim );

    ASSERT( aDim.GetSize() <= 3 );
    int iCellSize   = GetBaseTable()->GetCellSize();
    int iNumRows    = aDim.GetAt(0);
    int iNumCols    = (aDim.GetSize() >= 2 ) ? aDim.GetAt(1) : 0;
    int iNumLayers  = (aDim.GetSize() >= 3 ) ? aDim.GetAt(2) : 0;

    if( CAcum::Alloc( iCellSize, iNumRows, iNumCols, iNumLayers, pInitValue ) == NULL )
        return false;

    //if( iCellSize == 16 )
    //    m_iCellSize = 8; // RHF Dec 09, 2005

    return true;
}


// -----------------------------
// Method: Free
// -----------------------------
void CTableAcum::Free() {
    CAcum::Free();
}


// -----------------------------
// Method: Assign
//
// Observation: Assume than
// the objects are csprochar pointer
// -----------------------------
bool CTableAcum::Assign(CTableAcum* pAcum, csprochar cOper ) {
    if( !AreSimilar( pAcum ) ) // fits row,col,layer,numdim and cellsize
        return false;

    // Validate table type
    if( GetBaseTable()->GetTableType() != pAcum->GetBaseTable()->GetTableType()  )
        return false;

    /* RHF INIT Dec 09, 2005
    int     iSubCells=1;
    if(  cOper == '+' ) {
        if( GetBaseTable()->GetTableType() == CTableDef::Ctab_StatMean ) {
            iSubCells = 2;
        }
    }
    RHF END Dec 09, 2005 */

    byte*   pValue;
    byte*   pResult;
     for( int iRow=0; iRow < pAcum->GetNumRows(); iRow++ ) {
    //RHF Dec 09, 2005 for( int iRow=0; iRow < pAcum->GetNumRows(); iRow += iSubCells ) {
        for( int iCol=0; iCol < pAcum->GetNumCols(); iCol++ ) {

            for( int iLayer=0; iLayer < pAcum->GetNumLayers(); iLayer++ ) {

                switch ( cOper ) {
                case _T('+'):
                    pResult = this->GetValue( iRow, iCol, iLayer );
                    pValue = pAcum->GetValue( iRow, iCol, iLayer );

                    ASSERT( pResult != NULL );
                    ASSERT( pValue != NULL );

                     //_ASSERTE(_CrtCheckMemory());
                    AddOper( pResult, pResult, pValue );
                      //_ASSERTE(_CrtCheckMemory());
                    break;
                case _T('m'):  // Min
                    pResult = this->GetValue( iRow, iCol, iLayer );
                    pValue = pAcum->GetValue( iRow, iCol, iLayer );

                    MinOper( pResult, pResult, pValue );
                    break;
                case _T('M'):  // Max
                    pResult = this->GetValue( iRow, iCol, iLayer );
                    pValue = pAcum->GetValue( iRow, iCol, iLayer );

                    MaxOper( pResult, pResult, pValue );
                    break;
                case _T('A'):
                    pValue = pAcum->GetValue( iRow, iCol, iLayer );
                    this->PutValue( pValue, iRow, iCol, iLayer );
                default:
                    break;
                }
            }
        }
    }

    return true;
}

// -----------------------------
// Method: Assign
//
// Observation: Assume than
// the object are csprochar pointer
// -----------------------------
bool CTableAcum::Assign(CTableAcum* pAcum) {
    return Assign( pAcum, 'A' );
}

// -----------------------------
// Method: Addition
//
// Observation: Assume than
// the object are csprochar pointer
// -----------------------------
bool CTableAcum::Addition(CTableAcum* pAcum) {
    switch ( m_pBaseTable->GetTableType() ) {
    case CTableDef::Ctab_Freq:
    case CTableDef::Ctab_FreqDisjoint1Dim:
    case CTableDef::Ctab_FreqDisjoint2Dim:
    case CTableDef::Ctab_FreqDisjoint3Dim:
        return Merge( pAcum );
    default:
        return Assign( pAcum, '+' );
    }
}

// -----------------------------
// Method: Merge
//
// Observation: Assume than
// the object are csprochar pointer
//
// This method is only valid for the struct bellow:
//      CSlotFreq
//      CSlotFreqDisjoint1Dim
//      CSlotFreqDisjoint2Dim
//      CSlotFreqDisjoint3Dim
//
// -----------------------------
bool CTableAcum::Merge(CTableAcum* pAcumParam)
{
    CTableAcum*     pAcumResult;
    CArray<int,int> aDim;
    int*            piThisAcum;
    int             iCommon;
    int             iResultNumCells;
    int             iThisCellSize;
    int             i,j,k;

    CBitoper bitAcum(pAcumParam->GetNumCells());
    piThisAcum = (int *)calloc(this->GetNumCells(), sizeof(int));

    // 1) Scan the acum's cells to define the cells that are equivalente or not.
    //    Assume iRow = N, iCol = 1, iLay = 1
    // 2) Build a new acumresult with the result of the acum merge.
    // 3) Redefine this.acum with the acumresult instance.
    //
    // Summary: this algorithm is order : n*m + (n + m) = n^2 + 2n

    // 1)
    iCommon = 0;
    for ( i = 0; i < this->GetNumCells(); i++ ) {
        piThisAcum[i] = -1;
        for ( j = 0; j < pAcumParam->GetNumCells(); j++ ) {
            if ( !bitAcum.BitGet((long)j) &&
                  EqualFreq(GetValue(i), GetCellSize(),
                            pAcumParam->GetValue(j), pAcumParam->GetCellSize(),
                            m_pBaseTable->GetTableType()) ) {
                piThisAcum[i] = j; // Save the position of the equivalent cells in pAcumParam
                bitAcum.BitPut((long)j, true);
                iCommon++;
                break;
            }
        }
    }

    // 2)
    // The AcumResult will have (this->NumCells + pAcumParam->NumCells - iCommon) cells
    iResultNumCells = this->GetNumCells() + pAcumParam->GetNumCells() - iCommon;

    iThisCellSize = this->GetCellSize();
    if ( this->GetCellSize() < pAcumParam->GetCellSize() )
        m_pBaseTable->SetCellSize(pAcumParam->GetCellSize());


    m_pBaseTable->GetDimSize(aDim);
    aDim.SetAt(0, iResultNumCells);
    m_pBaseTable->SetDimSize(aDim); // falta determinar el máximo largo
    pAcumResult = new CTableAcum(m_pBaseTable);
    pAcumResult->Alloc();

    for ( i = 0; i < this->GetNumCells(); i++ ) {
        if ( piThisAcum[i] == -1 )
            CopyFreqValue(pAcumResult->GetValue(i), pAcumResult->GetCellSize(),
                          this->GetValue(i)      , iThisCellSize,
                          m_pBaseTable->GetTableType());
        else
            AddFreqValue(pAcumResult->GetValue(i), pAcumResult->GetCellSize(),
                         this->GetValue(i)      , iThisCellSize,
                         pAcumParam->GetValue(piThisAcum[i]), pAcumParam->GetCellSize(),
                         m_pBaseTable->GetTableType());
    }

    for (  j = 0, k = i; j < pAcumParam->GetNumCells() && k < iResultNumCells; j++ ) {
        if ( !bitAcum.BitGet((long) j) ) {
            CopyFreqValue(pAcumResult->GetValue(k), pAcumResult->GetCellSize(),
                          pAcumParam->GetValue(j)   , pAcumParam->GetCellSize(),
                          m_pBaseTable->GetTableType());
            k++;
        }
    }

    this->Free();
    this->Alloc();
    csprochar* pcThis = (csprochar*)this->GetAcumArea();
    csprochar* pcRes  = (csprochar*)pAcumResult->GetAcumArea();

    memcpy(pcThis, pcRes, this->GetCellSize() * this->GetNumCells());
    delete pAcumResult;
    free(piThisAcum);

    return true;
}

// -----------------------------
// Method: Min
//
// Observation: Assume than
// the object are csprochar pointer
// -----------------------------
bool CTableAcum::Min(CTableAcum* pAcum) {
    return Assign( pAcum, 'm' );
}

// -----------------------------
// Method: Max
//
// Observation: Assume than
// the object are csprochar pointer
// -----------------------------
bool CTableAcum::Max(CTableAcum* pAcum) {
    return Assign( pAcum, 'M' );
}


// -----------------------------
// Method: AddOper
//
// Complemented to Addition
// -----------------------------
bool CTableAcum::AddOper(byte* pcResult, byte *pcVal1, byte* pcVal2) {
    bool bRc = false;

    switch (m_pBaseTable->GetTableType()) {
    // RHF INIT Dec 12, 2005
    case CTableDef::Ctab_STable:
    case CTableDef::Ctab_Percent:
    case CTableDef::Ctab_Mode:
    case CTableDef::Ctab_Median:
    case CTableDef::Ctab_Percentil:
    case CTableDef::Ctab_Total:
    case CTableDef::Ctab_GrandTotal:
    case CTableDef::Ctab_Mean:
    case CTableDef::Ctab_SMean:
    case CTableDef::Ctab_StatMean:
    case CTableDef::Ctab_StdDev:
    case CTableDef::Ctab_Variance:
    case CTableDef::Ctab_StdErr:
    case CTableDef::Ctab_ValidPct:
    case CTableDef::Ctab_Prop:
        ASSERT( GetCellSize() == sizeof(double) );
    // RHF END Dec 12, 2005

    // --------------------------
    // Double values
    // --------------------------
    case CTableDef::Ctab_Crosstab:
        switch ( GetCellSize() ) {
        case 8: // Double
            double dVal1;
            double dVal2;
            double dValRes;
            dVal1 = *(double*)pcVal1;
            dVal2 = *(double*)pcVal2;
            dValRes = dVal1 + dVal2;
            memcpy(pcResult, (csprochar*)&dValRes, sizeof(double));
            bRc = true;
            break;
        case 4: // Long
            long lVal1;
            long lVal2;
            long lValRes;
            lVal1 = *(long*)pcVal1;
            lVal2 = *(long*)pcVal2;
            lValRes = lVal1 + lVal2;
            memcpy(pcResult, (csprochar*)&lValRes, sizeof(long));
            bRc = true;
            break;
        case 2: // Short
            short sVal1;
            short sVal2;
            short sValRes;
            sVal1 = *(short*)pcVal1;
            sVal2 = *(short*)pcVal2;
            sValRes = sVal1 + sVal2;
            memcpy(pcResult, (csprochar*)&sValRes, sizeof(short));
            bRc = true;
            break;
        }
        break;
        /* RHF COM Dec 12, 2005
    case CTableDef::Ctab_STable:
    case CTableDef::Ctab_Percent:
    case CTableDef::Ctab_Mode:
    case CTableDef::Ctab_Median:
    case CTableDef::Ctab_Percentil:
    case CTableDef::Ctab_Total: // RHF Sep 11, 2002
    case CTableDef::Ctab_GrandTotal: // RHF Jan 30, 2003
        double dVal1;
        double dVal2;
        double dValRes;
        dVal1 = *(double*)pcVal1;
        dVal2 = *(double*)pcVal2;
        dValRes = dVal1 + dVal2;
        memcpy(pcResult, (csprochar*)&dValRes, sizeof(double));
        bRc = true;
        break;

    case CTableDef::Ctab_Mean:
        CSlotMean *pMean1;
        CSlotMean *pMean2;
        CSlotMean sMeanRes;

        pMean1 = (CSlotMean*)pcVal1;
        pMean2 = (CSlotMean*)pcVal2;

        sMeanRes.sumx   = pMean1->sumx   + pMean2->sumx;
        sMeanRes.sumw   = pMean1->sumw   + pMean2->sumw;
        sMeanRes.sumxw  = pMean1->sumxw  + pMean2->sumxw;
        sMeanRes.sumxw2 = pMean1->sumxw2 + pMean2->sumxw2;

        memcpy( pcResult, (csprochar *)&sMeanRes, sizeof(CSlotMean));

        bRc = true;
        break;
    case CTableDef::Ctab_SMean:
        CSlotSMean *pSMean1;
        CSlotSMean *pSMean2;
        CSlotSMean sSMeanRes;

        pSMean1 = (CSlotSMean*)pcVal1;
        pSMean2 = (CSlotSMean*)pcVal2;

        sSMeanRes.num_weight   = pSMean1->num_weight   + pSMean2->num_weight;
        sSMeanRes.num_square   = pSMean1->num_square   + pSMean2->num_square;
        sSMeanRes.den_weight   = pSMean1->den_weight   + pSMean2->den_weight;
        sSMeanRes.den_square   = pSMean1->den_square   + pSMean2->den_square;
        sSMeanRes.den_unweight = pSMean1->den_unweight + pSMean2->den_unweight;
        sSMeanRes.cum_weight   = pSMean1->cum_weight   + pSMean2->cum_weight;
        sSMeanRes.cros_prod    = pSMean1->cros_prod    + pSMean2->cros_prod;
        sSMeanRes.cum_cases    = pSMean1->cum_cases    + pSMean2->cum_cases;

        memcpy(pcResult, (csprochar*)&sSMeanRes, sizeof(CSlotSMean));

        bRc = true;
        break;
    // RHF INIC Sep 11, 2002
    case CTableDef::Ctab_StatMean:
        CStatSlotMean *sSlotMean1;
        CStatSlotMean *sSlotMean2;
        CStatSlotMean sSlotMeanRes;

        sSlotMean1 = (CStatSlotMean *)pcVal1;
        sSlotMean2 = (CStatSlotMean *)pcVal2;

        sSlotMeanRes.m_dSumX     = sSlotMean1->m_dSumX     + sSlotMean2->m_dSumX;
        sSlotMeanRes.m_dNumCases = sSlotMean1->m_dNumCases + sSlotMean2->m_dNumCases;

        memcpy(pcResult, (csprochar*)&sSlotMeanRes, sizeof(CStatSlotMean));

        bRc = true;
        break;
    // RHF END Sep 11, 2002

    case CTableDef::Ctab_StdDev:
    case CTableDef::Ctab_Variance:
    case CTableDef::Ctab_StdErr:
        CStatSlotStdDev *sStdDev1;
        CStatSlotStdDev *sStdDev2;
        CStatSlotStdDev sStdDevRes;

        sStdDev1 = (CStatSlotStdDev *)pcVal1;
        sStdDev2 = (CStatSlotStdDev *)pcVal2;

        sStdDevRes.m_dSumX     = sStdDev1->m_dSumX     + sStdDev2->m_dSumX;
        sStdDevRes.m_dSumX2    = sStdDev1->m_dSumX2    + sStdDev2->m_dSumX2;
        sStdDevRes.m_dNumCases = sStdDev1->m_dNumCases + sStdDev2->m_dNumCases;

        memcpy(pcResult, (csprochar*)&sStdDevRes, sizeof(CStatSlotStdDev));

        bRc = true;
        break;

    case CTableDef::Ctab_ValidPct:
    case CTableDef::Ctab_Prop:
        CStatSlotValidPct *ssVal1;
        CStatSlotValidPct *ssVal2;
        CStatSlotValidPct ssRes;

        ssVal1 = (CStatSlotValidPct *)pcVal1;
        ssVal2 = (CStatSlotValidPct *)pcVal2;

        ssRes.m_dFreq     = ssVal1->m_dFreq     + ssVal2->m_dFreq;
        ssRes.m_dNumCases = ssVal1->m_dNumCases + ssVal2->m_dNumCases;

        memcpy(pcResult, (csprochar*)&ssRes, sizeof(CStatSlotValidPct));

        bRc = true;
        break;

        RHF COM Dec 12, 2005 */
    case CTableDef::Ctab_Hotdeck:
    case CTableDef::Ctab_MinTable:
    case CTableDef::Ctab_MaxTable:
        bRc = false;
        break;
    default:
        bRc = false;
        break;
    }

    return bRc;
}


// -----------------------------
// Method: MinOper
//
// Complemented to Addition
// -----------------------------
bool CTableAcum::MinOper(byte* pcResult, byte *pcVal1, byte* pcVal2) {
    bool bRc;

    switch (m_pBaseTable->GetTableType()) {
    case CTableDef::Ctab_MinTable:
        double dVal1;
        double dVal2;
        double dValRes;
        dVal1 = *(double*)pcVal1;
        dVal2 = *(double*)pcVal2;
        dValRes = std::min(dVal1, dVal2);
        memcpy(pcResult, (csprochar*)&dValRes, sizeof(double));
        bRc = true;
        break;
    default:
        bRc = false;
        break;
    }

    return bRc;
}

// -----------------------------
// Method: MaxOper
//
// Complemented to Addition
// -----------------------------
bool CTableAcum::MaxOper(byte* pcResult, byte *pcVal1, byte* pcVal2) {
    bool bRc;

    switch (m_pBaseTable->GetTableType()) {
    case CTableDef::Ctab_MaxTable:
        double dVal1;
        double dVal2;
        double dValRes;
        dVal1 = *(double*)pcVal1;
        dVal2 = *(double*)pcVal2;
        dValRes = std::max(dVal1, dVal2);
        memcpy(pcResult, (csprochar*)&dValRes, sizeof(double));
        bRc = true;
        break;
    default:
        bRc = false;
        break;
    }

    return bRc;
}

// -----------------------------
// Method: EqualFreq
// -----------------------------
bool CTableAcum::EqualFreq(byte* pbVal1, int iValLen1,
                           byte* pbVal2, int iValLen2,
                           CTableDef::ETableType eTableType) {
    bool    bRc;
    int     iNumDim;


    switch (eTableType) {
    case CTableDef::Ctab_Freq             :
        iNumDim = 0;

        break;
    case CTableDef::Ctab_FreqDisjoint1Dim :
        iNumDim = 1;

        break;
    case CTableDef::Ctab_FreqDisjoint2Dim :
        iNumDim = 2;

        break;
    case CTableDef::Ctab_FreqDisjoint3Dim :
        iNumDim = 3;

        break;
    default:
        return false;
    }

    // Compare m_pCode, assuming that it is in first place
    int iVal1 = CTableAcum::GetCodeSize(iValLen1,iNumDim);
    int iVal2 = CTableAcum::GetCodeSize(iValLen2,iNumDim);
    int iMin = std::min(iVal1, iVal2);
    if ( memcmp( (csprochar*)pbVal1, (csprochar*)pbVal2, iMin ) )
        bRc = false;
    else { // Compare Dim
        bRc = true;
        for ( int i = 0; i < iNumDim; i++ )
            bRc = bRc && (((CSlotFreqDisjoint3Dim*)pbVal1)->aIndex[i] == ((CSlotFreqDisjoint3Dim*)pbVal2)->aIndex[i] );
    }

    return bRc;
}

// -----------------------------
// Method: CopyFreqValue
// -----------------------------
bool CTableAcum::CopyFreqValue(byte* pbVal1, int iValLen1,
                               byte* pbVal2, int iValLen2,
                               CTableDef::ETableType eTableType) {
    int     iNumDim;

    switch (eTableType) {
    case CTableDef::Ctab_Freq             :
        iNumDim = 0;
        break;
    case CTableDef::Ctab_FreqDisjoint1Dim :
        iNumDim = 1;
        break;
    case CTableDef::Ctab_FreqDisjoint2Dim :
        iNumDim = 2;
        break;
    case CTableDef::Ctab_FreqDisjoint3Dim :
        iNumDim = 3;
        break;
    default:
        return false;
    }

    memcpy(((csprochar*)pbVal1) + CTableAcum::GetCodeSize(iValLen1,iNumDim), ((csprochar*)pbVal2) + CTableAcum::GetCodeSize(iValLen2, iNumDim), sizeof(double));

    // Copy m_pCode, assuming that always the pCode value is in first place
    memcpy((csprochar*)pbVal1, (csprochar*)pbVal2, CTableAcum::GetCodeSize(iValLen2, iNumDim));

    if ( iValLen1 > iValLen2 )
        memset(((csprochar*)pbVal1) + CTableAcum::GetCodeSize(iValLen1, iNumDim), _T(' '), iValLen1 - iValLen2);

    // Copy Dim value
    if ( iNumDim > 0 )
        memcpy(((csprochar*)pbVal1) + CTableAcum::GetCodeSize(iValLen1, iNumDim) + sizeof(double),
               ((csprochar*)pbVal2) + CTableAcum::GetCodeSize(iValLen2, iNumDim) + sizeof(double),
               sizeof(int)*iNumDim);


    return true;
}

// -----------------------------
// Method: CopyFreqValue
// -----------------------------
bool CTableAcum::AddFreqValue(byte* pbRes , int iResLen,
                              byte* pbVal1, int iValLen1,
                              byte* pbVal2, int iValLen2,
                              CTableDef::ETableType eTableType) {
    int     iNumDim;
    int     iMaxLen;
    double  dVal1;
    double  dVal2;
    double  dValRes;


    switch (eTableType) {
    case CTableDef::Ctab_Freq             :
        iNumDim = 0;
        break;
    case CTableDef::Ctab_FreqDisjoint1Dim :
        iNumDim = 1;
        break;
    case CTableDef::Ctab_FreqDisjoint2Dim :
        iNumDim = 2;
        break;
    case CTableDef::Ctab_FreqDisjoint3Dim :
        iNumDim = 3;
        break;
    default:
        return false;
    }

    // Assume that all the structures have the freq in second place.
    memcpy((csprochar*)&dVal1, pbVal1 + CTableAcum::GetCodeSize(iValLen1, iNumDim), sizeof(double));
    memcpy((csprochar*)&dVal2, pbVal2 + CTableAcum::GetCodeSize(iValLen2, iNumDim), sizeof(double));

    dValRes = dVal1 + dVal2;

    // Copy m_pCode, assuming that always the pCode value is in first place
    iMaxLen = std::max(CTableAcum::GetCodeSize(iValLen1, iNumDim), GetCodeSize(iValLen2, iNumDim));
    if (iValLen1 >= iValLen2)
        memcpy((csprochar*)pbRes, (csprochar*)pbVal1, iMaxLen);
    else
        memcpy((csprochar*)pbRes, (csprochar*)pbVal2, iMaxLen);

    if ( GetCodeSize(iResLen,iNumDim) > iMaxLen )
        memset(((csprochar*)pbRes) + iMaxLen, _T(' '), GetCodeSize(iResLen, iNumDim) - iMaxLen);

    memcpy(((csprochar*)pbRes) + GetCodeSize(iResLen,iNumDim), (csprochar *)&dValRes, sizeof(double));

    // Copy Dim value
    if ( iNumDim > 0 )
        memcpy(pbRes + GetCodeSize(iResLen,iNumDim) + sizeof(double),
               pbVal1 + CTableAcum::GetCodeSize(iValLen1, iNumDim) + sizeof(double),
               sizeof(int) * iNumDim);

    return true;
}

// -----------------------------
// Method: CopyFreqValue
// -----------------------------
int  CTableAcum::GetCodeSize(int iCellSize, int iNumDim) {
    return (iCellSize - sizeof(double) - (sizeof(int)*(iNumDim)));
}

