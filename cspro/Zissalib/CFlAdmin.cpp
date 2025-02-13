//---------------------------------------------------------------------------
//  File name: CFlAdmin.cpp
//
//  Description:
//          Implementation for the flow-administrator for the 3-D driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              25 Jul 01   vc      Created
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include "CFlAdmin.h"
#include <engine/Engdrv.h>
#include <engine/Engine.h>
#include <engine/IntDrive.h>

#if defined(_DEBUG) && defined(WIN_DESKTOP)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
//
// --- construction/destruction
//
/////////////////////////////////////////////////////////////////////////////

CFlAdmin::CFlAdmin() {
    // --- flows and CsDrivers
    m_iCurSlot      = -1;

    // --- engine links
    m_pEngineDriver = NULL;
    m_pEngineArea   = NULL;
    m_pIntDriver    = NULL;
    m_pEntryDriver  = NULL;
}

CFlAdmin::~CFlAdmin() {
    int     iSlot = (int)m_aFlowUnit.size();

    while( --iSlot >= 0 ) {
        FlowUnit*   pFlowUnit = GetFlowUnitAt( iSlot );
        delete pFlowUnit;
    }
}



/////////////////////////////////////////////////////////////////////////////
//
// --- FlowUnits management
//
/////////////////////////////////////////////////////////////////////////////

void CFlAdmin::CreateFlowUnit( FLOW* pFlow ) {
    FlowUnit*   pFlowUnit = new FlowUnit( (int)m_aFlowUnit.size(), pFlow );
    CsDriver*   pCsDriver = pFlowUnit->GetCsDriver();

    pCsDriver->SetFlAdmin( this );
    pCsDriver->SetFlow( pFlow );
    pCsDriver->SetEngineDriver( m_pEngineDriver, m_pEntryDriver );

    m_aFlowUnit.emplace_back( pFlowUnit );
}

bool CFlAdmin::EnableFlow( int iSymFlow ) {
    bool        bEnabled   = false;
    FlowUnit*   pFlowUnit  = GetFlowUnit( iSymFlow );
    int         iActiveNum = ( pFlowUnit != NULL ) ? pFlowUnit->GetActiveNum() : -1;

    // can activate only if not active
    if( iActiveNum == 0 ) {
        CsDriver*   pOldCsDriver = GetCsDriver();

        bEnabled   = true;

        // set the activation order to 1 more than the current FlowUnit (# 1 for the first activated)
        if( m_iCurSlot >= 0 )
            iActiveNum = 1 + GetFlowUnitAt( m_iCurSlot )->GetActiveNum();
        else
            iActiveNum = 1;
        ASSERT( ( iActiveNum == 1 && pFlowUnit->GetFlow()->GetSubType() == SymbolSubType::Primary ) ||
                ( iActiveNum != 1 && pFlowUnit->GetFlow()->GetSubType() == SymbolSubType::Secondary ) );
        pFlowUnit->SetActiveNum( iActiveNum );

        // Secondary flow (when iActiveNum > 1): save the environement into the CsDriver to be deactivated
        if( iActiveNum > 1 ) {                          // victor Jul 25, 01
            // ... save the environement into the old CsDriver
            pOldCsDriver->SaveEnvironmentInfo();

            // ... warns Entifaz to ask the new CsDriver to reach the first field in its flow
            pOldCsDriver->SetEnterFlowStarted( true );
        }

        // new current: this FlowUnit
        m_iCurSlot = pFlowUnit->GetSlot();

        CsDriver*   pNewCsDriver = pFlowUnit->GetCsDriver();

        // ... initialize CsDriver' reentrant conditions
        pNewCsDriver->InitSessionConditions();

        // RHF INIC Feb 13, 2003
        if( iActiveNum > 1 ) {
            pNewCsDriver->CopyPendingAdvance(pOldCsDriver);
        }
        // RHF END Feb 13, 2003
    }

    return bEnabled;
}

bool CFlAdmin::DisableFlow( int iSymFlow, bool bExitThroughTail ) {
    bool        bDisabled     = false;
    FlowUnit*   pOldFlowUnit  = GetFlowUnit( iSymFlow );
    CsDriver*   pOldCsDriver  = GetCsDriver();
    int         iOldActiveNum = ( pOldFlowUnit != NULL ) ? pOldFlowUnit->GetActiveNum() : -1;
    FlowUnit*   pNewFlowUnit  = NULL;
    CsDriver*   pNewCsDriver  = NULL;

    // can deactivate only if active
    if( iOldActiveNum > 0 ) {
        bDisabled  = true;

        // deletes all elements occupied by the old CsDriver at the enter stack of CEntryDriver
        pOldCsDriver->ClearEnterFlowLogicStack();

        // warns Entifaz the entered-flow was ended going either forward or backward
        int     iReturnWay = ( bExitThroughTail ) ? 1 : -1;

        pOldCsDriver->SetEnterFlowReturnWay( iReturnWay );

        // new current: the FlowUnit whose activation order precedes the deactivated slot
        pNewFlowUnit  = GetPrevFlowUnit();

        // set the activation order of the old unit to zero (deactivated)
        pOldFlowUnit->SetActiveNum( 0 );

        // setting up the new FlowUnit if any
        m_iCurSlot = -1;

        if( pNewFlowUnit != NULL ) {
            // updates the index to the active FlowUnit
            m_iCurSlot   = pNewFlowUnit->GetSlot();
            pNewCsDriver = GetCsDriver();

            // restore the environement of the reactivated CsDriver
            pNewCsDriver->RestoreEnvironmentInfo();

            m_pEntryDriver->SetEnterMode( NULL ); // obsolete condition???
        }
        ASSERT( pNewCsDriver != NULL );
    }

    return bDisabled;
}

