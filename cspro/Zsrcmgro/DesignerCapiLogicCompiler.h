#pragma once

#include <Zsrcmgro/zSrcMgrO.h>
#include <Zsrcmgro/BackgroundCompiler.h>
#include <Zsrcmgro/DesignerCompilerMessageProcessor.h>
#include <zCapiO/CapiLogicParameters.h>


class CLASS_DECL_ZSRCMGR DesignerCapiLogicCompiler : public BackgroundCompiler, public DesignerCompilerMessageProcessor
{
public:
    DesignerCapiLogicCompiler(Application& application);

    CEngineDriver* GetEngineDriver() override;
    CString GetProcName() const override               { return m_procName; }
    int GetLineNumberOfCurrentCompile() const override { return 0; }

    struct CompileResult
    {
        int expression;
        std::wstring error_message;
    };

    CompileResult Compile(const CapiLogicParameters& capi_logic_parameters);

private:
    Application& m_application;
    CString m_procName;
};
