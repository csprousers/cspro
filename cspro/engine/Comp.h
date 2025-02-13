#pragma once

//---------------------------------------------------------------------------
//  File name: Comp.h
//
//  Description:
//          Header for engine' functions compiler
//
//  History:  Date       Author  Comment
//            ----------------------------------------------------
//            15 Nov 99   RHF     Basic conversion
//            16 Dec 99   RHF     Handling many dimensions
//            05 Oct 00   vc      Isolating FNDISPLAY/FNERRMSG thru cfun_fnMsg
//            04 Sep 00   RHC     Adds function to compile BREAK and NEXT statements
//            26 Mar 01   vc      Adding VerifyAtTargetSymbol
//            04 Apr 01   vc      Tailoring for RepMgr compatibility
//            21 Jul 04   rcl     Change (some) method names to more meaninful alternatives
//                                cfun_fn2 -> cfun_compile_count
//                                cfun_fn3 -> cfun_compile_sum
//            25 Jul 04   rcl     Adds special parameter to varsanal (bCompleteCompilation)
//                                to be used in sum() compilation. Add varsanal method not to use
//                                (evil) default parameters
#ifdef USE_BINARY
//            06 Oct 05   rcl     Binary Serialization of compiled data
#endif
//---------------------------------------------------------------------------

// Prevents "C++ Exception Specification ignored" message // RCL 2004
#pragma warning(disable:4290)

// These classes are defined here:

class CEngineCompFunc;
class CLinkSubTable;
class CodeFile;
struct EXP_HEADER_NODE;

#include <engine/Engdrv.h>
#include <engine/COMPILAD.H>

#include <engine/NODES.H>

#include <ZTBDO/cttree.h>

#include <zEngineO/Compiler/LogicCompiler.h>


//////////////////////////////////////////////////////////////////////////

#include <engine/ttype.h>
#include <engine/ctterm.h>

#include <engine/ReadyFlags.h>
#include <engine/3dException.h>

struct CapiLogicParameters;

#define TEMP_VIRTUAL override // COMPILER_DLL_TODO


// Definitions useful for CEngineCompFunc::grpanal
const bool ALLOW_PARENTHESIS        = true;
const bool DO_NOT_ALLOW_PARENTHESIS = false;
const int  CHECK_LIMITS_NORMAL      = 0;

// Definitions used in varsanal
//
const bool COMPLETE_COMPILATION     = true;
const bool NOT_COMPLETE_COMPILATION = false;

const bool TRY_TO_COMPLETE_DIMS        = true;
const bool DO_NOT_TRY_TO_COMPLETE_DIMS = false;

// Definitions used in grpanal
//
const bool ALLOW_DIM_EXPR = true;
const bool DO_NOT_ALLOW_DIM_EXPR = false;

const int CHECKLIMIT_NORMAL = 0;
const int CHECKLIMIT_INSERT = 1;
const int CHECKLIMIT_DELETE = 2;

const bool FROM_FOR_GROUP = true;
const bool NOT_FROM_FOR_GROUP = false;

// NOT_COMPLETE_COMPILATION used in functions like count()
// that need a variable (item) inside its function like
//   count( this_item(a,b) )
// but the number of indexes specified (a and b, in the example)
// has to be 1 less than what 'this_item' needs to be fully
// specified.
//

// CONSIDER_ONLY_FOR_VARIABLE is used as a parameter for internal method
// searchInForStackThisGroupOrVar()
//
const bool CONSIDER_ONLY_FOR_VARIABLE = true;

// ctrange() consts
const bool ALLOW_DOUBLE_CONSTS = true;
const bool DO_NOT_ALLOW_DOUBLE_CONSTS = false;

namespace Logic { class Preprocessor; class ProcDirectory; }


typedef   int (CEngineCompFunc::*pCompileForInFunction)();


class CEngineCompFunc : public LogicCompiler
{
private:
    int     m_iErrors;
public:
    void    resetErrors()                    { m_iErrors = 0; }
    void    incrementErrors(int iAmount = 1) { m_iErrors += iAmount;   }
    int     getErrors()                      { return m_iErrors; }
private:
    bool    m_bIdChanger; // RHF Jul 02, 2005
public:
    int     m_Flagcomp;               // to activate code generation
    int     m_LvlInComp;              // Level of object compiled
    int     m_ProcInComp;             // Proc  currently compiled