CFlAdmin::FlowUnit* CFlAdmin::GetPrevFlowUnit() {
    FlowUnit*   pNewFlowUnit  = NULL;
    FlowUnit*   pOldFlowUnit  = GetActiveFlowUnit();
    int         iOldActiveNum = ( pOldFlowUnit != NULL ) ? pOldFlowUnit->GetActiveNum() : -1;

    if( iOldActiveNum > 1 ) {
        for( int iSlot = 0; pNewFlowUnit == NULL && iSlot < (int)m_aFlowUnit.size(); iSlot++ ) {
            FlowUnit*   pFlowUnit = GetFlowUnitAt( iSlot );

            if( pFlowUnit->GetActiveNum() == iOldActiveNum - 1 )
                pNewFlowUnit = pFlowUnit;
        }
    }

    return pNewFlowUnit;
}

bool CFlAdmin::IsActiveFlow( int iSymFlow ) {
    FlowUnit*   pFlowUnit = GetFlowUnit( iSymFlow );
    bool        bIsActive = ( pFlowUnit != NULL ) ? pFlowUnit->IsActive() : false;

    return bIsActive;
}

bool CFlAdmin::IsActiveFlow( FLOW* pFlow ) {
    int     iSymFlow = ( pFlow != NULL ) ? pFlow->GetSymbolIndex() : 0;

    return IsActiveFlow( iSymFlow );
}

FLOW* CFlAdmin::GetActiveFlow() {
    FlowUnit*   pFlowUnit   = GetActiveFlowUnit();
    FLOW*       pActiveFlow = ( pFlowUnit != NULL ) ? pFlowUnit->GetFlow() : NULL;

    return pActiveFlow;
}

CsDriver* CFlAdmin::GetCsDriver() {
    FlowUnit*   pFlowUnit = GetActiveFlowUnit();
    CsDriver*   pCsDriver = ( pFlowUnit != NULL ) ? pFlowUnit->GetCsDriver() : NULL;

    return pCsDriver;
}

CsDriver* CFlAdmin::GetPrimaryCsDriver() {        // victor Dec 10, 01
    FlowUnit*   pFlowUnit = GetFlowUnitAt( 0 );
    CsDriver*   pCsDriver = ( pFlowUnit != NULL ) ? pFlowUnit->GetCsDriver() : NULL;

    return pCsDriver;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- supporting special conditions
//
/////////////////////////////////////////////////////////////////////////////

void CFlAdmin::ReactivatePrimary() {
    while( 1 ) {
        FlowUnit*   pOldFlowUnit  = GetActiveFlowUnit();
        CsDriver*   pOldCsDriver  = GetCsDriver();
        int         iOldActiveNum = ( pOldFlowUnit != NULL ) ? pOldFlowUnit->GetActiveNum() : -1;
        ASSERT( iOldActiveNum > 0 );

        if( iOldActiveNum == 1 )        // ... at Primary flow
            break;

        // deletes all elements occupied by the old CsDriver at the enter stack of CEntryDriver
        pOldCsDriver->ClearEnterFlowLogicStack();

        // new current: the FlowUnit whose activation order precedes the deactivated slot
        FlowUnit*   pNewFlowUnit  = GetPrevFlowUnit();

        // set the activation order of the old unit to zero (deactivated)
        pOldFlowUnit->SetActiveNum( 0 );

        // setting up the new FlowUnit if any
        m_iCurSlot = -1;

        if( pNewFlowUnit != NULL ) {
            CEntryDriver*   pEntryDriver = m_pEntryDriver;

            // updates the index to the active FlowUnit
            m_iCurSlot = pNewFlowUnit->GetSlot();

            CsDriver*   pNewCsDriver = GetCsDriver();

            // restore the environement of the reactivated CsDriver
            pNewCsDriver->RestoreEnvironmentInfo();

            // takes care of the enter stack
            pEntryDriver->SetEnterMode( NULL ); // obsolete condition???

            // deletes all elements occupied by the new CsDriver at the enter stack of CEntryDriver
            pNewCsDriver->ClearEnterFlowLogicStack();
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
//
// --- engine links
//
/////////////////////////////////////////////////////////////////////////////

void CFlAdmin::SetEngineDriver( CEngineDriver* pEngineDriver, CEntryDriver* pEntryDriver ) {
    m_pEngineDriver = pEngineDriver;
    m_pEngineArea   = pEngineDriver->m_pEngineArea;
    m_pIntDriver    = pEngineDriver->m_pIntDriver.get();
    m_pEntryDriver  = pEntryDriver;
}

