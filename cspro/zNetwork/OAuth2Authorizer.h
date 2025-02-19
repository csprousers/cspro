#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/OAuth2AuthenticationParameters.h>
#include <zNetwork/OAuth2Token.h>

class HttpConnection;


// --------------------------------------------------------------------------
// OAuth2Authorizer
//
// OAuth2Authorizer is a portable OAuth 2.0 authorizer.
//
//     - On Windows, the authorization is done in the user's web browser with
//       the response handled by a local web server. A dialog allowing the
//       user to cancel authorization is shown on the UI thread if possible,
//       directly if not, and for .NET programs, is shown via
//       OAuth2Authorizer.exe.
//
//     - On Android, authorization is handled in Java/Kotlin code.
// --------------------------------------------------------------------------

class ZNETWORK_API OAuth2Authorizer
{
public:
    OAuth2Authorizer(OAuth2ClientType client_type, std::shared_ptr<HttpConnection> http_connection);

    const OAuth2AuthenticationParameters& GetParameters() const { return m_parameters; }

    // Returns an access token, and when present, a refresh token and other details (e.g., expiration and scope).
    // If an exception is not thrown, then an access token was successfully retrieved, but it is not guaranteed that a refresh token was returned by the server.
    // SyncConnectionError or SyncCancelException are thrown on authorization errors.
    OAuth2Token Authorize();

    // Returns an access token from an authorization token. The redirect_url should already be encoded as a URI component.
    // SyncConnectionError is thrown on error.
    OAuth2Token GetTokenFromAuthorizationCode(const std::string& authorization_code, const std::string& redirect_uri);

    // Returns an access token from a refresh token.
    // SyncConnectionError is thrown on error.
    OAuth2Token Refresh(const std::string& refresh_token);

private:
    static OAuth2AuthenticationParameters GetParameters(OAuth2ClientType client_type);

private:
    OAuth2AuthenticationParameters m_parameters;
    std::shared_ptr<HttpConnection> m_httpConnection;
};
