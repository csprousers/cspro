//---------------------------------------------------------------------------
//  File name: GroupT2.cpp
//
//  Description:
//          Implementation of execution GroupT methods
//
//  History:    Date       Author   Comment
//              ---------------------------
//              09 Aug 00   RHF     Created
//              17 Aug 00   RHF     Add occurrences tree support
//              23 Aug 00   RHF     Fix problem with alpha variables
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include "groupt2.h"
#include <engine/Tables.h>
#include <engine/Engdrv.h>
#include <engine/IntDrive.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>


#if defined(_DEBUG) && defined(WIN_DESKTOP)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


// InsertOcc
bool GROUPT::InsertOcc( const CNDIndexes& theIndex, const CLoopControl* pLoopControl ) {
    std::vector<int> m_aTarget;// Keep the source occurrence

    m_aTarget.emplace_back( -1 );

    // Last Occurrence is missing if UntilOcc==MaxOcc
    int     iMax=(pLoopControl->GetUntilOcc()==pLoopControl->GetMaxOcc()) ?
                  pLoopControl->GetUntilOcc()-1 : pLoopControl->GetUntilOcc();

    for( int iOcc=pLoopControl->GetFromOcc(); iOcc <= iMax; iOcc++ )
        m_aTarget.emplace_back( iOcc );

    CLoopInstruction cLoopInstruction( GrouptInsertOcc, &m_aTarget );

    return DoMoveOcc( theIndex, pLoopControl, &cLoopInstruction );
}

// DeleteOcc
bool GROUPT::DeleteOcc( const CNDIndexes& theIndex, const CLoopControl* pLoopControl ) {
    std::vector<int> m_aTarget; // Keep the source occurrence

    for( int iOcc=pLoopControl->GetFromOcc()+1; iOcc <= pLoopControl->GetUntilOcc(); iOcc++ )
        m_aTarget.emplace_back( iOcc );

    if( pLoopControl->GetUntilOcc() == pLoopControl->GetMaxOcc() )
        m_aTarget.emplace_back( -1 );
    else
        m_aTarget.emplace_back( pLoopControl->GetUntilOcc()+1 );

    CLoopInstruction cLoopInstruction( GrouptDeleteOcc, &m_aTarget );

    return DoMoveOcc( theIndex, pLoopControl, &cLoopInstruction );
}


struct SortGroupInfo
{
    VART* pVarT;
    bool ascending;
    int occurrence;
    void* data;
    SortGroupInfo* next_sort_group_info;
};

static inline int CompareProc(const void* elem1, const void* elem2)
{
    const SortGroupInfo* sort_info1 = (SortGroupInfo*)elem1;
    const SortGroupInfo* sort_info2 = (SortGroupInfo*)elem2;

    while( true )
    {
        int comparison = 0;

        if( sort_info1->pVarT->IsNumeric() )
        {
            double* value1 = (double*)sort_info1->data;
            double* value2 = (double*)sort_info2->data;

            if( *value1 < *value2 )
                comparison = -1;

            else if( *value1 > *value2 )
                comparison = 1;
        }

        else
        {
            TCHAR* value1 = (TCHAR*)sort_info1->data;
            TCHAR* value2 = (TCHAR*)sort_info2->data;
            comparison = _tmemcmp(value1, value2, sort_info1->pVarT->GetLength());
        }

        if( comparison != 0 )
        {
            if( !sort_info1->ascending )
                comparison *= -1;

            return comparison;
        }

        if( sort_info1->next_sort_group_info != nullptr )
        {
            // proceed along the linked link
            sort_info1 = sort_info1->next_sort_group_info;
            sort_info2 = sort_info2->next_sort_group_info;
            continue;
        }

        // if at the end of the list of variables, sort based on the occurrece number
        return ( sort_info1->occurrence < sort_info2->occurrence ) ? -1 : 1;
    }
}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GROUPT::SortOcc(const CNDIndexes& theIndex, const CLoopControl* pLoopControl, int iSymVar, bool bAscending)
{
    std::vector<int> sort_symbols = { iSymVar * ( bAscending ? 1 : -1 ) };
    return SortOcc(theIndex, pLoopControl, sort_symbols, nullptr);
}


