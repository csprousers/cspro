#pragma once

#include <zSrcMgrO/zSrcMgrO.h>
#include <zSrcMgrO/BackgroundCompiler.h>
#include <zSrcMgrO/DesignerCompilerMessageProcessor.h>
#include <zCapiO/CapiLogicParameters.h>


class CLASS_DECL_ZSRCMGR DesignerCapiLogicCompiler : public BackgroundCompiler, public DesignerCompilerMessageProcessor
{
public:
    DesignerCapiLogicCompiler(Application& application);
    virtual ~DesignerCapiLogicCompiler() { }

    CEngineDriver* GetEngineDriver() override;
    std::string GetProcName() const override           { return m_procName; }
    int GetLineNumberOfCurrentCompile() const override { return 0; }

    struct CompileResult { std::vector<Logic::ParserMessage> errors; };

    // marked as virtual to make it accessible from zFormF
    virtual CompileResult Compile(const CapiLogicParameters& capi_logic_parameters);

private:
    Application& m_application;
    std::string m_procName;
    bool m_procGlobalCompiled;
};
