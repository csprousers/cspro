//------------------------------------------------------------------------
//
//  EXAPPL.cpp    data and structures used by CSPRO executors
//
//------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Comp.h"
#include "CompilerCreator.h"
#include "Engine.h"
#include "Exappl.h"
#include <zEngineO/ApplicationLoader.h>
#include <zEngineO/Block.h>
#include <zEngineO/Userbar.h>
#include <zPlatformO/PlatformInterface.h>
#include <zUtilO/AppLdr.h>
#include <zUtilO/CommonStore.h>
#include <zUtilO/ExecutionStack.h>
#include <zUtilO/TraceMsg.h>
#include <zMessageO/MessageEvaluator.h>
#include <ZBRIDGEO/npff.h>
#include <zListingO/WriteFile.h>


// temporary...
#include <zAppO/Properties/ApplicationProperties.h>
#include <zHtml/UseHtmlDialogs.h>



//////////////////////////////////////////////////////////////////////////////
//
// --- constructor/destructor
//
//////////////////////////////////////////////////////////////////////////////

CEngineDriver::CEngineDriver(Application* pApplication, bool bDoInterpreter, CompilerCreator* compiler_creator/* = nullptr*/)
    :   m_pApplication(pApplication),
        m_processSummary(std::make_shared<ProcessSummary>())
{
    ASSERT(m_pApplication != nullptr);
    SetUseHtmlDialogs(m_pApplication->GetApplicationProperties().UseHtmlDialogs); // remove related include files when this is removed

    // --- engine links
    //SAVY 10/15/03 for tab special values
    m_bTabProcessSpecial = false;
    m_pEngineDriver   = this;

    m_pEngineArea = &m_EngineArea;
    m_engineData = &m_pEngineArea->GetEngineData();

#ifdef WIN_DESKTOP
    m_pEngineCompFunc = ( compiler_creator != nullptr ) ? compiler_creator->CreateCompiler(this) : 
                                                          std::make_unique<CEngineCompFunc>(this);
#endif

    m_pEngineDefines  = &m_EngineDefines;
    m_pEngineSettings = &m_EngineSettings;

    ASSERT( m_pEngineArea != 0 );
    m_pEngineArea->SetEngineDriver( this );

    if( bDoInterpreter )
        m_pIntDriver = std::make_unique<CIntDriver>(*this);

    // process options & flags
    m_Issamod       = ModuleType::Designer;
    m_ExMode        = 0;                // $MODE   for ENTRY & INDEX apps
    m_lpszExecutorLabel = nullptr;

    // basic arrays
    m_Dicxbase      = NULL;
    m_Secxbase      = NULL;
    m_Varxbase      = NULL;

    // --- associated or automatic files
    m_pPifFile      = NULL;

    m_TmpCtabTmp.Empty();

    // batch-skip management                            // victor Mar 14, 01
    m_bSkipping       = false;          // "skipping" flag
    m_pSkippingSource = NULL;           // skip-source description
    m_iSkippingSource = -1;             // skip-source proc-type (PROCTYPE_PRE or PROCTYPE_POST)
    m_pSkippingTarget = NULL;           // skip-target description
    m_iSkippingTarget = -1;             // skip-source proc-type (PROCTYPE_PRE or PROCTYPE_POST)

    // miscellaneous
    SetHasOutputDict( false ); // RHF Aug 23, 2002
    SetHasSomeInsDelSortOcc( false );

    SetUsePrevLimits( false ); // RHF May 14, 2003

    m_bBinaryLoaded = false;

    UpdateMessageIssuers();
}


CEngineDriver::CEngineDriver(CNPifFile* pPifFile)
    :   CEngineDriver(pPifFile->GetApplication(), true)
{
    SetPifFile(pPifFile);
}


CEngineDriver::~CEngineDriver()
{
    m_pEngineCompFunc.reset();
    m_pIntDriver.reset();

    ResetSkipping();                                    // victor Mar 14, 01

#ifdef WIN_DESKTOP
    // 20120822 unload all loaded keyboards
    for( int i = 0; i < m_vAddedKeyboards.GetSize(); i++ )
        UnloadKeyboardLayout(m_vAddedKeyboards[i]);
#endif
}