    bool    m_bcvarsubcheck;          // flag for variable subscript checking
    int     m_icGrpIdx;               // group in which variable subscript check has to be done

    FNGR_NODE* m_pCuroccGrpIdx;       // 20091028 for sum/count/etc.(A where ... = curocc()) so curocc refers to A

    std::tuple<int, bool> m_loneAlphaFunctionCallTester; // 20140422 count + 2024 flag indicating this is a standalone call to a function

private:
    std::unique_ptr<Logic::Preprocessor> m_preprocessor;

    std::unique_ptr<Logic::ProcDirectory> m_procDirectory;

public:
    CEngineCompFunc*    m_pEngineCompFunc;
    CEngineDefines*     m_pEngineDefines;
    CEngineDriver*      m_pEngineDriver;
    CEngineArea*        m_pEngineArea;
    CSettings*          m_pEngineSettings;

public:
    CEngineCompFunc( CEngineDriver* pEngineDriver );
    virtual ~CEngineCompFunc();

private:
    std::map<int, int> m_aCtCoordRepeatSymbol; // RHF Jun 20, 2002

public:
    int CompileProc(Symbol* symbol);

    void CompileExternalCode();
    virtual void CompileExternalCode(const CodeFile& code_file);

public:
    int     GetRelationSymbol(int symbol_index);

public:
    int     genSVARNode( int iSym, SymbolType eType ); // RHF Aug 07, 2000 Add iTokId parameter
    int     genMVARNode( int iSymVar, MVAR_NODE* pMVarNodeAux );

    void    FillImplicitIndex( MVAR_NODE* pMVarNode, int iVarT, int iDim, int iGroupIndex );

private:
    // expresc.cpp
    int     crelalpha();

    // bool CEngineCompFunc::forStackHasElements()
    // returns true if stack has elements, false otherwise
    // useful to prevent some useless search
    bool    forStackHasElements();
    // searchInForStackThisGroup() and searchInForStackThisGroupOrVar()
    // are helper methods used in varsanal, grpanal and relanal
    // rcl, Jul 28, 2004
    int     searchInForStackThisGroup( int iGroupIndex );            // rcl, Jul 28, 2004
    int     searchInForStackThisGroupOrVar( int iGroupIndex,
                                    int iVarT,
                                    CDimension::VDimType vType );    // rcl, Jul 28. 2004

    int     searchInForStackThisGroupOrVar( int iGroupIndex,
                                    int iVarT,
                                    CDimension::VDimType vType,
                                    bool bConsiderOnlyForVariable ); // rcl, Jul 28. 2004

public:
    // VarAnalysis class
    // keeps temporal information for varsanal implementation
    // Originally varsanal code was several inmeneagable pages or source
    // code, now it was divided in several internal methods, that
    // uses a VarAnalysis object to share information
    class VarAnalysis
    {
        int  m_aSubindexExpr[DIM_MAXDIM];
        bool m_abIsExprConstant[DIM_MAXDIM];
        bool m_abIsImplicit[DIM_MAXDIM]; // to be able to "undo" some operations
        void setIndexValue( int iIndex, int iValue );
    public:
        VarAnalysis( int iNumDim );
        virtual ~VarAnalysis() {}
        void addIndexValue( int iValue );
        void addImplicitIndex();
        void setPreviousAsConstant( int iValue ); // change previous inserted value to specified
        void setPreviousAsConstant();             // do not change previous inserted value
        bool isConstant( int iIndex );
        int  m_iSubIndexCount;
        int  m_iNumDim;
        int  m_iExplicitIndexes;

        bool canAddAnotherIndex( bool bCompleteCompilation );
        bool hasTooManyIndexes();

        bool thisIndexIsSpecified(int iIndex);
        bool allIndexesAreSpecified();
        bool allIndexesAreExplicit();
        int  howManyIndexesSpecified() const;

