#include "StdAfx.h"
#include "ProcessorActionInvoker.h"
#include "OutputWndCaseConstructionReporter.h"
#include <zToolsO/UniqueId.h>
#include <zAction/ActionInvoker.h>
#include <zAction/JsonExecutor.h>


// --------------------------------------------------------------------------
// ActionInvokerJsonCaller
// --------------------------------------------------------------------------

class ActionInvokerJsonCaller : public ActionInvoker::Caller
{
public:
    ActionInvokerJsonCaller(CodeDoc& code_doc, OutputWnd& output_wnd);

    int GetCallerId() const override { return m_callerId; }

    CancelFlag& GetCancelFlag() override { return m_cancelFlag; }

    std::string GetRootDirectory() override { return m_rootDirectory; }

    std::shared_ptr<CaseConstructionReporter> CreateCaseConstructionReporter() override;

private:
    OutputWnd& m_outputWnd;
    int m_callerId;
    CancelFlag m_cancelFlag;
    const std::string m_rootDirectory;
};


ActionInvokerJsonCaller::ActionInvokerJsonCaller(CodeDoc& code_doc, OutputWnd& output_wnd)
    :   m_outputWnd(output_wnd),
        m_callerId(UniqueId::CreateInt()),
        m_rootDirectory(PortableFunctions::PathGetDirectory(code_doc.GetFilePath()))
{
}


std::shared_ptr<CaseConstructionReporter> ActionInvokerJsonCaller::CreateCaseConstructionReporter()
{
    return std::make_unique<OutputWndCaseConstructionReporter>(m_outputWnd);
}



// --------------------------------------------------------------------------
// CSCodeJsonExecutor
// --------------------------------------------------------------------------

class CSCodeJsonExecutor : public ActionInvoker::JsonExecutor
{
public:
    CSCodeJsonExecutor(OutputWnd& output_wnd);

    virtual void DisplayResultsPostRunActions();

protected:
    static SharableString GetFormattedJson(std::string_view json_sv);

protected:
    OutputWnd& m_outputWnd;
};


CSCodeJsonExecutor::CSCodeJsonExecutor(OutputWnd& output_wnd)
    :   JsonExecutor(false),
        m_outputWnd(output_wnd)
{
}


void CSCodeJsonExecutor::DisplayResultsPostRunActions()
{
    m_outputWnd.AddText(GetFormattedJson(ReleaseResultsJson().GetString()));
}


SharableString CSCodeJsonExecutor::GetFormattedJson(const std::string_view json_sv)
{
    // format the JSON text nicely before displaying it
    try
    {
        const JsonNode json_node = Json::Parse(json_sv);
        return json_node.GetNodeAsSharableString(JsonFormattingOptions::PrettySpacing);
    }

    catch(...)
    {
        return ReturnProgrammingError(json_sv);
    }
}


// --------------------------------------------------------------------------
// CSCodeJsonExecutorDisplayingResultsAsNonJson
// --------------------------------------------------------------------------

class CSCodeJsonExecutorDisplayingResultsAsNonJson : public CSCodeJsonExecutor
{
public:
    using CSCodeJsonExecutor::CSCodeJsonExecutor;

protected:
    void DisplayResultsPostRunActions() override;
    void ProcessActionResult(ActionInvoker::Result result) override;
    void ProcessActionResult(const CSProException& exception) override;

private:
    void OutputResult(SharableString result_text);

private:
    bool m_spaceOutResultsWithNewline = false;
};


void CSCodeJsonExecutorDisplayingResultsAsNonJson::DisplayResultsPostRunActions()
{
    // everything has been displayed in the ProcessAction... methods
}


void CSCodeJsonExecutorDisplayingResultsAsNonJson::ProcessActionResult(const ActionInvoker::Result result)
{
    switch( result.GetType() )
    {
        case ActionInvoker::Result::Type::Bool:
        case ActionInvoker::Result::Type::Number:
        case ActionInvoker::Result::Type::String:
            OutputResult(result.GetResultAsString<false>());
            break;

        case ActionInvoker::Result::Type::JsonText:
            OutputResult(GetFormattedJson(result.GetStringResult().GetString()));
            break;

        default:
            ASSERT(result.GetType() == ActionInvoker::Result::Type::Undefined);
            break;
    }
}


