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

#include <zEngineO/Interpreter/LogicInterpreter.h>
#include <engine/Nodes.h>
#include <engine/DeFld.h>
#include <zTbdO/cttree.h>

class CapiCondition;
class CapiQuestion;
class CapiQuestionManager;
class CapiText;
class CaseItemIndex;
class CaseItemReference;
class CCoordValue;
class CDEField;
class CIntDriver;
class CMsgOptions;
class CsDriver;
class CSettings;
class CSubTable;
class DictValue;
enum class FieldStatus : int;
class ImputationDriver;
class ItemIndex;
class KeyboardLoader;
class LoopStack;
class NamedReference;
class SelcaseManager;
struct sqlite3;
class SyncClient;
struct SyncObjects;
class TraceHandler;
class VTSTRUCT;
namespace Nodes { struct SetAccessFirstLast; }
namespace Paradata { class ExternalApplicationEvent; }
namespace ParameterManager { enum class Parameter; }
namespace Pre77Report { class ReportManager; }


// TODO: review public/private for both data and methods


class CIntDriver : public LogicInterpreter
{
// --- Data members --------------------------------------------------------

    // --- procedure being executed
public:
    ProcType m_procType;                   // proc being executed: pre/post
    int     m_iExLevel;                    //                    : level
    int     m_iExSymbol;                   //                    : isym

    // --- execution flags
public:
    bool    m_bStopExec;
    bool    m_bSkipStmt;
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


    // --- engine links
public:
    CEngineDriver* m_pEngineDriver;
    CEngineArea* m_pEngineArea;
    CSettings* m_pEngineSettings;

    std::unique_ptr<LoopStack> m_loopStack;
    std::unique_ptr<TraceHandler> m_traceHandler;
    std::unique_ptr<EngineParadataDriver> m_paradataDriver; // non-null
    std::unique_ptr<Pre77Report::ReportManager> m_pre77reportManager;

private:
    CsDriver*           m_pCsDriver;                    // victor May 16, 01
    bool                m_bUseVector; // RHF Jul 06, 2001


// --- Methods -------------------------------------------------------------
public:
    // --- constructor/destructor/initialization
    CIntDriver(CEngineDriver& engine_driver);
    ~CIntDriver();

    void StartApplication();
    void StopApplication();

private:
    void AddIntDriverInstructions();

    void EvaluateApplicationStartupJavaScript();

public:
    LoopStack& GetLoopStack();

    EngineData& GetEngineData() { return *m_engineData; }

    // --- execution flags
    void    Enable3D_Driver()                  { m_bUse3D_Driver = true; }          // victor May 16, 01
    void    Disable3D_Driver()                 { m_bUse3D_Driver = false; }         // victor May 16, 01
    bool    IsUsing3D_Driver() const           { return m_bUse3D_Driver; }          // victor May 16, 01
    void    SetRequestIssued( bool bX = true ) { m_bRequestIssuedByEngine = bX; }   // victor May 16, 01
    bool    GetRequestIssued() const           { return m_bRequestIssuedByEngine; } // victor May 16, 01

    template<typename T>
    void MakeFullPathFileName(T& filename) const;
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
    double  GetSingVarFloatValue( const VART* pVarT ) const;
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
    void    PrepareForExportExec(int iSymbol, ProcType proc_type);

    template<typename T = double>
    T evalexpr(int program_index);

    std::wstring EvalAlphaExpr(int program_index) { return UTF8_TODO::GetWide(*Evaluate<SharableString>(program_index)); }
    CString EvalAlphaExprCS(int program_index)    { return UTF8_TODO::GetCString(*Evaluate<SharableString>(program_index)); }

    // --- basic interpreter functions
public:
    Engine::Value CallUserFunction(UserFunction& user_function, UserFunctionArgumentEvaluator& argument_evaluator);
    void ExecuteCallbackUserFunction(int field_symbol_index, UserFunctionArgumentEvaluator& argument_evaluator) override;
private:
    std::unique_ptr<UserFunctionArgumentEvaluator> EvaluateArgumentsForCallbackUserFunction(int program_index, FunctionCode function_code) override;

private:
    void    ResetSymbol(Symbol& symbol, int initialize_value = -1);
public:
    double  exsymbolreset(int iExpr);
    double  expersistentsymbolreset(int iExpr);