void CEngineDriver::BuildMessageManagers()
{
    if( m_pApplication->GetApplicationLoader() != nullptr )
    {
        try
        {
            if( m_systemMessageManager == nullptr )
                m_systemMessageManager = m_pApplication->GetApplicationLoader()->GetSystemMessages();

            if( m_userMessageManager == nullptr )
                m_userMessageManager = m_pApplication->GetApplicationLoader()->GetUserMessages();

            UpdateMessageIssuers();
        }

        catch( const ApplicationLoadException& APP_LOAD_TODO_exception )
        {
            // APP_LOAD_TODO when the application loader is complete, these exceptions should be caught elsewhere
            ErrorMessage::Display(APP_LOAD_TODO_exception.GetErrorMessage());
        }        
    }

    else
    {
        ASSERT(false); // APP_LOAD_TODO make sure this never happens
    }
}


void CEngineDriver::SetUserbar(std::unique_ptr<Userbar> userbar)
{
    m_userbar = std::move(userbar);
}


std::shared_ptr<CommonStore> CEngineDriver::GetCommonStore()
{
    if( m_commonStore == nullptr )
    {
        m_commonStore = std::make_shared<CommonStore>();

        if( !m_commonStore->Open({ CommonStore::TableType::UserSettings, CommonStore::TableType::PersistentVariables },
                                 CS2WS(m_pPifFile->GetCommonStoreFName())) )
        {
            m_commonStore.reset();
        }
    }

    return m_commonStore;
}


//////////////////////////////////////////////////////////////////////////////
//
// --- batch-skip management                            // victor Mar 14, 01
//
//////////////////////////////////////////////////////////////////////////////

void CEngineDriver::SetSkipping( int iSymbol, int* aIndex, int iProgType, bool bTarget ) {
    m_bSkipping = true;

    CNDIndexes theIndex( ONE_BASED, aIndex );

    if( !bTarget ) {                    // skip-source
        ASSERT( m_pSkippingSource == NULL );
        m_pSkippingSource = new C3DObject( iSymbol, theIndex );
        m_iSkippingSource = iProgType;
    }
    else {                              // skip-target
        ASSERT( m_pSkippingTarget == NULL );
        m_pSkippingTarget = new C3DObject( iSymbol, theIndex );
        m_iSkippingTarget = iProgType;
    }
}

//////////////////////////////////////////////////////////////////////////
// SetSkipping -> new 3d versions: SetSkippingSource + SetSkippingTarget
// rcl, Sept 04, 2004

void CEngineDriver::SetSkippingSource( C3DObject& theSourceObject, int iProgType ) // rcl, Sept 04, 04
{
    m_bSkipping = true;

    ASSERT( !theSourceObject.getIndexes().isZeroBased() );
    ASSERT( m_pSkippingSource == NULL );
    m_pSkippingSource = new C3DObject( theSourceObject.GetSymbol(), theSourceObject.GetIndexes() );
    m_iSkippingSource = iProgType;
}

void CEngineDriver::SetSkippingTarget( C3DObject& theTargetObject, int iProgType )  // rcl, Sept 04, 04
{
    m_bSkipping = true;

    ASSERT( !theTargetObject.getIndexes().isZeroBased() );
    ASSERT( m_pSkippingTarget == NULL );
    m_pSkippingTarget = new C3DObject( theTargetObject.GetSymbol(), theTargetObject.GetIndexes() );
    m_iSkippingTarget = iProgType;
}

//////////////////////////////////////////////////////////////////////////

void CEngineDriver::ResetSkipping( void ) {
    m_bSkipping = false;

    if( m_pSkippingSource != NULL )     // skip-source description
        delete m_pSkippingSource;
    if( m_pSkippingTarget != NULL )     // skip-target description
        delete m_pSkippingTarget;

    m_pSkippingSource = NULL;           // skip-source description
    m_iSkippingSource = -1;             // skip-source proc-type (PROCTYPE_PRE or PROCTYPE_POST)
    m_pSkippingTarget = NULL;           // skip-target description
    m_iSkippingTarget = -1;             // skip-source proc-type (PROCTYPE_PRE or PROCTYPE_POST)
}


