#include "StdAfx.h"
#include "ProcGlobalConditionalCompiler.h"
#include <zAppO/Application.h>
#include <engine/Comp.h>


struct ProcGlobalConditionalCompilerParameters
{
    std::optional<const TextSource*> external_code_text_source;
    bool include_this_external_code_text_source;
    std::optional<const NamedTextSource*> only_report_to_compile;
    std::shared_ptr<ProcGlobalConditionalCompilerCreator::CompileNotificationCallback> compile_notification_callback;
};


// a compiler subclass that can be used to compile:
//  - all or some external code files
//  - a specific report
class ProcGlobalConditionalCompiler : public CEngineCompFunc
{
public:
    ProcGlobalConditionalCompiler(CEngineDriver* pEngineDriver, std::shared_ptr<ProcGlobalConditionalCompilerParameters> parameters)
        :   CEngineCompFunc(pEngineDriver),
            m_parameters(parameters)
    {
    }

    void CompileExternalCode(const CodeFile& code_file) override
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

        RunCompilationRoutine(code_file.GetTextSource(),
            [&]()
            {
                CEngineCompFunc::CompileExternalCode(code_file);
            });
    }

    void CompileReport(const NamedTextSource& report_named_text_source) override
    {
        // for conditionally compiling reports...
        if( !m_parameters->only_report_to_compile.has_value() || *m_parameters->only_report_to_compile != &report_named_text_source )
            return;

        RunCompilationRoutine(*report_named_text_source.text_source,
            [&]()
            {
                CEngineCompFunc::CompileReport(report_named_text_source);
            });
    }

private:
    template<typename CR>
    void RunCompilationRoutine(const TextSource& text_source, CR compilation_routine)
    {
        bool use_compile_notification_callback = ( m_parameters->compile_notification_callback != nullptr &&
                                                   *m_parameters->compile_notification_callback );

        if( use_compile_notification_callback )
            (*m_parameters->compile_notification_callback)(text_source, nullptr);

        compilation_routine();

        if( use_compile_notification_callback )
            (*m_parameters->compile_notification_callback)(text_source, GetSourceBuffer());
    }

private:
    std::shared_ptr<ProcGlobalConditionalCompilerParameters> m_parameters;
};



ProcGlobalConditionalCompilerCreator::ProcGlobalConditionalCompilerCreator(std::shared_ptr<ProcGlobalConditionalCompilerParameters> parameters)
    :   m_parameters(std::move(parameters))
{
}


std::unique_ptr<ProcGlobalConditionalCompilerCreator> ProcGlobalConditionalCompilerCreator::CompileAllExternalCode()
{
    return std::unique_ptr<ProcGlobalConditionalCompilerCreator>(new ProcGlobalConditionalCompilerCreator(
        std::make_shared<ProcGlobalConditionalCompilerParameters>(ProcGlobalConditionalCompilerParameters
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
        std::make_shared<ProcGlobalConditionalCompilerParameters>(ProcGlobalConditionalCompilerParameters
        {
            &external_code_text_source_,
            include_this_external_code_text_source_,
            std::nullopt,
            nullptr
        })));
}


std::unique_ptr<ProcGlobalConditionalCompilerCreator>
ProcGlobalConditionalCompilerCreator::CompileReport(const NamedTextSource& report_named_text_source_)
{
    return std::unique_ptr<ProcGlobalConditionalCompilerCreator>(new ProcGlobalConditionalCompilerCreator(
        std::make_shared<ProcGlobalConditionalCompilerParameters>(ProcGlobalConditionalCompilerParameters
        {
            std::nullopt,
            false,
            &report_named_text_source_,
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
