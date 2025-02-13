#pragma once

//---------------------------------------------------------------------------
//  File name: IntDrv.h
//
//  Description:
//          Header for interpreter-driver class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   RHF     Basic conversion
//              12 May 00   vc      Basic customization
//              06 Jun 00   RHF     Adding alpha-arrays
//              20 Jun 00   vc      Fine tunning on export methods
//              10 Jul 00   vc      Adding hard-access for new approach
//              03 Aug 00   RHF     Fix problem with endsect command
//              08 Mar 01   vc      Adding support for executing selected ENTRY commands in BATCH
//              26 Mar 01   vc      Full remake to deal with @target for skip-to/skip-to-next commands
//              04 Apr 01   vc      Tailoring for RepMgr compatibility
//              16 May 01   vc      Expanding for 3D driver
//              01 Apr 02   vc+RHC  Adding support for ImbeddedUnit's in declared tables
//              25 Jun 04   rcl     index array -> index class
//              25 Jun 04   rcl     1st const correctness effort
//
//---------------------------------------------------------------------------

#include <engine/NODES.H>
#include <engine/Export.h>
#include <engine/ParameterManager.h>
#include <engine/DEFLD.H>
#include <engine/QuestionTextParamCache.h>
#include <zEngineO/SymbolReference.h>
#include <zUtilO/Viewers.h>
#include <ZTBDO/cttree.h>

class BinarySymbol;
class CapiCondition;
class CapiQuestion;
class CapiQuestionManager;
class CaseItemReference;
class CCoordValue;
class CIntDriver;
class CMsgOptions;
class CsDriver;
class CSettings;
class CSubTable;
class DictValue;
enum class FieldStatus : int;
class FrequencyDriver;
struct IBluetoothAdapter;
struct ICredentialStore;
class ImputationDriver;
struct InterpreterExecuteResult;
struct ISyncListener;
struct ISyncServerConnectionFactory;
class ItemIndex;
class JsonReaderInterface;
class LoopStack;
class NamedReference;
class ParadataDriver;
class SelcaseManager;
enum class SpecialFunction : int;
struct sqlite3;
class SyncClient;
class TraceHandler;
class VirtualFileMappingHandler;
class VTSTRUCT;
namespace ActionInvoker { class Caller; class Runtime; }
namespace Nodes { enum class EncodeType : int; struct SetAccessFirstLast; }
namespace Paradata { class Event; class ExternalApplicationEvent; class FieldInfo; }
namespace Pre77Report { class ReportManager; }


// TODO: review public/private for both data and methods


class CIntDriver
{
// --- Data members --------------------------------------------------------

    // --- procedure being executed
public:
    int     m_iProgType;                   // proc being executed: pre/post
    int     m_iExLevel;                    //                    : level
    int     m_iExSymbol;                   //                    : isym

    // --- execution flags
public:
    int     m_iStopExec;
    bool    m_bStopProc;
    int     m_iSkipStmt;
private:
    bool    m_bUse3D_Driver;                            // victor May 16, 01
    bool    m_bRequestIssuedByEngine;                   // victor May 16, 01
    bool    m_bAllowMultipleRelation; // RHF Jul 16, 2002

    //SAVY Added for mem-management
    int     m_iVTPool;
    int     m_iIntPool;

    int     m_FieldSymbol; // 20100708 so getsymbol() works with the userbar

    std::vector<std::shared_ptr<VTSTRUCT>> m_arrVtStructPool;
    std::vector<std::shared_ptr<std::vector<int>>> m_arrIntsPool;

    std::shared_ptr<VTSTRUCT> GetVTStructFromPool();
    void AddVTStructToPool(std::shared_ptr<VTSTRUCT> vtStruct);
    std::shared_ptr<std::vector<int>> GetIntArrFromPool();
    void AddIntArrToPool(std::shared_ptr<std::vector<int>> pIntArr);

    // --- counters for excount and related functions
public:
    int     m_iExOccur;                   // executing occurrence
    int     m_iExGroup;                   // executing group occurrence // RHF Aug 17, 2000
    int     m_iExDim;                     // dimension                  // RHF Aug 17, 2000
    int     m_iExFixedDimensions;   // number of explicit dimensions  // rcl, Jul 17, 2004
    CNDIndexes m_aFixedDimensions;  // the fixed dimensions           // rcl, Jul 17, 2004

// RHC INIC Sep 20, 2001
    // --- include runtime stack for For loops // RHC Jul 10, 2001
private:
#define FOR_STACK_MAX  100

    int     m_iForStackNext;
    struct FOR_STACK
    {
        int forVarIdx;
        int forGrpIdx;
        int forRelIdx;
        int forType;
    };
    FOR_STACK ForStack[FOR_STACK_MAX];
// RHC END Sep 20, 2001

private:
    std::vector<std::unique_ptr<std::tuple<CIntDriver&, UserFunction&>>> m_sqlCallbackFunctions;

    // --- functions' array
private:
    using pDoubleFunction = double (CIntDriver::*)(int);
    static pDoubleFunction m_pExFuncs[];

    // --- engine links
public:
    CEngineDriver* m_pEngineDriver;
    CEngineArea* m_pEngineArea;
    EngineData* m_engineData;
    const LogicByteCode& m_logicByteCode;
    Logic::SymbolTable& m_symbolTable;
    CEngineDefines* m_pEngineDefines;
    CSettings* m_pEngineSettings;
    bool m_usingLogicSettingsV0;

    std::shared_ptr<ActionInvoker::Runtime> m_actionInvokerRuntime;
    std::unique_ptr<ActionInvoker::Caller> m_actionInvokerCaller;
    std::unique_ptr<LoopStack> m_loopStack;
    std::unique_ptr<TraceHandler> m_traceHandler;
    ParadataDriver* m_pParadataDriver;
    std::unique_ptr<Pre77Report::ReportManager> m_pre77reportManager;

private:
    CsDriver*           m_pCsDriver;                    // victor May 16, 01
    bool                m_bUseVector; // RHF Jul 06, 2001

    std::vector<std::wstring> m_workingStrings; // temporary strings created by logic functions


// --- Methods -------------------------------------------------------------
public:
    // --- constructor/destructor/initialization
    CIntDriver(CEngineDriver& engine_driver);
    ~CIntDriver();

    void StartApplication();
    void StopApplication();

    Logic::SymbolTable& GetSymbolTable() const { return m_symbolTable; }

    LoopStack& GetLoopStack();

    // --- execution flags
    void    Enable3D_Driver()                  { m_bUse3D_Driver = true; }          // victor May 16, 01
    void    Disable3D_Driver()                 { m_bUse3D_Driver = false; }         // victor May 16, 01
    bool    IsUsing3D_Driver() const           { return m_bUse3D_Driver; }          // victor May 16, 01
    void    SetRequestIssued( bool bX = true ) { m_bRequestIssuedByEngine = bX; }   // victor May 16, 01
    bool    GetRequestIssued() const           { return m_bRequestIssuedByEngine; } // victor May 16, 01

    template<typename T>
    void MakeFullPathFileName(T& filename) const;
    void MakeFullPathFileName(ConnectionString& connection_string) const;
    std::wstring EvalFullPathFileName(int iExpr);

    // --- pre-allocating memory for execution
public:
    void    AllocExecBase();                      // formerly 'exalloc0'
    void    AllocExecTables();                    // formerly 'exallocdict'
    void    BuildRecordsMap();                    // formerly 'levassign'

    // ---  accessing & retrieving values and light-flags
public:
    double  svarvalue( VARX* pVarX ) const;
    double* svaraddr( VARX* pVarX ) const;
// RHF COM Aug, 04, 2000    double* svaraddr( int nva );

    double  mvarvalue( VARX* pVarX, double dOccur ) const;
    double* mvaraddr( VARX* pVarX, double dOccur ) const;

// RHF COM Aug 04, 2000    double* mvaraddr( int nva, double dOccur );
    TCHAR* GetVarAsciiValue( int iSymVar, int iOccur, bool bVisualValue = false ) const; // formerly 'ascii_value'
    TCHAR* GetVarAsciiValue( double dValue, TCHAR* pAsciiVal = nullptr ) const;

    // RHC INIC Aug 17, 2000
    double* mvaraddr( VARX* pVarX, double* dOccur ) const;
    double  mvarvalue( VARX* pVarX, double* dOccur ) const;
    // RHC END Aug 17, 2000

    void    GetCurrentVarSubIndexes( int iSymVar, CNDIndexes& dIndex );
    void    GetCurrentGroupSubIndexes( int iSymGroup, CNDIndexes& dIndex );

    void    mvarGetSubindexes( const MVAR_NODE *ptrvar, double* dIndex, int *indexType = NULL );   // RHC Sep 10, Add indexType

private:

    bool    hasSubitemInGroup(VART* pItemVarT, GROUPT* pGroup);
    bool    isParentItemInGroup(VART* pSubitemVarT, GROUPT* pGroup);

    double  GetVarValue( int iSymVar, int iOccur, bool bVisualValue ) const; // formerly 'getvarvalue'

