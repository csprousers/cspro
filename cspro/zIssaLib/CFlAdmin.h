#pragma once

//---------------------------------------------------------------------------
//  File name: CFlAdmin.h
//
//  Description:
//          Header for the flow-administrator for the 3-D driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              25 Jul 01   vc      Created
//
//---------------------------------------------------------------------------

#include <Zissalib/CsDriver.h>
#include <Zissalib/CFlow.h>


//---------------------------------------------------------------------------
//
//  class CFlAdmin : public CObject
//
//
//  Description:
//      Management of 3-D drivers of an Entry application
//
//  * Basic model:
//
//      An Entry application should manage a Primary flow and several Secondary
//      flows, each one controlled by its own CsDriver.  At the loading time of the
//      application, this object stores all accessible flows and their drivers, and
//      let both Engine and Entifaz know everything of the whole set, including which
//      which is the flow currently operated - the "active" flow - at any moment.
//
//      Engine has only one instance of this administrator, and only in Entry runs.
//
//      At any given moment, there can be only one Flow in activity (the most recently
//      "entered" because it is the Primary flow, or because it has been directly "entered"
//      by an Enter-flow' command).
//
//
//  Methods:
//
//  Construction/destruction/initialization
//      CFlAdmin                    Constructor
//      ~CFlAdmin                   Destructor
//
//  FlowUnits management
//      CreateFlowUnit              Create a FlowUnit with a new CsDriver for a given FLOW (and its attached flow-strip)
//      EnableFlow                  Activates a given Flow
//      DisableFlow                 Set a given Flow as inactive (it should be activated)
//      GetPrevFlowUnit             Locates the FlowUnit whose activation number precedes the currently active FlowUnit
//      IsActiveFlow                True if the given Flow is currently in activity
//      GetActiveFlow               Get the Flow of the currently active FlowUnit
//      GetCsDriver                 Get the CsDriver of the currently active FlowUnit
//
//  Supporting special conditions
//      ReactivatePrimary           To reactivate the primary Flow, no matter which is the currently active FlowUnit
//
//  Link to engine
//      SetEngineDriver             Installs engine environment
//
//---------------------------------------------------------------------------

class CFlAdmin : public CObject {

// --- Internal objects ----------------------------------------------------
private:
class FlowUnit {
private:
    int             m_iSlot;            // the index in the owner array
    FLOW*           m_pFlow;            // the FLOW
    CsDriver*       m_pCsDriver;        // the CsDriver
    int             m_iActiveNum;       // order of activation (Primary flow is always # 1, those currently disabled have # 0)

public:
    FlowUnit( int iSlot, FLOW* pFlow ) {
                        m_iSlot       = iSlot;
                        m_pFlow       = pFlow;
                        m_pCsDriver   = new CsDriver();
                        m_iActiveNum  = 0;
    }

    ~FlowUnit() {
        delete m_pCsDriver;
    }

    int             GetSlot() const                { return m_iSlot; }
    FLOW*           GetFlow()                      { return m_pFlow; }
    CsDriver*       GetCsDriver()                  { return m_pCsDriver; }
    bool            IsActive() const               { return( m_iActiveNum > 0 ); }
    int             GetActiveNum() const           { return m_iActiveNum; }
    void            SetActiveNum( int iActiveNum ) { m_iActiveNum = iActiveNum; }
};

// --- Data members --------------------------------------------------------

    // --- flows and CsDrivers
private:
    int                    m_iCurSlot;         // index of current FlowUnit
    std::vector<FlowUnit*> m_aFlowUnit;        // units

    // --- engine links
public:
    CEngineDriver*  m_pEngineDriver;
    CEngineArea*    m_pEngineArea;
    CIntDriver*     m_pIntDriver;
    CEntryDriver*   m_pEntryDriver;


// --- Methods -------------------------------------------------------------
    // --- construction/destruction
public:
    CFlAdmin();
    ~CFlAdmin();

    // --- FlowUnits management
public:
    void            CreateFlowUnit( FLOW* pFlow );
    bool            EnableFlow( int iSymFlow );
    bool            DisableFlow( int iSymFlow, bool bExitThroughTail = true );
    FlowUnit*       GetPrevFlowUnit();            // victor Aug 02, 01
    bool            IsActiveFlow( int iSymFlow );
    bool            IsActiveFlow( FLOW* pFlow );
    FLOW*           GetActiveFlow();
    CsDriver*       GetCsDriver();
    CsDriver*       GetPrimaryCsDriver();         // victor Dec 10, 01
private:
    FlowUnit*       GetFlowUnitAt( int iSlot ) {
                        ASSERT( iSlot >= 0 && iSlot < (int)m_aFlowUnit.size() );
                        return m_aFlowUnit[iSlot];
    }
    FlowUnit*       GetFlowUnit( int iSymFlow ) {
                        FlowUnit*   pFlowUnit = NULL;

                        if( iSymFlow > 0 ) {
                            for( int iSlot = 0; pFlowUnit == NULL && iSlot < (int)m_aFlowUnit.size(); iSlot++ ) {
                                FlowUnit*   pFUnit  = GetFlowUnitAt( iSlot );
                                FLOW*       pFlow   = pFUnit->GetFlow();
                                int         iSymbol = pFlow->GetSymbolIndex();

                                if( iSymbol == iSymFlow )
                                    pFlowUnit = pFUnit;
                            }
                        }

                        return pFlowUnit;
    }
    FlowUnit*       GetActiveFlowUnit() {
                        return( ( m_iCurSlot >= 0 ) ? GetFlowUnitAt( m_iCurSlot ) : NULL );
    }

    // --- supporting special conditions
public:
    void            ReactivatePrimary();

    // --- engine links
public:
    void            SetEngineDriver( CEngineDriver* pEngineDriver, CEntryDriver* pEntryDriver );
};