        // checks if index limits are correct.
        // it is incorrect to have
        //  m_iSubIndexCount > m_iNumDim, i.e. more indexes than needed or
        //  m_iExplicitIndexes < m_iSubIndexCount    [some implicit indces]
        //     and !allIndexesSpecified()
        //   [when an implicit index is used, all indexes must be specified]
        bool indexLimitsAreCorrect( int* pError );

        void copyTo( int* aIndexes, int* aTypes );
        void copyTo_OnlyOneIndexDirect( int* aIndexes, int* aTypes, int iIndex );
        void copyTo_OnlyOneIndex( int* aIndexes, int *aTypes,
                                  int iToIndex, int iFromIndex );
        bool changeImplicitToConstOk( int iIndex, int iValue );

        VART* m_pVarT;
        void setVarT( VART* pVarT ) { m_pVarT = pVarT; }
        VART* getVarT() { return m_pVarT; }
    };

private:
    // Check the use of varsanal to compile
    // "variables" that are really UserFunction, WorkVariable or SingleVariables
    //
    // if it returns false, then *piVarNode has a VarNode changed to return
    // if it returns true, varsanal can continue
    bool    varsanal_basicCheck( int* piVarNode, int fmt );// rcl, Sept 30, 2004

    // varsanal_precompile() parses input and fills VarAnalysis structure for
    // code generation
    //
    // status:
    //   variable  - read
    //   TOKLPAREN - read
    //   ready to read an expression or a TOKRPAREN
    void    varsanal_precompile( VarAnalysis& va, bool bCompleteCompilation );// rcl, Sept 30, 2004

    void    symbol_checkBelongToGroup(Symbol& symbol);

    // tryInheritance() used by generateVarNode()
    void    varsanal_tryInheritance( MVAR_NODE* pMVarNode, CReadyFlags& readyFlagSet );
    void    varsanal_generateVarNode( VarAnalysis& va, int iVarT, MVAR_NODE* pMVarNode,
        bool bTryToComplete );

    std::vector<MVAR_NODE*> m_varsanalData;

public:
    // getLocalVarNodePtr() only used [for now] in crosstab when
    // extra information about generated MVAR_NODES, but no GENCODE is availabe
    // [so, no PPT is available]
    // MVAR_NODE* getLocalVarNodePtr( int iWhichOne );
    //                 iWhichNode must be between 0 and (m_varsanalData's size) - 1.
    MVAR_NODE* getLocalMVarNodePtr( int iWhichOne ); // rcl, Nov 2004
    int getNewMVarNodeIndex();

    int     varsanal( int fmt, bool bCompleteCompilation, bool* pbAllIndexesSpecified, bool bbTryToComplete = true );
    int     varsanal( int fmt, bool bCompleteCompilation );
    int     varsanal( int fmt );

    // grpanal()
    // iCheckLimit can be: CHECKLIMIT_NORMAL, CHECKLIMIT_INSERT or CHECKLIMIT_DELETE
    int     grpanal( int iSymGroup, bool bAllowDimExpr = ALLOW_DIM_EXPR, int iChecklimit = CHECKLIMIT_NORMAL,
                     bool bFromForGroup = NOT_FROM_FOR_GROUP );
    int     relanal( int iSymRel, bool bAllowDimExpr=true );

    int     blockanal(int block_symbol_index);

    int     tvarsanal();

    std::vector<const DictNamedBase*> GetImplicitSubscriptCalculationStack(const EngineItem& engine_item) const override;

    int     rutfunc();

    int     cfun_fnmaxocc();
    int     cfun_fninvalueset();
    int     cfun_fnexecsystem();

    int     cfun_fnitemlist(); // 20091203 for functions that list items and records but still use FNN_NODE
    int     cfun_fndeck(); // 20100120 for putdeck and getdeck functions
    int     cfun_fncapturetype(); // 20100608

public:
    int     cfun_fnoccs(); // 20141006
    int     cfun_fnnote();
    int     cfun_fnstrparm();
    int     cfun_fnproperty();