    void    grpGetSubindexes( GRP_NODE* pgrp, double* dIndex ); // RHC Aug 18, 2000
    void    grpGetSubindexes( GROUPT* pGrpT, GRP_NODE* pgrp, double* dIndex );

    // --- field' flags management: get/set "color" and "valid" marks
public:
    // ... old, mono-index version
    int     GetFieldColor( int iSymVar, int iOccur ) const;
    void    SetFieldColor( int iSymVar, int iOccur, csprochar cColor );

    // ... new, index version: 'theIndex' must be 0-based   // victor Jul 22, 00
    int     GetFieldColor( int iSymVar, const CNDIndexes& theIndex ) const;
    int     GetFieldColor( VARX* pVarX, const CNDIndexes& theIndex ) const; // rcl Jun 25, 04
    int     GetFieldColor( VART* pVarT, const CNDIndexes& theIndex ) const; // rcl Jun 25, 04

    int     GetFieldColor( VARX* pVarX ) const; // rcl Jun 27,, 04
    int     GetFieldColor( int iSymVar ) const; // rcl Jun 27,, 04

    void    SetFieldColor( csprochar cColor, int iSymVar, const CNDIndexes& theIndex );
    void    SetFieldColor( csprochar cColor, VART* pVarT, const CNDIndexes& theIndex );
    void    SetFieldColor( csprochar cColor, VARX* pVarX, const CNDIndexes& theIndex );

    // --- flags' deep-management: get/set "color"
    int     GetFlagColor( csprochar* pFlag ) const;
    void    SetFlagColor( csprochar* pFlag, csprochar  cColor );

    FieldStatus GetFieldStatus(const CaseItem& case_item, const CaseItemIndex& index);

    // ---  hard-access to Flags/Ascii/Float            // victor Jul 10, 00
    // ... generic: for both Sing or Mult
    bool    SetVarAsciiValue( csprochar* pAscii, VARX* pVarX, int* aIndex = NULL );

    bool    SetVarFloatValue( double dValue, VARX* pVarX, const CNDIndexes& theIndex ); // rcl, Jun 21, 04
    bool    SetVarFloatValueSingle( double dValue, VARX* pVarX ); // rcl, Jun 21, 04

    double* GetVarFloatAddr( VART* pVarT ) const; // rcl, Jun 25, 04
    double* GetVarFloatAddr( VARX* pVarX ) const; // rcl, Jun 25, 04
    double* GetVarFloatAddr( VART* pVarT, const CNDIndexes& theIndex ) const; // rcl, Jun 25, 04
    double* GetVarFloatAddr( VARX* pVarX, const CNDIndexes& theIndex ) const; // rcl, Jun 25, 04

    // -- GetVarAsciiAddr
    csprochar*   GetVarAsciiAddr( VART* pVarT, const CNDIndexes& theIndex ) const;  // rcl, Jun 17, 04
    csprochar*   GetVarAsciiAddr( VARX* pVarX, const CNDIndexes& theIndex ) const;  // rcl, Jun 17, 04

    csprochar*   GetVarAsciiAddr( VARX* pVarX ) const;  // rcl, Jun 17, 04
    csprochar*   GetVarAsciiAddr( VART* pVarT ) const;  // rcl, Jun 17, 04

    // -- GetVarFloatValue
    double  GetVarFloatValue( VARX* pVarX, const CNDIndexes& theIndex ) const; // rcl, Jun 25, 04
    double  GetVarFloatValue( VART* pVarT, const CNDIndexes& theIndex ) const; // rcl, Jun 25, 04
    double  GetVarFloatValue( VARX* pVarX ) const; // rcl, Jun 25, 04
    double  GetVarFloatValue( VART* pVarT ) const; // rcl, Jun 25, 04

    // -- GetVarFlagsAddr
    csprochar*   GetVarFlagsAddr( VART* pVarT, const CNDIndexes& theIndex ) const;  // rcl, Jun 17, 04
    csprochar*   GetVarFlagsAddr( VARX* pVarX, const CNDIndexes& theIndex ) const;  // rcl, Jun 17, 04

    csprochar*   GetVarFlagsAddr( VART* pVarT ) const;  // rcl, Jun 27, 04
    csprochar*   GetVarFlagsAddr( VARX* pVarX ) const;  // rcl, Jun 27, 04

    // ... specific: for either Sing or Mult
    double  GetSingVarFloatValue( VART* pVarT ) const;
    double  GetSingVarFloatValue( VARX* pVarX ) const;

    double* GetSingVarFloatAddr( VART* pVarT ) const;
    double* GetSingVarFloatAddr( VARX* pVarX ) const;

    double  GetMultVarFloatValue( VARX* pVarX ) const; // rcl, Jun 24 2004
    double  GetMultVarFloatValue( VART* pVarT ) const; // rcl, Jun 24 2004
    double  GetMultVarFloatValue( VART* pVarT, const CNDIndexes& theIndex ) const; // rcl, Jun 23, 04
    double  GetMultVarFloatValue( VARX* pVarX, const CNDIndexes& theIndex ) const; // rcl, Jun 23, 04

    double* GetMultVarFloatAddr( VART* pVarT, const CNDIndexes& theIndex ) const; // rcl, Jun 24,, 04
    double* GetMultVarFloatAddr( VARX* pVarX, const CNDIndexes& theIndex ) const; // rcl, Jun 24,, 04

    csprochar*   GetSingVarAsciiAddr( VART* pVarT ) const;
    csprochar*   GetSingVarAsciiAddr( VARX* pVarX ) const;
    csprochar*   GetSingVarFlagsAddr( VART* pVarT ) const;
    csprochar*   GetSingVarFlagsAddr( VARX* pVarX ) const;

    csprochar*   GetMultVarFlagsAddr( VART* pVarT, const CNDIndexes& theIndex ) const; // rcl, Jun 23,, 04
    csprochar*   GetMultVarFlagsAddr( VARX* pVarX, const CNDIndexes& theIndex ) const; // rcl, Jun 23,, 04

private:
    csprochar*   GetMultVarAsciiAddr( VART* pVarT, int* aIndex ) const;
    csprochar*   GetMultVarAsciiAddr( VARX* pVarX, int* aIndex ) const;
public:
    csprochar*   GetMultVarAsciiAddr( VARX* pVarX, const CNDIndexes& theIndex ) const; // rcl, Jun 22,, 04
    csprochar*   GetMultVarAsciiAddr( VART* pVarT, const CNDIndexes& theIndex ) const; // rcl, Jun 22,, 04

    bool    CheckIndexArray( const VART* pVarT, const CNDIndexes& theIndex ) const;

    // --- main interpreting methods
public:
    void    PrepareForExportExec(int iSymbol, int iProgType);

    template<typename T = double>
    T evalexpr(int iExpr)
    {
        const int* function_code_ptr = m_logicByteCode.GetCodeAtPosition(iExpr);
        return static_cast<T>((this->*m_pExFuncs[*function_code_ptr])(iExpr));
    }

    template<typename T = std::wstring>
    T CharacterObjectToString(double working_string_index);

    template<typename T = std::wstring>
    T EvalAlphaExpr(int program_index);

    // if using the original logic settings, "\\n" characters will be converted to "\n" (or "\r\n"), and optionally, "\\\\" characters will be converted to "\\"
    enum class V0_EscapeType { NewlinesToSlashN, NewlinesToSlashRN, NewlinesToSlashN_Backslashes, NewlinesToSlashRN_Backslashes };
private:
    void ConvertV0EscapesWorker(std::wstring& text, V0_EscapeType v0_escape_type);
    void ConvertV0EscapesWorker(std::vector<std::wstring>& text_lines, V0_EscapeType v0_escape_type);
    void ApplyV0EscapesWorker(std::wstring& text, V0_EscapeType v0_escape_type);
public:
    template<typename T, bool AlreadyCheckedIsV0 = false>
    T ConvertV0Escapes(T text, V0_EscapeType v0_escape_type = V0_EscapeType::NewlinesToSlashN);

    std::wstring EvaluateStringExpressionConvertingV0Escapes(int program_index, V0_EscapeType v0_escape_type = V0_EscapeType::NewlinesToSlashN);

    std::wstring ApplyV0Escapes(std::wstring text, V0_EscapeType v0_escape_type = V0_EscapeType::NewlinesToSlashN);

    // node helpers
public:
    template<typename NodeType>
    const NodeType& GetNode(int program_index) const
    {
        return *reinterpret_cast<const NodeType*>(m_logicByteCode.GetCodeAtPosition(program_index));
    }

    const Nodes::List& GetListNode(int program_index) const;
    const Nodes::List& GetOptionalListNode(int program_index) const;

    std::vector<int> GetListNodeContents(int program_index) const;

