#include <engine/StandardSystemIncludes.h>
#include <engine/INTERPRE.H>
#include <engine/EXENTRY.H>
#include <engine/FrequencyDriver.h>
#include <engine/ImputationDriver.h>
#include <engine/SelcaseManager.h>
#include <zEngineO/LoopStack.h>
#include <zEngineF/TraceHandler.h>
#include <Zissalib/CFlAdmin.h>
#include <zJson/JsonNode.h>
#include <zHtml/VirtualFileMapping.h>
#include <zAction/Caller.h>
#include <zReportO/Pre77ReportManager.h>


CIntDriver::CIntDriver(CEngineDriver& engine_driver)
    :   m_logicByteCode(engine_driver.m_pEngineArea->GetLogicByteCode()),
        m_symbolTable(engine_driver.m_pEngineArea->GetSymbolTable())
{
    ASSERT(0);
}

CIntDriver::~CIntDriver() { ASSERT(0); }

double* CIntDriver::svaraddr( VARX* pVarX ) const { ASSERT(0); return NULL; }
csprochar*   CIntDriver::GetSingVarAsciiAddr( VARX* pVarX ) const {
     ASSERT(0); return NULL;
}

double* CIntDriver::mvaraddr( VARX* pVarX, double dOccur ) const { ASSERT(0); return NULL; }
bool    VARX::InRange(const CNDIndexes* pTheIndex) const { ASSERT(0); return false; }

TCHAR*   CIntDriver::GetVarAsciiAddr( VART* pVarT, const CNDIndexes& theIndex ) const { ASSERT(0); return NULL; }
TCHAR*   CIntDriver::GetVarAsciiAddr( VARX* pVarX, const CNDIndexes& theIndex ) const { ASSERT(0); return NULL; }

TCHAR*   CIntDriver::GetVarAsciiAddr( VARX* pVarX ) const { ASSERT(0); return NULL; };  // rcl, Jun 17, 04
TCHAR*   CIntDriver::GetVarAsciiAddr( VART* pVarT ) const { ASSERT(0); return NULL; };  // rcl, Jun 17, 04

double* CIntDriver::GetVarFloatAddr( VART* pVarT, const CNDIndexes& theIndex ) const { ASSERT(0); return NULL; }
double* CIntDriver::GetVarFloatAddr( VARX* pVarX, const CNDIndexes& theIndex ) const { ASSERT(0); return NULL; }

double* CIntDriver::GetVarFloatAddr( VART* pVarT ) const { ASSERT(0); return NULL; } // rcl, Jun 25, 04
double* CIntDriver::GetVarFloatAddr( VARX* pVarX ) const { ASSERT(0); return NULL; } // rcl, Jun 25, 04

int     CIntDriver::GetFieldColor( int iSymVar, int iOccur )  const { ASSERT(0); return 0; }
int     CIntDriver::GetFieldColor( int iSymVar, const CNDIndexes& theIndex ) const { ASSERT(0); return 0; }
int     CIntDriver::GetFieldColor( VART* pVarT, const CNDIndexes& theIndex ) const { ASSERT(0); return 0; }
int     CIntDriver::GetFieldColor( VARX* pVarX, const CNDIndexes& theIndex ) const { ASSERT(0); return 0; }

bool    CIntDriver::SetVarFloatValue( double dValue, VARX* pVarX, const CNDIndexes& theIndex ) { ASSERT(0); return false; }
bool    CIntDriver::SetVarFloatValueSingle( double dValue, VARX* pVarX ) { ASSERT(0); return false; } // rcl, Jun 21, 04

double  CIntDriver::GetVarFloatValue( VART* pVarT, const CNDIndexes& theIndex ) const { ASSERT(0); return 0; }
double  CIntDriver::GetVarFloatValue( VARX* pVarX, const CNDIndexes& theIndex ) const { ASSERT(0); return 0; }

