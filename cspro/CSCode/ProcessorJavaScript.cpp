#include "StdAfx.h"
#include "ProcessorJavaScript.h"
#include <zLogicO/ActionInvoker.h>


// --------------------------------------------------------------------------
// OutputWndJavaScriptPrinter
// --------------------------------------------------------------------------

OutputWndJavaScriptPrinter::OutputWndJavaScriptPrinter(OutputWnd& output_wnd)
    :   m_outputWnd(output_wnd)
{
}


void OutputWndJavaScriptPrinter::OnPrint(const std::string& text)
{
    ASSERT(m_outputWnd.GetSafeHwnd() != nullptr);

    m_outputWnd.AddText(text);
}



// --------------------------------------------------------------------------
// JavaScriptRunOperation
// --------------------------------------------------------------------------

namespace
{
    class JavaScriptRunOperation : public RunOperation
    {
    public:
        JavaScriptRunOperation(JavaScript::Executor& executor, JavaScript::ByteCode byte_code, OutputWnd& output_wnd)
            :   m_executor(executor),
                m_byteCode(byte_code),
                m_outputWnd(output_wnd)
        {
        }

        bool IsCancelable() const override
        {
            return true;
        }

        bool IsRunning() const override
        {
            return ( m_runThread != nullptr && m_runThread->joinable() );
        }

        void Run() override
        {
            m_runThread = std::make_unique<std::thread>([&]() { RunWorker(); });
        }

        void OnComplete() override
        {
            if( m_runThread != nullptr )
            {
                if( m_runThread->joinable() )
                    m_runThread->join();

                m_runThread.reset();
            }
        }

        void Cancel() override
        {
            if( m_runThread != nullptr )
            {
                if( m_runThread->joinable() )
                {
                    m_executor.CancelEvaluation();
                    m_runThread->join();
                }

                m_runThread.reset();
            }
        }

    private:
        void RunWorker()
        {
            try
            {
                std::string result = m_executor.EvaluateByteCode(m_byteCode);

                m_outputWnd.AddText(result);
            }

            catch( const JavaScript::Exception& exception )
            {
                m_outputWnd.AddText(_T("UNHANDLED EXCEPTION: ") + exception.GetErrorMessage());
            }

            WindowsDesktopMessage::Post(UWM::CSCode::RunOperationComplete);
        }

    private:
        JavaScript::Executor& m_executor;
        JavaScript::ByteCode m_byteCode;
        OutputWnd& m_outputWnd;
        std::unique_ptr<std::thread> m_runThread;
    };
}



// --------------------------------------------------------------------------
// ProcessorJavaScript
// --------------------------------------------------------------------------

ProcessorJavaScript::ProcessorJavaScript(CodeDoc& code_doc)
    :   m_codeDoc(code_doc),
        m_executor(PortableFunctions::PathGetDirectory(code_doc.GetPathName()))
{
    m_executor.UseActionInvoker(ActionInvoker::GetFunctions(), ActionInvoker::GetNamespaceNames());
}


JavaScript::ModuleType ProcessorJavaScript::GetModuleType()
{
    const std::optional<unsigned>& javascript_module_type = m_codeDoc.GetLanguageSettings().GetJavaScriptModuleType();
    ASSERT(javascript_module_type.has_value());

    return ( *javascript_module_type == ID_RUN_JAVASCRIPT_MODULE_AUTODETECT ) ? JavaScript::ModuleType::Autodetect :
           ( *javascript_module_type == ID_RUN_JAVASCRIPT_MODULE_GLOBAL )     ? JavaScript::ModuleType::Global :
                                                                                JavaScript::ModuleType::Module;
}


bool ProcessorJavaScript::CompileRunWorker(JavaScript::ByteCode* byte_code)
{
    const bool compile_mode = ( byte_code == nullptr );
    const bool make_build_window_visible_if_not = compile_mode;

    CodeView& code_view = m_codeDoc.GetPrimaryCodeView();
    CLogicCtrl* logic_ctrl = code_view.GetLogicCtrl();
    ASSERT(logic_ctrl->GetLexer() == SCLEX_JAVASCRIPT);

    CMainFrame* main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CSCodeBuildWnd* build_wnd = main_frame->GetBuildWnd(make_build_window_visible_if_not);

    if( build_wnd == nullptr )
        return false;

    build_wnd->Initialize(code_view, _T("JavaScript compilation"));

    bool compilation_success = false;

    try
    {
        m_executor.Reset();

        if( compile_mode )
        {
            m_executor.CompileScriptOnly(logic_ctrl->GetTextUtf8(), GetModuleType(), CS2WS(m_codeDoc.GetPathName()));
        }

        else
        {
            *byte_code = m_executor.CompileScript(logic_ctrl->GetTextUtf8(), GetModuleType(), CS2WS(m_codeDoc.GetPathName()));
        }

        compilation_success = true;
    }

    catch( const JavaScript::Exception& exception )
    {
        // add the location details when the error occurs in this file
        if( exception.HasLocationDetails() && SO::EqualsNoCase(m_codeDoc.GetPathName(), exception.GetFilename()) )
        {
            build_wnd->AddError(UTF8Convert::UTF8ToWide(exception.GetBaseMessage()), exception.GetLineNumber());
        }

        else
        {
            build_wnd->AddError(exception.GetErrorMessage());
        }
    }

    catch( const CSProException& exception )
    {
        build_wnd->AddError(exception.GetErrorMessage());
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
    JavaScript::ByteCode byte_code;

    if( !CompileRunWorker(&byte_code) )
        return;

    OutputWnd* output_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetOutputWnd();

    if( output_wnd == nullptr )
        return;

    output_wnd->Clear();

    m_executor.SetPrinter(std::make_unique<OutputWndJavaScriptPrinter>(*output_wnd));

    m_codeDoc.RegisterRunOperation(std::make_unique<JavaScriptRunOperation>(m_executor, std::move(byte_code), *output_wnd));
}