    // --- basic interpreter functions
public:
    double CallUserFunction(UserFunction& user_function, UserFunctionArgumentEvaluator& user_function_argument_evaluator);
    void ExecuteCallbackUserFunction(int field_symbol_index, UserFunctionArgumentEvaluator& user_function_argument_evaluator);
private:
    std::unique_ptr<UserFunctionArgumentEvaluator> EvaluateArgumentsForCallbackUserFunction(int program_index, FunctionCode function_code);

private:
    void    ResetSymbol(Symbol& symbol, int initialize_value = -1);
public:
    double  exsymbolreset(int iExpr);
    double  expersistentsymbolreset(int iExpr);

    double  exnumericconstant(int iExpr);
    double  exsvar(int iExpr);
    double  exmvar(int iExpr);
    double  exmvar( MVAR_NODE* ptrvar );                // rcl, Jul 22, 2004
    double  exavar(int iExpr);

    double  extavar(int iExpr);
    double  excpt(int iExpr);
    double  exstringcompute(int program_index);
    double  exworkvariable(int iExpr);
    double  exif(int iExpr);
    double  exbox(int iExpr);
    double  exstringliteral(int iExpr);
    double  exworkstring(int program_index);
    double  exworkstringcompute(int program_index);
    double  excharobj(int program_index);

    double  excpttbl(int iExpr);

    double  exnoopIgnore_numeric(int iExpr);
    double  exnoopIgnore_string(int iExpr);
    double  exnoopAbort(int iExpr);
    double  exnoopAbortPlaceholderForFutureFunction(int iExpr);

    double  extvar(int iExpr);

    double  exuserfunctioncall(int program_index);
    double  exinvoke(int program_index);
    template<typename T>
    InterpreterExecuteResult RunInvoke(const StringNoCase& function_name, const T& variable_arguments);

    double  exask(int iExpr);
    double  exskipto(int iExpr);        //{ENTRY only}
    double  exmoveto(int iExpr);        //{ENTRY only} // RHF Dec 09, 2003
    double  exadvance(int iExpr);       //{ENTRY only}
    double  exreenter(int iExpr);       //{ENTRY only}
    double  exnoinput(int iExpr);       //{ENTRY only}
    double  exendsect(int iExpr);       //{ENTRY only}
    double  exendlevl(int iExpr);       //{ENTRY only}
    double  exenter(int iExpr);         //{ENTRY only}

    // executing selected ENTRY commands in BATCH       // victor Mar 08, 01
private:
    // ... executing command' methods
    double  BatchExSkipTo(int iExpr);                 // victor Mar 08, 01
    double  BatchExAdvance(int iExpr);                // victor Mar 20, 01
    double  BatchExReenter(int iExpr);                // victor Mar 08, 01
    double  BatchExEndsect(int iExpr);                // victor Mar 08, 01
    double  BatchExEndLevel(int iExpr);               // victor Mar 20, 01
    // ... utility functions
private:
    void    BatchExSetSkipping( int iSymSource, int iOccSource, int iProgSource,              // victor Mar 26, 01
                                int iSymTarget, int iOccTarget, int iProgTarget );

    //////////////////////////////////////////////////////////////////////////
    // new 3D versions
    void    BatchExSetSkipping( C3DObject& objSource, int iProgSource,  // rcl, Sept 04, 04
                                C3DObject& objTarget, int iProgTarget );

    bool    BatchExScanOccur( std::vector<int>& aDirtySymbol, std::vector<int>& aDirtyOccur,    // victor Mar 14, 01
                              GROUPT* pGroupT, int iItemCheck, int iOccCheck, int iProgTarget );
    void    BatchExDisplayDirty( std::vector<int>& aDirtySymbol, std::vector<int>& aDirtyOccur, // victor Mar 14, 01
                                 int iHeadMessage );
    double  EntryExSkipToAt(int iExpr);                                                     // victor Mar 26, 01

    double  EntryExReenterToAt(int iExpr);
    double  EntryExAdvanceToAt(int iExpr);

    double  EntryExReenterAdvanceToAt( int iExpr, bool bAdvance ); // RHF Nov 24, 2003

    double  BatchExSkipToAt(int iExpr);                                                     // victor Mar 26, 01

    int GetReferredTargetSymbol( int iSymAt, bool bSkipToNext, bool bMove, int* iTargetOcc, bool* bExplicitOcc );                          // victor Mar 26, 01
    int GetReferredReenterAdvanceTargetSymbol( int iSymAt, bool bAdvance, bool bMove, int* iTargetOcc, bool* bExplicitOcc  ); // RHF Nov 24, 2003

    int GetReferredTargetSymbolChar(const CString& csTargetName, bool bSkipToNext, bool bMove,
                                    int *iTargetOcc, bool *bExplicitOcc, const Symbol* symbol_holding_name = nullptr);
    int GetReferredReenterAdvanceTargetSymbolChar(const CString& csTargetName, bool bAdvance, bool bMove,
                                    int* iTargetOcc, bool* bExplicitOcc, const Symbol* symbol_holding_name = nullptr); // 20120521

    bool CheckAtSymbol(const CString& csFullName, int* iSymTarget, int* iOccTarget, bool* bExplicitOcc); // RHF Dec 09, 2003

public:
    double exendcase(int iExpr);
    double exuniverse(int iExpr);
    double exskipcase(int iExpr);
    double exexit(int iExpr);

    double  exstop(int iExpr);
    double  exispartial(int iExpr);
    double  exisverified(int iExpr);
    double  exctab(int iExpr);
    double  exbreak(int iExpr);
    double  exexport(int iExpr);
    double  exset(int iExpr);
    double  exvisualvalue(int iExpr);
    double  exhighlight(int iExpr);
    double  exinadvance(int iExpr);
    double  exseed(int iExpr);
    double  exrandom(int iExpr);
    double  exrandomin(int iExpr); // 20110721

    double   exnoccurs(int iExpr);
    double   exsoccurs(int iExpr);
    double   exsoccurs_pre80(int iExpr);
    int      exsoccurs(const SECT* pSecT, bool use_rules_for_binary_dict_items = false); // RHF May 14, 2003

private:
    std::optional<std::tuple<const int*, const int*>> EvaluateSwitchConditions(int iExpr);
public:
    double  exrecode(int iExpr);
    double  exwhen(int iExpr);

    // Helper methods to reduce coding or repeating in some other methods
public:
    static bool ConditionalValueIsFalse(double value) { return ( value == 0 || IsSpecial(value) ); }
    static bool ConditionalValueIsTrue(double value)  { return !ConditionalValueIsFalse(value); }

private:
    template<typename T> double AssignAlphaValue(T&& value);
    double AssignAlphaValue(const std::optional<std::wstring>& value);
    double AssignAlphaValue(std::optional<std::wstring>&& value);
    double AssignBlankAlphaValue();

    double AssignVariantValue(std::variant<double, std::wstring> value);

public:
    template<typename T> void AssignValueToVART(int variable_compilation, T value);
    template<typename T> T EvaluateVARTValue(int variable_compilation);
    template<typename T> void ModifyVARTValue(int variable_compilation, const std::function<void(T&)>& modify_value_function,
                                              std::shared_ptr<Paradata::FieldInfo>* paradata_field_info = nullptr);

    template<typename T> void AssignValueToSymbol(const Nodes::SymbolValue& symbol_value_node, T value);
    template<typename T> T EvaluateSymbolValue(const Nodes::SymbolValue& symbol_value_node);
    template<typename T> void ModifySymbolValue(const Nodes::SymbolValue& symbol_value_node, const std::function<void(T&)>& modify_value_function);

private:
    // int calculateLimitsForGroup( int indexArray[], int iSymGroup )
    // used by calculateLimitsForGroupNode()
    int calculateLimitsForGroup( double doubleArray[], int iSymGroup );
    int calculateLimitsForGroup( int indexArray[], int iSymGroup );

    // CalculateLimitsForGroupNode( GRP_NODE* pgrpNode, int iSymGroup )
    // helper function used in excount, exsum, exmin, exmax, exavrge
    int calculateLimitsForGroupNode( GRP_NODE* pgrpNode, int iSymGroup ); // rcl, Jul 22, 2004

    // calculateLimitForVarNode
    // used in exsum, exmin, exmax, exavrge
    int calculateLimitForVarNode( MVAR_NODE* pMVAR, int iSymGroup ); // rcl, Jul 22, 2004

public:
    // math functions
    double exadd(int iExpr);
    double exsub(int iExpr);
    double exminus(int iExpr);
    double exmult(int iExpr);
    double exdiv(int iExpr);
    double exmod(int iExpr);
    double exexp(int iExpr);
    double exeq(int iExpr);
    double exne(int iExpr);
    double exle(int iExpr);
    double exlt(int iExpr);
    double exge(int iExpr);
    double exgt(int iExpr);
    double exequ(int iExpr);
    double exor(int iExpr);
    double exnot(int iExpr);
    double exand(int iExpr);
    double exabs(int iExpr);
    double exex(int iExpr);
    double exinc(int iExpr);
    double exint(int iExpr);
    double exlog(int iExpr);
    double exlowhigh(int iExpr);
    double exspecial(int iExpr);
    double exsqrt(int iExpr);
    double exround(int iExpr);