    // compile_parentGrouptSpec
    //
    // - short description:
    // helper method to be used in cfun_compile_count()
    // - long description:
    // used in compile_count to analyze the group specification
    // - basic logic:
    // if( Parent != 0 )
    //     Use grpanal allowing parenthesis
    // else
    //     Use grpanal without allowing parenthesis
    //     (Syntax error if used)
    //
    // count( group where condition )         <--
    // count( group(a,b,c) where condition )  <--
    //
    // depends on group situation
    //
    // rcl, Jul 22, 2004

    int     compile_parentGrouptSpec( int iSymGroup, GROUPT* pGroupParent );

    int     cfun_compile_count(); // used to be: cfun_fn2()
    int     cfun_compile_sum();   // used to be: cfun_fn3()
    int     cfun_fnh();
    int     cfun_fng();                           // RHF Aug 25, 2000
    int     cfun_fngr();                          // RHC Oct 29, 2000
    int     cfun_fn4();
    int     cfun_fn6();
    int     cfun_fn8();
    int     cfun_fnc();
    int     cfun_fnb();

    int     cfun_fns();

    int     cfun_fntc();

    int     cfun_fnins();       // Chirag    Jul 22, 2002
    int     cfun_fnsrt(); //Chirag Sep 11, 2002

private:
    int     old_do_cfun_fntc( CTAB* ct, int ndim, void* pvoid );
    int     do_cfun_fntc( CTAB* pCtab, int iNumDim, FNTC_NODE* pFntcNode );
    int     compile_expr( CTAB* ct, CTTERM* pctt, int ndim, void* pvoid );
    CTTERM* default_lastsubtable( CTAB* ct, int ndim, void* pvoid );
    CTTERM* search_subtable( CTAB* ct, int ndim, void* pvoid, int ivar );
    int     default_expr( CTAB* ct, CTTERM* pctt, int ndim, void* pvoid );
    int     default_expr1( CTAB* ct, CTTERM* pctt, int ndim, void* pvoid );
    int     default_expr2( CTAB* ct, CTTERM* pctt, int ndim, void* pvoid );

    CTTERM* ctmaketerm( int ictnode );
    int     ctnterm( int ictnode );
    void    ctaddterm( CTTERM* ctt, int* ntc, int nodeid, int ctnode, int prevcells );

    // instruc.cpp
public:
    int     instruc(bool create_new_local_symbol_stack = true, bool allow_multiple_statements = true);
private:
    void    CheckIdChanger(const VART* pVarT) TEMP_VIRTUAL; // RHF Jul 03, 2005
    int     CompileComputeInstruction();

    int     CompileAskStatement(bool bIsTargetlessSkip = false);
    int     CompileSkipStatement( int* code );          // RHC Aug 17, 2000
    int     CompileMoveStatement(bool bFromSelectStatement = false); // RHF Dec 09, 2003
    int     CompileReenterStatement( bool bNextTkn=true );            // RHF Aug 06, 2006 Add bNextTkn
    int     CompileAdvanceStatement();            // RHC Aug 17, 2000
    int     CompileBreakBy();
    int     CompileEnter();

    std::vector<CBreakById> CompileBreakByList(); // RHF Apr 16, 2003

    int     CompileProgramControl();

    int     CompileHas(int iVarNode); // 20120429

    // compiler' utility functions
    void    VerifyAtTargetSymbol( int iSymbol );        // victor Mar 26, 01
    int     VerifyTargetSymbol( int iSymbol, bool bMustBeInForm = true ); // victor Mar 08, 01 {add 2nd arg}

    void    MarkAllInSectionUsed(SECT* pSecT) TEMP_VIRTUAL;

private:

    // compctab.cpp
    int     ct_title( CTAB* ct,int flag );
    int     ctexpdep( CTAB* ct );
    int     ctvardep( CTAB* ct, int i_dep );

    void    ctauxalloc();
    void    ctauxfree();
    int     ctexp1( int iDim, int* ctbase1,int ctmxent1, int* ctnext1 );
    int     ctexp2( int* ctbase1, int ctdesp1, int ctoptions, int* ctbase2, int ctmxent2, int* ctnext2 );
    int     ctadd();
    int     ctmul();