void    SECX::InitSecOccArray( double dInitValue, TCHAR cInitLight, int iMaxOccs ) { ASSERT(0); }
TCHAR*   SECX::GetAsciiAreaAtOccur( int iOccur ) { ASSERT(0); return NULL; }
bool    VARX::PassTheOnlyIndex( CNDIndexes& theIndex, int iOccur ) { ASSERT(0); return false; }

int     CIntDriver::GetFlagColor( TCHAR* pFlag ) const { ASSERT(0); return 0; }

void    CEngineArea::SecxEnd() { ASSERT(0); }
void    CEngineArea::DicxEnd() { ASSERT(0); }

CString CIntDriver::ProcName() { return CString(); }

void    GROUPT::OccTreeFree() {}

TCHAR*  CIntDriver::GetVarAsciiValue(int,int,bool) const { return NULL; }        // RHF Apr 17, 2001
double CIntDriver::svarvalue(struct VARX *) const { return 0; }                 // RHF Apr 17, 2001
double CIntDriver::mvarvalue(struct VARX *,double) const { return 0; }           // RHF Apr 17, 2001
double CIntDriver::GetSingVarFloatValue(struct VARX *) const { return 0; }       // RHF Apr 17, 2001
double CIntDriver::GetMultVarFloatValue(struct VARX *,const CNDIndexes&) const { return 0; } // RHF Apr 17, 2001

bool CIntDriver::ExecuteOnSystemMessage(MessageType, int, const std::wstring&) { return true; }

CIterator::CIterator(void){}
CIterator::~CIterator(void){}

bool CFlAdmin::EnableFlow(int){ return false; }
void CFlAdmin::CreateFlowUnit(class CSymbolFlow *){}

void CIntDriver::CtPos( CTAB* pCtab, int iCtNode, int *vector, CSubTable* pSubTable,
                       CCoordValue* pCoordValue, bool bMarkAllPos ) {}

double CIntDriver::val_coord( int i_node, double i_coord ){ return (double) -1; }
double CIntDriver::val_high( void ) { return (double) 0; }    // BMD 13 Oct 2005

bool CIntDriver::IsDataAccessible(const Symbol& /*symbol*/, bool /*issue_error_if_inaccessible*/) { return ReturnProgrammingError(false); }

std::shared_ptr<InterpreterAccessor> CEngineDriver::CreateInterpreterAccessor() { return ReturnProgrammingError(nullptr); }
bool CEngineDriver::IsBlankField( int , int  ) { ASSERT(0); return false;}
bool CEngineDriver::IsBlankField( VART*, int ) { ASSERT(0); return false;}
bool CEngineDriver::IsBlankField( VART*, CNDIndexes& ) { ASSERT(0); return false;}
void CEngineDriver::LoadCompiledBinary() { ASSERT(0); }
void CEngineDriver::CloseListerAndWriteFiles() { }
void CEngineDriver::PrepareCaseFromEngineForQuestionnaireViewer(DICT*, Case&) { }

void CExport::ExportDescriptions( void ) {}
void CExport::ExportClose( void ) {}

void CExport::MakeCommonRecord( CDictRecord* pDictCommonRecord, int iFromLevel, int iToLevel ) { ASSERT(0); }
CString CExport::GetDcfExpoName() const { ASSERT(0); return CString(); }

//////////////////////////////////////////////////////////////////////////
// CBasicIterator methods
// rcl, Aug 2005
#define NOTHING { ASSERT(0); }

void CBasicIterator::getIndexes( int iWhichOne, int aIndex[DIM_MAXDIM] ) throw(CIteratorException) NOTHING
void CBasicIterator::getIndexes( int iWhichOne, double dOccur[DIM_MAXDIM] ) throw(CIteratorException) NOTHING
void CBasicIterator::Add3DIndex( const threeDim& the3Ddim ) NOTHING

//////////////////////////////////////////////////////////////////////////
