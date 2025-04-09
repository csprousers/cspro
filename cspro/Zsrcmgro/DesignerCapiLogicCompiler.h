#pragma once

#include <Zsrcmgro/zSrcMgrO.h>
#include <Zsrcmgro/BackgroundCompiler.h>
#include <Zsrcmgro/DesignerCompilerMessageProcessor.h>
#include <zCapiO/CapiLogicParameters.h>


class CLASS_DECL_ZSRCMGR DesignerCapiLogicCompiler : public BackgroundCompiler, public DesignerCompilerMessageProcessor
{
public:
    DesignerCapiLogicCompiler(Application& application);
    virtual ~DesignerCapiLogicCompiler() { }

    CEngineDriver* GetEngineDriver() override;
    std::string GetProcName() const override           { return m_procName; }
    int GetLineNumberOfCurrentCompile() const override { return 0; }

    struct CompileResult
    {
        int expression;
        std::string error_message;
    };

    // marked as virtual to make it accessible from zFormO
    virtual CompileResult Compile(const CapiLogicParameters& capi_logic_parameters);

private:
    Application& m_application;
    std::string m_procName;
};
