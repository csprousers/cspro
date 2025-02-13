#include "stdafx.h"
#include "CSHtmlDlgRunner.h"
#include "HtmlDlgBase.h"
#include <zPlatformO/PlatformInterface.h>
#include <zEngineF/EngineUI.h>
#include <zAction/OnGetInputDataListener.h>


#ifdef WIN_DESKTOP

class CSHtmlDlg : public HtmlDlgBase
{
public:
    CSHtmlDlg(CSHtmlDlgRunner& cshtml_dlg_runner)
        :   m_cshtmlDlgRunner(cshtml_dlg_runner)
    {
        ASSERT(m_resizable == false);
    }

protected:
    NavigationAddress GetNavigationAddress() override
    {
        return m_cshtmlDlgRunner.GetNavigationAddress();
    }

    std::wstring GetInputData() override
    {
        return m_cshtmlDlgRunner.GetJsonArgumentsText();
    }

private:
    CSHtmlDlgRunner& m_cshtmlDlgRunner;
};

#endif


NavigationAddress CSHtmlDlgRunner::GetNavigationAddress()
{
    std::wstring html_filename = SO::Concatenate(GetDialogName(), _T(".html"));

    // the location of the HTML dialogs may be overriden
    std::wstring html_dialogs_directory;
    SendEngineUIMessage(EngineUI::Type::HtmlDialogsDirectoryQuery, html_dialogs_directory);

    if( !html_dialogs_directory.empty() )
    {
        std::wstring full_path = PortableFunctions::PathAppendToPath(html_dialogs_directory, html_filename);

        if( PortableFunctions::FileIsRegular(full_path) )
            return NavigationAddress::CreateHtmlFilenameReference(std::move(full_path));
    }

    std::wstring full_path = PortableFunctions::PathAppendToPath(Html::GetDirectory(Html::Subdirectory::Dialogs), html_filename);

    return NavigationAddress::CreateHtmlFilenameReference(std::move(full_path));
}


#ifdef WIN_DESKTOP

std::unique_ptr<HtmlDlgBase> CSHtmlDlgRunner::CreateHtmlDlg()
{
    return std::make_unique<CSHtmlDlg>(*this);
}

#else

std::optional<std::wstring> CSHtmlDlgRunner::RunHtmlDlg()
{
    // set up an Action Invoker listener to serve the input data
    std::unique_ptr<ActionInvoker::ListenerHolder> action_invoker_listener_holder = ActionInvoker::ListenerHolder::Create<ActionInvoker::OnGetInputDataListener>(
        [&]()
        {
            return GetJsonArgumentsText();
        });

    return PlatformInterface::GetInstance()->GetApplicationInterface()->DisplayCSHtmlDlg(GetNavigationAddress(), GetActionInvokerAccessTokenOverride());
}

#endif


INT_PTR CSHtmlDlgRunner::ProcessResults(const std::optional<std::wstring>& results_text)
{
    // if there are no results, then a HTML UI element canceled the dialog
    if( results_text.has_value() )
    {
        auto json_results = Json::Parse(*results_text);

        if( !json_results.IsEmpty() )
        {
            ProcessJsonResults(json_results);
            return IDOK;
        }
    }

    return IDCANCEL;
}