    double  exsvar(int iExpr);
    double  exmvar(int iExpr);
    double  exmvar( MVAR_NODE* ptrvar );                // rcl, Jul 22, 2004
    Engine::Value exavar(int iExpr);

    SharableString extavar(int iExpr);
    double  excpt(int iExpr);
    double  exif(int iExpr);
    double  exbox(int iExpr);
    Engine::Value excharobj(int program_index);

    double  excpttbl(int iExpr);

    double  exnoopIgnore_numeric(int iExpr);
    double  exnoopIgnore_string(int iExpr);
    double  exnoopAbort(int iExpr);
    double  exnoopAbortPlaceholderForFutureFunction(int iExpr);

    double  extvar(int iExpr);

    Engine::Value ex_UserFunction_call(int program_index);
    Engine::Value ex_invoke(int program_index);
    template<typename T>
    InterpreterExecuteResult RunInvoke(std::string_view function_name_sv, const T& variable_arguments, CancelFlag* cancel_flag);

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
    void    BatchExSetSkipping(int iSymSource, int iOccSource, ProcType source_proc_type, // victor Mar 26, 01
                               int iSymTarget, int iOccTarget, ProcType target_proc_type);

    //////////////////////////////////////////////////////////////////////////
    // new 3D versions
    void    BatchExSetSkipping(C3DObject& objSource, ProcType source_proc_type, // rcl, Sept 04, 04
                               C3DObject& objTarget, ProcType target_proc_type);

