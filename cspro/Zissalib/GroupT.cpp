//------------------------------------------------------------------------
//  File name: GroupT.cpp
//
//  Description:
//          Implementation for engine-GroupT class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   vc      Created
//              16 Dec 99   vc      Adding flow management
//              23 Mar 00   vc      Adding DataOccs
//              10 Jun 00   vc      Adding ptrs (to improve execution speed)
//              09 Aug 00   RHF     Adding insert/delete/sort group ocurrences methods
//              16 Apr 01   vc      Install the engine-symbol into its CDEGroup - visible groups only
//              28 May 01   vc      Move 'AdjustTotalOccs' method from EngineArea to GROUPT class
//              30 Apr 04   RCL     3d Occurrences handling
//
//------------------------------------------------------------------------
#include "StdAfx.h"
#include "GroupVisitor.h"
#include <zFormO/Roster.h>
#include <engine/Tables.h>
#include <engine/Engdrv.h>
#include <engine/IntDrive.h>

#define RTRACE  TRACE
#ifdef WIN_DESKTOP
#define RTRACE2 AfxTrace
#else
#define RTRACE2(...)
#endif

#if defined(_DEBUG) && defined(WIN_DESKTOP)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

//
#undef TRACE
void TRACE( csprochar* pszFormat, ... ) {
    static bool bTrace=false;
    if( !bTrace ) return;
    va_list ap;
    CString csMsg;

    va_start( ap, pszFormat );

    csMsg.FormatV( pszFormat, ap );

    va_end( ap );
}
//

#include <zUtilO/TraceMsg.h>

#include <zUtilO/AppLdr.h>

//////////////////////////////////////////////////////////////////////////
//
// --- construction/destruction/initialization
//
//////////////////////////////////////////////////////////////////////////
CSymbolGroup::CSymbolGroup(std::wstring name, CEngineDriver* pEngineDriver)
    :   ChainedSymbol(std::move(name), SymbolType::Group),
        m_pEngineDriver(pEngineDriver),
        m_pEngineArea(pEngineDriver->m_pEngineArea),
        m_engineData(m_pEngineDriver->m_engineData)
{
    // --- basic data
    m_pFlow          = NULL;
    m_GroupSource    = Source::Uninitialized;
    m_GroupType      = 0;               // 1: Level, 2: Group/Record, 3-Mult.item
    SetLevel(-1);                       // Group Level (0: process, 1-4: visible hierarchy)
    m_iDepthInFlow   = 0;                               // victor Jan 16, 01

    SYMTfirst        = 0;
    SYMTlast         = 0;

    // --- occurrences' tree
    m_pOccTree = 0;

    // --- linked IMSA Group
    SetCDEGroup( NULL );

    // --- owner-and-parent GroupT' accelerators        // victor Jun 10, 00
    m_pOwnerGroupT   = NULL;            // pointer to formal parent
    SetParentGPT( NULL );               // pointer to effective parent

    // --- group dimension
    m_iNumDim        = 0;               // # of dimensions of this GroupT
    SetDimType( CDimension::VoidDim );                  // RHF Aug 10, 2000

    // --- occurrences control
    m_iMinOccs       = 1;               // Physical occurrences: minimum
    m_iMaxOccs       = 1;               //                       maximum
    m_iCurOccurrence = 0;               // Current occurrence of this Group
    m_iTotalOccs     = 0;               // Maximum occurrence reached in this Group
    m_iDataOccs      = 0;               // Maximum occurrence in source data (always >= m_iTotalOccs) // victor Mar 23, 00
    m_iExOccur       = 0;               // Engine-managed occ   // victor Jun 10, 00
    m_iHighOccs      = 0;               // Highest number of occurences of this Group // BMD 14 Aug 2003
    SetPartialOccurrences(0);           // Partial number occurrences of this Group. Use when a case is partially saved // RHF May 07, 2004

    // --- list of items
    m_iNumItems      = 0;
    m_aItemList      = NULL;

    // --- multipurpose flag
    SetFlag( 0 );                                       // RHF Dec 20, 2000

    SetAbsoluteFlowOrder(-1);// RHF Dec 10, 2003 BUCEN_2003 Changes

    SetCanDelete(false);
}


//////////////////////////////////////////////////////////////////////////
//
// --- basic data
//
//////////////////////////////////////////////////////////////////////////

void GROUPT::SetDepthInFlow( void ) {                   // victor Jan 16, 01
    int     iDepth = -1;                // depth-in-flow is -1 for invisible groups
    bool    bIsVisible = ( GetSource() == Source::FrmFile );
    if( bIsVisible ) {                  // only for visible groups:
        GROUPT* pGroupT = this;

        // minimal depth-in-flow is its own level
        iDepth = GetLevel();

        // calculation stops when the level-group is found
        while( pGroupT->GetGroupType() > 1 ) {
            iDepth++;
            pGroupT = pGroupT->GetOwnerGPT();
        }
    }

    m_iDepthInFlow = iDepth;
}


//////////////////////////////////////////////////////////////////////////
//
// --- linked IMSA group
//
//////////////////////////////////////////////////////////////////////////

void GROUPT::DeleteCDEGroup( void ) {                   // RHF 26/10/99
    CDEGroup*   pGroup = GetCDEGroup();
    if( pGroup != NULL && ( GetGroupType() == MultItem || CanDelete() ) ) {
        // group type 3: a "mult-item container" inserted by the engine
        delete pGroup;
        SetCDEGroup( NULL );
    }
}

//////////////////////////////////////////////////////////////////////////
//
// --- owner-and-parent GroupT' accelerators            // victor Jun 10, 00
//
//////////////////////////////////////////////////////////////////////////

