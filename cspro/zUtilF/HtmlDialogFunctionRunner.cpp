#include "StdAfx.h"
#include "HtmlDialogFunctionRunner.h"
#include <zPlatformO/PlatformInterface.h>
#include <zHtml/UWM.h>
#include <zAction/OnGetInputDataListener.h>


#ifdef WIN_DESKTOP

// --------------------------------------------------------------------------
// HtmlDialogFunctionDlg
// --------------------------------------------------------------------------

class HtmlDialogFunctionDlg : public HtmlDlgBase
{
public:
    HtmlDialogFunctionDlg(ExceptionHolder* const exception_holder, HtmlDialogFunctionRunner& html_dialog_function_runner)
        :   HtmlDlgBase(exception_holder),
            m_htmlDialogFunctionRunner(html_dialog_function_runner)
    {
        m_resizable = true;
    }

protected:
    BOOL OnInitDialog() override
    {
        if( m_htmlDialogFunctionRunner.m_displayOptionsJson.IsSet() )
            PostMessage(UWM::Html::ProcessDisplayOptions, reinterpret_cast<WPARAM>(&*m_htmlDialogFunctionRunner.m_displayOptionsJson));

        return HtmlDlgBase::OnInitDialog();
    }

    NavigationAddress GetNavigationAddress() override
    {
        return m_htmlDialogFunctionRunner.m_navigationAddress;
    }

    SharableString GetInputData() override
    {
        return m_htmlDialogFunctionRunner.m_inputData;
    }

private:
    HtmlDialogFunctionRunner& m_htmlDialogFunctionRunner;
};

#endif



// --------------------------------------------------------------------------
// HtmlDialogFunctionRunner
// --------------------------------------------------------------------------

HtmlDialogFunctionRunner::HtmlDialogFunctionRunner(NavigationAddress navigation_address, SharableString input_data, SharableString display_options_json)
    :   m_navigationAddress(std::move(navigation_address)),
        m_inputData(std::move(input_data)),
        m_displayOptionsJson(std::move(display_options_json))
{
}


#ifdef WIN_DESKTOP

std::unique_ptr<HtmlDlgBase> HtmlDialogFunctionRunner::CreateHtmlDlg()
{
    return std::make_unique<HtmlDialogFunctionDlg>(&m_exceptionHolder, *this);
}

#else

SharableString HtmlDialogFunctionRunner::RunHtmlDlg()
{
    // set up an Action Invoker listener to serve the input data
    std::unique_ptr<ActionInvoker::ListenerHolder> action_invoker_listener_holder = ActionInvoker::ListenerHolder::Create<ActionInvoker::OnGetInputDataListener>(
        [&]()
        {
            return m_inputData;
        });

    return PlatformInterface::GetInstance()->GetApplicationInterface()->DisplayHtmlDialogFunctionDlg(m_navigationAddress, GetActionInvokerAccessTokenOverride(),
                                                                                                     m_displayOptionsJson, &m_exceptionHolder);
}

#endif


INT_PTR HtmlDialogFunctionRunner::ProcessResults(const SharableString& results_text)
{
    m_resultsText = results_text;
    return IDOK;
}


void HtmlDialogFunctionRunner::ParseSingleInputText(const std::string& single_input_text, SharableString& input_data, SharableString& display_options_json)
{
    try
    {
        const JsonNode json_node = Json::Parse(single_input_text);

        if( json_node.Contains(JK::inputData) )
            input_data = json_node.Get(JK::inputData).GetNodeAsSharableString();

        if( json_node.Contains(JK::displayOptions) )
            display_options_json = json_node.Get(JK::displayOptions).GetNodeAsSharableString();

        // only use the inputs if something was specified and there were no parsing errors
        if( input_data.IsSet() || display_options_json.IsSet() )
        {
            if( !input_data.IsSet() )
                input_data.ResetToBlank();

            return;
        }
    }
    catch(...) { }

    input_data = single_input_text;
}