    int     ctvar();
    int     ctloadvar( const VART* pVarT, const ValueSet* pValueSetVar );

    //////////////////////////////////////////////////////////////////////////
    // Changes to method signatures, and changes to type used for range limits
    // ["int" has been changed to "double"]
    // rcl, Apr 2005
    // {

    // ctrange( int iSpecialValuesFlags, int *piCollapsed )
    //    Changed signature from several bool parameters to 1 int flag parameter [iSpecialValuesFlags]
    // rcl, Apr 2005
    int     ctrange( int iSpecialValuesFlags, int* piCollapsed, bool bAllowDoubleConsts );
    int     ctaddrange( double iLow, double iHigh, int iCollapsed, int* piAuxNumCells );
    bool    ctnewrange( double rLow, double rHigh, int iCollapsed, int* piAuxNumCells );
    bool    ctinranges( double iLow, double iHigh );
    int     ctcomplete( int ctdesp, int ctoptions );
    int     ctaddinclude( int iCtNode );
    int     ctadjustparent( int* pNodeBase, int iCtNode, int iDesp ); // RHF Jul 01, 2004

    // }
    //////////////////////////////////////////////////////////////////////////

    // RHF INIC Jun 18, 2002
    void    CompileSubTablesList( CTAB* pCtab, int* pNodeBase[TBD_MAXDIM], std::vector<int>& aSubTables, bool bCheckUsed, std::vector<CLinkSubTable>* pLinkSubTables );
    void    CompileOneSubTable( int* pNodeBase[TBD_MAXDIM], std::vector<int>& aSubTables, bool bCheckUsed, std::vector<CLinkSubTable>* pLinkSubTables );

    void    GetSubTableList(  int* pNodeBase[TBD_MAXDIM],
                              std::vector<int>& aSubTables, bool bCheckUsed,
                              int   iDimType[TBD_MAXDIM],
                              int   iVar[TBD_MAXDIM][TBD_MAXDEPTH],
                              int   iSeq[TBD_MAXDIM][TBD_MAXDEPTH], int iTheDimType );

    int     CompileOneSubTableDim( int* pNodeBase[TBD_MAXDIM], int iDimNum, int iDim[TBD_MAXDIM], int iVar[TBD_MAXDIM][TBD_MAXDEPTH], int iSeq[TBD_MAXDIM][TBD_MAXDEPTH],
                                   int* iCatValues=NULL,
                                   int* iCatValuesSeq=NULL
                                    );
    // CompileUnit_GetNewToken: to be used inside CompileUnit method to
    // reduce code redundancy
    void    CompileUnit_GetNewToken( SymbolType theNewTypeToAsk, int iTokenType, int iErrorCode );
    void    CompileUnit( int* pNodeBase[TBD_MAXDIM] );

    // RHF END Jun 18, 2002

    void    CompileStat( int* pNodeBase[TBD_MAXDIM] );// RHF Jul 01, 2002
    int     GetNextStat();

public:
    int     ctopernode( int iLeft, int iRight, int iNodeType );

public:
    // comptbl.cpp
    int     tbl_expr();
    int     tbl_term();
    int     tbl_factor();
    int     ctblref();
    int     ctblsum();
    int     ctblmed();

    int     CompIntervals( int* pValues, std::vector<double>* pIntervals=NULL );// RHF Sep 12, 2002

    // compissa.cpp
    void    CompileApplication();
    void    CompileSymbolProcs();

    int CompileCapiLogic(const CapiLogicParameters& capi_logic_parameters);

public:
    // compall.cpp
    bool    CompSetBehavior( SETOTHER_NODE* pset_ot );

    // compctab.cpp
    int     comp_ctab( CTableDef::ETableType eTableType );

private:
    bool    CompConstantList( VART* pVarT, CString& csOccExpr );
    // --- export compiler (see compexport.cpp)
public:
    int     compexport();
    int     comp_export();
    int     excase_id();
    int     exEnsembledModel();  // The new model // RHF Oct 08, 2004
    bool    AddExportSection( int iSymSec, bool bUseAll, int iSecOccExpr );