void GROUPT::SetOwnerGPT( void ) {
    // SetOwnerGPT: set the GroupT' pointer of the formal owner GroupT
    int     iSymOwner = GetOwnerSymbol();

    m_pOwnerGroupT = ( iSymOwner > 0 ) ? GPT(iSymOwner) : NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// --- group dimension
//
//////////////////////////////////////////////////////////////////////////

void GROUPT::SetDimAndParentGPT( void ) {
    // SetDimAndParentGPT: search and set the GroupT' pointer of the "effective parent"
    //               GroupT for occurrences-control purposes, and the number of
    //               dimensions to get this GroupT fully qualified for compiler
    //               and executor purposes

    SetDepthInFlow();                                   // victor Jan 16, 01

    // --- not for Level-groups
    if( GetGroupType() == Level )
        return;

    //     Remark 1: An "effective parent" for occurrences-control purposes
    //               is the most immediate formal ascendant
    //     Remark 2: Level-groups are effective parents by axiom
    //     Remark 3: MultItem-groups have an empty IMSA group created at Flow
    //               loading time.  However, they are never owners of another
    //               GroupT, so their switch will never be queried
    CDEGroup*   pGroup = GetCDEGroup();
    CDimension::VDimType vType = CDimension::VoidDim;

    m_iNumDim = ( GetMaxOccs() > 1 ); // RHF Sep 10, 2001 TODO Check with more than 1 dimension!!!

    // establish what kind of dimension this group will be.
    // Do *not* set it yet, do it after knowing the
    // exact number of dimensions.
    // rcl, 2004

    ASSERT( pGroup != 0 );

    // The logic below was only applied when the group was visible.
    // Now it is always applied, because we have built everything
    // to make pGroup != 0 and we can get its info here

    if( pGroup != 0 )
    {
        // install the engine-symbol into the CDEGroup  // victor Apr 16, 01
        pGroup->SetSymbol( GetSymbol() );               // victor Apr 16, 01

        if( m_iNumDim ) {                                   // victor Jan 08, 00
            CDEFormBase::eRIType     xType = pGroup->GetRIType();

            switch( xType ) {
                case CDEFormBase::Record:
                    vType = CDimension::Record;
                    break;
                case CDEFormBase::Item:
                    vType = CDimension::Item;
                    break;
                case CDEFormBase::SubItem:
                {
                    vType = CDimension::SubItem;

                    CString csItemName = pGroup->GetTypeName();
                    CString csFullName = csItemName;

                    //TODO pGroup->GetFormFile()->GetDictName();
                    //TODO csFullName.Format( "%s.%s", csDictName, csItemName );

                    int iSymItem = m_pEngineArea->SymbolTableSearch(csFullName, { SymbolType::Variable });

                    if( iSymItem <= 0 ) {
                        ASSERT( 0 );            // can't be: if mult-occs, must refer to an axis
                        vType = CDimension::VoidDim;
                    }
                    else {
                        VART*   pVarT=VPT(iSymItem);
                        bool    bItem = ( !pVarT->GetOwnerSymItem() );

                        if( bItem )
                            vType = CDimension::Item;
                    }

                    break;
                }
                default:
                    ASSERT( 0 );            // can't be: if mult-occs, must refer to an axis
                    vType = CDimension::VoidDim;
                    break;
            }
        } // if( m_iNumDim )
    } // if( pGroup != 0 )

    GROUPT* pOwnerGroupT = GetOwnerGPT();
    bool    bSetParent   = true;

    while( pOwnerGroupT != NULL ) {
        int iOwnerType = pOwnerGroupT->GetGroupType(); // 1: Level, 2: Group/Record, 3: Mult.item
        if( iOwnerType == Level )
            break;

        ASSERT( iOwnerType != GROUPT::MultItem);  // can't be a MultItem group
        CDEGroup* pGroup = pOwnerGroupT->GetCDEGroup();// RHF Aug 04, 2000
        bool      bIsEffectiveParent = ( pGroup != NULL && pGroup->GetMaxLoopOccs() > 1 ); // RHF Aug 04, 2000

        if( bIsEffectiveParent ) {
            // if( pOwnerGroupT->GetSource() == eGroupSource::FrmFile ) // RCL, Dec 2004, blame on RHF
            m_iNumDim += ( pOwnerGroupT->GetMaxOccs() > 1 );

            if( bSetParent )
            {
                bSetParent = false; // do not do this again, please
                SetParentGPT( pOwnerGroupT );
            }
        }

        // must climb-up to ascendant
        pOwnerGroupT = pOwnerGroupT->GetOwnerGPT();
    }
    // non-Level groups coming from FrmFile: check there is an effective parent
    if( GetGroupType() != Level && GetSource() == Source::FrmFile )
        ASSERT( pOwnerGroupT != NULL );

    if( bSetParent )
        SetParentGPT( pOwnerGroupT ); // pointer to effective parent // RHF Jul 31, 2000
    // RHF COM Jul 31, 2000 m_pParentGroupT = pOwnerGroupT;

    // after knowing exactly the number of dimensions
    // we can set the dimension type
    SetDimType( vType );

}


//////////////////////////////////////////////////////////////////////////
//
// --- occurrences control
//
//////////////////////////////////////////////////////////////////////////

#include "OccurrenceVisitor.h"

void GROUPT::SetCurrentOccurrences( int iOccur ) {
    ASSERT( iOccur >= 0 );

#ifdef _DEBUG
    CString csMsg;
    csMsg.Format(_T("%s.setCurrentOcc(%d)"), this->GetName().c_str(), iOccur );
    DebugMsg( 0, (csprochar*)(const csprochar*) csMsg );
#endif

    // setting new value of current occurrence
    m_iCurOccurrence = iOccur;

    if( m_pEngineArea->Issamod == ModuleType::Entry ) {
        // visible groups: pass info to IMSA    <begin> // RHF Mar 17, 2000
        CDEFormBase* pBase = GetCDEGroup();

        if( pBase ) {
            int     iThisType = GetGroupType(); // 1: Level, 2: Group/Record, 3: Mult.item
            if( iThisType == Level ) {
                CDELevel*   pLevel = (CDELevel*) pBase;

                pLevel->SetCurOccurrence( iOccur );
            }
            else {
                CDEGroup*   pGroup = (CDEGroup*) pBase;

                C3DObject the3dObject = GetCurrent3DObject();

                // RTODO: Al CDEGroup le serviria tener toda la informacion
                // y no solo la ocurrencia de la dimension actual
                // algo asi como:
                // pGroup->SetCurOccurrence( the3dObject );

                pGroup->SetCurOccurrence( iOccur );
            }
        }
        // visible groups: pass info to IMSA    <end>   // RHF Mar 17, 2000
    }
}

void GROUPT::SetTotalOccurrences( int iOccur ) {
    ASSERT( iOccur >= 0 );

    // setting new value of total occurrences
    m_iTotalOccs = iOccur;

    // adjusting data occurrences if needed     <begin> // victor Mar 23, 00
    if( GetDataOccurrences() < iOccur )
        SetDataOccurrences( iOccur );
    // adjusting data occurrences if needed     <end>   // victor Mar 23, 00

    if( m_pEngineArea->Issamod == ModuleType::Entry ) {
        // visible groups: pass info to IMSA    <begin> // RHF Mar 17, 2000
        CDEFormBase* pBase = GetCDEGroup();

        if( pBase ) {
            int     iThisType = GetGroupType(); // 1: Level, 2: Group/Record, 3: Mult.item
            if( iThisType == Level ) {
                CDELevel*   pLevel = (CDELevel*) pBase;

                pLevel->SetTotalOccs( iOccur );
            }
            else {
                CDEGroup*   pGroup = (CDEGroup*) pBase;

                pGroup->SetTotalOccs( iOccur );
            }
        }
        // visible groups: pass info to IMSA    <end>   // RHF Mar 17, 2000
    }
}

void GROUPT::SetDataOccurrences( int iOccur ) { // new  // victor Mar 23, 00
    ASSERT( iOccur >= 0 && iOccur >= m_iTotalOccs );

    // setting new value of data occurrences
    m_iDataOccs = iOccur;

    if( m_pEngineArea->Issamod == ModuleType::Entry ) {
        // visible groups: pass info to IMSA    <begin> // RHF Mar 17, 2000
        CDEFormBase* pBase = GetCDEGroup();

        if( pBase ) {
            int     iThisType = GetGroupType(); // 1: Level, 2: Group/Record, 3: Mult.item
            if( iThisType == Level ) {
                    CDELevel*   pLevel = (CDELevel*) pBase;

                pLevel->SetDataOccs( m_iDataOccs );
            }
            else {
                CDEGroup*   pGroup = (CDEGroup*) pBase;

                pGroup->SetDataOccs( m_iDataOccs );
            }
        }
        // visible groups: pass info to IMSA    <end>   // RHF Mar 17, 2000
    }
}

void GROUPT::AdjustTotalOccs( void ) {                  // victor May 28, 01
    bool    bPathOn = m_pEngineArea->m_pEngineSettings->IsPathOn();

    if( !bPathOn )
    {
        // GHM 20130711 if a roster (group) was saved with 0 occurrences, this code, in modify mode, would give the roster a default
        // of 1 occurrence; this was problematic in interactive edit mode because CSEntry then stopped on this (not real) first occurrence,
        // complaining about the missing value; this change prevents, while in interactive edit mode, the group from being assigned the fake occurrence
        if( m_pEngineArea->m_pEngineSettings->GetInteractiveEdit() )
            return;
    }

    int iCurrentOccurrences = GetCurrentOccurrences();

    if( bPathOn )
        SetTotalOccurrences( iCurrentOccurrences );
    else if( GetTotalOccurrences() < iCurrentOccurrences )
        SetTotalOccurrences( iCurrentOccurrences );
}

void GROUPT::AdjustTotalOccs( CNDIndexes& theIndex ) {                  // victor May 28, 01
    bool    bPathOn = m_pEngineArea->m_pEngineSettings->IsPathOn();

    int iCurrentOccurrences = GetCurrentOccurrences( theIndex );

    if( bPathOn )
        SetTotalOccurrences( theIndex, iCurrentOccurrences );
    else if( GetTotalOccurrences( theIndex ) < iCurrentOccurrences )
        SetTotalOccurrences( theIndex, iCurrentOccurrences );
}


// GetMaxExOcc:
// calcs MaxExOcc considering IssaMod and using 3Ds
int GROUPT::GetMaxExOcc( void )
{
    int iMaxOccur = 0;
    try
    {
        CalcCurrent3DObjectEx();
        CNDIndexes theIndex = GetCurrent3DObject().getIndexes();

        if( m_pEngineArea->Issamod == ModuleType::Entry )
        {
            iMaxOccur = GetCurrentOccurrences();
            // should be
            // iMaxOccur = GetCurrentOccurrences( theIndex );
        }
        else
        {
            iMaxOccur = GetTotalOccurrences( theIndex );
        }
    }
    catch( const C3DException& )
    {
        // do it the old way, when 3d does not apply
        iMaxOccur = ( m_pEngineArea->Issamod == ModuleType::Entry ) ?
            GetCurrentOccurrences() :
            GetTotalOccurrences();
    }

    return iMaxOccur;
}

// new 'GetxxxxExOccurrence' methods            <begin> // victor Jun 10, 00
int GROUPT::GetFirstExOccurrence( void ) {
    int iMaxOccur = GetMaxExOcc();

    m_iExOccur = ( iMaxOccur > 0 );     // producing 1 or 0

    return m_iExOccur;                  // 0 means "requested occurrence not found"
}

int GROUPT::GetNextExOccurrence( void ) {
    int iMaxOccur = GetMaxExOcc();

    m_iExOccur = ( m_iExOccur < iMaxOccur ) ? m_iExOccur + 1 : 0;

#ifdef _DEBUG
    CString csMsg;
    csMsg.Format( _T("  %s::GetNextExOccurrence = %d"), this->GetName().c_str(), m_iExOccur );
    DebugMsg( 0, csMsg.GetString() );
#endif

    return m_iExOccur;                  // 0 means "requested occurrence not found"
}

int GROUPT::GetLastExOccurrence( void ) {
    int iMaxOccur = GetMaxExOcc();

    m_iExOccur = ( iMaxOccur > 0 ) ? iMaxOccur : 0;

    return m_iExOccur;                  // 0 means "requested occurrence not found"
}

int GROUPT::GetPrevExOccurrence( void ) {
    m_iExOccur = ( m_iExOccur > 0 ) ? m_iExOccur - 1 : 0;

    return m_iExOccur;                  // 0 means "requested occurrence not found"
}

// RHF INIC Aug 07, 2000
int GROUPT::GetCurrentExOccurrence( void ) const {
    int     iCurOcc = ( m_pEngineArea->Issamod == ModuleType::Entry ) ?
        ((GROUPT*)this)->GetCurrentOccurrences() : m_iExOccur;

    return iCurOcc;                  // 0 means "requested occurrence not found"
}

// RHF INIC Jul 09, 2001
void GROUPT::SetCurrentExOccurrence( int iExOccur ) {
    ASSERT( m_pEngineArea->Issamod == ModuleType::Batch );

    m_iExOccur = iExOccur;
}
// RHF END Jul 09, 2001

// RHF END Aug 07, 2000
// new 'GetxxxxExOccurrence' methods            <end>   // victor Jun 10, 00



void GROUPT::SetOccVisibility(int iOcc, bool visible)
{
    if( iOcc >= 0 && iOcc < m_iMaxOccs )
    {
        // set the visibility of an already existing occurrence
        if( iOcc < (int)m_occVisibility.size() )
        {
            m_occVisibility[iOcc] = visible;
        }

        // otherwise, if not visible, fill the occurrences up to that point, and then set as not visible
        // (there is no reason to set the visibility when true because that is the default setting)
        else if( !visible )
        {
            m_occVisibility.resize(iOcc, true);
            m_occVisibility.emplace_back(false);
        }
    }
}


void GROUPT::SaveOccLabel(int iOcc)
{
    // if already in the list, the label has already been changed and doesn't need to be saved again
    if( m_mapSavedOccLabels.find(iOcc) != m_mapSavedOccLabels.end())
        return;

    LabelSet& occurrence_label_set = m_mapSavedOccLabels[iOcc];

    CDEGroup* pGroup = GetCDEGroup();

    if( pGroup->GetItemType() == CDEFormBase::Roster )
        occurrence_label_set = ((CDERoster*)pGroup)->GetStubTextSet().GetText(iOcc).GetLabel();

    if( pGroup->GetRIType() == CDEFormBase::Record )
    {
        const CDictItem* pItem = GetFirstDictItem();
        occurrence_label_set = pItem->GetRecord()->GetOccurrenceLabels().GetLabelSet(iOcc);
    }

    else if( pGroup->GetRIType() == CDEFormBase::Item || pGroup->GetRIType() == CDEFormBase::SubItem )
    {
        int iSymItem = m_pEngineArea->SymbolTableSearch(pGroup->GetRepeatName(), { SymbolType::Variable });

        if( iSymItem > 0 )
        {
            const CDictItem* pItem = VPT(iSymItem)->GetDictItem();
            occurrence_label_set = pItem->GetOccurrenceLabels().GetLabelSet(iOcc);
        }
    }
}

void GROUPT::ResetOccLabels()
{
    for( const auto& [iOcc, occurrence_label_set] : m_mapSavedOccLabels )
    {
        CDEGroup* pGroup = GetCDEGroup();

        if( pGroup->GetItemType() == CDEFormBase::Roster )
            ((CDERoster*)pGroup)->GetStubTextSet().GetText(iOcc).SetLabel(occurrence_label_set.GetLabel());

        if( pGroup->GetRIType() == CDEFormBase::Record )
        {
            const CDictItem* pItem = GetFirstDictItem();
            // DD_STD_REFACTOR_TODO store dictionary occurrence labels somewhere else
            const_cast<CDictItem*>(pItem)->GetRecord()->GetOccurrenceLabels().SetLabels(iOcc, occurrence_label_set);
        }

        else if( pGroup->GetRIType() == CDEFormBase::Item || pGroup->GetRIType() == CDEFormBase::SubItem )
        {
            //Item Or SubItem with occurrences
            int iSymItem = m_pEngineArea->SymbolTableSearch(pGroup->GetRepeatName(), { SymbolType::Variable });

            if( iSymItem > 0 )
            {
                const CDictItem* pItem = VPT(iSymItem)->GetDictItem();
                // DD_STD_REFACTOR_TODO store dictionary occurrence labels somewhere else
                const_cast<CDictItem*>(pItem)->GetRecord()->GetOccurrenceLabels().SetLabels(iOcc, occurrence_label_set);
            }
        }
    }

    m_mapSavedOccLabels.clear();
}



//////////////////////////////////////////////////////////////////////////
//
// --- records management
//
//////////////////////////////////////////////////////////////////////////
// RHF INIC Mar 01, 2001
int GROUPT::CalculateOccurrence( void ) {

    SECT*   pSecT;
    int     iNumOcc = 0;
    int     iSymOwner = GetOwnerSymbol();
    ASSERT( m_GroupType != GROUPT::Level );
    ASSERT( m_iMaxOccs >= 1 );
    int iSecNumOccs = 0; // rcl, Jun 12, 2004

    if( GetDimType() == CDimension::Record || GetDimType() == CDimension::VoidDim ) {
    //if( m_GroupType == eGroupType::GroupOrRecord ) {
        for( int i = 0; ( pSecT = GetRecord( i ) ) != NULL; i++ ) {
            ASSERT( pSecT->GetSecX() );
            iNumOcc = std::max( iNumOcc, pSecT->GetSecX()->GetOccurrences() );
        }
    }
    else if( GetDimType() == CDimension::Item || GetDimType() == CDimension::SubItem ) {
    //else if( m_GroupType == eGroupType::MultItem ) {}
       // mult-vars: get the highest occurrence, scanning up to the maximum Group occs:
        pSecT = GetRecord( 0 ); // Use the first record
        ASSERT( pSecT );
        ASSERT( GetRecord( 1 ) == NULL ); // There is only 1 record
        iSecNumOccs = pSecT->GetSecX()->GetOccurrences();
        CNDIndexes theIndex( ZERO_BASED, DEFAULT_INDEXES );

        // sing-vars: increase the Group' number of occs up to the occurrences of the Sect
        if( m_iMaxOccs <= 1 )
            iNumOcc = iSecNumOccs;
        else {

            // iterate for every record [sect]
            if( this->GetNumDim() == 2 )
            {
                theIndex.setHomePosition();

                int iDimLimit = iSecNumOccs;

                // when the 2nd dimension is item, the first got to be a record
                if( GetDimType() == CDimension::Item )
                {
                    theIndex.specifyIndexesUsed( USE_DIM_1_2 );
                }
                else
                {
                    theIndex.specifyIndexesUsed( USE_DIM_1_2 );
                }

                // iterate over 1st and 2nd dimension
                for( int iDimValue = 0; iDimValue < iDimLimit; iDimValue++ )
                {

                    theIndex.setIndexValue( GetOwnerGPT()->GetDimType(), iDimValue );

                    iNumOcc = 0;
                    //SAVY to fix the problem of TotOcc in 2 dimension record repeats && item / subitem repeats
                    SetCurrentOccurrences( theIndex, iNumOcc ); //for each occurrence of the parent dim . set the num occ of current  dim as zero
                    SetTotalOccurrences( theIndex, iNumOcc );
                    SetDataOccurrences( theIndex, iNumOcc );
                    VART*   pVarT = NULL;
                    int     iNumGroupItems = GetNumItems();

                    // foreach VART in this group
                    for( int iItem = 0; iItem < iNumGroupItems; iItem++ )
                    {
                        int     iSymItem = GetItemSymbol( iItem );

                        if( m_pEngineArea->GetTypeGRorVA( iSymItem ) != SymbolType::Variable )
                            continue;

                        pVarT = VPT(iSymItem);
                        for( int iOccur = m_iMaxOccs; iOccur >= 1; iOccur-- )
                        {
                            ///Savy 05/18/2012
                            //theIndex here refers to the index of the item in the item group item000
                            //this is being stepped on by the groups's index. hence make a copy
                            CNDIndexes itemIndex = theIndex;
                            itemIndex.setIndexValue( GetDimType(), iOccur - 1 );
                            if( !m_pEngineDriver->IsBlankField( pVarT, itemIndex ) ) {
                                if( iOccur > iNumOcc )
                                    iNumOcc = iOccur;
                                break;
                            }
                        }

                        if( iNumOcc == m_iMaxOccs ) break; // RHF Mar 20, 2001
                    }

                    if( iNumOcc > GetCurrentOccurrences( theIndex ) )
                    {
                        SetCurrentOccurrences( theIndex, iNumOcc );
                        SetTotalOccurrences( theIndex, iNumOcc );
                        ////SAVY to fix the problem of TotOcc in 2 dimension record repeats && item / subitem repeats
                        //SAVY &&& inside SetTotalOcc .. above DataOccurrences is not getting set all the time
                        SetDataOccurrences( theIndex, iNumOcc );

                        //SAVY&&& Install the occs to fix the occurrences when record occurs and item occurs.
                        if( GetCurrentOccurrences() < iNumOcc ) {
                            SetCurrentOccurrences( iNumOcc );
                            SetTotalOccurrences( iNumOcc );
                        }

                    }
                }

                // iNumOcc will keep last number of elements of this group
            }
            else // no 3d? do as before
            {
                VART* pVarT = NULL;
                int   iNumGroupItems = GetNumItems();

                // foreach VART in this group
                for( int iItem = 0; iItem < iNumGroupItems; iItem++ )
                {
                    int     iSymItem = GetItemSymbol( iItem );

                    if( m_pEngineArea->GetTypeGRorVA( iSymItem ) != SymbolType::Variable )
                        continue;

                    pVarT = VPT(iSymItem);
                    for( int iOccur = m_iMaxOccs; iOccur >= 1; iOccur-- )
                    {
                        if( !m_pEngineDriver->IsBlankField( pVarT, iOccur ) ) {
                            if( iOccur > iNumOcc )
                                iNumOcc = iOccur;
                            break;
                        }
                    }

                    if( iNumOcc == m_iMaxOccs ) break; // RHF Mar 20, 2001
                }
            }
        }
    }
    else { // ( GetDimType() == CDimension::VoidDim ) // Dummy Group
        ASSERT(0); // TODO
       //Heredar occurencias del grupo padre
    }

    if( iNumOcc == 0 ) {
        // no occurrences detected: don't touch owner groups
        iSymOwner = 0;
    }
    else {
        // install the number of occurrences...

        if( GetCurrentOccurrences() < iNumOcc ) {
            SetCurrentOccurrences( iNumOcc );
            SetTotalOccurrences( iNumOcc );
        }
    }

    // climb up to owner groups up to the level...
    while( iSymOwner ) {
        if( !m_pEngineArea->IsSymbolTypeGR( iSymOwner ) )
            break;

        // get the owner group and...
        GROUPT* pGroupTOwner  = GPT(iSymOwner);
        bool    bIsLevelGroup = ( pGroupTOwner->GetGroupType() == Level );

        // ... force its existence
        if( pGroupTOwner->GetDataOccurrences() < 1 ) {
            pGroupTOwner->SetCurrentOccurrences( 1 );
            pGroupTOwner->SetTotalOccurrences( 1 );
        }
        if( !bIsLevelGroup &&
           pGroupTOwner->GetDimType() == CDimension::Record )
        {
            if( iSecNumOccs <= 0 )
                iSecNumOccs = 1;
            pGroupTOwner->SetTotalOccurrences( iSecNumOccs );
        }

        // don't climb up beyond a Level-group
        iSymOwner = ( bIsLevelGroup ) ? 0 : pGroupTOwner->GetOwnerSymbol();
    }

    return iNumOcc;
}

void GROUPT::CreateRecordArray( void ) {
    ASSERT( m_aRecords.empty() );
    // For each variable
    for( int iItem = 0; iItem < GetNumItems(); iItem++ ) {
        int iSymItem = GetItemSymbol( iItem );

        if( m_pEngineArea->IsSymbolTypeVA( iSymItem ) ) {
            VART* pVarT = VPT(iSymItem);

            if( FindRecord( pVarT->GetSPT() ) < 0 ) // Not found
                AddRecord( pVarT->GetSPT() );
        }
    }
}
// RHF END Mar 01, 2001

bool GROUPT::RecordsAreIdentical(const GROUPT& rhs_group) const
{
    bool records_are_identical = ( m_aRecords.size() == rhs_group.m_aRecords.size() );

    for( size_t i = 0; records_are_identical && i < m_aRecords.size(); i++ )
        records_are_identical = ( m_aRecords[i] == rhs_group.m_aRecords[i] );

    return records_are_identical;
}


//////////////////////////////////////////////////////////////////////////
//
// --- bound conditions and relationships
//
//////////////////////////////////////////////////////////////////////////

bool GROUPT::IsAncestor( int iSymGroup, bool bCountingOcurrences, bool bStartFromThis ) { // RHF Oct 31, 2000
    GROUPT* pGroupT;

    if( bStartFromThis )
        pGroupT = this;
    else
        pGroupT = ( bCountingOcurrences ) ? GetParentGPT() : GetOwnerGPT();

    bool    bFound = false;

    while( pGroupT != NULL && !bFound ) {
        if( pGroupT->GetSymbolIndex() == iSymGroup ) {
            bFound = true;
            continue;
        }
        pGroupT = bCountingOcurrences ? pGroupT->GetParentGPT() : pGroupT->GetOwnerGPT();
    }

    return bFound;
}


//////////////////////////////////////////////////////////////////////////////
//
// --- list of items
//
//////////////////////////////////////////////////////////////////////////////

int GROUPT::GetItemIndex(  int iSymItem, int* pFlowOrder ) const {
    int     iItemFound = -1;

    if( !iSymItem ) {                   // no item - assume first item
        iItemFound = 0;
    }
    else {
        for( int iItem = 0; iItemFound < 0 && iItem < GetNumItems(); iItem++ ) { // RHF Feb 23, 2000
            if( iSymItem == GetItemSymbol( iItem ) )
                iItemFound = iItem;
        }
    }

    if( pFlowOrder != NULL )
        *pFlowOrder = GetFlowOrder( iItemFound );

    return iItemFound;
}

int GROUPT::GetItemLevel( int iItem ) {
    int     iSymGroupT = 0;

    if( IsAnItem( iItem ) ) {
        int     iSymbol = GetItemSymbol( iItem );

        if( iSymbol ) {
            if( m_pEngineArea->IsSymbolTypeGR( iSymbol ) )
                iSymGroupT = iSymbol;
            else if( m_pEngineArea->IsSymbolTypeVA( iSymbol ) )
                iSymGroupT = VPT(iSymbol)->GetOwnerGroup();
        }
    }

    return( iSymGroupT ? GPT(iSymGroupT)->GetLevel() : 0 );
}

bool GROUPT::SetItemDescription( int iItem, int iSymItem, int iFlowOrder ) {
    bool    bDone = false;

    if( iItem >= 0 && iItem < GetNumItems() ) {
        m_aItemList[iItem].SYMTitem   = iSymItem;
        m_aItemList[iItem].iFlowOrder = iFlowOrder;
        m_aItemList[iItem].index      = iItem;
        bDone = true;
    }

    return bDone;
}

bool GROUPT::CreateItemList( int iNumItems ) {
    GRITEM* aNewList = (GRITEM*) calloc( iNumItems, sizeof(GRITEM) );

    if( aNewList != NULL ) {
        int     iItem;

        // blanks new list
        for( iItem = 0; iItem < iNumItems; iItem++ ) {
            aNewList[iItem].SYMTitem   = -1;
            aNewList[iItem].iFlowOrder = -1;
            aNewList[iItem].index      = -1;
        }

        // take care of old list
        GRITEM* aOldList = m_aItemList;

        if( aOldList != NULL ) {
            // old list had any members?
            if( GetNumItems() ) {
                // copy contents of old list into new list (maybe truncating)
                int     iNumToCopy = ( iNumItems < GetNumItems() ) ?
                                       iNumItems : GetNumItems();
                for( iItem = 0; iItem < iNumToCopy; iItem++ ) {
                    aNewList[iItem].SYMTitem   = aOldList[iItem].SYMTitem;
                    aNewList[iItem].iFlowOrder = aOldList[iItem].iFlowOrder;
                    aNewList[iItem].index      = aOldList[iItem].index;
                }
            }

            // release old list
            free( aOldList );
        }

        // installs new list
        m_iNumItems = iNumItems;
        m_aItemList = aNewList;
    }

    return( aNewList != NULL );
}

void GROUPT::DestroyItemList( void ) {

    if( m_aItemList != NULL )
        free( m_aItemList );
    m_iNumItems = 0;
    m_aItemList = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// --- occurrences' tree management
//
//////////////////////////////////////////////////////////////////////////

  // --> see Groupt2.cpp

//////////////////////////////////////////////////////////////////////////////
//
// --- general support
//
//////////////////////////////////////////////////////////////////////////////

GROUPT* GROUPT::GetRootGPT( void ) {
    return GetFlow()->GetGroupTRoot();
}

GROUPT* GROUPT::GetLevelGPT( int iLevel ) {
    int     iSymLevel = GetLevelSymbol( iLevel );
    ASSERT( iSymLevel );

    return( iSymLevel ? GPT(iSymLevel) : NULL );
}

int GROUPT::GetLevelSymbol( int iLevel ) {
    GROUPT* pGroupTRoot = GetRootGPT();

    return( iLevel <= 0                          ? pGroupTRoot->GetSymbol() :
            iLevel <= pGroupTRoot->GetNumItems() ? pGroupTRoot->GetItemSymbol( iLevel - 1 ) :
                                                   0 );
}


CString GROUPT::GetLabel() const
{
    const CDEFormBase* pGroupOrLevel = GetCDEGroup();
    return ( pGroupOrLevel != NULL ) ? pGroupOrLevel->GetLabel() : CString();
}


//////////////////////////////////////////////////////////////////////////
//
// --- flow support
//
//////////////////////////////////////////////////////////////////////////
int GROUPT::SearchNextItemInGroup( int iOccur, int iItem, int* pOccur, int* pItem ) const {
    int     iSymNewItem = 0;
    int     iNewOccur   = 0;
    int     iNewItem    = -1;
    int     iTopOccur   = GetMaxOccs();

    if( !IsLastItem( iItem ) ) {

        iNewOccur = iOccur;             // ... same occurrence,
        iNewItem  = iItem + 1;          // ... next item
    }
    else if( iOccur < iTopOccur ) {

        iNewOccur = iOccur + 1;         // ... next occurrence,
        iNewItem  = 0;                  // ... first item
    }

    // returns item found
    if( IsAnItem( iNewItem ) )
        iSymNewItem = GetItemSymbol( iNewItem );
    *pOccur    = iNewOccur;
    *pItem     = iNewItem;

    return iSymNewItem;
}


int g_iIndent = -1;

static CString createBlanks(int iNum)
{
    CString csBlank;

    for( int i = 0; i < g_iIndent; i++ )
        csBlank += _T(" ");

    return csBlank;
}

void GROUPT::InitOneGroup( void ) {                     // victor Jul 27, 99

    g_iIndent++;

#ifdef _DEBUG
    CString csMsg;
    csMsg.Format( _T("%s%s::Init()"), createBlanks(g_iIndent).GetString(), this->GetName().c_str() );
    DebugMsg( 0, csMsg.GetString() );
    csMsg.Format( _T("%s{"), createBlanks(g_iIndent).GetString() );
    DebugMsg( 0, csMsg.GetString() );
#endif

    // initializes this group
    SetCurrentOccurrences( 0 );
    SetTotalOccurrences(  0 );

    // now intializing data occurrences also            // victor Mar 23, 00
    SetDataOccurrences( 0 );                            // victor Mar 23, 00
    SetPartialOccurrences( 0 ); // RHF Jul 07, 2004

    // for each group in this group
    for( int iItem = 0; iItem < GetNumItems(); iItem++ ) {
        int     iSymItem = GetItemSymbol( iItem );

        if( m_pEngineArea->IsSymbolTypeGR( iSymItem ) ) {
            GPT(iSymItem)->InitOneGroup();
        }
    }

#ifdef _DEBUG
    csMsg.Format( _T("%s} // %s"), createBlanks(g_iIndent).GetString(), this->GetName().c_str());
    DebugMsg( 0, csMsg.GetString() );
#endif

    g_iIndent--;
}


//////////////////////////////////////////////////////////////////////////
//
// --- deleting and inserting occurrences
//
//////////////////////////////////////////////////////////////////////////

  // --> see Groupt2.cpp
  // --> see also Int_Set.cpp for 'GROUPT::Trip'

//////////////////////////////////////////////////////////////////////////
//
//
// CEngineArea::methods related to GROUPT management
//
//
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//
// --- initialization for 'DeFuncs'
//
//////////////////////////////////////////////////////////////////////////

void CEngineArea::InitLevelGroups( int iLevel ) {
    GROUPT* pGroupTRoot = m_pEngineDriver->GetGroupTRootInProcess();

    if( iLevel > pGroupTRoot->GetNumItems() )
        return;

    GROUPT*     pGroupT = pGroupTRoot->GetLevelGPT( iLevel );

    // for each group in this level-group
    for( int iItem = 0; iItem < pGroupT->GetNumItems(); iItem++ ) {
        int     iSymItem = pGroupT->GetItemSymbol( iItem );

        if( IsSymbolTypeGR( iSymItem ) ) {
            GPT(iSymItem)->InitOneGroup();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
// --- occurrences management
//
//////////////////////////////////////////////////////////////////////////

int CEngineArea::GroupMaxNumOccs( int iSymObj ) {       // victor Jul 28, 99
    int     iMaxNumOccs = 0;
    int     iSymGroup = GetGroupOfSymbol( iSymObj );
    ASSERT( iSymGroup );

    if( iSymGroup ) {
        iMaxNumOccs = GPT(iSymGroup)->GetMaxOccs();

        //TODO add evaluation of "the field that determines max # of repeats" value
        //     (name given by GPT(iSymGroup)->pImsaGroup->GetMaxField()??? to be
        //     added to GROUPT info, zero for none, etc).  When evaluating current
        //     value, use 'varvalue'
    }

    return iMaxNumOccs;
}


bool CEngineArea::GroupSetCurOccurrenceUsingVar( C3DObject& theObject )
{
    // setup the target group
    int      iSymVar   = theObject.GetSymbol();
    int      iSymGroup = GetGroupOfSymbol( iSymVar );
    GROUPT*  pGroupT = GPT(iSymGroup);
    ASSERT( pGroupT );

    bool bCanSet = GroupSetCurOccurrence( pGroupT, theObject.getIndexes(), iSymGroup, iSymVar );

    return bCanSet;
}

bool CEngineArea::GroupSetCurOccurrence( int iSymObj, int iNewOccurrence )
{
    // setup the target group
    int      iSymGroup = GetGroupOfSymbol( iSymObj );
    GROUPT*  pGroupT   = GPT(iSymGroup);
    ASSERT( pGroupT );
    bool     bCanSet;

//    Superfluo hacer este chequeo aqui, pues se hace nuevamente al
//    interior del metodo que se invocara, haciendo las mismas
//    operaciones para los 2 casos (cuando el if resulta true o cuando
//    resulta falso, i.e. cuando iSymObj es 0 - el default que se intenta
//    ocupar aqui - o cuando iSymGroup == iSymObj, condicion que es cierta
//    para usar el default )
//
//    RCL, April 2004

//    if( iSymGroup == iSymObj )
//        bCanSet = GroupSetCurOccurrence( pGroupT, iNewOccurrence, iSymGroup );
//    else
    bCanSet = GroupSetCurOccurrence( pGroupT, iNewOccurrence, iSymGroup, iSymObj );

    return bCanSet;
}

bool CEngineArea::GroupSetCurOccurrence( GROUPT* pGroupT, CNDIndexes& theIndex, int iSymGroup, int iSymObj ) // rcl, Nov 02, 04
{
    ASSERT( pGroupT );

    // setup the target group
    if( iSymGroup <= 0 ) {
        iSymGroup = pGroupT->GetSymbolIndex();
        ASSERT( iSymGroup > 0 );
    }

    CDimension::VDimType gType = pGroupT->GetDimType();
    if( gType == CDimension::VoidDim )
        gType = CDimension::Record;

    ASSERT( gType == CDimension::Record || gType == CDimension::Item || gType == CDimension::SubItem );

    int iNewOccurrence = theIndex.getIndexValue( gType );

    // verify if passed occurrences are valid
    int   iMaxOccs = pGroupT->GetMaxOccs();
    bool  bCanSet  = ( iNewOccurrence >= 0 && iNewOccurrence <= iMaxOccs );


    if( !bCanSet )
    {
        DebugMsg( 0, _T(" GrouptSetCurOccurrence: iNewOcc < 0 or iNewOcc is > MaxOccs") );
        return bCanSet;
    }

    // update target-group whenever the occurrences are to be changed:
    int iOldOccurrence = pGroupT->GetCurrentOccurrences( theIndex );

    if( iOldOccurrence == pGroupT->GetMaxOccs() )
    {
        iOldOccurrence = 0;
    }

    bool bTryToUpdateOwner = false;

    if( iNewOccurrence != iOldOccurrence ) {

        // set occurrences of target-group to the new amount
        pGroupT->SetCurrentOccurrences( theIndex, iNewOccurrence );

        pGroupT->SetDataOccurrences( theIndex, iNewOccurrence );
        pGroupT->SetCurrentOccurrences( theIndex, iNewOccurrence );
        pGroupT->AdjustTotalOccs( theIndex );     // new          // victor May 28, 01

        bTryToUpdateOwner = true;
    }
    else
    {
    }

    // decide on changing the owner-group if any
    int iSymOwner = pGroupT->GetOwnerSymbol();

    if( bTryToUpdateOwner && iSymOwner > 0 ) {               // only when has an owner-group
        GROUPT*     pOwnerGroupT  = GPT(iSymOwner);
        bool        bOwnerIsLevel = ( pOwnerGroupT->GetGroupType() == GROUPT::Level );
        int         iOwnerLevel   = pOwnerGroupT->GetLevel();

        // setup the associated OccTree

        // checks if the given item or group is a potential trigger for
        // updating the owner-group occurrences: it must be the first
        // member of its group.  Pay attention to the following: this
        // function is started with a true item-field arriving, then
        // it is recursively called for each owner-group.  Owner-groups
        // changes are solved in sequence of ancestor-to-child, where
        // the top ancestor is the Level-1 group (the owner-group for
        // Level-0 is excluded, its occurrences are never changed).  The
        // owner-group Level-1 is in practice modified once and only once
        // in order to acquire a first/unique occurrence when a Level-2 node
        // arises for the first time.
        int  iItem = 1;

        if( iSymObj <= 0 || iSymObj == iSymGroup ) {
            // the group -not an item- was passed
            iItem = pOwnerGroupT->GetItemIndex( iSymGroup );
        }
        else {
            // an item was passed
            iItem = pGroupT->GetItemIndex( iSymObj );
            if( iItem == 0 )
                iItem = pOwnerGroupT->GetItemIndex( iSymGroup );
        }

        if( iItem == 0 || pOwnerGroupT->GetCurrentOccurrences() < 1 ) { // it is a potential trigger // RHF Aug 07, 2002
            // check if owner-group occurrences are to be updated: each
            // time the trigger-group is created (occurrences set to 1)
            // or deleted (occurrences set to zero), the occurrences
            // of the owner-group must be correspondingly increased or
            // reduced.

            // the decision on updating owner-group is based on:
            // - the owner group can be touched (is not the Level-0)
            // - the new occurrence for the trigger-group is increased
            //   to 1 (creation) or decreased to 0 (deletion)
            bTryToUpdateOwner = ( !bOwnerIsLevel || iOwnerLevel > 0 ) &&
                ( ( iNewOccurrence == 1 && iOldOccurrence <  1 ) ||
                ( iNewOccurrence <  1 && iOldOccurrence >= 1 ) );
        }
        else
            bTryToUpdateOwner = false;

        if( bTryToUpdateOwner ) {            // it is a true trigger
            int  iOwnerOldOccurrence = pOwnerGroupT->GetCurrentOccurrences();
            int  iOwnerNewOccurrence = -1;

            if( iNewOccurrence < 1 ) {
                // new occurrence is zero: reducing occurrences
                if( iOwnerOldOccurrence < 1 ) {
                    // ... owner did not exist: set its occs to 0
                    iOwnerNewOccurrence = 0;
                }
                else {
                    // ... owner did exist: decreases its occs by 1
                    iOwnerNewOccurrence = iOwnerOldOccurrence - 1;
                }
            }
            else {
                // new occurrence is >= 1: increasing occurrences
                if( iOwnerOldOccurrence < 1 ) {
                    // ... owner did not exist: set its occs to 1
                    iOwnerNewOccurrence = 1;
                }
                else {
                    // ... owner did exist: increases its occs by 1
                    iOwnerNewOccurrence = iOwnerOldOccurrence + 1;
                }
            }

            gType = pOwnerGroupT->GetDimType();
            if( gType == CDimension::VoidDim )
                gType = CDimension::Record;

            ASSERT( gType == CDimension::Record || gType == CDimension::Item || gType == CDimension::SubItem );

            int iPrevValue = theIndex.getIndexValue( gType );
            theIndex.setIndexValue( gType, iOwnerNewOccurrence );
            GroupSetCurOccurrence( pOwnerGroupT, theIndex, iSymOwner );
            theIndex.setIndexValue( gType, iPrevValue );
        }                               // end 'if bUpdateOwner'
    }                                   // end 'if iSymOwner > 0'

    return bCanSet;
}

bool CEngineArea::GroupSetCurOccurrence( GROUPT* pGroupT, int iNewOccurrence, int iSymGroup, int iSymObj ) {
    ASSERT( pGroupT );

    // setup the target group
    if( iSymGroup <= 0 ) {
        iSymGroup = pGroupT->GetSymbolIndex();
        ASSERT( iSymGroup > 0 );
    }

    // verify if passed occurrences are valid
    int   iMaxOccs = pGroupT->GetMaxOccs();
    bool  bCanSet  = ( iNewOccurrence >= 0 && iNewOccurrence <= iMaxOccs );


    if( !bCanSet )
    {
        DebugMsg( 0, _T(" GrouptSetCurOccurrence: iNewOcc < 0 or iNewOcc is > MaxOccs") );
        return bCanSet;
    }

    // update target-group whenever the occurrences are to be changed:
    int         iOldOccurrence = pGroupT->GetCurrentOccurrences();

//////////////////////////////////////////////////////////////////////////
// RTODO: Test this
    if( iOldOccurrence == pGroupT->GetMaxOccs() )
    {
        iOldOccurrence = 0;
    }
//////////////////////////////////////////////////////////////////////////

    bool        bTryToUpdateOwner = false;

    if( iNewOccurrence != iOldOccurrence ) {

        // set occurrences of target-group to the new amount
        pGroupT->SetCurrentOccurrences( iNewOccurrence );

        // RTODO: Check if this update is ok always
        pGroupT->CalculateAndSetDataOccurrences( iNewOccurrence );
        /*
        if( pGroupT->GetDataOccurrences() < pGroupT->GetCurrentOccurrences() )
            pGroupT->SetDataOccurrences( pGroupT->GetCurrentOccurrences() );
            */

        // Calculate index array and update occurrences
        pGroupT->CalculateAndSetCurrentOccurrences( iNewOccurrence );
        pGroupT->AdjustTotalOccs();     // new          // victor May 28, 01

        // adjust OccTree to mirror the target-group update
        //$TODO pOccTree->AdjustTree( iSymGroup, iOldOccurrence, iNewOccurrence );

        bTryToUpdateOwner = true;
    }
    else
    {
    }

    // decide on changing the owner-group if any
    int     iSymOwner = pGroupT->GetOwnerSymbol();

    if( bTryToUpdateOwner && iSymOwner > 0 ) {               // only when has an owner-group
        GROUPT*     pOwnerGroupT  = GPT(iSymOwner);
        bool        bOwnerIsLevel = ( pOwnerGroupT->GetGroupType() == GROUPT::Level );
        int         iOwnerLevel   = pOwnerGroupT->GetLevel();

        // setup the associated OccTree

        // checks if the given item or group is a potential trigger for
        // updating the owner-group occurrences: it must be the first
        // member of its group.  Pay attention to the following: this
        // function is started with a true item-field arriving, then
        // it is recursively called for each owner-group.  Owner-groups
        // changes are solved in sequence of ancestor-to-child, where
        // the top ancestor is the Level-1 group (the owner-group for
        // Level-0 is excluded, its occurrences are never changed).  The
        // owner-group Level-1 is in practice modified once and only once
        // in order to acquire a first/unique occurrence when a Level-2 node
        // arises for the first time.
        int         iItem = 1;

        if( iSymObj <= 0 || iSymObj == iSymGroup ) {
            // the group -not an item- was passed
            iItem = pOwnerGroupT->GetItemIndex( iSymGroup );
        }
        else {
            // an item was passed
            iItem = pGroupT->GetItemIndex( iSymObj );
            if( iItem == 0 )
                iItem = pOwnerGroupT->GetItemIndex( iSymGroup );
        }

        // RHF COM Aug 07, 2002 if( iItem == 0 ) {                  // it is a potential trigger
        if( iItem == 0 || pOwnerGroupT->GetCurrentOccurrences() < 1 ) {                  // it is a potential trigger // RHF Aug 07, 2002
            // check if owner-group occurrences are to be updated: each
            // time the trigger-group is created (occurrences set to 1)
            // or deleted (occurrences set to zero), the occurrences
            // of the owner-group must be correspondingly increased or
            // reduced.

            // the decision on updating owner-group is based on:
            // - the owner group can be touched (is not the Level-0)
            // - the new occurrence for the trigger-group is increased
            //   to 1 (creation) or decreased to 0 (deletion)
            bTryToUpdateOwner = ( !bOwnerIsLevel || iOwnerLevel > 0 ) &&
                ( ( iNewOccurrence == 1 && iOldOccurrence <  1 ) ||
                ( iNewOccurrence <  1 && iOldOccurrence >= 1 ) );

        }
        else
            bTryToUpdateOwner = false;

        if( bTryToUpdateOwner ) {            // it is a true trigger
            int     iOwnerOldOccurrence = pOwnerGroupT->GetCurrentOccurrences();
            int     iOwnerNewOccurrence = -1;

            if( iNewOccurrence < 1 ) {
                // new occurrence is zero: reducing occurrences
                if( iOwnerOldOccurrence < 1 ) {
                    // ... owner did not exist: set its occs to 0
                    iOwnerNewOccurrence = 0;
                }
                else {
                    // ... owner did exist: decreases its occs by 1
                    iOwnerNewOccurrence = iOwnerOldOccurrence - 1;
                }
            }
            else {
                // new occurrence is >= 1: increasing occurrences
                if( iOwnerOldOccurrence < 1 ) {
                    // ... owner did not exist: set its occs to 1
                    iOwnerNewOccurrence = 1;
                }
                else {
                    // ... owner did exist: increases its occs by 1
                    iOwnerNewOccurrence = iOwnerOldOccurrence + 1;
                }
            }

            GroupSetCurOccurrence( pOwnerGroupT, iOwnerNewOccurrence, iSymOwner );
        }                               // end 'if bUpdateOwner'
    }                                   // end 'if iSymOwner > 0'

    return bCanSet;
}


#ifdef _DEBUG

//////////////////////////////////////////////////////////////////////////
//
// --- TEST ONLY
//
//////////////////////////////////////////////////////////////////////////
CString CEngineArea::DumpGroupTName( int iSymbol )
{
    CString csName;

    if( iSymbol > 0 )
        csName = WS2CS(NPT(iSymbol)->GetName());
    else
        csName = ( iSymbol < 0 ) ? _T("unknown") : _T("none");

    // adding detailed description
    CString csDetail;

    if( IsSymbolTypeGR(iSymbol) ) {
        FLOW* pFlow = GPT(iSymbol)->GetFlow();
        CString csFlow;

        if( pFlow != NULL ) {
            int iSymFlow = pFlow->GetSymbolIndex();
            csFlow.Format(_T("of Flow %s{%d})"), NPT(iSymFlow)->GetName().c_str(), iSymFlow );
        }
        else
            csFlow = _T("without Flow");

        csDetail.Format(_T(" (%d, %s::%s %s)"),
                        iSymbol,
                        pFlow == nullptr ? _T("unknown") : ToString(pFlow->GetType()),
                        pFlow == nullptr ? _T("unknown") : ToString(pFlow->GetSubType()),
                        csFlow.GetString());
    }
    else if( iSymbol > 0 )
        csDetail.Format( _T(" (%d, %s::%s)"), iSymbol, ToString(NPT(iSymbol)->GetType()), ToString(NPT(iSymbol)->GetSubType()) );
    else
        csDetail.Format( _T(" (%d)"), iSymbol );

    csName.Append(csDetail);

    return csName;
}

#endif//_DEBUG


//////////////////////////////////////////////////////////////////////////
// 3d handling -- RCL Aprl 2004
//////////////////////////////////////////////////////////////////////////

// void GROUPT::SetDimType( CDimension::VDimType xType )
//
// RCL, Apr 2004
void GROUPT::SetDimType( CDimension::VDimType xType )
{
    CString csMsg;

    m_eDimType = xType;

    delete m_pOccTree;
    m_pOccTree = 0;

    #define MAXocc(p) p->GetMaxOccs()

    switch( xType )
    {
    case CDimension::VoidDim:
        break;
    case CDimension::Record:
        m_pOccTree = 0; // new RecordOccurrenceInfoSet;
        break;
    default:
        // deduce what to do according to the group's dimension number
        {
            GROUPT* pOwner = this->GetOwnerGPT();
            CString csGroupName = WS2CS(this->GetName());

            int iNumDim = GetNumDim();
            CString csNumDim;
            csNumDim.Format( _T("SetDimType subitem [%s] with %d dimensions\n"), GetName().c_str(), iNumDim );
            RTRACE2( _T("%s"), csNumDim.GetString() );
            switch( iNumDim )
            {
            case 1:
                // Single record, single item, multiple subitem
                m_pOccTree = 0;
                break;
            case 2:
                // multiple record, single item, multiple subitem
                if( pOwner->GetNumDim() == 0 )
                {
                    GROUPT* pOwnerOwner = pOwner->GetOwnerGPT();
                    ASSERT( pOwnerOwner != 0 );
                    ASSERT( pOwnerOwner->GetNumDim() == 1 );
                    // quick 'magic' number to detect
                    // big numbers [usually garbage]
                    ASSERT( MAXocc(pOwnerOwner) <= 2000 );
                    m_pOccTree = new ItemOccurrenceInfoSet( MAXocc(pOwnerOwner) );
                }
                // single record, multiple item, multiple subitem
                else
                {
                    ASSERT( pOwner->GetNumDim() == 1 );
                    m_pOccTree = new ItemOccurrenceInfoSet( MAXocc(pOwner) );
                }
                break;
            case 3:
                GROUPT* pOwnerOwner = pOwner->GetOwnerGPT();
                ASSERT( pOwnerOwner != 0 );

                if( pOwnerOwner != 0 )
                {
                    #ifdef _DEBUG
                    // cannot use usual TRACE macro, because of TRACE function defined above.
                    RTRACE2( _T("LEAK Trace: Building occ tree for group %s\n"), GetName().c_str() );
                    #endif
                    m_pOccTree =
                        new SubItemOccurrenceInfoSet( MAXocc(pOwnerOwner),
                        MAXocc(pOwner) );
                }
                break;
            }
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
static
void convert3DIndexToZeroBasedUserIndexes( const CNDIndexes& theIndex, int aIndex[DIM_MAXDIM] )
{

    int j = 0;
    for( j = 0; j < DIM_MAXDIM; j++ )
        aIndex[j] = -1;

    int iFix = 0;

    if( !theIndex.isZeroBased() )
        iFix = -1;

    int i = 0;
    if( theIndex.isUsingThisDimension( USE_DIM_1 ) )
        aIndex[i++] = theIndex.getIndexValue(0) + iFix;

    if( theIndex.isUsingThisDimension( USE_DIM_2 ) )
        aIndex[i++] = theIndex.getIndexValue(1) + iFix;

    if( theIndex.isUsingThisDimension( USE_DIM_3 ) )
        aIndex[i++] = theIndex.getIndexValue(2) + iFix;

    for( i = 0; i < DIM_MAXDIM; i++ )
        if( aIndex[i] < 0 )
            aIndex[i] = 0;

}

//////////////////////////////////////////////////////////////////////////

int GROUPT::GetTotalOccurrences( const CNDIndexes& theIndex ) const
{
    int iRetValue = 0;
    if( m_pOccTree == 0 )
    {
        ASSERT( this->GetDimType() == CDimension::Record ||
                this->GetNumDim() <= 1 );

        iRetValue = this->GetTotalOccurrences();
    }
    else
    {
        ASSERT( m_pOccTree != 0 );
        int aIndex[DIM_MAXDIM];
        convert3DIndexToZeroBasedUserIndexes( theIndex, aIndex );

//        TRACE( _T("About to calculate gettotalocc for group %s\n"), this->GetName() );
        GetTotalOccVisitor v( aIndex );
        m_pOccTree->accept( v );

        iRetValue = v.GetValue();
    }

    return iRetValue;
}

void GROUPT::SetTotalOccurrences( const CNDIndexes& theIndex, int iOcc )
{

    if( m_pOccTree == 0 )
    {
        ASSERT( this->GetDimType() == CDimension::Record ||
                this->GetNumDim() <= 1 );

        SetTotalOccurrences( iOcc );
    }
    else
    {
        ASSERT( m_pOccTree != 0 );

        // we may fix data occurrence...
        if( GetDataOccurrences( theIndex ) < iOcc )
            SetDataOccurrences( theIndex, iOcc );

        int aIndex[DIM_MAXDIM];
        convert3DIndexToZeroBasedUserIndexes( theIndex, aIndex );

        SetTotalOccVisitor v( aIndex );
        v.SetValue( iOcc );

        m_pOccTree->accept( v );
    }
}

int GROUPT::GetDataOccurrences( const CNDIndexes& theIndex ) const
{
    int iRetValue = 0;

    if( m_pOccTree == 0 )
    {
        ASSERT( this->GetDimType() == CDimension::Record ||
                this->GetNumDim() <= 1 );

        iRetValue = this->GetDataOccurrences();
    }
    else
    {
        ASSERT( m_pOccTree != 0 );
        int aIndex[DIM_MAXDIM];
        convert3DIndexToZeroBasedUserIndexes( theIndex, aIndex );

        ASSERT( m_pOccTree != 0 );

        GetDataOccVisitor v( aIndex );
        m_pOccTree->accept( v );

        iRetValue = v.GetValue();
    }

    return iRetValue;
}

void GROUPT::SetDataOccurrences(  const CNDIndexes& theIndex, int iOcc )
{

    if( m_pOccTree == 0 )
    {
        ASSERT( this->GetDimType() == CDimension::Record ||
                this->GetNumDim() <= 1 );


        SetDataOccurrences( iOcc );
    }
    else
    {
        ASSERT( m_pOccTree != 0 );
        int aIndex[DIM_MAXDIM];
        convert3DIndexToZeroBasedUserIndexes( theIndex, aIndex );

        SetDataOccVisitor v( aIndex );
        v.SetValue( iOcc );

        m_pOccTree->accept( v );
    }
}

// int GROUPT::GetCurrentOccurrences( int aInfo[DIM_MAXDIM] )
//
// RCL, Apr 2004
int GROUPT::GetCurrentOccurrences( int aInfo[DIM_MAXDIM] ) const
{
    int iRetValue = 0;
    if( m_pOccTree == 0 )
    {
        ASSERT( this->GetDimType() == CDimension::Record ||
                this->GetNumDim() <= 1 );
        iRetValue = this->GetCurrentOccurrences();
    }
    else
    {
        ASSERT( m_pOccTree != 0 );

        GetCurrOccVisitor v( aInfo );
        m_pOccTree->accept( v );

        iRetValue = v.GetValue();
    }

    return iRetValue;
}

int GROUPT::GetCurrentOccurrences( const CNDIndexes& theIndex ) const
{
    int iRetValue = 0;
    if( m_pOccTree == 0 )
    {
        ASSERT( this->GetDimType() == CDimension::Record ||
                this->GetNumDim() <= 1 );

        iRetValue = this->GetCurrentOccurrences();
    }
    else
    {
        ASSERT( m_pOccTree != 0 );

        int aIndex[DIM_MAXDIM];
        convert3DIndexToZeroBasedUserIndexes( theIndex, aIndex );

        GetCurrOccVisitor v( aIndex );
        m_pOccTree->accept( v );

        iRetValue = v.GetValue();
    }

    return iRetValue;
}

// void GROUPT::SetCurrentOccurrences( int aInfo[DIM_MAXDIM], int iOcc )
//
// RCL, Apr 2004
void GROUPT::SetCurrentOccurrences( int aInfo[DIM_MAXDIM], int iOcc )
{
    if( m_pOccTree == 0 )
    {
        ASSERT( this->GetDimType() == CDimension::Record ||
                this->GetNumDim() <= 1 );
        // should be superfluous
        SetCurrentOccurrences( iOcc );
    }
    else
    {
        ASSERT( m_pOccTree != 0 );

        SetCurrOccVisitor v( aInfo );
        v.SetValue( iOcc );

        m_pOccTree->accept( v );
    }
}

void GROUPT::SetCurrentOccurrences( const CNDIndexes& theIndex, int iOcc )
{
    if( m_pOccTree == 0 )
    {
        ASSERT( this->GetDimType() == CDimension::Record ||
                this->GetNumDim() <= 1 );

        // should be superfluous
        SetCurrentOccurrences( iOcc );
    }
    else
    {
        ASSERT( m_pOccTree != 0 );

        int aIndex[DIM_MAXDIM];
        convert3DIndexToZeroBasedUserIndexes( theIndex, aIndex );

        SetCurrOccVisitor v( aIndex );
        v.SetValue( iOcc );

        m_pOccTree->accept( v );
    }
}

// void GROUPT::CalculateAndSetOccurrences( int iOcc, int iWhichOcc )
//
const int CALCULATE_CURRENTOCC = 1;
const int CALCULATE_TOTALOCC   = 2;
const int CALCULATE_DATAOCC    = 3;

// GROUPT::CalculateAndSetOccurrences
// Internal method used by
//        CalculateAndSetCurrentOccurrences( int iOcc )
//        CalculateAndSetDataOccurrences( int iOcc )
//        CalculateAndSetTotalOccurrences( int iOcc )
//
// Internally it builds a 3D Object, and specifies
// iOcc as the occurrence (Current, Data or Total)
// for that object.
//
void GROUPT::CalculateAndSetOccurrences( int iOcc, int iWhichOcc )
{
    if( this->GetDimType() == CDimension::Record )
    {
        return;
    }

    try
    {
        CalcCurrent3DObject();
        CNDIndexes theIndex = GetCurrent3DObject().getIndexes();

        switch( iWhichOcc )
        {
        case CALCULATE_CURRENTOCC:
            SetCurrentOccurrences( theIndex, iOcc );
            break;
        case CALCULATE_DATAOCC:
            SetDataOccurrences( theIndex, iOcc );
            break;
        case CALCULATE_TOTALOCC:
            SetTotalOccurrences( theIndex, iOcc );
            break;
        }
    }
    catch( const C3DException& )
    {
        // VoidDim detected, do nothing ... for now
    }
}

void GROUPT::CalculateAndSetCurrentOccurrences( int iOcc )
{
    CalculateAndSetOccurrences( iOcc, CALCULATE_CURRENTOCC );
}

void GROUPT::CalculateAndSetDataOccurrences( int iOcc )
{
    CalculateAndSetOccurrences( iOcc, CALCULATE_DATAOCC );
}

void GROUPT::CalculateAndSetTotalOccurrences( int iOcc )
{
    CalculateAndSetOccurrences( iOcc, CALCULATE_TOTALOCC );
}


static
void tryToSetRecordDimension_local( C3DObject& theObject, GROUPT* pGroupT, bool bUseEx, CString csName )
{
    #define CURROCC_METHOD(p) (bUseEx ? p->GetCurrentExOccurrence() : p->GetCurrentOccurrences())
    if( pGroupT->GetGroupType() == GROUPT::GroupOrRecord )
    {
        theObject.setIndexValue( CDimension::Record, CURROCC_METHOD(pGroupT) );
    }
}

void GROUPT::CalcCurrent3DObject_internal(bool bUseEx) /*throw(C3DException)*/
{
    CDimension::VDimType theType = this->GetDimType();
    if( theType == CDimension::VoidDim )
        throw C3DException();

    m_curr3dObject.SetEmpty();
    m_curr3dObject.SetSymbol( this->GetSymbol() );

    ASSERT( theType != CDimension::VoidDim );
    m_curr3dObject.setIndexValue( theType, CURROCC_METHOD(this) );

    GROUPT* pOwner = this->GetOwnerGPT();

    switch( theType )
    {
    case CDimension::Record:
        // everything is set, do nothing ...
        break;

    case CDimension::Item:
        ASSERT( pOwner != 0 );
        if( pOwner->GetDimType() == CDimension::Record )
            m_curr3dObject.setIndexValue( CDimension::Record, CURROCC_METHOD(pOwner) );
        else
            tryToSetRecordDimension_local( m_curr3dObject, pOwner, bUseEx, WS2CS(GetName()) );
        break;

    case CDimension::SubItem:

        ASSERT( pOwner != 0 );
        if( pOwner->GetDimType() == CDimension::Item )
        {
            m_curr3dObject.setIndexValue( CDimension::Item, CURROCC_METHOD(pOwner) );
            pOwner = pOwner->GetOwnerGPT();
            ASSERT( pOwner != 0 );
        }

        if( pOwner->GetDimType() == CDimension::Record )
            m_curr3dObject.setIndexValue( CDimension::Record, CURROCC_METHOD(pOwner) );
        else
            tryToSetRecordDimension_local( m_curr3dObject, pOwner, bUseEx, WS2CS(GetName()) );
        break;
    }

}

void GROUPT::CalcCurrent3DObject() /*throw(C3DException)*/
{
    CalcCurrent3DObject_internal(false);
}

void GROUPT::CalcCurrent3DObjectEx() /*throw(C3DException)*/
{
    CalcCurrent3DObject_internal(true);
}

C3DObject GROUPT::GetCurrent3DObject()
{
    return m_curr3dObject;
}
//////////////////////////////////////////////////////////////////////////

#if defined(USE_BINARY) || defined(GENERATE_BINARY)
void GROUPT::accept( GroupVisitor* visitor )
{
    visitor->visit( this );
}
//////////////////////////////////////////////////////////////////////////
#endif


const CDictItem* GROUPT::GetFirstDictItem() const
{
    const CDictItem* pDictItem = nullptr;

    for( int i = 0; pDictItem == nullptr && i < m_iNumItems; i++ )
    {
        const Symbol* symbol = GetItemSymbolPtr(i);

        if( symbol->IsA(SymbolType::Variable) )
            pDictItem = assert_cast<const VART*>(symbol)->GetDictItem();

        else if( symbol->IsA(SymbolType::Group) )
            pDictItem = assert_cast<const GROUPT*>(symbol)->GetFirstDictItem();
    }

    ASSERT(pDictItem != nullptr);

    return pDictItem;
}

Symbol* GROUPT::GetItemSymbolPtr( int iItem ) const
{
    Symbol* pSymbol = 0;
    int iSymbolIdx = GetItemSymbol(iItem);

    if( iSymbolIdx >= 0 )
        pSymbol = NPT(iSymbolIdx);

    return pSymbol;
}


void GROUPT::serialize_subclass(Serializer& ar)
{
    RunnableSymbol::serialize(ar);
}


const Logic::SymbolTable& GROUPT::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}
