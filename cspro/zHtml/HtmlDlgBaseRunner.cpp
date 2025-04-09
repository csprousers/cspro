#include "stdafx.h"
#include "HtmlDlgBaseRunner.h"
#include "HtmlDlgBase.h"
#include <zMessageO/MessageBoxSystemMessageIssuer.h>
#include <zAction/AccessToken.h>


HtmlDlgBaseRunner::HtmlDlgBaseRunner()
    :   m_actionInvokerAccessTokenOverride(nullptr)
{
}


INT_PTR HtmlDlgBaseRunner::DoModal(const bool on_ui_thread)
{
    const NavigationAddress navigation_address = GetNavigationAddress();

    if( navigation_address.IsHtmlFilePath() )
    {
        if( !PortableFunctions::FileIsRegular(navigation_address.GetHtmlFilePath()) )
        {
            MessageBoxSystemMessageIssuer().Issue(MessageType::Error, 2031,
                                                  navigation_address.GetName().c_str(),
                                                  navigation_address.GetHtmlFilePath().c_str());

            return IDCANCEL;
        }

        RegisterActionInvokerAccessTokenOverride(navigation_address.GetHtmlFilePath());
    }

    try
    {
#ifdef WIN_DESKTOP
        // refresh the screen so that if logic changed any of the fields on the form, they will appear correctly
        WindowsDesktopMessage::Send(WM_IMSA_CSENTRY_REFRESH_DATA);

        const std::unique_ptr<HtmlDlgBase> html_dlg = CreateHtmlDlg();
        ASSERT(html_dlg != nullptr);

        if( m_actionInvokerAccessTokenOverride != nullptr )
            html_dlg->SetActionInvokerAccessTokenOverride(*m_actionInvokerAccessTokenOverride);

        const INT_PTR dlg_result = on_ui_thread ? html_dlg->DoModalOnUIThread() :
                                                  html_dlg->DoModal();

        if( dlg_result != IDOK )
            return dlg_result;

        return ProcessResults(html_dlg->GetResultsText());

#else
        on_ui_thread; // unreferenced parameter

        return ProcessResults(RunHtmlDlg());

#endif
    }

    catch( const CSProException& exception )
    {
        MessageBoxSystemMessageIssuer().Issue(MessageType::Error, 2032,
                                              navigation_address.GetName().c_str(),
                                              exception.what());

        return IDCANCEL;
    }
}


const std::string* HtmlDlgBaseRunner::RegisterActionInvokerAccessTokenOverride(const std::string& path)
{
    static std::map<std::string, std::unique_ptr<std::string>> cached_access_tokens;

    auto access_token_lookup = cached_access_tokens.find(path);

    if( access_token_lookup == cached_access_tokens.cend() )
    {
        std::unique_ptr<std::string> access_token;

        // only register an access token if the file is in the html directory
        if( SO::StartsWithNoCase(path, Html::GetDirectory()) )
            access_token = std::make_unique<std::string>(ActionInvoker::AccessToken::CreateAccessTokenForHtmlDirectoryFile(path));

        access_token_lookup = cached_access_tokens.try_emplace(path, std::move(access_token)).first;
    }

    m_actionInvokerAccessTokenOverride = access_token_lookup->second.get();

    return m_actionInvokerAccessTokenOverride;
}
