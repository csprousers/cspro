#include "StdAfx.h"
#include "DesignerCompiler.h"
#include "ProcGlobalConditionalCompiler.h"
#include "SrcCode.h"
#include <CSPro/AplDoc.h>


DesignerCompiler::DesignerCompiler(CAplDoc* pAplDoc)
    :   m_pAplDoc(pAplDoc),
        m_pApplication(&m_pAplDoc->GetAppObject()),
        m_pSourceCode(m_pApplication->GetAppSrcCode())
{
}


void DesignerCompiler::CreateCompiler(CompilerCreator* compiler_creator/* = nullptr*/)
{
    m_compiler = std::make_unique<CCompiler>(m_pApplication, compiler_creator);
    m_compiler->SetOptimizeFlowTree(true);
}


bool DesignerCompiler::ProcessCompilerResult(CCompiler::Result result, bool throw_on_some_errors)
{
    if( result == CCompiler::Result::CantInit || result == CCompiler::Result::NoInit )
    {
        throw CSProException("Cannot initialize the compiler!"); // BMD 11 Dec 2002
    }

    else if( result != CCompiler::Result::NoErrors )
    {
        if( throw_on_some_errors || result != CCompiler::Result::SomeErrors )
            throw CSProException("Compile Failed. See application ('GLOBAL') procedure."); // BMD 11 Dec 2002

        // there were some errors
        return false;
    }

    // there were no errors
    return true;
}


bool DesignerCompiler::CompileAll()
{
    CWaitCursor wait;

    CreateCompiler();

    CCompiler::Result result = m_compiler->FullCompile(m_pSourceCode);

    // compile the CAPI conditions and fills
    if( result == CCompiler::Result::NoErrors && m_pApplication->GetUseQuestionText() && m_pAplDoc->m_pQuestMgr != nullptr )
        result = m_compiler->Compile(*m_pAplDoc->m_pQuestMgr);

    return ProcessCompilerResult(result, false);
}


bool DesignerCompiler::CompileExternalCode(const CodeFile& code_file)
{
    CWaitCursor wait;

    std::unique_ptr<ProcGlobalConditionalCompilerCreator> compiler_creator = ProcGlobalConditionalCompilerCreator::CompileSomeExternalCode(code_file.GetTextSource(), true);
    CreateCompiler(compiler_creator.get());

    CCompiler::Result result = m_compiler->CompileExternalLogicOnly();

    return ProcessCompilerResult(result, false);
}


bool DesignerCompiler::CompileReport(const NamedTextSource& report_named_text_source)
{
    CWaitCursor wait;

    std::unique_ptr<ProcGlobalConditionalCompilerCreator> compiler_creator = ProcGlobalConditionalCompilerCreator::CompileReport(report_named_text_source);
    CreateCompiler(compiler_creator.get());

    CCompiler::Result result = m_compiler->CompileReport(report_named_text_source);

    return ProcessCompilerResult(result, false);
}


bool DesignerCompiler::CompileProc(const CString& proc_name, const CStringArray& proc_lines)
{
    CWaitCursor wait;

    CreateCompiler();

    // first compile PROC GLOBAL...
    m_compiler->SetFullCompile(true);

    const CString ProcGlobalName = _T("GLOBAL");

    CStringArray proc_global_lines;
    m_pSourceCode->GetProc(proc_global_lines, ProcGlobalName);

    // compile
    CCompiler::Result result = m_compiler->Compile(ProcGlobalName, proc_global_lines);

    ProcessCompilerResult(result, true);
  
    m_compiler->SetFullCompile(false);

    // clear any parser messages from the application procedure
    m_compiler->ClearParserMessages();


    // ...then compile the specific PROC
    result = m_compiler->Compile(proc_name, proc_lines);

    return ProcessCompilerResult(result, false);
}