bool GROUPT::SortOcc(const CNDIndexes& theIndex, const CLoopControl* pLoopControl, const std::vector<int>& sort_symbols, std::vector<int>* validIndices)
{
    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    int iNumValues = ( pLoopControl->GetUntilOcc() - pLoopControl->GetFromOcc() + 1 );

    if( validIndices != nullptr )
    {
        // no need to sort if nothing meets the search criteria
        if( validIndices->empty() )
            return true;

        iNumValues = (int)validIndices->size();
    }

    ASSERT(iNumValues >= 1);

    // prepare the variables
    std::vector<VART*> varts;
    std::vector<bool> ascendings;

    for( int sort_symbol : sort_symbols )
    {
        VART* pVarT = VPT(abs(sort_symbol));
        varts.emplace_back(pVarT);
        ascendings.emplace_back(sort_symbol >= 0);

        if( Issamod == ModuleType::Batch )   // RHF Sep 28, 2005
            m_pEngineDriver->prepvar( pVarT, NO_VISUAL_VALUE );  // RHF Sep 28, 2005
    }

    int sort_infos_size = iNumValues;
    auto sort_infos = std::make_unique<SortGroupInfo[]>(sort_infos_size);

    CNDIndexes theIndexTarget = theIndex;

    int whereIndex = 0; // GHM 20110810

    // Generate array for sorting
    int iValue = 0;

    for( int iTargetOcc = pLoopControl->GetFromOcc(); iTargetOcc <= pLoopControl->GetUntilOcc(); iTargetOcc++, iValue++ )
    {
        if( validIndices != nullptr )
        {
            if( whereIndex < (int)validIndices->size() && iTargetOcc == ( validIndices->at(whereIndex) - 1 ) )
            {
                iValue = whereIndex;
                whereIndex++;
            }

            else
            {
                continue;
            }
        }

        theIndexTarget.setIndexValue(pLoopControl->GetLoopIndex(), iTargetOcc);

        SortGroupInfo* sort_info = &sort_infos[iValue];

        for( size_t i = 0; i < varts.size(); i++ )
        {
            if( i > 0 )
            {
                sort_info->next_sort_group_info = new SortGroupInfo;
                sort_info = sort_info->next_sort_group_info;
            }

            sort_info->pVarT = varts[i];
            sort_info->ascending = ascendings[i];
            sort_info->occurrence = iTargetOcc;
            sort_info->next_sort_group_info = nullptr;

            VARX* pVarX = varts[i]->GetVarX();

            if( sort_info->pVarT->IsNumeric() )
            {
                m_pEngineDriver->varatof(pVarX, theIndexTarget);
                sort_info->data = pIntDriver->GetVarFloatAddr(pVarX, theIndexTarget);
            }

            else
            {
                sort_info->data = pIntDriver->GetVarAsciiAddr(pVarX, theIndexTarget);
            }
        }
    }

    // Sort
    qsort(sort_infos.get(), sort_infos_size, sizeof(SortGroupInfo), CompareProc);

    std::vector<int> m_aTarget; // Keep the source occurrence

    if( validIndices != nullptr )
    {
        iNumValues = pLoopControl->GetUntilOcc() - pLoopControl->GetFromOcc() + 1;

        for( iValue = 0; iValue < iNumValues; iValue++ ) // create an array that would imply nothing is sorted
            m_aTarget.emplace_back(iValue);

        // replace the sort targets with their proper indices
        for( iValue = 0; iValue < (int)validIndices->size(); iValue++ )
        {
            m_aTarget[validIndices->at(iValue) - 1] = sort_infos[iValue].occurrence;
        }
    }

    else
    {
        // Fill m_aTarget array
        for( iValue = 0; iValue < iNumValues; iValue++ )
            m_aTarget.emplace_back(sort_infos[iValue].occurrence);
    }

    // delete the linked list
    for( int i = 0; i < sort_infos_size; i++ )
    {
        SortGroupInfo* this_sort_group_info = sort_infos[i].next_sort_group_info;

        while( this_sort_group_info != nullptr )
        {
            SortGroupInfo* next_sort_group_info = this_sort_group_info->next_sort_group_info;
            delete this_sort_group_info;
            this_sort_group_info = next_sort_group_info;
        }
    }

    // Ready
    CLoopInstruction cLoopInstruction(GrouptSortOcc, &m_aTarget);

    return DoMoveOcc(theIndex, pLoopControl, &cLoopInstruction);
}


