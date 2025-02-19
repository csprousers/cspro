#pragma once

#include <zNetwork/OAuth2Authorizer.h>
#include <zHtml/SimpleServer.h>


// --------------------------------------------------------------------------
// WindowsOAuth2Authorizer
//
// The Windows implementation of the portable OAuth2Authorizer.
// --------------------------------------------------------------------------

class WindowsOAuth2Authorizer
{
public:
    WindowsOAuth2Authorizer(OAuth2Authorizer& oauth2_authorizer);
    ~WindowsOAuth2Authorizer();

    OAuth2Token ShowAuthorizationDialog();

private:
    static std::string GenerateState();

    void HandleAuthorizationRedirect(SimpleServer::Handler& handler);
    void HandleTokenPage(SimpleServer::Handler& handler);

    void ShowAuthorizationDialogFromDotNet();

private:
    void ShowTextAsHtml(SimpleServer::Handler& handler, std::string_view text_sv);

private:
    OAuth2Authorizer& m_oauth2Authorizer;
    const OAuth2AuthenticationParameters& m_parameters;

    std::string m_authorizationUrl;
    std::string m_state;

    SimpleServer m_server;
    std::string m_redirectUri;

    std::unique_ptr<CDialog> m_waitDlg;

    std::string m_authorizationCode;
    std::optional<std::variant<OAuth2Token, std::string>> m_tokenOrExceptionMessage;
};