    // --- export compiler - nodes generation
    char*   CompExpDelimiterNode( int iNodeType );
    char*   CompExpNewNode( int iNodeSize );
    bool    CompCaseId(EXP_HEADER_NODE* pHeadNode );

    int     FillImplicitIndexes( int iSymVar );

    // --- export compiler - miscellaneous checking
    bool    CompExpIsExternalOrWork( int iSymbol );
    csprochar    CompExpWhichDicType( int iSymbol );
    bool    CompExpIsValidVar( bool bCheckLevel=false );

public:
    void SetIdChanger(bool bIdChanger) { m_bIdChanger = bIdChanger; }
    bool GetIdChanger() const          { return m_bIdChanger; }

typedef struct
  {
    int     forType;
    int     forRelIdx;
    int     forVarIdx;
    int     forGrpIdx;

    int     iSuggestedDimension; // rcl, Jul 16, 2004

  } FORTABLE;

#define FORTABLE_MAX  100

    FORTABLE m_ForTable[FORTABLE_MAX];
    int      m_ForTableNext;

public:
    std::optional<int> ci_set();
    int     compall( int comptype );

    int     compctab( int mode, CTableDef::ETableType eTableType );

    int     rutasync(int symbol_index, const std::function<void()>* compilation_function = nullptr);
    int     rutcpttbl();

    int CompileForStatement(pCompileForInFunction pCompileFunction = nullptr);
    void CompileForRelation(int iVarIdx, int iRelIdx, pCompileForInFunction pCompileFunction = nullptr);
    void CompileForGroup(int iVarIdx, int iGrpIdx, pCompileForInFunction pCompileFunction = nullptr);

    int cfun_fnshow(void);
    int CompileShowList();

    bool CompileDeclarations();
    void CompileSetDeclarations();
    void CompileDictRelations();

    void CompileRelation();

    // expresc.cpp
public:
    bool    compobjOk(Symbol* symbol);
public:
    int  MakeRelationWorkVar(); // RHF Sep 19, 2001

    bool ScanTables();      // RHF Oct 31, 2002
    bool CheckProcTables(); // RHF Jan 23, 2003

    bool StopTableScan();// RHF Feb 28, 2003

// SERPRO_CALC
private:
    bool    m_bHasBreakBy; // RHF Apr 16, 2003

public:
    void    SetHasBreakBy(bool bHasBreakBy) { m_bHasBreakBy = bHasBreakBy; }
    bool    GetHasBreakBy() const           { return m_bHasBreakBy; }

    void    CheckUnusedFileNames();

    // BasicTokenCompiler overrides
protected:
    const LogicSettings& GetLogicSettings() const override;
    void FormatMessageAndProcessParserMessage(Logic::ParserMessage& parser_message, va_list parg) override;

    // overrides of LogicCompiler methods
    MessageManager& GetUserMessageManager() override;
    MessageEvaluator& GetUserMessageEvaluator() override;

    // BaseCompiler overrides
protected:
    void MarkSymbolAsUsed(Symbol& symbol) override;

public:
    Logic::ParserMessage CreateParserMessageFromIssaError(MessageType message_type, int message_number, std::wstring message_text) const;

    const Logic::ProcDirectory* CreateProcDirectory();
    const Logic::ProcDirectoryEntry* GetProcDirectoryEntry(int symbol_index) const;


    // (old compiler ... do not use) helper functions for setting up compilation nodes
private:
    int* OC_CreateCompilationSpace(int ints_needed)
    {
        int* compilation_space = nullptr;

#ifdef GENCODE
        if( m_pEngineCompFunc->m_Flagcomp )
        {
            compilation_space = m_engineData->logic_byte_code.AdvancePosition(ints_needed);

            if( compilation_space == nullptr )
                IssueError(4);
        }
#endif

        return compilation_space;
    }


    // --------------------------------------------------------------------------
    // from the old CBaseComp
    // --------------------------------------------------------------------------
public:
    int     m_iForRecordIdx;          // Added by Savy (R) 20090716
    int     m_iShowfnGroupIdx;        // Added by Savy (R) 20090731 //Fix for show() warning issue

