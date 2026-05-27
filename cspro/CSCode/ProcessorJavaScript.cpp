#include "StdAfx.h"
#include "ProcessorJavaScript.h"
#include "OutputWndCaseConstructionReporter.h"
#include <zToolsO/CancelFlag.h>
#include <zLogicO/ActionInvoker.h>


// --------------------------------------------------------------------------
// OutputWndJavaScriptPrinter
// --------------------------------------------------------------------------

OutputWndJavaScriptPrinter::OutputWndJavaScriptPrinter(OutputWnd& output_wnd)
    :   m_outputWnd(output_wnd)
{
}


void OutputWndJavaScriptPrinter::OnPrint(SharableString text)
{
    ASSERT(m_outputWnd.GetSafeHwnd() != nullptr);

    m_outputWnd.AddText(std::move(text));
}



// --------------------------------------------------------------------------
// JavaScriptRunOperation
// --------------------------------------------------------------------------

class JavaScriptRunOperation : public RunOperation
{
public:
    JavaScriptRunOperation(JavaScript::Executor& executor, JavaScript::Bytecode bytecode, OutputWnd& output_wnd);

    bool IsCancelable() const override { return true; }
    bool IsRunning() const override;
    void Run() override;
    void OnComplete() override;
    void Cancel() override;

private:
    void RunWorker();

private:
    JavaScript::Executor& m_executor;
    JavaScript::Bytecode m_bytecode;
    OutputWnd& m_outputWnd;
    CancelFlag m_cancelFlag;
    std::unique_ptr<std::thread> m_runThread;
};


JavaScriptRunOperation::JavaScriptRunOperation(JavaScript::Executor& executor, JavaScript::Bytecode bytecode, OutputWnd& output_wnd)
    :   m_executor(executor),
        m_bytecode(std::move(bytecode)),
        m_outputWnd(output_wnd)
{
    m_executor.SetCancelFlag(&m_cancelFlag);
}


bool JavaScriptRunOperation::IsRunning() const
{
    return ( m_runThread != nullptr && m_runThread->joinable() );
}


void JavaScriptRunOperation::Run()
{
    m_runThread = std::make_unique<std::thread>([&]() { RunWorker(); });
}


void JavaScriptRunOperation::OnComplete()
{
    if( m_runThread != nullptr )
    {
        if( m_runThread->joinable() )
            m_runThread->join();

        m_runThread.reset();
    }
}


void JavaScriptRunOperation::Cancel()
{
    if( m_runThread != nullptr )
    {
        if( m_runThread->joinable() )
        {
            m_cancelFlag = true;
            m_runThread->join();
        }

        m_runThread.reset();
    }
}


void JavaScriptRunOperation::RunWorker()
{
    try
    {
        // forward any cancelation requests to the JavaScript executor
        const CancelFlag::ListenerHolder cancel_flag_listener_holder = m_cancelFlag.AddListener([&]() { m_executor.CancelEvaluation(); });

        SharableString result = m_executor.EvaluateBytecode(m_bytecode);

        m_outputWnd.AddText(std::move(result));
    }

    catch( const JavaScript::Exception& exception )
    {
        m_outputWnd.AddText("UNHANDLED EXCEPTION: %s", exception.what());
    }

    WindowsDesktopMessage::Post(UWM::CSCode::RunOperationComplete);
}



// --------------------------------------------------------------------------
// ProcessorJavaScript
// --------------------------------------------------------------------------

ProcessorJavaScript::ProcessorJavaScript(CodeDoc& code_doc)
    :   m_codeDoc(code_doc),
        m_executor(PortableFunctions::PathGetDirectory(code_doc.GetFilePath()))
{
    OutputWnd* const output_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetOutputWnd();

    m_executor.UseActionInvoker(
        ActionInvoker::GetFunctions(),
        ActionInvoker::GetNamespaceNames(),
        ( output_wnd != nullptr ) ? std::make_unique<OutputWndCaseConstructionReporter>(*output_wnd) : nullptr
    );
}


JavaScript::ModuleType ProcessorJavaScript::GetModuleType()
{
    const std::optional<unsigned>& javascript_module_type = m_codeDoc.GetLanguageSettings().GetJavaScriptModuleType();
    ASSERT(javascript_module_type.has_value());

    return ( *javascript_module_type == ID_RUN_JAVASCRIPT_MODULE_GLOBAL ) ? JavaScript::ModuleType::Global :
                                                                            JavaScript::ModuleType::Module;
}


bool ProcessorJavaScript::CompileRunWorker(JavaScript::Bytecode* const bytecode)
{
    const bool compile_mode = ( bytecode == nullptr );
    const bool make_build_window_visible_if_not = compile_mode;

    CodeView& code_view = m_codeDoc.GetPrimaryCodeView();
    CLogicCtrl* const logic_ctrl = code_view.GetLogicCtrl();
    ASSERT(logic_ctrl->GetLexer() == SCLEX_JAVASCRIPT);

    CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CSCodeBuildWnd* const build_wnd = main_frame->GetBuildWnd(make_build_window_visible_if_not);

    if( build_wnd == nullptr )
        return false;

    build_wnd->Initialize(code_view, "JavaScript compilation");

    bool compilation_success = false;

    try
    {
        m_executor.Reset();

        if( compile_mode )
        {
            m_executor.CompileScriptOnly(logic_ctrl->GetText(), GetModuleType(), m_codeDoc.GetFilePath());
        }

        else
        {
            *bytecode = m_executor.CompileScript(logic_ctrl->GetText(), GetModuleType(), m_codeDoc.GetFilePath());
        }

        compilation_success = true;
    }

    catch( const JavaScript::Exception& exception )
    {
        // add the location details when the error occurs in this file
        if( exception.HasLocationDetails() && SO::EqualsNoCase(m_codeDoc.GetFilePath(), exception.GetFilePath()) )
        {
            build_wnd->AddError(exception.GetBaseMessage(), exception.GetLineNumber());
        }

        else
        {
            build_wnd->AddError(exception.what());
        }
    }

    catch( const CSProException& exception )
    {
        build_wnd->AddError(exception.what());
    }

    build_wnd->Finalize();

    // in runtime mode, only force showing the build window when there are errors
    if( !compilation_success && !make_build_window_visible_if_not && !build_wnd->IsVisible() )
        main_frame->GetBuildWnd(true);

    return compilation_success;
}


void ProcessorJavaScript::Compile()
{
    CompileRunWorker(nullptr);
}


void ProcessorJavaScript::Run()
{
    JavaScript::Bytecode bytecode;

    if( !CompileRunWorker(&bytecode) )
        return;

    OutputWnd* const output_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetOutputWnd();

    if( output_wnd == nullptr )
        return;

    output_wnd->Clear();

    m_executor.SetPrinter(std::make_unique<OutputWndJavaScriptPrinter>(*output_wnd));

    m_codeDoc.RegisterRunOperation(std::make_unique<JavaScriptRunOperation>(m_executor, std::move(bytecode), *output_wnd));
}
