//---------------------------------------------------------------------------
//
//  wExbatch: Batch Processor main program
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include <engine/Engine.h>

//----------------------------------------------------------------------------
//    Batch processor null functions & symbols
//----------------------------------------------------------------------------
// compiling forbidden objects
#include <engine/COMPILAD.H>
#include <engine/ProgramControl.h>
#include <Zissalib/CsDriver.h>

void CEntryDriver::BuildQuestMgr() { ASSERT(0); }
void CEntryDriver::dedriver_start(void) { ASSERT(0); }
void CEntryDriver::LevCtStartLevel(int) { ASSERT(0); }
void CEntryDriver::LevCtInitLevel(int) { ASSERT(0); }
void CEntryDriver::LevCtAddWrittenSon(int) { ASSERT(0); }
bool CEntryDriver::CheckIdCollision(int) { ASSERT(0); return false; }
bool CEntryDriver::IsPartial(void) { ASSERT(0); return false; }
bool CEntryDriver::InEnterMode( void )    { return false; }
void CEntryDriver::SetEnterMode( class CSymbolFlow* ) {}
void CEntryDriver::ResetEnterMode() {}
void CEntryDriver::LevCtSetInfo( int, eNodeInfo ) {}
void CEntryDriver::DoQid( void )           { ASSERT(0); }

bool CEntryDriver::QidReady( int )         { ASSERT(0); return 0; }

void CEntryDriver::TakeCareOfEndingNode( int, bool )                 { ASSERT(0); }
bool CEntryDriver::CheckBeforeWriteEndingNode( int, int, int, bool ) { ASSERT(0); return false; }
bool CEntryDriver::WriteEndingNode( int, bool )                      { ASSERT(0); return false; }
int  CEntryDriver::GetTotalNumberNodes() { ASSERT(0); return 0; }
bool CEntryDriver::ReportToInterface( int iSymbol, int iOcc, int iDirection, CFlowAtom::AtomType xAtomType ) { ASSERT(0); return false; }

CsDriver* CEntryDriver::GetCsDriver()     { ASSERT(false); return nullptr; }
void CEntryDriver::SetCsDriver(CsDriver*) { }

int  CEntryDriver::dedemode() { ASSERT(0); return 0; }
void CEntryDriver::DeInitLevel(int) { ASSERT(0); }

CsDriver::CsDriver(void) { ASSERT(0); }
CsDriver::~CsDriver(void) {}

const Logic::SymbolTable& CsDriver::GetSymbolTable() const { throw ProgrammingErrorException(); }

int  CsDriver::FindActualOccs(const C3DObject &) { ASSERT(0);return 0; }

void CsDriver::SetEngineDriver(class CEngineDriver *,class CEntryDriver *) { ASSERT(0); }
void CsDriver::SetFlow(class CSymbolFlow *) { ASSERT(0); }
void CsDriver::SetFlAdmin(class CFlAdmin *) { ASSERT(0); }

void CsDriver::InitSessionConditions(void) { ASSERT(0); }
void CsDriver::SaveEnvironmentInfo(void) { ASSERT(0); }
void CsDriver::RestoreEnvironmentInfo(void) { ASSERT(0); }

bool CsDriver::SetInterRequestNature(CsDriver::RequestNature,bool) { ASSERT(0);return false; }
void CsDriver::ResetRequest() { ASSERT(0); }
void CsDriver::ResetRequests(CsDriver::RequestNature, CsDriver::RequestNature) { ASSERT(0); }
bool CsDriver::IsValidSkipTarget(bool,C3DObject*) { ASSERT(0);return false; }
bool CsDriver::Set3DTarget(C3DObject*) { ASSERT(0);return false; }
bool CsDriver::SetLogicRequestNature(CsDriver::RequestNature,bool) { ASSERT(0);return false; }
bool CsDriver::IsValidAdvanceTarget(bool,C3DObject*) { ASSERT(0);return false; }
bool CsDriver::IsValidReenterTarget(bool,bool,C3DObject*) { ASSERT(0);return false; }
void CsDriver::CopyPendingAdvance(CsDriver* pCsDriver ) { ASSERT(0); }
//savy modidied signature in zissalib to optimize skip to target computation
int CsDriver::SearchTargetLocation( C3DObject* p3DTarget, int iRefAtom, int iSearchWay, bool bFillTargetAtomIndex /*= false*/) { ASSERT(0); return 0; }

void CsDriver::SetEnterFlowLogicStack(const LogicStackSaver&) { ASSERT(false); }
void CsDriver::ClearEnterFlowLogicStack() { ASSERT(false); }
bool CsDriver::RunEnterFlowLogicStack() { ASSERT(false); return false; }

DEFLD3* CsDriver::GetCurDeFld( void ) { ASSERT(0); return 0; }

double CIntDriver::exenter( int iExpr ) {
    ASSERT( Issamod != ModuleType::Entry );

    if( m_bExecSpecFunc )
        issaerror( MessageType::Error, 9100 );

    return 0;
}