    int     m_Flagvars;               // to request index for Mult vars
protected:
    int     m_SyntErr;                // Syntax error number (compiler)
public:
    void    clearSyntaxErrorStatus() { m_SyntErr = 0; }
    int     GetSyntErr() const       { return m_SyntErr; }
    int     SetSyntErr(int iError)   { m_SyntErr = iError; return iError; }

protected:
    bool    m_bIgnoreForPrecedence;          // use internal index in count
    int     m_iGroupToIgnoreForPrecedence;   // if using internal index, specify here the index to use
public:
    void    useForPrecedence();
    void    stopUsingForPrecedence( int iGroupToUse );

    int& GetLastFoundVariableIndex();


    // --------------------------------------------------------------------------
    // COMPILER_DLL_TODO...
    // --------------------------------------------------------------------------
public:
    int& get_COMPILER_DLL_TODO_Tokstindex() override { return GetLastFoundVariableIndex(); }
private:
    int& get_COMPILER_DLL_TODO_InCompIdx() override { return m_InCompIdx; }
    std::tuple<int, bool>& get_COMPILER_DLL_TODO_m_loneAlphaFunctionCallTester() override { return m_loneAlphaFunctionCallTester; }

    template<typename CF>
    int HandleErrors_COMPILER_DLL_TODO(CF callback_function);

    int CompileHas_COMPILER_DLL_TODO(int iVarNode) override;
    int crelalpha_COMPILER_DLL_TODO() override;
    int varsanal_COMPILER_DLL_TODO(int fmt) override;
    int tvarsanal_COMPILER_DLL_TODO() override;
    int rutfunc_COMPILER_DLL_TODO() override;
    int instruc_COMPILER_DLL_TODO(bool create_new_local_symbol_stack = true, bool allow_multiple_statements = true) override;
    int CompileReenterStatement_COMPILER_DLL_TODO(bool bNextTkn = true) override;
    int CompileMoveStatement_COMPILER_DLL_TODO(bool bFromSelectStatement = false) override;
    void rutasync_as_global_compilation_COMPILER_DLL_TODO(const Symbol& compilation_symbol, const std::function<void()>& compilation_function) override;
    DICT* GetInputDictionary(bool issue_error_if_no_input_dictionary) override;
    void MarkAllDictionaryItemsAsUsed() override;

    template<typename NodeType> NodeType& CreateCompilationNode(std::optional<FunctionCode> function_code = std::nullopt, int node_size_offset = 0) { return CreateNode<NodeType>(std::move(function_code), node_size_offset); }
    template<typename NodeType> NodeType& CreateVariableArgumentCompilationNode(std::optional<FunctionCode> function_code, int number_arguments) { return CreateVariableSizeNode<NodeType>(std::move(function_code), number_arguments); }
    template<typename NodeType> int GetCompilationNodeProgramIndex(const NodeType& compilation_node) { return GetProgramIndex<NodeType>(compilation_node); }
    int CreateVariousNode(FunctionCode function_code, std::initializer_list<int> arguments) { return CreateVariableArgumentsNode(function_code, arguments); }
    int CreateVariousNode(FunctionCode function_code, const std::vector<int>& arguments) { return CreateVariableArgumentsNode(function_code, arguments); }

public:
    SymbolType m_ObjInComp = SymbolType::None;  // Type of object compiled
    int        m_InCompIdx = -1;                // SYMT index of object compiled
};

#undef TEMP_VIRTUAL // COMPILER_DLL_TODO


#define Flagcomp      m_pEngineCompFunc->m_Flagcomp
#define ObjInComp     m_pEngineCompFunc->m_ObjInComp // COMPILER_DLL_TODO, note: for reports and user-defined functions, ObjInComp = SymbolType::Application
#define LvlInComp     m_pEngineCompFunc->m_LvlInComp
#define ProcInComp    m_pEngineCompFunc->m_ProcInComp

#define Tokstindex    m_pEngineCompFunc->GetLastFoundVariableIndex()

#define Flagvars      m_pEngineCompFunc->m_Flagvars
#define InCompIdx     m_pEngineCompFunc->m_InCompIdx