    // string functions
    double exstring_eq(int program_index);
    double exstring_ne(int program_index);
    double exstring_lt(int program_index);
    double exstring_le(int program_index);
    double exstring_ge(int program_index);
    double exstring_gt(int program_index);
    double excompare(int program_index);
    double excompareNoCase(int program_index);
    double exconcat(int iExpr);
    double exischecked(int iExpr);
    double exlength(int iExpr);
    double expos_poschar(int iExpr);
    double exregexmatch(int iExpr);
    double exreplace(int iExpr);
    double exstartswith(int iExpr);
    double exstrip(int iExpr);
    double extolower_toupper(int iExpr);
private:
    template<TokenCode token_code>
    double exstring_operators(int program_index);


public:
    // date functions
    double extimestamp(int iExpr);
    double extimestring(int iExpr);
    double exsysdate(int iExpr);
    double exsystime(int iExpr);
    double exdateadd(int iExpr);
    double exdatediff(int iExpr);
    double exdatevalid(int iExpr);
    double excmcode(int iExpr);
    double exsetlb_setub(int iExpr);
    double exadjlba(int iExpr);
    double exadjuba(int iExpr);
    double exadjlbi(int iExpr);
    double exadjubi(int iExpr);


    // other functions
    double  excount(int iExpr);
    double  exsum(int iExpr);
    double  exavrge(int iExpr);
    double  exmin(int iExpr);
    double  exmax(int iExpr);
    double  exseek(int iExpr); // 20100602
    double  exseekMinMax(int iExpr); // 20130119

    std::vector<int> EvaluateValidIndices(int iSymGroup, int iSymItem, int iWhere); // 20110810
    int GetTrueGroupOccs(int iSymGroup, bool use_rules_for_binary_dict_items = false); // victor Dec 10, 01

    double  exdisplay(int iExpr);
    double  exerrmsg(int iExpr);
    double  exwritemsg(int iExpr);
    double  exmaketext(int iExpr);
    double  exlogtext(int iExpr);
    double  exwarning(int iExpr);

    double  extonumber(int program_index);
    double  exedit(int iExpr);
    double  exdecryptstring(int iExpr);

    double  exinvalueset(int iExpr);
    double  exsetvalueset(int iExpr); // RHF Aug 28, 2002
    double  exsetvalueset_pre80(int iExpr);
    double  exsetvaluesets(int iExpr);    // 20100523
    double  exrandomizevs(int iExpr);     // 20110811

    double  exgetusername(int iExpr);     // 20111028
    double  exgetdeviceid(int iExpr);     // 20141023
    double  exuuid(int iExpr);

    double  exparadata(int program_index);
    double  exsqlquery(int program_index);
    double  exsqlquery(int program_index, const std::function<double(sqlite3*, const std::string&)>* setreportdata_callback);
    double  expre77_setreportdata(int iExpr);
    double  expre77_report(int iExpr);

    void RegisterSqlCallbackFunctions(sqlite3* db);
    void ProcessSqlCallbackFunction(UserFunction& user_function, void* void_context, int iArgC, void* void_ppArgV);

    double exsyncconnect(int iExpr);
    double exsyncdisconnect(int iExpr);
    double exsyncdata(int iExpr);
    double exsyncfile(int iExpr);
    double exsyncserver(int iExpr);
    double exsyncapp(int iExpr);
    double exsyncmessage(int iExpr);
    double exsyncparadata(int iExpr);
    double exsynctime(int iExpr);

    double  exgetbluetoothname(int iExpr);
    double  exsetbluetoothname(int iExpr);

    double exsavesetting(int iExpr);
    double exloadsetting(int iExpr);

    double exsavepartial(int iExpr);
    double exgetoperatorid(int iExpr);
    double exsetoperatorid(int iExpr);

    double  exdemode(int iExpr);
    double  exclrcase(int iExpr);

private:
    std::optional<CSize> EvaluateSize(int width_expression, int height_expression);
    std::unique_ptr<ViewerOptions> EvaluateViewerOptions(int viewer_options_node_index);

public:
    double  exaccept(int iExpr);
    double  exaccept_pre77(int iExpr);
    double  exprompt(int iExpr);
    double  exprompt_pre77(int iExpr);
    double  exhtmldialog(int iExpr);

    double  exview(int program_index);

    double  excountvalid(int iExpr); // 20091202

    double  exdeckarray(int iExpr); // for getdeck and putdeck

    double  exgetlanguage(int iExpr); // 20100309
    double  exsetlanguage(int iExpr);
    double  extr(int iExpr);

    double  exuserbar(int iExpr); // 20100414

    double  exmessageoverrides(int iExpr); // 20100518

    double  extrace(int iExpr);

    double  exgetcapturetype(int iExpr);    // 20100608
    double  exsetcapturetype(int iExpr);    // 20100608
    double  exsetcapturepos(int iExpr);     // 20110502

    double  exchangekeyboard(int iExpr);    // 20120820

    double  exsetfont(int iExpr);           // 20100618
    double  exorientation(int iExpr);       // 20100618

    double  expathname(int iExpr);          // 20110107

    double  exgps(int iExpr);               // 20110223
    std::shared_ptr<Paradata::Event> CreateParadataGpsEvent(const CString& event_type, const CString& event_information);

    double  exgetrecord(int iExpr);     // 20110302

    double  exsetoutput(int iExpr);     // 20121126

    double  exgetos(int iExpr);
    double  exconnection(int iExpr);

    double  exfreealphamem(int program_index);

    double  exsetvalue(int iExpr);      // 20140228
    double  exgetvalue(int iExpr);      // 20140422
    double  exgetvaluealpha(int iExpr); // 20140422
    VARX*   AssignParser(int iExpr,CNDIndexes *& pTheIndex,int * aIndex); // 20140422

    std::wstring GetValueLabel(int iCurVar, VART* pVarT); // for CAPI text
    std::wstring GetValueLabel(const VART* pVarT, const std::variant<double, std::wstring>& value);
    double  exgetvaluelabel(int iExpr);
    double  exvariablevalue(int iExpr);

    double  exxtab(int iExpr);
    double  extblcoord(int iExpr); // tblrow, tblcol, tbllay
    double  extblsum(int iExpr);
    double  extblmed(int iExpr);
    double  exfilename(int iExpr);

    double  exkey(int iExpr); // key + currentkey
    double  exkeylist(int iExpr);
    double  exfind_locate(int program_index);
    double  exdictaccess(int program_index);
    double  exdictaccess(const Nodes::SetAccessFirstLast& set_access_first_last_node);
    double  exretrieve(int iExpr);

    // data access functions
public:
    bool IsDataAccessible(const Symbol& symbol, bool issue_error_if_inaccessible);
    void EnsureDataIsAccessible(const Symbol& symbol); // calls IsDataAccessible and throws an exception on error
private:
    double exDataAccessValidityCheck(int program_index);

    // EngineDictionary functions
    double exdictcompute(int iExpr);

    // Symbol functions
public:
    double exSymbol_getLabel(int program_index);
    double exSymbol_getName(int program_index);


    // Array functions
public:
    double exarrayvar(int iExpr);
    double exarrayclear(int iExpr);
    double exarraylength(int iExpr);
private:
    // returns the index, or an empty vector if the index is invalid
    std::vector<size_t> EvaluateArrayIndex(int arrayvar_node_expression, LogicArray** out_logic_array);


    // Audio functions
public:
    double exAudio_compute(int program_index);
    double exAudio_clear(int program_index);
    double exAudio_concat(int program_index);
    double exAudio_length(int program_index);
    double exAudio_load(int program_index);
    double exAudio_play(int program_index);
    double exAudio_save(int program_index);
    double exAudio_stop(int program_index);
    double exAudio_record(int program_index);
    double exAudio_recordInteractive(int program_index);


    // Barcode functions
public:
    double exBarcode_read(int program_index);
    double exBarcode_createQRCode(int program_index);


    // Case functions
public:
    double exCase_view(int program_index);
    double exCase_view(const DICT& dictionary, const ViewerOptions* viewer_options);


    // CS functions
public:
    double exActionInvoker(int program_index);


    // Document functions
public:
    double exDocument_compute(int program_index);
    double exDocument_clear(int program_index);
    double exDocument_load(int program_index);
    double exDocument_save(int program_index);
    double exDocument_view(int program_index);
    double exDocument_view(const LogicDocument& logic_document, const ViewerOptions* viewer_options);

    // Geometry functions
public:
    double exGeometry_compute(int program_index);
    double exGeometry_clear(int program_index);
    double exGeometry_load(int program_index);
    double exGeometry_save(int program_index);
    double exGeometry_tracePolygon_walkPolygon(int program_index);
    double exGeometry_area_perimeter(int program_index);
    double exGeometry_minLatitude_maxLatitude_minLongitude_maxLongitude(int program_index);
    double exGeometry_getProperty(int program_index);
    double exGeometry_setProperty(int program_index);
private:
    bool EnsureGeometryExistsAndHasValidContent(const LogicGeometry& logic_geometry, const TCHAR* action_for_displayed_error_message);