    bool    BatchExScanOccur(std::vector<int>& aDirtySymbol, std::vector<int>& aDirtyOccur,    // victor Mar 14, 01
                             GROUPT* pGroupT, int iItemCheck, int iOccCheck, ProcType target_proc_type);
    void    BatchExDisplayDirty(std::vector<int>& aDirtySymbol, std::vector<int>& aDirtyOccur, // victor Mar 14, 01
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
    CString CheckAtSymbol_ExpandText(const CString& csText, bool& bSomeError);

public:
    double exendcase(int iExpr);
    double exuniverse(int iExpr);
    double exskipcase(int iExpr);
    double ex_exit(int program_index);

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

    double   exnoccurs(int iExpr);
    double   exsoccurs(int iExpr);
    double   exsoccurs_pre80(int iExpr);
    int      exsoccurs(const SECT* pSecT, bool use_rules_for_binary_dict_items = false); // RHF May 14, 2003

    template<typename T> void AssignValueToVART(int variable_compilation, T value);
    template<typename T> T EvaluateVARTValue(int variable_compilation);
    template<typename T> Engine::Value ModifyVARTValue(int variable_compilation, const std::function<void(T&)>& modify_value_function,
                                                       std::unique_ptr<Paradata::FieldInfo>* paradata_field_info = nullptr);

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

    double  exdisplay(int program_index);
    double  exerrmsg(int program_index);
    double  exwrite(int program_index);
    double  exmaketext(int program_index);
    double  exlogtext(int program_index);
    double  exwarning(int program_index);

    Engine::Value exedit(int iExpr);

    double  ex_paradata(int program_index);
    double  exsqlquery(int program_index);
    double  exsqlquery(int program_index, const std::function<double(sqlite3*, const std::string&)>* setreportdata_callback);
    double  expre77_setreportdata(int iExpr);
    double  expre77_report(int iExpr);

    void RegisterSqlCallbackFunctions(sqlite3* db);
    void ProcessSqlCallbackFunction(UserFunction& user_function, void* void_context, int iArgC, void* void_ppArgV);

    double ex_syncconnect(int program_index);
    double ex_syncdisconnect(int program_index);
    double ex_syncdata(int program_index);
    double ex_syncfile(int program_index);
    double ex_syncserver(int program_index);
    double ex_syncapp(int program_index);
    double ex_syncmessage(int program_index);
    double ex_syncparadata(int program_index);
    double ex_synctime(int program_index);

    double ex_getbluetoothname(int program_index);
    double ex_setbluetoothname(int program_index);

    double exsavepartial(int iExpr);
    Engine::Value ex_getoperatorid(int program_index);
    double exsetoperatorid(int iExpr);

    double  exdemode(int iExpr);
    double  exclrcase(int iExpr);

    Engine::Value exaccept_pre77(int iExpr);
    Engine::Value exprompt_pre77(int iExpr);

    double  excountvalid(int iExpr); // 20091202

    double  exdeckarray(int iExpr); // for getdeck and putdeck

    double  ex_getlanguage(int program_index);
    double  ex_setlanguage(int program_index);
    double  ex_tr(int program_index);

    double  exuserbar(int iExpr); // 20100414

    double  exmessageoverrides(int program_index);

    double  ex_trace(int program_index);

    double ex_getcapturetype(int program_index);
    double ex_setcapturetype(int program_index);
    double ex_setcapturepos(int program_index);

    double  ex_changekeyboard(int iExpr);

    double  exorientation(int iExpr);       // 20100618

    double  exgps(int iExpr);               // 20110223
    std::unique_ptr<Paradata::Event> CreateParadataGpsEvent(std::string_view event_type_sv, std::string_view event_information_sv);

    double  exgetrecord(int iExpr);     // 20110302

    double  ex_setoutput(int program_index);

    double  exfreealphamem(int program_index);

    double  exsetvalue(int iExpr);      // 20140228
    double  exgetvalue(int iExpr);      // 20140422
    Engine::Value ex_getvaluealpha(int program_index);
    VARX*   AssignParser(int iExpr, std::unique_ptr<CNDIndexes>& pTheIndex, int* aIndex); // 20140422

    SharableString GetValueLabel(const VART* pVarT, const std::variant<double, SharableString>& value);
    double  exgetvaluelabel(int iExpr);
    double  exvariablevalue(int program_index);

    double  exxtab(int iExpr);
    double  extblcoord(int iExpr); // tblrow, tblcol, tbllay
    double  extblsum(int iExpr);
    double  extblmed(int iExpr);
    double  exfilename(int iExpr);

    Engine::Value ex_key_currentkey(int program_index);
    double  exkeylist(int iExpr);
    double  exfind_locate(int program_index);
    double  exdictaccess(int program_index);
    double  exdictaccess(const Nodes::SetAccessFirstLast& set_access_first_last_node);
    double  exretrieve(int iExpr);

    // data access functions
public:
    bool IsDataAccessible(const Symbol& symbol, bool issue_error_if_inaccessible) override;
    void EnsureDataIsAccessible(const Symbol& symbol); // calls IsDataAccessible and throws an exception on error
private:
    double exDataAccessValidityCheck(int program_index);

    // EngineDictionary functions
    double exdictcompute(int iExpr);


    // Case functions
public:
    Engine::Value exCase_view(int program_index);
    Engine::Value exCase_view(const DICT& dictionary, const ViewerOptions* viewer_options) override;


    // Item functions
public:
    double exItem_getValueLabel(int program_index);
    double exItem_hasValue_isValid(int program_index);

private:
    std::tuple<EngineItemAccessor*, bool> GetEngineItemAccessorAndVisualValueFlag(const Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node, int visual_value_argument_index);

private:
    // evaluates the implicit and explicit components of the item's subscript
    EvaluatedEngineItemSubscript EvaluateEngineItemSubscript(const EngineItem& engine_item, const Nodes::ItemSubscript& item_subscript_node) override;

    template<typename SymbolT>
    SymbolT GetFromSymbolOrEngineItemWorker(const SymbolReference<SymbolT>& symbol_reference, bool use_exceptions);
    Symbol* GetFromSymbolOrEngineItemWorker_INTERPRETER_DLL_TODO(const SymbolReference<Symbol*>& symbol_reference, bool use_exceptions) override;
    std::shared_ptr<Symbol> GetFromSymbolOrEngineItemWorker_INTERPRETER_DLL_TODO(const SymbolReference<std::shared_ptr<Symbol>>& symbol_reference, bool use_exceptions) override;

    // evaluates the subscript for the item (implicit if subscript_text is null), throwing an exception if the subscript is invalid
    Symbol& GetWrappedEngineItemSymbol(EngineItem& engine_item, const char* subscript_text);

    const Symbol* GetCurrentProcSymbol() const;

    int CalculateEngineItemImplicitOccurrence(const EngineItem& engine_item, bool get_record_occurrence);


    // named frequency and unnamed frequency functions
public:
    double ex_Freq_unnamed(int program_index);
    double ex_Freq_clear(int program_index);
    double ex_Freq_save(int program_index);
    double ex_Freq_tally(int program_index);
    Engine::Value ex_Freq_view(int program_index);
    Engine::Value ex_Freq_view(const NamedFrequency& named_frequency, const ViewerOptions* viewer_options, int frequency_parameters_node_index) override;
    double ex_Freq_var(int program_index);
    double ex_Freq_compute(int program_index);

private:
    std::unique_ptr<FrequencyDriver> m_frequencyDriver;


    // dynamic logic evaluation functions
public:
    InterpreterExecuteResult EvaluateLogic(SharableString logic, CancelFlag& cancel_flag);


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

    double  ex_open(int program_index);
    double  ex_close(int program_index);
    double  ex_setfile(int program_index);
    bool    ex_setfile_dictionary(EngineDictionary& engine_dictionary, const ConnectionString& connection_string, bool create_new, bool open_or_create);
    bool    ex_setfile_dictionary(DICT* pDicT, const ConnectionString& connection_string, bool create_new, bool open_or_create);

private:
    void    EntryInputRepositoryChangingActions();

public:
    double  exgetcaselabel(int iExpr);
    double  exsetcaselabel(int iExpr);

    double  exsetattr(int iExpr);
    double  exfor_group(int iExpr);                   // RHC Aug 17, 2000
    double  exfor_relation(int iExpr);
    //////////////////////////////////////////////////////////////////////////
    // to ease exdofor_relation max calculation
    double  getMaxIndexForVariableUsingStack( int iVar, REL_NODE* pRelNode );     // rcl, Dec 18, 2004
    double  getMaxIndexForVariableUsingStack( VART* pVarT, REL_NODE* pRelNode );  // rcl, Dec 18, 2004
    //////////////////////////////////////////////////////////////////////////
    double  exdofor_relation( FORRELATION_NODE* pFor, double* dTableWeight=NULL, int* iTabLogicExpr=NULL, LIST_NODE* pListNode=NULL  ); // RHF Jul 03, 2002
    Engine::Value exfucall(int iExpr);                // RHF Aug 21, 2000

    double  exupdate(int iExpr);                      // RHF Nov 17, 2000
    Engine::Value exgetbuffer(int iExpr);

private:
    std::tuple<std::shared_ptr<NamedReference>, int> EvaluateNoteReference(const FNNOTE_NODE& note_node);
    std::unique_ptr<std::string> EvaluateNoteOperatorId(const FNNOTE_NODE& note_node, int field_symbol);
public:
    double  exgetnote(int program_index);
    double  exeditnote(int program_index);
    double  exputnote(int program_index);

    // both "get symbol" methods throw an exception if the symbol is not found;
    // the "evaluated" version allows the specification of subscripts and returns the base symbol (non-null), as well as the wrapped symbol (potentially null)
    Symbol& GetSymbolFromSymbolName(std::string_view symbol_name_sv, SymbolType preferred_symbol_type = SymbolType::None);
    std::tuple<Symbol*, Symbol*> GetEvaluatedSymbolFromSymbolName(const std::string& symbol_name_and_potential_subscript, SymbolType preferred_symbol_type = SymbolType::None);

    Engine::Value exgetlabel(int iExpr);

    std::string EvaluateOccurrenceLabel(const Symbol& symbol, const std::optional<int>& zero_based_occurrence);
    Engine::Value ex_getocclabel(int program_index);
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
    double  ex_filecopy(int program_index);
    double  ex_filerename(int program_index);
    double  exfilesize(int iExpr);
    double  exfileempty(int iExpr);
    double  exfileconcat(int iExpr);
    double  exfileread(int iExpr);
    double  exfilewrite(int iExpr);

    double  exfiletime(int iExpr);
    double  exdirexist(int iExpr);
    double  exdircreate(int iExpr);
    double  exdirdelete(int program_index);

private:
    ParameterManager::Parameter GetSetPropertyParser(int program_index, std::set<int>& symbol_set,
                                                     std::variant<double, std::string>* out_value = nullptr);
public:
    std::string GetProperty(ParameterManager::Parameter parameter, std::set<int>* symbol_set = nullptr);
    double ex_getproperty(int program_index);
    double ex_setproperty(int program_index);
    double ex_protect(int program_index);

    Engine::Value ExExecSystem(int iExpr);
    std::unique_ptr<Paradata::ExternalApplicationEvent> ExExecCommonBeforeExecute(FunctionCode source, const std::string& command, int flags);
    bool ExExecCommonExecute(const std::string& command, int flags);
    Engine::Value ExExecCommonAfterExecute(FunctionCode source, int flags, bool success, std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event);
    Engine::Value ExExecPFF(int iExpr);
    Engine::Value ExExecPFF(std::variant<LogicPff*, std::string> logic_pff_or_pff_file_path, std::optional<int> flags = std::nullopt);

    double exwhile(int iExpr);
    double ex_do(int program_index);
    double exfornext(int iExpr);
    double exforbreak(int iExpr);

public:
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

    int SelectDlgHelper_pre77(int iFunCode, const CString& csHeading, const std::vector<std::vector<CString>*>* paData,
                              const std::vector<CString>* paColumnTitles, std::vector<bool>* pbaSelections,
                              const std::vector<PortableColor>* row_text_colors) override;

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
    void RunGlobalOnFocus(int symbol_index);

private:
    std::map<const DictValue*, int> m_deckarrayIndexMappings;

public:
    SharableString EvaluateCapiText(const std::string& language_name, const bool is_question, const int symbol_index);
private:
    SharableString EvaluateCapiText(const CapiQuestion& question, const Symbol& symbol, const std::string& language_name, bool is_question);

    template<typename T>
    auto EvaluateCapiLogic(const Symbol& symbol, int program_index);

    SharableString EvaluatePre81CapiText(const Symbol& symbol, const CapiQuestion& question, const CapiText& capi_text);

    SharableString EvaluateTextFill(int program_index) override;

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
    SharableString EvaluateUserMessage(int message_node_index, FunctionCode function_code, int* out_message_number = nullptr) override;
    double DisplayUserMessage(int message_node_index);

public:
    SharableString EvaluateVariableParameter(const std::variant<double, SharableString>& value, int value_expression, bool request_label);

    // --- miscellaneous
public:
    std::string ProcName();

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

    template<typename T = bool>
    T ExecuteProgramStatements(int program_index);
    Engine::Value ExecuteInstructions(int program_index) override;

    // Runs the callback function and returns the evaluation result, including a flag
    // indicating whether a movement or program control action has occurred.
    // Any thrown ProgramControlException exceptions will be stored and can be processed
    // by calling RethrowProgramControlExceptions.
    template<typename CF>
    InterpreterExecuteResult Execute(CF callback_function);


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
    bool HasSpecialFunction(SpecialFunction::Code special_function) override;
    Engine::Value ExecSpecialFunction(int symbol_index, SpecialFunction::Code special_function, std::vector<std::variant<double, SharableString>> arguments) override;

    bool ExecuteOnSystemMessage(MessageType message_type, int message_number, const std::string& message_text);

    enum class SetLanguageSource { SystemLocale, Pff, Logic, Interface };
    bool SetLanguage(std::string_view language_name_sv, SetLanguageSource language_source, bool show_failure_message = false);
    void SetStartupLanguage();
    std::vector<Language> GetLanguages(bool include_only_capi_languages = true) const;

    SyncClient& GetSyncClient();

    std::unique_ptr<C3DObject> ConvertIndex(const CaseItem& case_item, const ItemIndex& item_index);
    std::unique_ptr<C3DObject> ConvertIndex(const CaseItemReference& case_item_reference);
    void ConvertIndex(const C3DIndexes& the3dObject, ItemIndex& item_index);
    void ConvertIndex(const C3DObject& the3dObject, ItemIndex& item_index);

    // imputation
public:
    double ex_impute(int program_index);
    template<typename T, typename TIN, typename TIO> double eximpute_worker(const TIN& impute_node, TIO imputation);
private:
    std::unique_ptr<ImputationDriver> m_imputationDriver;


    // --------------------------------------------------------------------------
    // INTERPRETER_DLL_TODO...
    // --------------------------------------------------------------------------
    Engine::Value evalexpr_INTERPRETER_DLL_TODO(Engine::Value (CIntDriver::*instruction)(int), int program_index) override;
    double evalexpr_INTERPRETER_DLL_TODO(double (CIntDriver::*instruction)(int), int program_index) override;
    void RegisterAndLogEvent_INTERPRETER_DLL_TODO(std::shared_ptr<Paradata::Event> event, const void* instance_object = nullptr) override;
    void IssueMessageWorker(MessageType message_type, int message_number, ...) override;
    std::string GetFormattedMessageWorker(int message_number, ...) override;
    InterpreterExecuteResult Report_Evaluate_INTERPRETER_DLL_TODO(Report& report) override;
    Engine::Value RunSoonToBeRemovedFeature(std::string_view feature_sv, int program_index, void* tag) override;
    int Get_m_iExSymbol_INTERPRETER_DLL_TODO() override { return m_iExSymbol; }
    bool IsExecutionInterrupted() const override;
    EngineParadataDriver& GetEngineParadataDriver_INTERPRETER_DLL_TODO() override;
    Engine::Value ExExecPFF_INTERPRETER_DLL_TODO(LogicPff& logic_pff) override;
    FrequencyDriver* GetFrequencyDriver_INTERPRETER_DLL_TODO() override;
    void AssignValueToVART_INTERPRETER_DLL_TODO(int variable_compilation, double value) override;
    void AssignValueToVART_INTERPRETER_DLL_TODO(int variable_compilation, SharableString value) override;
    double EvaluateVARTValue_double_INTERPRETER_DLL_TODO(int variable_compilation) override;
    SharableString EvaluateVARTValue_SharableString_INTERPRETER_DLL_TODO(int variable_compilation) override;
    Engine::Value ModifyVARTValue_INTERPRETER_DLL_TODO(int variable_compilation, const std::function<void(double&)>& modify_value_function, std::unique_ptr<Paradata::FieldInfo>* paradata_field_info = nullptr) override;
    Engine::Value ModifyVARTValue_INTERPRETER_DLL_TODO(int variable_compilation, const std::function<void(SharableString&)>& modify_value_function, std::unique_ptr<Paradata::FieldInfo>* paradata_field_info = nullptr) override;
    int SymbolTableSearch_INTERPRETER_DLL_TODO(std::string_view full_symbol_name_sv, SymbolType preferred_symbol_type,
                                               const std::vector<SymbolType>* allowable_symbol_types) const override;


private:
    // ExShow
    std::vector<CString> m_aShowLines;

    std::unique_ptr<SelcaseManager> m_selcaseManager;

    std::shared_ptr<SyncObjects> m_syncObjects;

    std::unique_ptr<KeyboardLoader> m_keyboardLoader; // non-null
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T/* = double*/>
T CIntDriver::evalexpr(const int program_index)
{
    return static_cast<T>(ExecuteInstruction(program_index).get<double>());
}
