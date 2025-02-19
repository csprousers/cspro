#pragma once

#include <Zsrcmgro/zSrcMgrO.h>
#include <zLogicO/BasicTokenCompiler.h>
#include <engine/CompilerCreator.h>

class CEngineCompFunc;
struct ProcGlobalConditionalCompilerParameters;
class ReportFile;


class CLASS_DECL_ZSRCMGR ProcGlobalConditionalCompilerCreator : public CompilerCreator
{
public:
    using CompileNotificationCallback = std::function<void(const TextSource&, std::shared_ptr<const Logic::SourceBuffer>)>;

private:
    ProcGlobalConditionalCompilerCreator(std::unique_ptr<ProcGlobalConditionalCompilerParameters> parameters);

public:
    static std::unique_ptr<ProcGlobalConditionalCompilerCreator> CompileAllExternalCode();

    static std::unique_ptr<ProcGlobalConditionalCompilerCreator> CompileSomeExternalCode(const TextSource& external_code_text_source_,
                                                                                         bool include_this_external_code_text_source_);

    static std::unique_ptr<ProcGlobalConditionalCompilerCreator> CompileReport(const ReportFile& report_file);

    void SetCompileNotificationCallback(std::shared_ptr<CompileNotificationCallback> compile_notification_callback_);

    std::unique_ptr<CEngineCompFunc> CreateCompiler(CEngineDriver* pEngineDriver) override;

private:
    std::shared_ptr<ProcGlobalConditionalCompilerParameters> m_parameters;
};