void CSCodeJsonExecutorDisplayingResultsAsNonJson::ProcessActionResult(const CSProException& exception)
{
    const ActionInvoker::Exception* const action_invoker_exception = dynamic_cast<const ActionInvoker::Exception*>(&exception);

    if( action_invoker_exception == nullptr )
    {
        ASSERT(false);
        OutputResult(exception.what());
    }

    else
    {
        OutputResult(SO::CreateColonSeparatedString(action_invoker_exception->GetName(), action_invoker_exception->what()));
    }
}


void CSCodeJsonExecutorDisplayingResultsAsNonJson::OutputResult(SharableString result_text)
{
    if( m_spaceOutResultsWithNewline )
    {
        m_outputWnd.AddText(SharableString());
    }

    else
    {
        m_spaceOutResultsWithNewline = true;
    }

    m_outputWnd.AddText(std::move(result_text));
}



// --------------------------------------------------------------------------
// ActionInvokerJsonRunOperation
// --------------------------------------------------------------------------

class ActionInvokerJsonRunOperation : public RunOperation
{
public:
    ActionInvokerJsonRunOperation(CodeDoc& code_doc, std::unique_ptr<CSCodeJsonExecutor> cscode_json_executor, OutputWnd& output_wnd);

    bool IsCancelable() const override { return true; }

    bool IsRunning() const override { return ( m_runThread != nullptr && m_runThread->joinable() ); }

    void Run() override;

    void OnComplete() override;

    void Cancel() override;

private:
    void RunWorker();

private:
    ActionInvokerJsonCaller m_actionInvokerCaller;
    std::unique_ptr<CSCodeJsonExecutor> m_cscodeJsonExecutor;
    OutputWnd& m_outputWnd;
    std::unique_ptr<std::thread> m_runThread;
};


ActionInvokerJsonRunOperation::ActionInvokerJsonRunOperation(CodeDoc& code_doc, std::unique_ptr<CSCodeJsonExecutor> cscode_json_executor, OutputWnd& output_wnd)
    :   m_actionInvokerCaller(code_doc, output_wnd),
        m_cscodeJsonExecutor(std::move(cscode_json_executor)),
        m_outputWnd(output_wnd)
{
    ASSERT(m_cscodeJsonExecutor != nullptr);
}


void ActionInvokerJsonRunOperation::Run()
{
    m_runThread = std::make_unique<std::thread>([&]() { RunWorker(); });
}


void ActionInvokerJsonRunOperation::OnComplete()
{
    if( m_runThread != nullptr )
    {
        if( m_runThread->joinable() )
            m_runThread->join();

        m_runThread.reset();
    }
}


void ActionInvokerJsonRunOperation::Cancel()
{
    if( m_runThread != nullptr )
    {
        if( m_runThread->joinable() )
        {
            m_actionInvokerCaller.GetCancelFlag() = true;
            m_runThread->join();
        }

        m_runThread.reset();
    }
}


void ActionInvokerJsonRunOperation::RunWorker()
{
    m_cscodeJsonExecutor->RunActions(m_actionInvokerCaller);

    m_cscodeJsonExecutor->DisplayResultsPostRunActions();

    WindowsDesktopMessage::Post(UWM::CSCode::RunOperationComplete);
}



// --------------------------------------------------------------------------
// ProcessorActionInvoker
// --------------------------------------------------------------------------

bool ProcessorActionInvoker::ValidateJson(CodeView& code_view, ActionInvoker::JsonExecutor* const json_executor)
{
    const bool run_mode = ( json_executor != nullptr );
    const bool make_build_window_visible_if_not = !run_mode;

    CLogicCtrl* const logic_ctrl = code_view.GetLogicCtrl();
    ASSERT(logic_ctrl->GetLexer() == SCLEX_JSON);

    CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CSCodeBuildWnd* const build_wnd = main_frame->GetBuildWnd(make_build_window_visible_if_not);

    if( build_wnd == nullptr )
        return false;

    build_wnd->Initialize(code_view, "Action Invoker validation");

    bool validation_success = false;

    try
    {
        const std::string actions_text = logic_ctrl->GetText();

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
    OutputWnd* const output_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetOutputWnd();

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
