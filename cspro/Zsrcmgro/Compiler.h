#pragma once
// Compiler.h: interface for the CCompiler class.
//
//////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// CLASS NAME           : CCompiler
// PROTOTYPE            : class CLASS_DECL_ZSRCMGR CCompiler
// OBJECTIVE            : The objective of this class is to compile a buffer
//                        for design time. The class does not generate code,
//                        so it is useless for execution.
// REMARKS              : none
// CHANGES              : 15-Jun-99 | JOO | Creation
//                        10-Jul-99 | RHF | Make compatible with Application
//                        class.
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// FUNCTION NAME        : CCompiler
// FUNCTION PROTOTYPE   : CCompiler(Application* pApplication);
// OBJECTIVE            : This is the constructor of the class.
//
// RETURN TYPE          : None
// PARAMETERS           : pAppplication indicating the target application
//                                                with all the internal elements loaded.
// CHANGE               : 15-Jun-99 | JOO | Creation
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// FUNCTION NAME        : Compile
// FUNCTION PROTOTYPE   : CCompiler::Result Compile (const CString csSymbName,
//                        const CStringArray* pSourceArray);
// OBJECTIVE            : This function does the comilation. It takes a
//                        buffer as input.
//
// RETURN TYPE          : CCompiler::Result
// PARAMETERS           : const CString csSymbName = the symbol to be compiled
//                      : const CStringArray* pSourceArray = the buffer
// CHANGE               : 15-Jun-99 | JOO | Creation
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// FUNCTION NAME        : Compile
// FUNCTION PROTOTYPE   : CCompiler::Result Compile (CSourceCode* pSourceCode);
// OBJECTIVE            : Overloaded function for compilation. This function takes
//                        as input a CSourceCode class and compiles the all of
//                        the procedures it finds. It is important to notice
//                        that it first compiles the application proc and then
//                        continues with the rest of the procs.
//
// RETURN TYPE          : CCompiler::Result
// PARAMETERS           : CSourceCode* pSourceCode = where the application is stored
// CHANGE               : 15-Jun-99 | JOO | Creation
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// FUNCTION NAME        : GetProcName
// FUNCTION PROTOTYPE   : CString GetProcName();
// OBJECTIVE            : This is for internal process. It gets the symbol
//                        that is currently being compiled. Only one symbol
//                        at a time can be compiled.
//
// RETURN TYPE          : CString, the name of the symbol
// PARAMETERS           : None
// CHANGE               : 15-Jun-99 | JOO | Creation
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// FUNCTION NAME        : GetCurrentSession
// FUNCTION PROTOTYPE   : static CCompiler* GetCurrentSession();
// OBJECTIVE            : This is for internal process. It gets the current
//                        session of compilation.
//
// RETURN TYPE          : static CCompiler*
// PARAMETERS           : None
// CHANGE               : 15-Jun-99 | JOO | Creation
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// FUNCTION NAME        : Init
// FUNCTION PROTOTYPE   : CCompiler::Result Init();
// OBJECTIVE            : Initializes the settings for compilation. Most
//                        of this will disapear once the class is hooked
//                        with Application.
// RETURN TYPE          : CCompiler::Result
// PARAMETERS           : None
// CHANGE               : 15-Jun-99 | JOO | Creation
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// FUNCTION NAME        : End
// FUNCTION PROTOTYPE   : void End();
// OBJECTIVE            : Finish and free engine internal tables. Most
//                        of this will disapear once the class is hooked
//                        with Application.
// RETURN TYPE          : None
// PARAMETERS           : None
// CHANGE               : 20-Jul-99 | RHF | Creation
////////////////////////////////////////////////////////////////////////

    //----------------------------------------------------------------
    //    What can you compile:
    //
    //    Statement Example:
    //    ------------------
    //
    //      if MYVAR = 1 then                           line 1
    //          e = display("Hello World!!!");          line 2
    //      endif;                                      line 3
    //
    //    If you want to compile a statement you have to insert
    //    a line with the name of any symbol in the input dict.
    //    So for the example, you would hand out the following
    //    buffer:
    //
    //    PROC XXX                                      line 1
    //      if MYVAR = 1 then                           line 2
    //          e = display("Hello World!!!");          line 3
    //      endif;                                      line 4
    //
    //    Procedure Example:
    //    ------------------
    //    Hand out a buffer like he following:
    //
    //      PROC MYVAR                                  line 1
    //      PostProc                                    line 2
    //          if MYVAR = 1 the                        line 3
    //              e = display("Hello World!!!");      line 4
    //          endif;                                  line 5
    //
    //
    //    Application Example:
    //    ---------------------
    //    First load the class in CSourceCode, then pass that
    //    as you parameter.
    //
    //----------------------------------------------------------------

#include <Zsrcmgro/zSrcMgrO.h>
#include <Zsrcmgro/DesignerCompilerMessageProcessor.h>
#include <Wcompile/Wcompile.h>

class Application;
class CIntDriver;
class CLinkTable;
class CSourceCode;
struct CapiLogicParameters;
class CapiQuestionManager;
struct NamedTextSource;


class CLASS_DECL_ZSRCMGR CCompiler : public DesignerCompilerMessageProcessor
{
public:
    enum class Result
    {
        NoErrors,   // Successful compilation
        SomeErrors, // Errors issued with ParserMessages
        CantInit,   // Compile can't start
        NoInit      // Compile is not inited.
    };

public:
    CCompiler(Application* pApplication, CompilerCreator* compiler_creator = nullptr);

    virtual ~CCompiler();

    Result Compile(const CString& csSymbName, const CStringArray& source_array);

    // Compiles a complete Application
    Result FullCompile(CSourceCode* pSourceCode);

    Result CompileExternalLogicOnly();

    Result CompileReport(const NamedTextSource& report_named_text_source);

    Result Compile(CapiQuestionManager& capi_questions);

    //Gets the name of the Proc being compiled
    CString GetProcName() const override { return m_bInit ? m_csProcName : _T(""); }

    int GetLineNumberOfCurrentCompile() const override { return m_iLineNumberOfCurrentCompile; }

    //Gets the current compiling session. Necessary for integration with C language.
    static CCompiler* GetCurrentSession();

    // Indicate to compiler don't free tables when finish. true indicate don't free tables
    void SetFullCompile( bool bFullCompile ) { m_bFullCompile = bFullCompile; }

// SERPRO_CALC
    int  GetLinkTables( CArray<CLinkTable*,CLinkTable*>& aLinkTables );
// SERPRO_CALC

    //SAVY use this in the designer to optimize flow tree i.e not use maxloop occs for compile
    void SetOptimizeFlowTree( bool bOptimize ) { m_bOptimizeFlowTree = bOptimize; }

    //Initializes compiler settings //SAVY made it public 01/08/2004
    Result Init();
    CIntDriver* GetIntDriver();
    CEngineDriver* GetEngineDriver() override { return m_CompIFaz.m_pEngineDriver; }

private:
    // Finish and free engine internal tables
    void End();

    Result AddUserMessageParserMessages();

private:
    CCompIFaz m_CompIFaz;
    Application* m_pApplication;
    CString m_csProcName;
    bool m_bInit;
    bool m_bFullCompile; // Flag indicating full-compile.
    bool m_bOptimizeFlowTree; //optimize occ in designer  compile
    int m_iLineNumberOfCurrentCompile;
};
