#include "StandardSystemIncludes.h"
#include "Engine.h"
#include "IntDrive.h"


int DICT::Common_DoBuffer(TCHAR* pBuffer, int iFromLevel, int iUntilLevel)
{
    return m_pEngineDriver->DataItemCopy(pBuffer, NULL,
                                         this->CommonInfo.arrayCommonItems,
                                         iFromLevel, iUntilLevel, false );
}


int DICT::DictDoMainKey(TCHAR* pBuffer, const TCHAR* pSourceBuffer, int iFromLevel, int iUntilLevel)
{
    return m_pEngineDriver->DataItemCopy(pBuffer, pSourceBuffer,
                                         this->CommonInfo.arrayCommonItems,
                                         iFromLevel, iUntilLevel, true);
}


int CEngineDriver::DataItemCopy(TCHAR* pTargetBuffer, const TCHAR* pSourceBuffer,
                                std::vector<const CDictItem*>& arrItems,
                                int iFromLevel, int iUntilLevel, bool bCompacted)
{
    // Copy to 'pTargetBuffer' the information of 'pArrItems'.
    // If bCompacted is true, copy all item togheter
    // If bCompacted is false, copy the item in their respective locations
    // iUntilLevel 1 based
    // Return the last position used.
    // WARNING: pArrItem must be order by level.

    bool            bBlank = false;     // for future use
    ASSERT(!arrItems.empty());

    int     iNumItems = (int)arrItems.size();
    int     iLastPos = 0;
    int     iPos = 0;

    for( int iItem = 0; iItem < iNumItems; iItem++ ) {
        const CDictItem* pItem = arrItems[iItem];

        // RHF INIC 24/11/99
        int     iSymVar = pItem->GetSymbol();
        ASSERT( iSymVar > 0 );
        VARX*   pVarX = VPX(iSymVar);
        int     iLevel = pVarX->GetVarT()->GetLevel();

        if( iLevel < iFromLevel )
            continue;

        if( iLevel > iUntilLevel ) { // RHF 24/11/99 Does not generate blanks at end ***
            // if( bCompacted )
            //     break;
            // bBlank = true;
            break;
        }
        // RHF END 24/11/99

        int     iLoc = pItem->GetStart();
        int     iLen = pItem->GetLen();
        TCHAR*  pVarAsciiAddr = 0; // will use its value later: check if it has a valid pointer

        if( !bBlank ) {
            // HINT - common' vars are always true Sing-class (non-array)
            if( pSourceBuffer == NULL )
                pVarAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarX ); // victor Jul 10, 00
            else
                pVarAsciiAddr = (TCHAR*)pSourceBuffer + iLoc - 1;
        }

        if( bCompacted )
        {
            ASSERT( pVarAsciiAddr != 0 );
            _tmemcpy( pTargetBuffer + iPos, pVarAsciiAddr, iLen );
        }
        else {
            iPos = iLoc - 1;
            if( bBlank )
                _tmemset( pTargetBuffer + iPos, _T(' '), iLen );
            else
            {
                ASSERT( pVarAsciiAddr != 0 );
                _tmemcpy( pTargetBuffer + iPos, pVarAsciiAddr, iLen );
            }
        }

        iPos += iLen;

        iLastPos = std::max( iLastPos, iPos );
    }
    ASSERT( iLastPos >= 0 );

    return( iLastPos );
}


void DICT::Common_Start()
{
    // Generate an list of CDEItem for common record regardless the level.
    // One list for each dictionary is generated
    if( !this->CommonInfo.arrayCommonItems.empty() )
        return;

    const CDataDict* pDict = this->GetDataDict();

    for( int i = 0; i <= (int)MaxNumberLevels; i++ ) {
        this->CommonInfo.iCommonFirstPos[i] = MAXLONG;
        this->CommonInfo.iCommonLastPos[i] = 0;
    }

    for( size_t level_number = 0; level_number < pDict->GetNumLevels(); ++level_number ) {
        const DictLevel& dict_level = pDict->GetLevel(level_number);
        const CDictRecord* pRecord = dict_level.GetIdItemsRec();

        for( int iItem = 0; iItem < pRecord->GetNumItems(); iItem++ ) {
            const CDictItem* pItem = pRecord->GetItem(iItem);
            int         iPos = pItem->GetStart();
            int         iLen = pItem->GetLen();

            this->CommonInfo.arrayCommonItems.emplace_back( pItem );

            this->CommonInfo.iCommonFirstPos[level_number+1] = std::min( this->CommonInfo.iCommonFirstPos[level_number+1], iPos );
            this->CommonInfo.iCommonLastPos[level_number+1]  = std::max( this->CommonInfo.iCommonLastPos[level_number+1], iPos + iLen - 1 );

            this->CommonInfo.iCommonFirstPos[0] = std::min( this->CommonInfo.iCommonFirstPos[0],
                                                            this->CommonInfo.iCommonFirstPos[level_number+1] );
            this->CommonInfo.iCommonLastPos[0] = std::max( this->CommonInfo.iCommonLastPos[0],
                                                           this->CommonInfo.iCommonLastPos[level_number+1] );
        }
    }

    Common_DoSect( false );
}