    // HashMap functions
public:
    double exhashmapvar(int iExpr);
    double exhashmapcompute(int iExpr);
    double exhashmapclear(int iExpr);
    double exhashmapcontains(int iExpr);
    double exhashmapgetkeys(int iExpr);
    double exhashmaplength(int iExpr);
    double exhashmapremove(int iExpr);
private:
    // returns the index, or an empty vector if the index is invalid
    std::vector<std::variant<double, std::wstring>> EvaluateHashMapIndex(int hashmap_node_expression, LogicHashMap** out_hashmap, bool bounds_checking);


    // Image functions
public:
    double exImage_compute(int program_index);
    double exImage_clear(int program_index);
    double exImage_load(int program_index);
    double exImage_resample(int program_index);
    double exImage_save(int program_index);
    double exImage_captureSignature_takePhoto(int program_index);
    double exImage_captureSignature_native(int program_index);
    double exImage_takePhoto_native(int program_index);
    double exImage_view(int program_index);
    double exImage_view(const LogicImage& logic_image, const ViewerOptions* viewer_options);
    double exImage_width_height(int program_index);


    // Item functions
public:
    double exItem_getValueLabel(int program_index);
    double exItem_hasValue_isValid(int program_index);

private:
    std::tuple<EngineItemAccessor*, bool> GetEngineItemAccessorAndVisualValueFlag(const Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node, int visual_value_argument_index);

public:
    // evaluates the symbol reference and any associated subscript;
    // the subscript is not checked for validity
    template<typename SymbolT = Symbol*>
    SymbolReference<SymbolT> EvaluateSymbolReference(int symbol_index, int subscript_compilation);

    // returns a pointer (or shared pointer) to a symbol;
    // if the symbol's subscript is invalid, a runtime message appears and null is returned;
    // if the symbol is an item, the wrapped symbol (Document, Image, etc.,) is returned
    template<typename SymbolT = Symbol*, typename SymbolReferenceT>
    auto GetFromSymbolOrEngineItem(const SymbolReference<SymbolReferenceT>& symbol_reference, bool use_exceptions = false);

    template<typename SymbolT = Symbol*>
    auto GetFromSymbolOrEngineItem(int symbol_index, int subscript_compilation, bool use_exceptions = false);

    // evaluates the subscript if provided; because a subscript is not needed for static functions,
    // if the subscript is invalid, a runtime message will appear, but the symbol will still be returned
    template<typename SymbolT = Symbol>
    SymbolT& GetFromSymbolOrEngineItemForStaticFunction(int symbol_index, int subscript_compilation);

private:
    // evaluates the implicit and explicit components of the item's subscript
    EvaluatedEngineItemSubscript EvaluateEngineItemSubscript(const EngineItem& engine_item, const Nodes::ItemSubscript& item_subscript_node);

    template<typename SymbolT>
    SymbolT GetFromSymbolOrEngineItemWorker(const SymbolReference<SymbolT>& symbol_reference, bool use_exceptions);

    // evaluates the subscript for the item (implicit if subscript_text is null), throwing an exception if the subscript is invalid
    Symbol& GetWrappedEngineItemSymbol(EngineItem& engine_item, const TCHAR* subscript_text);

    const Symbol* GetCurrentProcSymbol() const;

    int CalculateEngineItemImplicitOccurrence(const EngineItem& engine_item, bool get_record_occurrence);

    const Nodes::SymbolVariableArgumentsWithSubscript& GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(int program_index);
    const Nodes::SymbolComputeWithSubscript& GetOrConvertPre80SymbolComputeWithSubscriptNode(int program_index);

private:
    std::map<int, std::unique_ptr<int[]>> m_convertedPre80Nodes;


    // List functions
public:
    double exlistvar(int iExpr);
    double exlistcompute(int iExpr);
    double exlistadd(int iExpr);
    double exlistclear(int iExpr);
    double exlistinsert(int iExpr);
    double exlistlength(int iExpr);
    double exlistremove(int iExpr);
    double exlistremoveduplicates(int iExpr);
    double exlistremovein(int iExpr);
    double exlistseek(int iExpr);
    double exlistshow(int iExpr);
    double exlistshow_pre77(int iExpr);
    double exlistsort(int iExpr);
private:
    // returns the one-based index, or std::nullopt if the index is invalid
    std::optional<size_t> EvaluateListIndex(int listvar_node_expression, LogicList** out_logic_list, bool for_assignment);


    // Localhost functions
public:
    // this can return a string (with the URL), or std::unique_ptr<VirtualFileMappingHandler>
    // in the latter case, the calling function is in charge of the lifecycle of the virtual file
    template<typename T = std::wstring>
    T LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol, std::optional<std::wstring> content_type_override = std::nullopt,
                                           bool evaluate_immediately = false);
private:
    std::vector<std::unique_ptr<VirtualFileMappingHandler>> m_localHostVirtualFileMappingHandlers;


    // Map functions
public:
    double exmapaddmarker(int iExpr);
    double exmaphide(int iExpr);
    double exmapshow(int iExpr);
    double exmapsetmarkerimage(int iExpr);
    double exmapsetmarkertext(int iExpr);
    double exmapsetmarkeronclickonclickinfo(int iExpr);
    double exmapsetmarkerdescription(int iExpr);
    double exmapsetmarkerondrag(int iExpr);
    double exmapsetmarkerlocation(int iExpr);
    double exmapgetmarkerlatitudelongitude(int iExpr);
    double exmapremovemarker(int iExpr);
    double exmapsetonclick(int iExpr);
    double exmapshowcurrentlocation(int iExpr);
    double exmapaddtextbutton(int iExpr);
    double exmapaddimagebutton(int iExpr);
    double exmapremovebutton(int iExpr);
    double exmapsetbasemap(int iExpr);
    double exmapsettitle(int iExpr);
    double exmapzoomto(int iExpr);
    double exmap_clear_clearButtons_clearGeometry_clearMarkers(int program_index);
    double exmapgetlastclicklatitudelongitude(int iExpr);
    double exmapaddgeometry(int iExpr);
    double exmapremovegeometry(int iExpr);
    double exmapsavesnapshot(int iExpr);
private:
    template<typename T>
    int SetBaseMap(LogicMap& logic_map, const T& base_map_selection);


    // named frequency and unnamed frequency functions
public:
    double exfrequnnamed(int iExpr);
    double exfreqclear(int iExpr);
    double exfreqsave(int iExpr);
    double exfreqtally(int iExpr);
    double exFreq_view(int program_index);
    double exFreq_view(const NamedFrequency& named_frequency, const ViewerOptions* viewer_options, int frequency_parameters_node_index);
    double exfreqvar(int iExpr);
    double exfreqcompute(int iExpr);

private:
    std::unique_ptr<FrequencyDriver> m_frequencyDriver;


    // Path functions
public:
    double expathconcat(int iExpr);
    double expathgetdirectoryname(int iExpr);
    double expathgetextension(int iExpr);
    double expathgetfilename(int iExpr);
    double expathgetfilenamewithoutextension(int iExpr);
    double expathgetrelativepath(int iExpr);
    double expathselectfile(int iExpr);


    // Pff functions
public:
    double expffexec(int iExpr);
    double expffgetproperty(int iExpr);
    double expffload(int iExpr);
    double expffsave(int iExpr);
    double expffsetproperty(int iExpr);
    double expffcompute(int iExpr);


    // Report functions
public:
    double exReport_view(int program_index);
    double exreport_save(int iExpr);
    double exreport_write(int Expr);
private:
    double exReport_view(Report& report, const ViewerOptions* viewer_options);
    std::wstring* GetReportTextBuilderWithValidityCheck(Report& report);
    std::unique_ptr<std::wstring> GenerateReport(Report& report, const std::optional<std::wstring>& output_filename);

    // SystemApp functions
public:
    double exsystemapp_clear(int iExpr);
    double exsystemapp_setargument(int iExpr);
    double exsystemapp_getresult(int iExpr);
    double exsystemapp_exec(int iExpr);


    // ValueSet functions
public:
    double exvaluesetadd(int iExpr);
    double exvaluesetclear(int iExpr);
    double exvaluesetlength(int iExpr);
    double exvaluesetremove(int iExpr);
    double exvaluesetshow(int iExpr);
    double exvaluesetshow_pre77(int iExpr);
    double exvaluesetsort(int iExpr);
    double exvaluesetcompute(int iExpr);


    // JSON-related functions
public:
    JsonReaderInterface* GetEngineJsonReaderInterface();
    std::wstring GetSymbolJson(const Symbol& symbol, Symbol::SymbolJsonOutput symbol_json_output, const JsonNode<wchar_t>* serialization_options_node);
    void UpdateSymbolValueFromJson(Symbol& symbol, const JsonNode<wchar_t>& json_node);
    double exSymbol_getJson_getValueJson(int program_index);
    double exSymbol_updateValueFromJson(int program_index);


    // dynamic logic evaluation functions
