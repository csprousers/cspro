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
    CSHtmlDlg(ExceptionHolder* const exception_holder, CSHtmlDlgRunner& cshtml_dlg_runner)
        :   HtmlDlgBase(exception_holder),
            m_cshtmlDlgRunner(cshtml_dlg_runner)
    {
        ASSERT(m_resizable == false);
    }

protected:
    NavigationAddress GetNavigationAddress() override
    {
        return m_cshtmlDlgRunner.GetNavigationAddress();
    }

    SharableString GetInputData() override
    {
        return m_cshtmlDlgRunner.GetJsonArgumentsText();
    }

private:
    CSHtmlDlgRunner& m_cshtmlDlgRunner;
};

#endif


NavigationAddress CSHtmlDlgRunner::GetNavigationAddress()
{
    const std::string html_filename = PortableFunctions::PathAppendFileExtension(GetDialogName(), FileExtensions::HTML);

    // the location of the HTML dialogs may be overridden
    std::string html_dialogs_directory;
    SendEngineUIMessage(EngineUI::Type::HtmlDialogsDirectoryQuery, html_dialogs_directory);

    if( !html_dialogs_directory.empty() )
    {
        std::string full_path = Path::Combine(html_dialogs_directory, html_filename);

        if( PortableFunctions::FileIsRegular(full_path) )
            return NavigationAddress::CreateHtmlFilePathReference(std::move(full_path));
    }

    std::string full_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Dialogs), html_filename);

    return NavigationAddress::CreateHtmlFilePathReference(std::move(full_path));
}


#ifdef WIN_DESKTOP

std::unique_ptr<HtmlDlgBase> CSHtmlDlgRunner::CreateHtmlDlg()
{
    return std::make_unique<CSHtmlDlg>(&m_exceptionHolder, *this);
}

#else

SharableString CSHtmlDlgRunner::RunHtmlDlg()
{
    // set up an Action Invoker listener to serve the input data
    std::unique_ptr<ActionInvoker::ListenerHolder> action_invoker_listener_holder = ActionInvoker::ListenerHolder::Create<ActionInvoker::OnGetInputDataListener>(
        [&]()
        {
            return GetJsonArgumentsText();
        });

    return PlatformInterface::GetInstance()->GetApplicationInterface()->DisplayCSHtmlDlg(GetNavigationAddress(), GetActionInvokerAccessTokenOverride(), &m_exceptionHolder);
}

#endif


INT_PTR CSHtmlDlgRunner::ProcessResults(const SharableString& results_text)
{
    // if there are no results, then a HTML UI element canceled the dialog
    if( results_text.IsSet() )
    {
        const JsonNode json_results = Json::Parse(*results_text);

        if( !json_results.IsEmpty() )
        {
            ProcessJsonResults(json_results);
            return IDOK;
        }
    }

    return IDCANCEL;
}
