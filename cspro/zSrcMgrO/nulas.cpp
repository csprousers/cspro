#include "StdAfx.h"
#include "Wcompile.h"
#include <engine/StandardSystemIncludes.h>
#include <engine/Interpreter.h>
#include <engine/Export.h>
#include <engine/FrequencyDriver.h>
#include <engine/ImputationDriver.h>
#include <engine/InterpreterAccessor.h>
#include <engine/ParadataDriver.h>
#include <engine/SelcaseManager.h>
#include <zEngineO/LoopStack.h>
#include <zEngineO/UserFunctionArgumentEvaluator.h>
#include <zEngineF/WindowsApplicationInterface.h>
#include <zUtilF/KeyboardLoader.h>
#include <zIssaLib/CFlAdmin.h>
#include <zReportO/Pre77ReportManager.h>


CIntDriver::CIntDriver(CEngineDriver& engine_driver)
    :   LogicInterpreter(engine_driver.m_pEngineArea->GetSharedEngineData(),
                         std::make_shared<WindowsApplicationInterface>())
{
    ASSERT(0);
}

CIntDriver::~CIntDriver() { ASSERT(false); }

Engine::Value CIntDriver::evalexpr_INTERPRETER_DLL_TODO(Engine::Value (CIntDriver::*instruction)(int), int program_index) { return ReturnProgrammingError(0); }
double CIntDriver::evalexpr_INTERPRETER_DLL_TODO(double (CIntDriver::*instruction)(int), int program_index) { return ReturnProgrammingError(0); }
void CIntDriver::RegisterAndLogEvent_INTERPRETER_DLL_TODO(std::shared_ptr<Paradata::Event> event, const void* instance_object) { ASSERT(false); }
void CIntDriver::IssueMessageWorker(MessageType message_type, int message_number, ...) { ASSERT(false); }
std::string CIntDriver::GetFormattedMessageWorker(int message_number, ...) { return ReturnProgrammingError(""); }
bool CIntDriver::IsExecutionInterrupted() const { return ReturnProgrammingError(false); }
InterpreterExecuteResult CIntDriver::Report_Evaluate_INTERPRETER_DLL_TODO(Report& report) { throw ProgrammingErrorException(); }
SharableString CIntDriver::EvaluateTextFill(int program_index) { return ReturnProgrammingError(SharableString()); }
SharableString CIntDriver::EvaluateUserMessage(int message_node_index, FunctionCode function_code, int* out_message_number) { return ReturnProgrammingError(std::string()); }
Engine::Value CIntDriver::RunSoonToBeRemovedFeature(std::string_view feature_sv, int program_index, void* tag) { return ReturnProgrammingError(0); }
bool CIntDriver::HasSpecialFunction(SpecialFunction::Code special_function) { return ReturnProgrammingError(false); }
Engine::Value CIntDriver::ExecSpecialFunction(int symbol_index, SpecialFunction::Code special_function, std::vector<std::variant<double, SharableString>> arguments) { return ReturnProgrammingError(DEFAULT); }
Symbol* CIntDriver::GetFromSymbolOrEngineItemWorker_INTERPRETER_DLL_TODO(const SymbolReference<Symbol*>& symbol_reference, bool use_exceptions) { return ReturnProgrammingError(nullptr); }
std::shared_ptr<Symbol> CIntDriver::GetFromSymbolOrEngineItemWorker_INTERPRETER_DLL_TODO(const SymbolReference<std::shared_ptr<Symbol>>& symbol_reference, bool use_exceptions)  { return ReturnProgrammingError(nullptr); }
EvaluatedEngineItemSubscript CIntDriver::EvaluateEngineItemSubscript(const EngineItem& engine_item, const Nodes::ItemSubscript& item_subscript_node) { return ReturnProgrammingError(EvaluatedEngineItemSubscript()); }
int CIntDriver::SelectDlgHelper_pre77(int iFunCode, const CString& csHeading, const std::vector<std::vector<CString>*>* paData,
                                      const std::vector<CString>* paColumnTitles, std::vector<bool>* pbaSelections,
                                      const std::vector<PortableColor>* row_text_colors) { return ReturnProgrammingError(0); }