public:
    InterpreterExecuteResult EvaluateLogic(const std::wstring& logic);


private:
    template<typename T>
    bool EvaluateCaseFunctionParameters(const CDataDict& dictionary, const T& key_arguments_list_node_or_function_node, std::wstring& key);
public:
    double  exloadcase(int program_index);
    double  exloadcase_pre80(int program_index);
    double  exdelcase(int program_index);
    double  exdelcase_pre80(int program_index);
    double  exwritecase(int program_index);
    double  exwritecase_pre80(int program_index);

    double  exselcase(int iExpr);
    double  exselcase_pre77(int iExpr);
    double  exnmembers(int iExpr);
    double  exfor_dict(int iExpr);
    double  exforcase(int iExpr);
    double  excountcases(int iExpr);

    double  exopen(int iExpr);
    double  exclose(int iExpr);
    double  exsetfile(int program_index);
    double  exsetfile_dictionary(EngineDictionary& engine_dictionary, const ConnectionString& connection_string, bool create_new, bool open_or_create);
    double  exsetfile_dictionary(DICT* pDicT, const ConnectionString& connection_string, bool bCreateNew, bool bOpenOrCreate);

private:
    void    EntryInputRepositoryChangingActions();

public:
    double  exgetcaselabel(int iExpr);
    double  exsetcaselabel(int iExpr);

    double  exdiagnostics(int iExpr);
    double  exsysparm(int iExpr);
    double  exsetattr(int iExpr);
    double  exvaluelimit(int iExpr);
    double  exfor_group(int iExpr);                   // RHC Aug 17, 2000
    double  exfor_relation(int iExpr);
    //////////////////////////////////////////////////////////////////////////
    // to ease exdofor_relation max calculation
    double  getMaxIndexForVariableUsingStack( int iVar, REL_NODE* pRelNode );     // rcl, Dec 18, 2004
    double  getMaxIndexForVariableUsingStack( VART* pVarT, REL_NODE* pRelNode );  // rcl, Dec 18, 2004
    //////////////////////////////////////////////////////////////////////////
    double  exdofor_relation( FORRELATION_NODE* pFor, double* dTableWeight=NULL, int* iTabLogicExpr=NULL, LIST_NODE* pListNode=NULL  ); // RHF Jul 03, 2002
    double  exfucall(int iExpr);                      // RHF Aug 21, 2000

    double  exupdate(int iExpr);                      // RHF Nov 17, 2000
    double  exgetbuffer(int iExpr);                   // RHF Sep 21, 2001

private:
    bool EvaluateNoteReference(const FNNOTE_NODE& note_node, std::shared_ptr<NamedReference>& named_reference, int& field_symbol);
    std::optional<CString> EvaluateNoteOperatorId(const FNNOTE_NODE& note_node, int field_symbol);
public:
    double  exgetnote(int iExpr);
    double  exeditnote(int iExpr);
    double  exputnote(int iExpr);

    // both "get symbol" methods throw an exception if the symbol is not found;
    // the "evaluated" version allows the specification of subscripts and returns the base symbol (non-null), as well as the wrapped symbol (potentially null)
    Symbol& GetSymbolFromSymbolName(const StringNoCase& symbol_name, SymbolType preferred_symbol_type = SymbolType::None);
    std::tuple<Symbol*, Symbol*> GetEvaluatedSymbolFromSymbolName(const std::wstring& symbol_name_and_potential_subscript, SymbolType preferred_symbol_type = SymbolType::None);

    double  exgetlabel(int iExpr);                    // RHF Aug 25, 2000
    double  exgetimage(int iExpr);                    // 20150809

    CString EvaluateOccurrenceLabel(const Symbol* symbol, const std::optional<int>& zero_based_occurrence);
    double  exgetocclabel(int iExpr);
    double  exsetocclabel(int iExpr);
    double  exshowocc(int iExpr);

    double  exmaxocc(int iExpr);
    double  exmaxocc_pre80(int iExpr);

private:
    std::optional<std::wstring> ExGetFileName(int iExpr);
    std::vector<std::wstring> ExGetFileNames(int iExpr);

    template<typename CF>
    double ExFileCopyRenameProcessor(int program_index, CF callback_function);

public:
    double  exfilecreate(int iExpr);
    double  exfileexist(int iExpr);
    double  exfiledelete(int iExpr);
    double  exfilecopy(int program_index);
    double  exfilerename(int iExpr);
    double  exfilesize(int iExpr);
    double  exfileempty(int iExpr);
    double  exfileconcat(int iExpr);
    double  exfileread(int iExpr);
    double  exfilewrite(int iExpr);

    double  exfiletime(int iExpr);
    double  exdirexist(int iExpr);
    double  exdircreate(int iExpr);
    double  exdirdelete(int iExpr);
    double  exdirlist(int program_index);
    double  exdirlist_pre77(int iExpr);

    double  excompress(int iExpr);
    double  exdecompress(int iExpr);

    double  exhash(int iExpr);
    double  exencode(int iExpr);

private:
    ParameterManager::Parameter GetSetPropertyParser(int iExpr, std::set<int>* symbol_set, std::variant<double, CString>* out_value = nullptr);
public:
    CString GetProperty(ParameterManager::Parameter parameter, std::set<int>* symbol_set = nullptr);
    double  exgetproperty(int iExpr);
    double  exsetproperty(int iExpr);
    double  exprotect(int iExpr);

    double  ExExecSystem(int iExpr);
    std::unique_ptr<Paradata::ExternalApplicationEvent> ExExecCommonBeforeExecute(FunctionCode source, const std::wstring& command, int flags);
    bool    ExExecCommonExecute(std::wstring command, int flags);
    double  ExExecCommonAfterExecute(FunctionCode source, int flags, bool success, std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event);
    double  ExExecPFF(int iExpr);
    double  ExExecPFF(std::variant<LogicPff*, std::wstring> logic_pff_or_pff_filename, std::optional<int> flags = std::nullopt);

    double exwhile(int iExpr);
    double exdo(int iExpr);
    double exfornext(int iExpr);
    double exforbreak(int iExpr);

private:
    bool InWorker(int in_node_expression, const std::variant<double, std::wstring>& value,
                  const std::function<const std::variant<double, std::wstring>&(int)>* expression_evaluator = nullptr);
public:
    double  exin(int iExpr);                          // RHC Oct 16,2000

    double  exfncurocc(int iExpr);                    // RHC Oct 30, 2000
    double  exfntotocc(int iExpr);                    // RHC Oct 30, 2000
    // double  ExCurTotOcc( int iExpr, bool bCurOcc );     // RHF Oct 31, 2000

    static int GetCurOccFromGroup(const GROUPT* pGroupT, bool bUseBatchLogic);

    // Signature changed to make debugging easier
    // RCL, May 2004
    double  ExCurTotOcc( FNGR_NODE * pNode, bool bCurOcc );
    double  exinsert_delete(int iExpr);
    bool    ExInsertWorker(GROUPT* pGroupT, int occurrence);
    bool    ExDeleteWorker(GROUPT* pGroupT, int occurrence);
    double  exsort(int iExpr);  // Chirag Sep 11, 2002
    double  exswap(int iExpr);  // 20100105

    double  exshow(int iExpr); // RHF Jun 28, 2006
    double  exshow_pre77(int iExpr, int iActualForNode); // RHF Jun 28, 2006
    double  exshowlist(int iExpr); // RHF Jun 30, 2006
    double  exshowarray(int iExpr);
    double  exshowarray_pre77(int iExpr);

    int SelectDlgHelper_pre77(int iFunCode, const CString* csHeading, const std::vector<std::vector<CString>*>* paData,
                              const std::vector<CString>* paColumnTitles, std::vector<bool>* pbaSelections,
                              const std::vector<PortableColor>* row_text_colors);

public:
    int     exset_attr( int iSymVar, int iOcc, SET_ATTR_NODE* setpa_node );

    int     exset_behavior( int iSymVar, void* pInfo );
    void    ResetAllVarsBehavior();

private:
    bool    exboxrow( const void* pBoxNode_void, BOX_ROW* pBoxRow, double aVarValues[] );
    int     exset_markform( int iSymFrm, SET_ATTR_NODE* setpa_node );
    int     exset_markgroup( int iSymGroup, SET_ATTR_NODE* setpa_node );
    int     ExSetBehaviorList( int iBehaviorItem, LIST_NODE* pListNode, bool bSetOn, bool bConfirm ); // RHF May 10, 2001

    // --- changing attributes of fields-in-forms
    int     frm_varpause( int iSymVar, FieldBehavior  eBehavior );
    int     frm_varvisible( int iSymVar, bool bOnOff );
    void    frm_capimode( int iiSymVar, int iCapiMode );

    // --- Capi
public:
    void RunGlobalOnFocus( int iVar );

#ifdef WIN_DESKTOP
    void UpdateKeyboardInputMethod(HKL hKL);
#endif