bool GROUPT::DoMoveOcc( const CNDIndexes& theIndex, const CLoopControl* pLoopControl, const CLoopInstruction* pLoopInstruction )
{
    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    csprochar*  pAscii = NULL;
    csprochar*  pAsciiTarget = NULL;
    csprochar*  pFlag = NULL;
    csprochar*  pFlagTarget = NULL;
    double      dValue;
    int         iSymb;
    CNDIndexes  theIndexTarget( ONE_BASED, DEFAULT_INDEXES );
    bool        bRet=true;
    bool        bInsert=pLoopInstruction->m_eMode == GrouptInsertOcc;
    bool        bDelete=pLoopInstruction->m_eMode == GrouptDeleteOcc;
    bool        bSort  =pLoopInstruction->m_eMode == GrouptSortOcc;
    bool        bPathOff=m_pEngineDriver->m_pEngineSettings->IsPathOff();
    bool        bChangeOcc=false;

    bChangeOcc = bDelete || bInsert;

    DICX* pDicX = DIX(0);

    for( int iItem=0; bRet && iItem < GetNumItems(); iItem++ ) {
        if( ( iSymb = GetItemSymbol( iItem ) ) == 0 )
            break;

        theIndexTarget = theIndex;

        switch( m_pEngineArea->GetTypeGRorVAorBlock( iSymb ) ) {
        case SymbolType::Variable: // ... member is an item or sub-item
            {
                VART*       pVarT=VPT(iSymb);

                if( Issamod == ModuleType::Batch && bSort )   // RHF Sep 29, 2005
                   m_pEngineDriver->prepvar( pVarT, NO_VISUAL_VALUE );  // RHF Sep 29, 2005

                bool        bNumeric= pVarT->IsNumeric();
                VARX*       pVarX=pVarT->GetVarX();
                GROUPT*     pGroupT=pVarT->GetOwnerGPT();
                int         iFlagLen=1;
                int         iAsciiLen=pVarT->GetLength();
                int         iTargetOcc;
                int         iDataOcc=pGroupT->GetDataOccurrences();

                if( pVarT->GetOwnerVarT() != NULL )
                {
                    // We will end up here if this variable is a subitem; if
                    // the parent item is also in this group then we need to
                    // skip out of this because the variable was already moved
                    // as part of the parent item.
                    if (GetItemIndex(pVarT->GetOwnerVarT()->GetSymbolIndex()) != -1)
                        break;
                }

                // iDataOcc 1 based
                int         iNewWaveBorder = bInsert ? iDataOcc   :
                                             bDelete ? iDataOcc-2 : 0;

                // only for no CsDriver!        <begin> // victor Mar 26, 02
                if( !pIntDriver->IsUsing3D_Driver() ) {
                    // RHF INIC Aug 18, 2000
                    //Insert don't allowed if we are in DataOcc or there is no dataocc
                    if( bInsert && ( iDataOcc == 0 || iDataOcc == pLoopControl->GetFromOcc() ) ) {
                        bChangeOcc = false;
                        break;
                    }
                    // RHF END Aug 18, 2000
                }
                // only for no CsDriver!        <end>   // victor Mar 26, 02

                double* pSourceFloats=NULL;
                csprochar*   pSourceFlags;
                csprochar*   pSourceAsciis;
                const   int     iNumValues= (pLoopControl->GetUntilOcc() - pLoopControl->GetFromOcc() + 1);

                ASSERT( iNumValues == (int)pLoopInstruction->m_pTarget->size() );
                ASSERT( iNumValues >= 1 );

                if( bNumeric )
                    pSourceFloats  = (double*) calloc( iNumValues, sizeof(double) );
                pSourceFlags   = (csprochar*)   calloc( iNumValues, iFlagLen * sizeof(csprochar) );
                pSourceAsciis  = (csprochar*)   calloc( iNumValues, iAsciiLen * sizeof(csprochar) );

                if( (bNumeric && pSourceFloats == NULL) || pSourceFlags == NULL || pSourceAsciis == NULL ) {
                    if( pSourceFloats != NULL ){ free( pSourceFloats ); pSourceFloats=NULL; }
                    if( pSourceFlags != NULL ){ free( pSourceFlags ); pSourceFlags=NULL; }
                    if( pSourceAsciis != NULL ){ free( pSourceAsciis ); pSourceAsciis=NULL; }
                    return false;
                }


                // Save Original Values. IndexTarget here is used like IndexSource
                int     iValue=0;
                for( iTargetOcc=pLoopControl->GetFromOcc(); iTargetOcc <= pLoopControl->GetUntilOcc(); iTargetOcc++, iValue++ ) {

                    theIndexTarget.setIndexValue(pLoopControl->GetLoopIndex(),iTargetOcc);

                    pFlag = pIntDriver->GetVarFlagsAddr( pVarX, theIndexTarget );
                    pAscii = pIntDriver->GetVarAsciiAddr( pVarX, theIndexTarget );

                    if( bNumeric ) {
                        dValue = pIntDriver->GetVarFloatValue( pVarX, theIndexTarget );
                        pSourceFloats[iValue] = dValue;
                    }

                    _tmemcpy( pSourceFlags + iValue * iFlagLen, pFlag, iFlagLen );
                    _tmemcpy( pSourceAsciis + iValue * iAsciiLen, pAscii, iAsciiLen );
                }


                // Copy Values
                for( iTargetOcc=pLoopControl->GetFromOcc(); iTargetOcc <= pLoopControl->GetUntilOcc(); iTargetOcc++ ) {
                    int         iSourceIndex;
                    int         iTargetIndex;

                    iTargetIndex = iTargetOcc - pLoopControl->GetFromOcc();
                    iSourceIndex = pLoopInstruction->m_pTarget->at( iTargetIndex ) - pLoopControl->GetFromOcc();

                    ASSERT( iTargetIndex >= 0 );

                    theIndexTarget.setIndexValue(pLoopControl->GetLoopIndex(),iTargetOcc);

                    if( iSourceIndex < 0 )  // Is Produced in Insert when iTarget==0 or in Delete when iTarget==pLoopControl()->GetMaxOcc()
                        ASSERT( bInsert ? iTargetIndex==0 :
                                bDelete ? iTargetIndex==iNumValues-1 : false );

                    // Float
                    if( bNumeric ) {
                        dValue = (iSourceIndex>= 0) ? pSourceFloats[iSourceIndex] : NOTAPPL;
                        pIntDriver->SetVarFloatValue( dValue, pVarX, theIndexTarget ); // Copy, Clean
                    }

                    // Flag
                    if( iSourceIndex >= 0 ) {
                        pFlag = pSourceFlags + iSourceIndex*iFlagLen;
                        ASSERT( pFlag );
                    }

                    pFlagTarget = pIntDriver->GetVarFlagsAddr( pVarX, theIndexTarget );
                    ASSERT( pFlagTarget );

                    // RHF INIC Aug 18, 2000
                    if( bPathOff && (bInsert || bDelete) ) {
                        if( iTargetOcc >= pLoopControl->GetFromOcc() &&
                            iTargetOcc <=  iNewWaveBorder &&
                            pIntDriver->GetFlagColor( pFlagTarget ) == FLAG_NOLIGHT )
                            pIntDriver->SetFlagColor( pFlagTarget, FLAG_MIDLIGHT ); // Yellow

                        if( bDelete && iTargetOcc > iNewWaveBorder )
                              pIntDriver->SetFlagColor( pFlagTarget, FLAG_NOLIGHT ); // Clean

                    }
                    // RHF END Aug 18, 2000

                    // Ascii
                    if( iSourceIndex >= 0 ) {
                        pAscii = pSourceAsciis + iSourceIndex * iAsciiLen;
                        ASSERT( pAscii );
                    }

                    pAsciiTarget = pIntDriver->GetVarAsciiAddr( pVarX, theIndexTarget );
                    ASSERT( pAsciiTarget );

                    if( iSourceIndex >= 0 )
                        _tmemcpy( pAsciiTarget, pAscii, iAsciiLen ); //Copy
                    else
                        _tmemset( pAsciiTarget, _T(' '), iAsciiLen ); // Clean

                    //Related
                    if( pVarX->iRelatedSlot >= 0 )
                        pVarX->VarxRefreshRelatedData( theIndexTarget );
                }

                if( pSourceFloats != NULL ){ free( pSourceFloats ); pSourceFloats=NULL; }
                if( pSourceFlags != NULL ){ free( pSourceFlags ); pSourceFlags=NULL; }
                if( pSourceAsciis != NULL ){ free( pSourceAsciis ); pSourceAsciis=NULL; }


                // process the notes for this variable
                auto& notes = pDicX->GetCase().GetNotes();

                for( size_t i = notes.size() - 1; i < notes.size(); --i )
                {
                    auto& note = notes[i];

                    auto case_item_reference = dynamic_cast<CaseItemReference*>(&note.GetNamedReference());

                    if( case_item_reference == nullptr || case_item_reference->GetCaseItem().GetDictionaryItem().GetSymbol() != pVarT->GetSymbolIndex() )
                        continue;

                    bool delete_note = false;

                    // a non-required non-repeating record
                    if( pLoopControl->GetLoopIndex() < 0 )
                        delete_note = true;

                    else
                    {
                        // see if the note's two non-loop occurrences match or if the loop occurrence is less than the target
                        size_t target_occurrence = pLoopControl->GetFromOcc();
                        size_t comparison_occurrence = case_item_reference->GetOccurrence(pLoopControl->GetLoopIndex());
                        bool occurrences_match = true;

                        for( int iDim = 0; occurrences_match && iDim < DIM_MAXDIM; ++iDim )
                        {
                            if( iDim == pLoopControl->GetLoopIndex() )
                                occurrences_match = ( comparison_occurrence >= target_occurrence );

                            else
                                occurrences_match = ( case_item_reference->GetOccurrence(iDim) == (size_t)theIndexTarget.getIndexValue(iDim) );
                        }

                        if( !occurrences_match )
                            continue;

                        if( bDelete )
                        {
                            if( comparison_occurrence == target_occurrence )
                                delete_note = true;

                            else
                                case_item_reference->SetOccurrence(pLoopControl->GetLoopIndex(), comparison_occurrence - 1);
                        }

                        else if( bInsert )
                        {
                            size_t new_occurrence = comparison_occurrence + 1;

                            // delete the note if the new occurrence isn't valid
                            if( new_occurrence >= (size_t)pGroupT->GetMaxOccs() )
                                delete_note = true;

                            else
                                case_item_reference->SetOccurrence(pLoopControl->GetLoopIndex(), new_occurrence);
                        }

                        // sort
                        else
                        {
                            ASSERT(pLoopControl->GetFromOcc() == 0);

                            for( int j = 0; j < (int)pLoopInstruction->m_pTarget->size(); ++j )
                            {
                                if( comparison_occurrence == (size_t)pLoopInstruction->m_pTarget->at(j) )
                                {
                                    case_item_reference->SetOccurrence(pLoopControl->GetLoopIndex(), j);
                                    break;
                                }

                                ASSERT(j < ( (int)pLoopInstruction->m_pTarget->size() - 1 ));
                            }
                        }
                    }

                    if( delete_note )
                        m_pEngineDriver->DeleteNote_pre80(pDicX, note);

                    m_pEngineDriver->SetNotesModified(pDicX->GetDicT());
                }
            }

            break;

        case SymbolType::Group:
            {
                GROUPT*     pGroupT=GPT(iSymb);
                int         iNewFromOcc = 0; // All ocurrences
                int         iNewUntilOcc=pGroupT->GetMaxOccs()-1;
                int         iNewMaxOcc=pGroupT->GetMaxOccs()-1;
                int         iNewLoopIndex=pGroupT->GetDimType();

                ASSERT( iNewLoopIndex >= 0 );

                bool    bDummyGroup = iNewMaxOcc == 0;
                if( bDummyGroup ) { // Doesn't count occurrences
                    bRet = pGroupT->DoMoveOcc( theIndex, pLoopControl, pLoopInstruction ); // Recursion
                }
                else {
                    ASSERT( iNewLoopIndex >= pLoopControl->GetLoopIndex() ); // Is equal if the parent group is single

                    // If bInsert  descending loop from iNewUntilOcc to iNewFromOcc,
                    // If !bInsert ascending  loop from iNewFromOcc to iNewUntilOcc
                    for( int iOcc=iNewFromOcc; bRet && iOcc <= iNewUntilOcc; iOcc++ ) {
                        ASSERT( pLoopControl->GetLoopIndex() < DIM_MAXDIM - 1 );
                        theIndexTarget.setIndexValue( iNewLoopIndex, iOcc );

                        bRet = pGroupT->DoMoveOcc( theIndexTarget, pLoopControl, pLoopInstruction ); // Recursion
                    }
                }

            }
            break;

        case SymbolType::Block:
            break;

        default:
            ASSERT(0);
            return( false );
            break;
        }
    }

    if( bChangeOcc ) {
        // Recalculate new ocurrences.

        // If Path ON always: CurrentOcc==TotalOcc but when we moved to the first column after this operation
        // Current and Total need to be decremented this is done in the caller.
        if( bPathOff ) {
            int     iTotal = this->GetTotalOccurrences();
            int     iData  = this->GetDataOccurrences();

            if( bInsert ) {
                iTotal++;
                iData++;
                if( iTotal >= 1 && iTotal <= this->GetMaxOccs() ) {
                    ASSERT( iData >= iTotal );
                    this->SetTotalOccurrences( iTotal );
                    this->SetDataOccurrences( iData );
                }
            }
            else if( bDelete ) {
                iTotal--;
                iData--;
                if( iTotal >= 0 ) {
                    ASSERT( iData >= iTotal );
                    this->SetTotalOccurrences( iTotal );
                    this->SetDataOccurrences( iData );
                }
            }
            else if( bSort ) { // Reenter change the ocurrences
                iTotal = 0;
                this->SetTotalOccurrences( iTotal );
//                this->SetDataOccurrences( iTotal );
            }
            else
                ASSERT(0);
        }
        else { // Path On
            //Data Occurrences are not updated on the tree after insert or delete.
            //This code is now  similar to the PathOff Code. Exinsert and exdelete work similarly irrespective
            //of path on or off.
            //Savy - added this to make it work similar to exinsert and exdelete
            int     iTotal = this->GetTotalOccurrences();
            int     iData  = this->GetDataOccurrences();

            if( bInsert ) {
                iTotal++;
                iData++;
                if( iTotal >= 1 && iTotal <= this->GetMaxOccs() ) {
                    ASSERT( iData >= iTotal );
                    this->SetTotalOccurrences( iTotal );
                    iData > this->GetMaxOccs() ? iData = this->GetMaxOccs() : iData = iData;
                    this->SetDataOccurrences( iData );
                }
            }
            else if( bDelete ) {
                iTotal--;
                iData--;
                if( iTotal >= 0 ) {
                    ASSERT( iData >= iTotal );
                    this->SetTotalOccurrences( iTotal );
                    iData < 1 ? iData =1 : iData = iData;
                    this->SetDataOccurrences( iData );
                }
            }
            else if( bSort ) { // Reenter change the ocurrences
                iTotal = 0;
                this->SetTotalOccurrences( iTotal );
//                this->SetDataOccurrences( iTotal );
            }
            else
                ASSERT(0);
        }
    }


    return bRet;
}

void GROUPT::OccTreeFree() {
    TRACE( _T("LEAK: Inside OccTreeFree() deleting m_pocctree for group %s\n"), GetName().c_str() );
    delete m_pOccTree;
    m_pOccTree = 0;
}

// RHF END Aug 16, 2000