EngineParadataDriver& CIntDriver::GetEngineParadataDriver_INTERPRETER_DLL_TODO() { throw ProgrammingErrorException(); }
Engine::Value CIntDriver::ExecuteInstructions(int program_index) { return ReturnProgrammingError(0); }
void CIntDriver::ExecuteCallbackUserFunction(int field_symbol_index, UserFunctionArgumentEvaluator& argument_evaluator) { ASSERT(false); }
std::unique_ptr<UserFunctionArgumentEvaluator> CIntDriver::EvaluateArgumentsForCallbackUserFunction(int program_index, FunctionCode function_code) { return ReturnProgrammingError(nullptr); }
Engine::Value CIntDriver::ex_Freq_view(const NamedFrequency& named_frequency, const ViewerOptions* viewer_options, int frequency_parameters_node_index) { return ReturnProgrammingError(0); }
Engine::Value CIntDriver::exCase_view(const DICT& dictionary, const ViewerOptions* viewer_options) { return ReturnProgrammingError(0); }
Engine::Value CIntDriver::ExExecPFF_INTERPRETER_DLL_TODO(LogicPff& logic_pff) { throw ProgrammingErrorException(); }
FrequencyDriver* CIntDriver::GetFrequencyDriver_INTERPRETER_DLL_TODO() { throw ProgrammingErrorException(); }
void CIntDriver::AssignValueToVART_INTERPRETER_DLL_TODO(int variable_compilation, double value) { throw ProgrammingErrorException(); }
void CIntDriver::AssignValueToVART_INTERPRETER_DLL_TODO(int variable_compilation, SharableString value) { throw ProgrammingErrorException(); }
double CIntDriver::EvaluateVARTValue_double_INTERPRETER_DLL_TODO(int variable_compilation) { throw ProgrammingErrorException(); }
SharableString CIntDriver::EvaluateVARTValue_SharableString_INTERPRETER_DLL_TODO(int variable_compilation) { throw ProgrammingErrorException(); }
Engine::Value CIntDriver::ModifyVARTValue_INTERPRETER_DLL_TODO(int variable_compilation, const std::function<void(double&)>& modify_value_function, std::unique_ptr<Paradata::FieldInfo>* paradata_field_info/* = nullptr*/) { throw ProgrammingErrorException(); }
Engine::Value CIntDriver::ModifyVARTValue_INTERPRETER_DLL_TODO(int variable_compilation, const std::function<void(SharableString&)>& modify_value_function, std::unique_ptr<Paradata::FieldInfo>* paradata_field_info/* = nullptr*/) { throw ProgrammingErrorException(); }
int CIntDriver::SymbolTableSearch_INTERPRETER_DLL_TODO(std::string_view full_symbol_name_sv, SymbolType preferred_symbol_type,
                                                       const std::vector<SymbolType>* allowable_symbol_types) const { throw ProgrammingErrorException(); }
LoopStack& CIntDriver::GetLoopStack() { throw ProgrammingErrorException(); }


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

std::string CIntDriver::ProcName() { return std::string(); }

void    GROUPT::OccTreeFree() {}

TCHAR*  CIntDriver::GetVarAsciiValue(int,int,bool) const { return NULL; }        // RHF Apr 17, 2001
double CIntDriver::svarvalue(struct VARX *) const { return 0; }                 // RHF Apr 17, 2001
double CIntDriver::mvarvalue(struct VARX *,double) const { return 0; }           // RHF Apr 17, 2001
double CIntDriver::GetSingVarFloatValue(struct VARX *) const { return 0; }       // RHF Apr 17, 2001
double CIntDriver::GetMultVarFloatValue(struct VARX *,const CNDIndexes&) const { return 0; } // RHF Apr 17, 2001

bool CIntDriver::ExecuteOnSystemMessage(MessageType, int, const std::string&) { return true; }

CIterator::CIterator(void){}
CIterator::~CIterator(void){}

bool CFlAdmin::EnableFlow(int){ return false; }
void CFlAdmin::CreateFlowUnit(class CSymbolFlow *){}

void CIntDriver::CtPos( CTAB* pCtab, int iCtNode, int *vector, CSubTable* pSubTable,
                       CCoordValue* pCoordValue, bool bMarkAllPos ) {}

double CIntDriver::val_coord( int i_node, double i_coord ){ return (double) -1; }
double CIntDriver::val_high( void ) { return (double) 0; }    // BMD 13 Oct 2005

bool CIntDriver::IsDataAccessible(const Symbol& /*symbol*/, bool /*issue_error_if_inaccessible*/) { return ReturnProgrammingError(false); }

std::unique_ptr<InterpreterAccessor> CEngineDriver::CreateInterpreterAccessor() { return ReturnProgrammingError(nullptr); }
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