private:

    QuestionTextParamCache m_question_text_param_cache;
    std::map<const DictValue*, int> m_deckarrayIndexMappings;

#ifdef WIN_DESKTOP
    HKL     m_hLastDefaultKL;
    HKL     m_hCurrentKL;
#endif

public:
    // HTML_QSF_TODO which of the following are needed?
    void Evaluate(int iCurVar, const ParsedCapiParam* pCapiParam, CString& csEvaluatedText );
    int EvaluateCapiVariableCurrentOccurrence(int iCurVar, VART* pVarT);
    CString EvaluateCapiText(const std::wstring& language_name, bool bQuestion, int iSym, int iOcc);
    CString EvaluateCapiText(const CapiQuestion& question, const Symbol* symbol, const std::wstring& language_name, bool bQuestion);
    CString EvaluateCapiTextPre76(const CapiQuestion& question, const std::wstring& language_name, bool bQuestion, int symbol_index, int iOcc);
    CString ExpandText(const CString& csText, bool bShowErrors = true, bool* bSomeError = nullptr, std::vector<ParsedCapiParam>* capi_params = nullptr);
    bool EvaluateCapiConditionPre76(const CapiCondition& condition, int iSym);
    int GetCapiConditionScorePre76(const CapiCondition& condition, int iSym, int iOcc);

    bool EvaluateQuestionTextCondition(const Symbol* symbol, int iExpr);
    CString EvaluateQuestionTextFill(const Symbol* symbol, int iExpr);
private:
    std::wstring EvaluateTextFill(int program_index);

    // --- tables & arrays processing
public:
    double  DoXtab( CTAB* ct, double dTableWeight, int iTabLogicExpr, LIST_NODE* pListNode );

private:
    double  DoXtabRelUnit( CTAB* pCtab, double dTableWeight, int iTabLogicExpr, LIST_NODE* pListNode );
    double  DoXtabGroupUnit( CTAB* pCtab, double dTableWeight, int iTabLogicExpr, LIST_NODE* pListNode );

    bool    ExecTabLogic( CTAB* pCtab, int iTabLogicExpr );
    double  DoOneXtabForSubTable( CTAB* pCtab, double dWeight, CSubTable* pSubTable, int iTabLogicExpr );
    double  DoOneXtabFor4AllSubTablesOfCurUnit( CTAB* pCtab, double dWeight,int iTabLogicExpr );
    double  oldtblcoord( int iExpr, int iDim );
    double  tblcoord( int iExpr, int iDim );

public:
    double  DoOneXtab( CTAB* pCtab, double dWeight, int iTabLogicExpr, LIST_NODE* pListNode );

    VART*   GetVarT( Symbol* pSymbol ); // RHF Jul 16, 2001

    CDEField* GetCDEFieldFromVART(VART* pVarT);

    void    CtPos( CTAB* ct, int ct_node, int* vector, CSubTable* pSubTable, CCoordValue* pCoordValue, bool bMarkAllPos );

    // CtPos() method  used to be a long [many source code lines] method,
    //         Now it has been refactorized into the methods below
    //             CtPos_Add, CtPos_Mul and CtPos_Var
    //
    // rcl, Oct 26, 2004
    void    CtPos_Add( CTAB* ct, CTNODE* pNode, int* vector, CSubTable* pSubTable, CCoordValue* pCoordValue, bool bMarkAllPos ); // rcl, Oct 26, 2004
    void    CtPos_Mul( CTAB* ct, CTNODE* pNode, int* vector, CSubTable* pSubTable, CCoordValue* pCoordValue, bool bMarkAllPos ); // rcl, Oct 26, 2004
    //SAVY 4 SPEED
    void    CtPos_Add( CTAB* ct, CTNODE* pNode, std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors, CCoordValue* pCoordValue, bool bMarkAllPos ); // rcl, Oct 26, 2004
    void    CtPos_Mul( CTAB* ct, CTNODE* pNode, std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors, CCoordValue* pCoordValue, bool bMarkAllPos ); // rcl, Oct 26, 2004
    void    CtPos( CTAB* ct, int ct_node, std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors,CCoordValue* pCoordValue, bool bMarkAllPos );
    void    CtPos_Var(  CTAB* pCtab, int iCtNode, std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors,CCoordValue* pCoordValue, bool bMarkAllPos ); // rcl, Oct 26, 2004
private:
    void    CtPos_FillIndexArray( CTAB* ct, VART* pVarT, int iOccExpr ); // rcl, Oct 26, 2004
public:
    void    CtPos_Var( CTAB* ct, int ct_node, int* vector, CSubTable* pSubTable, CCoordValue* pCoordValue, bool bMarkAllPos ); // rcl, Oct 26, 2004


    int     DoTally( int iCtNode, double dValue, int* iVector, int* iNumMatches ); // RHF Jul 09, 2001
    int     DoTally( int iCtNode, double dValue, std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors, int* iNumMatches );
    double  evaltblexpr(int iExpr);
    void    tblsetlu( TBL_NODE* tp );
    void    docpttbl(int iExpr);
    bool    tblchkexpr(int iExpr);
    int     tblsummed( TBL_NODE* tleft, TBL_NODE* tright, int* rl, int* lindex, int axis );
    bool    tbldimchk( TBL_NODE* t1, TBL_NODE* t2 );
    double  val_coord( int i_node, double i_coord );
    double  val_high();    // BMD 13 Oct 2005
    void    _val_coord( int i_node );

    int     ctvarpos( int iCtNode, double  dValue, int iSeq=1 );
    double  ct_ivalue( int  iCtNode, double  dValue );

    // export processing
public:
    void    ExpWriteThisExport();                          // formerly 'expwrecords'
    void    CleanExportTo();
private:
    void    ExpWrite1Record();
    void    ExpWriteVar( VART* pVarT, int iOccur = 0 );    // formerly 'expoutvar'

    // --- ISSA-like messages management
private:
    std::wstring EvaluateUserMessage(int message_node_index, FunctionCode function_code, int* out_message_number = nullptr);
    double DisplayUserMessage(int message_node_index);

public:
    std::wstring EvaluateVariableParameter(const std::variant<double, std::wstring>& value, int value_expression, bool request_label);

    // --- miscellaneous
public:
    CString ProcName();
    bool    CompleteHiddenGroup();           // RHF Nov 09, 2000 Add hidden groups for special section. The new ones create in compiling time.

    // --- engine links
public:
    CsDriver* GetCsDriver()               { return m_pCsDriver; }      // victor May 16, 01
    void SetCsDriver(CsDriver* pCsDriver) { m_pCsDriver = pCsDriver; } // victor May 16, 01

    // --- Implementation for the 3-D interpreter driver
private:
    bool ExecuteSymbolProcs(const Symbol& symbol, ProcType proc_type);
public:
    bool ExecuteProcLevel(int iLevel, ProcType proc_type);
    bool ExecuteProcGroup(int iSymGroup, ProcType proc_type, bool bCheckOccs = true);
    bool ExecuteProcVar(int iSymVar, ProcType proc_type);
    bool ExecuteProcBlock(int iSymBlock, ProcType proc_type);
    void ExecuteProcTable(int iCtab, ProcType proc_type);

    bool ExecuteProgramStatements(int program_index);

    // runs the callback function and returns whether a movement or program control action has occurred;
    // any thrown ProgramControlException exceptions will be stored and can be processed by calling RethrowProgramControlExceptions
    template<typename CF>
    bool Execute(CF callback_function);

    // calls Execute and converts the callback function's result to a string
    template<typename CF>
    InterpreterExecuteResult Execute(DataType callback_result_data_type, CF callback_function);

    void RethrowProgramControlExceptions();

private:
    std::exception_ptr m_caughtProgramControlException;


    // scope functions
    double exScopeChange(int program_index);

private:
    // runs the callback function on each of the current scope change nodes; the function should return true to continue processing
    template<typename CF>
    void IterateOverScopeChangeNodes(CF callback_function);

private:
    std::vector<const Nodes::ScopeChange*> m_scopeChangeNodeIndices;

private:
    const std::vector<UserFunction*>& GetSpecialFunctions();
    std::vector<UserFunction*> m_specialFunctions;
    bool m_bExecSpecFunc;

public:
    bool HasSpecialFunction(SpecialFunction special_function);
    double ExecSpecialFunction(int iVar, SpecialFunction special_function, const std::vector<std::variant<double, std::wstring>>& arguments);

    bool ExecuteOnSystemMessage(MessageType message_type, int message_number, const std::wstring& message_text);

    enum class SetLanguageSource { SystemLocale, Pff, Logic, Interface };
    bool SetLanguage(wstring_view language_name, SetLanguageSource language_source, bool show_failure_message = false);
    void SetStartupLanguage();
    std::vector<Language> GetLanguages(bool include_only_capi_languages = true) const;

    SyncClient*     GetSyncClient() { return m_pSyncClient; }

    std::unique_ptr<C3DObject> ConvertIndex(const CaseItem& case_item, const ItemIndex& item_index);
    std::unique_ptr<C3DObject> ConvertIndex(const CaseItemReference& case_item_reference);
    void ConvertIndex(const C3DIndexes& the3dObject, ItemIndex& item_index);
    void ConvertIndex(const C3DObject& the3dObject, ItemIndex& item_index);

    // imputation
