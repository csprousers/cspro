#pragma once

#include <Zsrcmgro/zSrcMgrO.h>
#include <Zsrcmgro/Compiler.h>

class CAplDoc;
class CodeFile;


class CLASS_DECL_ZSRCMGR DesignerCompiler
{
public:
    DesignerCompiler(CAplDoc* pAplDoc);

    bool CompileAll();
    bool CompileExternalCode(const CodeFile& code_file);
    bool CompileReport(const NamedTextSource& report_named_text_source);
    bool CompileProc(const CString& proc_name, const CStringArray& proc_lines);

private:
    void CreateCompiler(CompilerCreator* compiler_creator = nullptr);

    bool ProcessCompilerResult(CCompiler::Result result, bool throw_on_some_errors);

private:
    CAplDoc* m_pAplDoc;
    Application* m_pApplication;
    CSourceCode* m_pSourceCode;
    std::unique_ptr<CCompiler> m_compiler;
};