void DICT::Common_End()
{
    if( this->CommonInfo.arrayCommonItems.empty() )
        return;

    Common_DoSect( true );
}


void DICT::Common_DoSect( int bClear )
{
    const CDataDict* pDict = this->GetDataDict();

    for( size_t level_number = 0; level_number < MaxNumberLevels; ++ level_number ) {
        if( bClear ) {
            this->CommonInfo.iCommonSect[level_number] = 0;
        }
        else if( level_number < pDict->GetNumLevels() ) {
            const DictLevel& dict_level = pDict->GetLevel(level_number);
            const CDictRecord* pRecord = dict_level.GetIdItemsRec();
            int iSymSec = pRecord->GetSymbol();
            ASSERT( iSymSec > 0 );

            this->CommonInfo.iCommonSect[level_number] = iSymSec;
        }
        else {
            break;
        }
    }
}


int DICT::Common_GetCommonSection( int iLevel )
{
    ASSERT( iLevel >= 0 );
    return( this->CommonInfo.iCommonSect[iLevel] );
}


void DICT::Common_GetData( int iFromLevel, int iUntilLevel )
{
    ASSERT( iUntilLevel >= 1 );
    ASSERT( !this->CommonInfo.arrayCommonItems.empty() );
    int     iNumItems = (int)this->CommonInfo.arrayCommonItems.size();
    int     iSymDic = this->GetSymbolIndex();
    DICX*   pDicX = DPX(iSymDic);
    TCHAR*  pRecArea = pDicX->GetRecordArea();

    for( int iItem = 0; iItem < iNumItems; iItem++ ) {
        const CDictItem* pItem = this->CommonInfo.arrayCommonItems[iItem];

        // RHF INIC 24/11/99
        int     iSymVar = pItem->GetSymbol();
        ASSERT( iSymVar > 0 );
        VARX*   pVarX = VPX(iSymVar);
        int     iLevel = pVarX->GetVarT()->GetLevel();

        if( iLevel < iFromLevel )
            continue;
        if( iLevel > iUntilLevel )
            break;
        // RHF END 24/11/99

        int     iVarLoc = pItem->GetStart();
        int     iVarLen = pItem->GetLen();
        TCHAR*  pVarAsciiAddr = m_pEngineDriver->m_pIntDriver->GetVarAsciiAddr( pVarX );

        // HINT - common' vars are always true Sing-class (non-array)
        _tmemcpy( pVarAsciiAddr, pRecArea + iVarLoc - 1, iVarLen );
    }
}


int DICT::Common_GetCommonFirstPos( int iLevel )
{
    // Return: 1 based
    // iLevel 1 based. 0 indicating all.
    ASSERT( iLevel >= 0 && iLevel <= maxlevel );
    return( CommonInfo.iCommonFirstPos[iLevel] );
}


int DICT::Common_GetCommonLastPos( int iLevel )
{
    ASSERT( iLevel >= 0 && iLevel <= maxlevel );
    return( CommonInfo.iCommonLastPos[iLevel] );
}


int DICT::Common_GetCommonLen( int iLevel )
{
    int     iLen;

    ASSERT( iLevel >= 0 && iLevel <= maxlevel );

    if( iLevel == 0 )  // Return the complete size!!!
        iLen = CommonInfo.iCommonLastPos[iLevel];
    else
        iLen = ( CommonInfo.iCommonLastPos[iLevel] - CommonInfo.iCommonFirstPos[iLevel] + 1 );

    return( iLen );
}