public:
    double eximpute(int iExpr);
    template<typename T, typename TIN, typename TIO> double eximpute_worker(const TIN& impute_node, TIO imputation);
private:
    std::unique_ptr<ImputationDriver> m_imputationDriver;


    // convenience methods
private:
    double AssignInvalidValue(DataType data_type);

public:
    template<typename T = double>
    std::optional<T> EvaluateOptionalNumericExpression(int value_expression);

    template<typename T>
    T EvaluateOptionalNumericExpression(int value_expression, T default_value);

    bool EvaluateConditionalExpression(int value_expression);
    bool EvaluateOptionalConditionalExpression(int value_expression, bool default_value);

    std::optional<std::wstring> EvaluateOptionalStringExpression(int value_expression);

    template<typename T>
    std::wstring EvaluateOptionalStringExpression(int value_expression, T&& default_value);

    template<typename T>
    T EvaluateExpression(int value_expression);

    std::wstring EvaluateExpressionAsString(DataType value_data_type, int value_expression);

    std::variant<double, std::wstring> EvaluateVariantExpression(bool numeric, int value_expression);
    std::variant<double, std::wstring> EvaluateVariantExpression(DataType value_data_type, int value_expression);


private:
    std::unique_ptr<JsonReaderInterface> m_engineJsonReaderInterface;

    Nodes::EncodeType m_currentEncodeType;

    // ExShow
    std::vector<CString> m_aShowLines;

    std::unique_ptr<SelcaseManager> m_selcaseManager;

    IBluetoothAdapter* m_pBluetoothAdapter;
    SyncClient* m_pSyncClient;
    ISyncListener* m_pSyncListener;
    ISyncServerConnectionFactory* m_pSyncServerConnectionFactory;
    ICredentialStore* m_pSyncCredentialStore;
};



// --------------------------------------------------------------------------
// definitions of some convenience methods
// --------------------------------------------------------------------------

inline double CIntDriver::AssignInvalidValue(DataType data_type)
{
    if( IsNumeric(data_type) )
    {
        return DEFAULT;
    }

    else
    {
        ASSERT(IsString(data_type));
        return AssignBlankAlphaValue();
    }
}


template<typename T>
double CIntDriver::AssignAlphaValue(T&& value)
{
    double working_string_index = m_workingStrings.size();
    m_workingStrings.emplace_back(std::forward<T>(value));
    return working_string_index;
}


inline double CIntDriver::AssignBlankAlphaValue()
{
    double working_string_index = m_workingStrings.size();
    m_workingStrings.emplace_back();
    return working_string_index;
}


inline double CIntDriver::AssignVariantValue(std::variant<double, std::wstring> value)
{
    return std::holds_alternative<double>(value) ? std::get<double>(value) :
                                                   AssignAlphaValue(std::wstring(std::move(std::get<std::wstring>(value))));
}


template<typename T/* = double*/>
std::optional<T> CIntDriver::EvaluateOptionalNumericExpression(int value_expression)
{
    return ( value_expression != -1 ) ? std::make_optional(static_cast<T>(evalexpr(value_expression))) :
                                        std::nullopt;
}


inline bool CIntDriver::EvaluateConditionalExpression(int value_expression)
{
    return ConditionalValueIsTrue(evalexpr(value_expression));
}


inline bool CIntDriver::EvaluateOptionalConditionalExpression(int value_expression, bool default_value)
{
    return ( value_expression != -1 ) ? EvaluateConditionalExpression(value_expression) :
                                        default_value;
}


template<typename T>
T CIntDriver::EvaluateOptionalNumericExpression(int value_expression, T default_value)
{
    return ( value_expression != -1 ) ? static_cast<T>(evalexpr(value_expression)) :
                                        default_value;
}


inline std::optional<std::wstring> CIntDriver::EvaluateOptionalStringExpression(int value_expression)
{
    return ( value_expression != -1 ) ? std::make_optional(EvalAlphaExpr(value_expression)) :
                                        std::nullopt;
}


template<typename T>
std::wstring CIntDriver::EvaluateOptionalStringExpression(int value_expression, T&& default_value)
{
    return ( value_expression != -1 ) ? EvalAlphaExpr<T>(value_expression) :
                                        std::forward<T>(default_value);
}


template<typename T>
T CIntDriver::EvaluateExpression(int value_expression)
{
    if constexpr(std::is_same_v<T, double>)
    {
        return evalexpr(value_expression);
    }

    else
    {
        return EvalAlphaExpr<T>(value_expression);
    }
}


inline std::wstring CIntDriver::EvaluateExpressionAsString(DataType value_data_type, int value_expression)
{
    return ( value_data_type == DataType::String )  ? EvalAlphaExpr(value_expression) :
           ( value_data_type == DataType::Numeric ) ? DoubleToString(evalexpr(value_expression)) :
                                                      ReturnProgrammingError(std::wstring());
}


inline std::variant<double, std::wstring> CIntDriver::EvaluateVariantExpression(bool numeric, int value_expression)
{
    if( numeric )
    {
        return evalexpr(value_expression);
    }

    else
    {
        return EvalAlphaExpr(value_expression);
    }
}


inline std::variant<double, std::wstring> CIntDriver::EvaluateVariantExpression(DataType value_data_type, int value_expression)
{
    ASSERT(IsNumeric(value_data_type) || IsString(value_data_type));
    return EvaluateVariantExpression(IsNumeric(value_data_type), value_expression);
}


template<typename T, bool AlreadyCheckedIsV0/* = false*/>
T CIntDriver::ConvertV0Escapes(T text, V0_EscapeType v0_escape_type/* = V0_EscapeType::NewlinesToSlashN*/)
{
    if constexpr(!AlreadyCheckedIsV0)
    {
        if( !m_usingLogicSettingsV0 )
            return text;
    }

    ASSERT(m_usingLogicSettingsV0);
    ConvertV0EscapesWorker(text, v0_escape_type);
    return text;
}


inline std::wstring CIntDriver::EvaluateStringExpressionConvertingV0Escapes(int program_index, V0_EscapeType v0_escape_type/* = V0_EscapeType::NewlinesToSlashN*/)
{
    return m_usingLogicSettingsV0 ? ConvertV0Escapes<std::wstring, true>(EvalAlphaExpr(program_index), v0_escape_type) :
                                    EvalAlphaExpr(program_index);
}


inline std::wstring CIntDriver::ApplyV0Escapes(std::wstring text, V0_EscapeType v0_escape_type/* = V0_EscapeType::NewlinesToSlashN*/)
{
    if( m_usingLogicSettingsV0 )
        ApplyV0EscapesWorker(text, v0_escape_type);

    return text;
}


template<typename SymbolT /*= Symbol* */, typename SymbolReferenceT>
auto CIntDriver::GetFromSymbolOrEngineItem(const SymbolReference<SymbolReferenceT>& symbol_reference, const bool use_exceptions/* = false*/)
{
    if constexpr(std::is_same_v<SymbolT, std::shared_ptr<Symbol>>)
    {
        return GetFromSymbolOrEngineItemWorker<SymbolReferenceT>(symbol_reference, use_exceptions);
    }

    else
    {
        return assert_nullable_cast<SymbolT>(GetFromSymbolOrEngineItemWorker<SymbolReferenceT>(symbol_reference, use_exceptions));
    }
}


template<typename SymbolT /*= Symbol* */>
auto CIntDriver::GetFromSymbolOrEngineItem(const int symbol_index, const int subscript_compilation, const bool use_exceptions/* = false*/)
{
    using SymbolReferenceT = typename std::conditional<std::is_same_v<SymbolT, std::shared_ptr<Symbol>>, SymbolT, Symbol*>::type;
    return GetFromSymbolOrEngineItem<SymbolT, SymbolReferenceT>(EvaluateSymbolReference<SymbolReferenceT>(symbol_index, subscript_compilation), use_exceptions);
}


template<typename SymbolT /* = Symbol */>
SymbolT& CIntDriver::GetFromSymbolOrEngineItemForStaticFunction(const int symbol_index, const int subscript_compilation)
{
    // this function is for the "this" symbol of a static dot-notation function;
    // even though the subscript does not need to be evaluated to run the static function, it will be evaluated:
    //     - for functions that use FunctionDetails::StaticType::StaticWhenNecessary
    //     - so that something like SEX(MyFunc()).getLabel() results in MyFunc() being called, which people would expect
    Symbol* symbol = nullptr;

    if( subscript_compilation != -1 )
    {
        try
        {
            symbol = GetFromSymbolOrEngineItem<Symbol*>(symbol_index, subscript_compilation, true);
        }
        catch(...) { }
    }

    if( symbol == nullptr )
        symbol = &NPT_Ref(symbol_index);

    return assert_cast<SymbolT&>(*symbol);
}
