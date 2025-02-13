#include "StdAfx.h"
#include "ProcessorActionInvoker.h"
#include <zAction/ActionInvoker.h>
#include <zAction/JsonExecutor.h>


// --------------------------------------------------------------------------
// ActionInvokerJsonCaller
// --------------------------------------------------------------------------

namespace
{
    class ActionInvokerJsonCaller : public ActionInvoker::Caller
    {
    public:
        ActionInvokerJsonCaller(CodeDoc& code_doc)
            :   m_cancelFlag(false),
                m_rootDirectory(PortableFunctions::PathGetDirectory(code_doc.GetPathName()))
        {
        }

        bool& GetCancelFlag() override { return m_cancelFlag; }

        std::wstring GetRootDirectory() override { return m_rootDirectory; }

    private:
        bool m_cancelFlag;
        const std::wstring m_rootDirectory;
    };
}



// --------------------------------------------------------------------------
// CSCodeJsonExecutor +
// CSCodeJsonExecutorDisplayingResultsAsNonJson
// --------------------------------------------------------------------------

namespace
{
    class CSCodeJsonExecutor : public ActionInvoker::JsonExecutor
    {
    public:
        CSCodeJsonExecutor(OutputWnd& output_wnd)
            :   JsonExecutor(false),
                m_outputWnd(output_wnd)
        {
        }

        virtual void DisplayResultsPostRunActions()
        {
            m_outputWnd.AddText(GetFormattedJson(*GetResultsJson()));
        }

    protected:
        template<typename T>
        static std::wstring GetFormattedJson(T&& json)
        {
            // format the JSON text nicely before displaying it
            try
            {
                const auto json_node = Json::Parse(json);
                return json_node.GetNodeAsString(JsonFormattingOptions::PrettySpacing);
            }

            catch(...)
            {
                return ReturnProgrammingError(std::forward<T>(json));
            }
        }

    protected:
        OutputWnd& m_outputWnd;
    };


    class CSCodeJsonExecutorDisplayingResultsAsNonJson : public CSCodeJsonExecutor
    {
    public:
        using CSCodeJsonExecutor::CSCodeJsonExecutor;

    protected:
        void DisplayResultsPostRunActions() override
        {
            // everything has been displayed in the ProcessAction... methods
        }

        void ProcessActionResult(ActionInvoker::Result result) override
        {
            switch( result.GetType() )
            {
                case ActionInvoker::Result::Type::Bool:
                case ActionInvoker::Result::Type::Number:
                case ActionInvoker::Result::Type::String:
                    OutputResult(result.ReleaseResultAsString<false>());
                    break;

                case ActionInvoker::Result::Type::JsonText:
                    OutputResult(GetFormattedJson(result.ReleaseStringResult()));
                    break;

                default:
                    ASSERT(result.GetType() == ActionInvoker::Result::Type::Undefined);
                    break;
            }
        }

        void ProcessActionResult(const CSProException& exception) override
        {
            OutputResult(ActionInvoker::JsonResponse::GetExceptionText(exception));
        }

    private:
        void OutputResult(std::wstring result_text)
        {
            if( m_spaceOutResultsWithNewline )
            {
                m_outputWnd.AddText(std::wstring());
            }

            else
            {
                m_spaceOutResultsWithNewline = true;
            }

            m_outputWnd.AddText(std::move(result_text));
        }

    private:
        bool m_spaceOutResultsWithNewline = false;
    };
}



// --------------------------------------------------------------------------
// ActionInvokerJsonRunOperation
// --------------------------------------------------------------------------

namespace
{
    class ActionInvokerJsonRunOperation : public RunOperation
    {
    public:
        ActionInvokerJsonRunOperation(CodeDoc& code_doc, std::unique_ptr<CSCodeJsonExecutor> cscode_json_executor, OutputWnd& output_wnd)
            :   m_actionInvokerCaller(code_doc),
                m_cscodeJsonExecutor(std::move(cscode_json_executor)),
                m_outputWnd(output_wnd)
        {
            ASSERT(m_cscodeJsonExecutor != nullptr);
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
                    m_actionInvokerCaller.SetCancelFlag(true);
                    m_runThread->join();
                }

                m_runThread.reset();
            }
        }

    private:
        void RunWorker()
        {
            m_cscodeJsonExecutor->RunActions(m_actionInvokerCaller);

            m_cscodeJsonExecutor->DisplayResultsPostRunActions();

            WindowsDesktopMessage::Post(UWM::CSCode::RunOperationComplete);
        }

    private:
        ActionInvokerJsonCaller m_actionInvokerCaller;
        std::unique_ptr<CSCodeJsonExecutor> m_cscodeJsonExecutor;
        OutputWnd& m_outputWnd;
        std::unique_ptr<std::thread> m_runThread;
    };
}



// --------------------------------------------------------------------------
// ProcessorActionInvoker
// --------------------------------------------------------------------------

bool ProcessorActionInvoker::ValidateJson(CodeView& code_view, ActionInvoker::JsonExecutor* const json_executor)
{
    const bool run_mode = ( json_executor != nullptr );
    const bool make_build_window_visible_if_not = !run_mode;

    CLogicCtrl* logic_ctrl = code_view.GetLogicCtrl();
    ASSERT(logic_ctrl->GetLexer() == SCLEX_JSON);

    CMainFrame* main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CSCodeBuildWnd* build_wnd = main_frame->GetBuildWnd(make_build_window_visible_if_not);

    if( build_wnd == nullptr )
        return false;

    build_wnd->Initialize(code_view, _T("Action Invoker validation"));

    bool validation_success = false;

    try
    {
        const std::wstring actions_text = logic_ctrl->GetText();

        if( run_mode )
        {
            json_executor->ParseActions(actions_text);
        }

        else
        {
            ActionInvoker::JsonExecutor json_executor_for_validation(true);
            json_executor_for_validation.ParseActions(actions_text);
        }

        validation_success = true;
    }

    catch( const CSProException& exception )
    {
        build_wnd->AddError(exception);
    }

    build_wnd->Finalize();

    // in runtime mode, only force showing the build window when there are errors
    if( !validation_success && !make_build_window_visible_if_not && !build_wnd->IsVisible() )
        main_frame->GetBuildWnd(true);

    return validation_success;
}


void ProcessorActionInvoker::ValidateJson(CodeView& code_view)
{
    ValidateJson(code_view, nullptr);
}


void ProcessorActionInvoker::Run(CodeDoc& code_doc)
{
    OutputWnd* output_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetOutputWnd();

    if( output_wnd == nullptr )
        return;

    const LanguageSettings& language_settings = code_doc.GetLanguageSettings();

    auto json_executor = language_settings.GetActionInvokerDisplayResultsAsJson() ? std::make_unique<CSCodeJsonExecutor>(*output_wnd) :
                                                                                    std::make_unique<CSCodeJsonExecutorDisplayingResultsAsNonJson>(*output_wnd);

    json_executor->SetAbortOnException(language_settings.GetActionInvokerAbortOnException());

    if( !ValidateJson(code_doc.GetPrimaryCodeView(), json_executor.get()) )
        return;

    output_wnd->Clear();

    code_doc.RegisterRunOperation(std::make_unique<ActionInvokerJsonRunOperation>(code_doc, std::move(json_executor), *output_wnd));
}
