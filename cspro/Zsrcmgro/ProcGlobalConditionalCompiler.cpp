#include "StdAfx.h"
#include "ProcGlobalConditionalCompiler.h"
#include <zAppO/Application.h>
#include <engine/Comp.h>


// --------------------------------------------------------------------------
// ProcGlobalConditionalCompilerParameters
// --------------------------------------------------------------------------

struct ProcGlobalConditionalCompilerParameters
{
    std::optional<const TextSource*> external_code_text_source;
    bool include_this_external_code_text_source;
    std::optional<std::string> only_report_to_compile;
    std::shared_ptr<ProcGlobalConditionalCompilerCreator::CompileNotificationCallback> compile_notification_callback;
};



// --------------------------------------------------------------------------
// ProcGlobalConditionalCompiler
//
// a compiler subclass that can be used to compile:
//  - all or some external code files
//  - a specific report
// --------------------------------------------------------------------------

class ProcGlobalConditionalCompiler : public CEngineCompFunc
{
public:
    ProcGlobalConditionalCompiler(CEngineDriver* pEngineDriver, std::shared_ptr<ProcGlobalConditionalCompilerParameters> parameters);

    void CompileExternalCode(const CodeFile& code_file) override;
    void CompileReport(const ReportFile& report_file) override;

private:
    template<typename CR>
    void RunCompilationRoutine(const TextSource& text_source, CR compilation_routine);

private:
    std::shared_ptr<ProcGlobalConditionalCompilerParameters> m_parameters;
};


ProcGlobalConditionalCompiler::ProcGlobalConditionalCompiler(CEngineDriver* const pEngineDriver,
                                                             std::shared_ptr<ProcGlobalConditionalCompilerParameters> parameters)
    :   CEngineCompFunc(pEngineDriver),
        m_parameters(std::move(parameters))
{
    ASSERT(m_parameters != nullptr);
}


void ProcGlobalConditionalCompiler::CompileExternalCode(const CodeFile& code_file)
{
    // for conditionally compiling external code...
    if( m_parameters->external_code_text_source.has_value() )
    {
        // ...return if the external code file has already been compiled
        if( *m_parameters->external_code_text_source == nullptr )
            return;

        if( m_parameters->external_code_text_source == &code_file.GetTextSource() )
        {
            // mark the external code file as having been compiled
            m_parameters->external_code_text_source = nullptr;

            if( !m_parameters->include_this_external_code_text_source )
                return;
        }
    }

    RunCompilationRoutine(code_file.GetTextSource(), [&]() { CEngineCompFunc::CompileExternalCode(code_file); });
}


void ProcGlobalConditionalCompiler::CompileReport(const ReportFile& report_file)
{
    // for conditionally compiling reports...
    if( m_parameters->only_report_to_compile != report_file.GetName() )
        return;

    RunCompilationRoutine(report_file.GetTextSource(), [&]() { CEngineCompFunc::CompileReport(report_file); });
}


template<typename CR>
void ProcGlobalConditionalCompiler::RunCompilationRoutine(const TextSource& text_source, CR compilation_routine)
{
    const bool use_compile_notification_callback = ( m_parameters->compile_notification_callback != nullptr &&
                                                     *m_parameters->compile_notification_callback );

    if( use_compile_notification_callback )
        (*m_parameters->compile_notification_callback)(text_source, nullptr);

    compilation_routine();

    if( use_compile_notification_callback )
        (*m_parameters->compile_notification_callback)(text_source, GetSourceBuffer());
}



// --------------------------------------------------------------------------
// ProcGlobalConditionalCompilerCreator
// --------------------------------------------------------------------------

ProcGlobalConditionalCompilerCreator::ProcGlobalConditionalCompilerCreator(std::unique_ptr<ProcGlobalConditionalCompilerParameters> parameters)
    :   m_parameters(std::move(parameters))
{
    ASSERT(m_parameters != nullptr);
}


std::unique_ptr<ProcGlobalConditionalCompilerCreator> ProcGlobalConditionalCompilerCreator::CompileAllExternalCode()
{
    return std::unique_ptr<ProcGlobalConditionalCompilerCreator>(new ProcGlobalConditionalCompilerCreator(
        std::make_unique<ProcGlobalConditionalCompilerParameters>(ProcGlobalConditionalCompilerParameters
        {
            std::nullopt,
            false,
            std::nullopt,
            nullptr
        })));
}


std::unique_ptr<ProcGlobalConditionalCompilerCreator>
ProcGlobalConditionalCompilerCreator::CompileSomeExternalCode(const TextSource& external_code_text_source_,
                                                              bool include_this_external_code_text_source_)
{
    return std::unique_ptr<ProcGlobalConditionalCompilerCreator>(new ProcGlobalConditionalCompilerCreator(
        std::make_unique<ProcGlobalConditionalCompilerParameters>(ProcGlobalConditionalCompilerParameters
        {
            &external_code_text_source_,
            include_this_external_code_text_source_,
            std::nullopt,
            nullptr
        })));
}


std::unique_ptr<ProcGlobalConditionalCompilerCreator>
ProcGlobalConditionalCompilerCreator::CompileReport(const ReportFile& report_file)
{
    return std::unique_ptr<ProcGlobalConditionalCompilerCreator>(new ProcGlobalConditionalCompilerCreator(
        std::make_unique<ProcGlobalConditionalCompilerParameters>(ProcGlobalConditionalCompilerParameters
        {
            std::nullopt,
            false,
            report_file.GetName(),
            nullptr
        })));
}


void ProcGlobalConditionalCompilerCreator::SetCompileNotificationCallback(std::shared_ptr<CompileNotificationCallback> compile_notification_callback_)
{
    m_parameters->compile_notification_callback = std::move(compile_notification_callback_);
}


std::unique_ptr<CEngineCompFunc> ProcGlobalConditionalCompilerCreator::CreateCompiler(CEngineDriver* pEngineDriver)
{
    return std::make_unique<ProcGlobalConditionalCompiler>(pEngineDriver, m_parameters);
}