#include "helper.h"


static bool IsEndGroupReached(Symbol* pSourceSymbol, Symbol* pCurrentSymbol)
{
    bool bTargetReached = false;
    GROUPT* pCurrentGroupT=NULL;

    if( pCurrentSymbol->IsA(SymbolType::Variable) ) {
        pCurrentGroupT = LocalGetParent( (VART*) pCurrentSymbol );
    }
    else if( pCurrentSymbol->GetType() == SymbolType::Block ) {
        pCurrentGroupT = assert_cast<const EngineBlock*>(pCurrentSymbol)->GetGroupT();
    }
    else if( pCurrentSymbol->IsA(SymbolType::Group) ) {
        pCurrentGroupT=(GROUPT*) pCurrentSymbol;
    }
    if( pCurrentGroupT ) {
        GROUPT* pSourceGroupT=NULL;

        if( pSourceSymbol->IsA(SymbolType::Group) )
            pSourceGroupT=(GROUPT*)pSourceSymbol;
        else if( pSourceSymbol->GetType() == SymbolType::Block ) {
            pSourceGroupT = assert_cast<const EngineBlock*>(pSourceSymbol)->GetGroupT();
        }
        else if( pSourceSymbol->IsA(SymbolType::Variable) ) {
            VART*   pVarT=(VART*)pSourceSymbol;
            pSourceGroupT = LocalGetParent( pVarT );
        }
        else
            ASSERT(0);

        if( pSourceGroupT ) {
            int iSourceSymbol = pSourceGroupT->GetSymbolIndex();

            // if pCurrentGroupT is not a son of pSourceGroupT
            bool bIsAncestor= pCurrentGroupT->IsAncestor( iSourceSymbol, false, true );
            if( !bIsAncestor )
                bTargetReached = true;
        }
    }

    return bTargetReached;
}

bool CEngineDriver::IsSkippingTargetReached( int iCurrentSymbol, CNDIndexes& aIndex ) {
    bool    bTargetReached = true;

    if( !m_bSkipping )
        return bTargetReached;

    if( m_pSkippingTarget->GetSymbol() < 0 ) {
        int     iSourceSymbol=-m_pSkippingTarget->GetSymbol();
        bTargetReached = IsEndGroupReached( NPT(iSourceSymbol), NPT(iCurrentSymbol) );
    }
    else {
        bool bFitSymbol=(m_pSkippingTarget->GetSymbol()==iCurrentSymbol);

        int iOcc[DIM_MAXDIM];

        //Make zero based
        for( int i=0; i < DIM_MAXDIM; i++ ) {
            iOcc[i] = m_pSkippingTarget->getIndexValue(i);
            iOcc[i] = ( iOcc[i] >= 1 ) ? iOcc[i]-1 : 0;
        }

        bool indices_match =
            iOcc[0] == aIndex.getIndexValue(0) &&
            iOcc[1] == aIndex.getIndexValue(1) &&
            iOcc[2] == aIndex.getIndexValue(2);

        if( bFitSymbol && indices_match )
            bTargetReached = true;

        // special processing to check if the target was a block that didn't
        // have any procs defined, which is why the target wasn't cleared from calling
        // the version of IsSkippingTargetReached below
        else if( !bFitSymbol && indices_match && NPT(m_pSkippingTarget->GetSymbol())->IsA(SymbolType::Block) )
            bTargetReached = true;

        else
            bTargetReached = false;
    }

    return bTargetReached;
}

//#else
bool CEngineDriver::IsSkippingTargetReached( int iCurrentSymbol, int* aIndex, int iProgType ) {
    // IsSkippingTargetReached: calculate if {iCurrentSymbol, aIndex, iProgType} is at or after m_pSkippingTarget
    bool    bTargetReached = true;

    if( !m_bSkipping )
        return bTargetReached;

    if( m_pSkippingTarget->GetSymbol() < 0 ) {
        int     iSourceSymbol=-m_pSkippingTarget->GetSymbol();
        bTargetReached = IsEndGroupReached( NPT(iSourceSymbol), NPT(iCurrentSymbol) );

        return bTargetReached;
    }

    // remark - in Batch, the control is never passed to external dicts

    // completing target coordinates
    int     iTargetSymbol = m_pSkippingTarget->GetSymbol();
    int     iTargetProg   = m_iSkippingTarget;
    SymbolType eTargetType = NPT(iTargetSymbol)->GetType();
    int     iTargetSymVar = 0;
    int     iTargetSymBlock = 0;
    int     iTargetOcc = 0;
    int     iTargetSymGroup;
    int     iTargetOrder;
    int     iTargetGroupOrder;
    int     iTargetLevel;

    if( eTargetType == SymbolType::Variable )
    {
        iTargetSymVar = iTargetSymbol;
        VART* pTargetVarT = VPT(iTargetSymVar);

        iTargetOcc = m_pSkippingTarget->getIndexValue( 0 ); // TRANSITION!

        iTargetSymGroup = pTargetVarT->GetOwnerGroup();
        GROUPT* pTargetGroupT = GPT(iTargetSymGroup);
        pTargetGroupT->GetItemIndex( iTargetSymVar, &iTargetOrder );
        pTargetGroupT->GetOwnerGPT()->GetItemIndex( iTargetSymGroup, &iTargetGroupOrder );
        iTargetLevel = pTargetVarT->GetLevel();
    }

    else if( eTargetType == SymbolType::Block )
    {
        iTargetSymBlock = iTargetSymbol;
        const EngineBlock& engine_block = GetSymbolEngineBlock(iTargetSymBlock);

        iTargetOcc = m_pSkippingTarget->getIndexValue(0);

        GROUPT* pTargetGroupT = engine_block.GetGroupT();
        iTargetSymGroup = pTargetGroupT->GetSymbolIndex();

        pTargetGroupT->GetItemIndex(iTargetSymBlock, &iTargetOrder);
        pTargetGroupT->GetOwnerGPT()->GetItemIndex(iTargetSymGroup, &iTargetGroupOrder);
        iTargetLevel = pTargetGroupT->GetLevel();
    }

    else
    {
        ASSERT(eTargetType == SymbolType::Group);
        iTargetSymGroup = iTargetSymbol;
        GROUPT* pTargetGroupT = GPT(iTargetSymGroup);
        pTargetGroupT->GetOwnerGPT()->GetItemIndex( iTargetSymGroup, &iTargetOrder );
        iTargetGroupOrder = iTargetOrder;
        iTargetLevel = pTargetGroupT->GetLevel();
    }

    // completing tested coordinates
    int     iTestedSymbol = iCurrentSymbol;
    int     iTestedProg = iProgType;
    SymbolType eTestedType = NPT(iTestedSymbol)->GetType();
    int     iTestedSymVar = 0;
    int     iTestedSymBlock = 0;
    int     iTestedOcc = 0;
    int     iTestedSymGroup;
    int     iTestedOrder;
    int     iTestedGroupOrder;
    int     iTestedLevel;

    if( eTestedType == SymbolType::Variable )
    {
        iTestedSymVar = iTestedSymbol;
        VART* pTestedVarT = VPT(iTestedSymVar);
        iTestedOcc = aIndex[0];                        // TRANSITION!
        iTestedSymGroup = pTestedVarT->GetOwnerGroup();
        GROUPT* pTestedGroupT = GPT(iTestedSymGroup);
        pTestedGroupT->GetItemIndex( iTestedSymVar, &iTestedOrder );
        pTestedGroupT->GetOwnerGPT()->GetItemIndex( iTestedSymGroup, &iTestedGroupOrder );
        iTestedLevel = pTestedVarT->GetLevel();
    }

    else if( eTestedType == SymbolType::Block )
    {
        iTestedSymBlock = iTestedSymbol;
        const EngineBlock& engine_block = GetSymbolEngineBlock(iTestedSymBlock);

        iTestedOcc = aIndex[0];

        GROUPT* pTestedGroupT = engine_block.GetGroupT();
        iTestedSymGroup = pTestedGroupT->GetSymbolIndex();

        pTestedGroupT->GetItemIndex(iTestedSymBlock, &iTestedOrder);
        pTestedGroupT->GetOwnerGPT()->GetItemIndex(iTestedSymGroup, &iTestedGroupOrder);
        iTestedLevel = pTestedGroupT->GetLevel();
    }

    else
    {
        ASSERT(eTestedType == SymbolType::Group);
        iTestedSymGroup = iTestedSymbol;
        GROUPT* pTestedGroupT = GPT(iTestedSymGroup);
        pTestedGroupT->GetOwnerGPT()->GetItemIndex( iTestedSymGroup, &iTestedOrder );
        iTestedGroupOrder = iTestedOrder;
        iTestedLevel = pTestedGroupT->GetLevel();
    }

    if( iTestedLevel != iTargetLevel )
        bTargetReached = true;

    else if( eTargetType == SymbolType::Group ) {      // target is a Group
        if( eTestedType == SymbolType::Block || eTestedType == SymbolType::Variable ) { // ... tested is a block or VA
            bTargetReached = ( iTestedSymGroup != iTargetSymGroup );

            // RHF INIC Oct 17, 2003 Skip to ROSTER (Ejemplo: V1, V2, R1 todos en mismo form).
            // SKIP TO R1 en V1 generaba mensaje en V2 (ejecutaba el PreProc).
            if( GPT(iTargetSymGroup)->IsAncestor( iTestedSymGroup, false ) )
                bTargetReached = false;
            // RHF END Oct 17, 2003
        }
                                        // ... tested is a Group
        else if( !GPT(iTestedSymGroup)->IsAncestor( iTargetSymGroup, false ) )
            bTargetReached = ( iTestedOrder >= iTargetOrder );
        else
            bTargetReached = false;
    }

    else if( eTestedType == SymbolType::Group ) {      // target is a block or Var, testing a Group
        if( iTargetProg == PROCTYPE_PRE )
            bTargetReached = ( iTestedOrder > iTargetOrder );

        else if( iTargetProg == PROCTYPE_NONE ) // RHF+VC Jun 06, 2001
            bTargetReached = true;// RHF+VC Jun 06, 2001
        else if( iTargetProg == PROCTYPE_ONFOCUS )
            bTargetReached = ( iTestedOrder > iTargetOrder );
        else if( iTargetProg == PROCTYPE_KILLFOCUS )
            bTargetReached = ( iTestedGroupOrder >= iTargetGroupOrder );

        else if( iTargetProg == PROCTYPE_POST )
            bTargetReached = ( iTestedGroupOrder >= iTargetGroupOrder );
        else
            ASSERT(0);
    }

    else if( eTargetType == SymbolType::Block )
    {
        const EngineBlock& target_engine_block = GetSymbolEngineBlock(iTargetSymBlock);
        bool reached_block_or_field_in_block =
            ( iTargetSymBlock == iTestedSymBlock ) || target_engine_block.ContainsField(iTestedSymVar);
        bool block_or_field_in_same_group = ( iTargetSymGroup == iTestedSymGroup );

        if( reached_block_or_field_in_block || block_or_field_in_same_group )
        {
            if( iTestedOcc > iTargetOcc )
            {
                bTargetReached = true;
            }

            else if( iTestedOcc == iTargetOcc )
            {
                if( reached_block_or_field_in_block )
                {
                    bTargetReached = ( iTestedSymVar != 0 ) || ( iTestedProg == iTargetProg );
                }

                else
                {
                    bTargetReached = ( iTestedOrder > iTargetOrder );
                }
            }

            else
            {
                bTargetReached = false;
            }
        }
    }

    else {                              // target is a Var, testing a Var

        if( iTestedSymVar == iTargetSymVar ) {
            if( iTestedOcc > iTargetOcc )
                bTargetReached = true;
            else if( iTestedOcc == iTargetOcc )
                bTargetReached = ( iTestedProg == iTargetProg );
            else
                bTargetReached = false;
        }
        else if( iTestedSymGroup == iTargetSymGroup ) {
            if( iTestedOcc > iTargetOcc )
                bTargetReached = true;
            else if( iTestedOcc == iTargetOcc )
                bTargetReached = ( iTestedOrder > iTargetOrder );
            else
                bTargetReached = false;
        }
        else
            bTargetReached = false;     // MAYBE WRONG!!!
    }

    return bTargetReached;
}
//#endif
