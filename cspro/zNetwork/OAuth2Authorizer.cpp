#include "stdafx.h"
#include "OAuth2Authorizer.h"
#include "HttpConnection.h"

#ifdef WIN_DESKTOP
#include "WindowsOAuth2Authorizer.h"
#else
#include <zPlatformO/PlatformInterface.h>
#endif

#if __has_include(<zToolsO/ApiKeys.h>)
#include <zToolsO/ApiKeys.h>
#else
static_assert(false, "You need ApiKeys.h to build CSPro")
#endif


OAuth2Authorizer::OAuth2Authorizer(const OAuth2ClientType client_type, std::shared_ptr<HttpConnection> http_connection)
    :   m_parameters(GetParameters(client_type)),
        m_httpConnection(std::move(http_connection))
{
    ASSERT(m_httpConnection != nullptr);
}


OAuth2Token OAuth2Authorizer::Authorize()
{
    try
    {
#ifdef WIN_DESKTOP
        WindowsOAuth2Authorizer authorizer(*this);
        return authorizer.ShowAuthorizationDialog();
#else
        return PlatformInterface::GetInstance()->GetApplicationInterface()->OAuth2Authorize(*this);
#endif
    }

    catch( const SyncException& exception )
    {
        ASSERT(dynamic_cast<const SyncConnectionError*>(&exception) != nullptr ||
               dynamic_cast<const SyncCancelException*>(&exception) != nullptr);
        throw;
    }

    catch( const CSProException& exception )
    {
        throw SyncConnectionError(exception.what());
    }
}


OAuth2Token OAuth2Authorizer::GetTokenFromAuthorizationCode(const std::string& authorization_code, const std::string& redirect_uri)
{
    ASSERT(!authorization_code.empty());

    // send a POST query to get the token from the authorization code
    std::string request_body = SO::Concatenate("grant_type=authorization_code"
                                               "&code=", authorization_code,
                                               "&client_id=", m_parameters.client_id,
                                               "&client_secret=", m_parameters.client_secret,
                                               "&redirect_uri=", redirect_uri);

    const int64_t request_body_length = request_body.length();

    HeaderList headers;
    headers.Add_Accept_Json()
           .Add_ContentType_FormUrlEncoded()
           .Add_ContentLength(request_body_length);

    HttpRequestBuilder request_builder(m_parameters.token_endpoint, std::move(headers));

    std::istringstream input_stream(std::move(request_body));
    request_builder.post(input_stream, request_body_length);

    const HttpRequest request = request_builder.build();

    try
    {
        const HttpResponse response = m_httpConnection->Request(request);

        if( response.http_status != HttpResponse::Status_200_OK )
            throw CSProException("GetTokenFromAuthorizationCode: Invalid server response: (%d)", response.http_status);

        const JsonNode json_node = Json::Parse(response.body.ToString());
        return json_node.Get<OAuth2Token>();
    }

    catch( const CSProException& exception )
    {
        throw SyncConnectionError("There was an error retrieving credentials from %s: %s",
                                  m_parameters.client_name.c_str(), exception.what());
    }
}


OAuth2Token OAuth2Authorizer::Refresh(const std::string& refresh_token)
{
    ASSERT(!refresh_token.empty());

    // send a POST query to get the refresh token
    std::string request_body = SO::Concatenate("grant_type=refresh_token"
                                               "&refresh_token=", refresh_token,
                                               "&client_id=", m_parameters.client_id,
                                               "&client_secret=", m_parameters.client_secret);

    const int64_t request_body_length = request_body.length();

    HeaderList headers;
    headers.Add_Accept_Json()
           .Add_ContentType_FormUrlEncoded()
           .Add_ContentLength(request_body_length);

    HttpRequestBuilder request_builder(m_parameters.token_endpoint, std::move(headers));

    std::istringstream input_stream(std::move(request_body));
    request_builder.post(input_stream, request_body_length);

    const HttpRequest request = request_builder.build();

    try
    {
        const HttpResponse response = m_httpConnection->Request(request);

        if( response.http_status != HttpResponse::Status_200_OK )
            throw SyncConnectionError("There was an error refreshing credentials from " + m_parameters.client_name);

        const JsonNode json_node = Json::Parse(response.body.ToString());
        return json_node.Get<OAuth2Token>();
    }

    catch( const SyncException& exception )
    {
        ASSERT(dynamic_cast<const SyncConnectionError*>(&exception) != nullptr);
        throw;
    }

    catch( const CSProException& exception )
    {
        throw SyncConnectionError(exception.what());
    }
}


OAuth2AuthenticationParameters OAuth2Authorizer::GetParameters(const OAuth2ClientType client_type)
{
    // Dropbox
    if( client_type == OAuth2ClientType::Dropbox )
    {
        return OAuth2AuthenticationParameters
        {
            client_type,
            "Dropbox",
            DropboxKeys::client_id,
            DropboxKeys::client_secret,
            "https://www.dropbox.com/oauth2/authorize",
            "https://api.dropbox.com/oauth2/token",
            "error_description",
            {
                { "token_access_type", "offline"                 },
                { "locale",             GetLocaleLanguage(false) },
            }
        };
    }

    // Google Drive
    else if( client_type == OAuth2ClientType::GoogleDrive )
    {
        return OAuth2AuthenticationParameters
        {
            client_type,
            "Google Drive",
            GoogleDriveKeys::client_id,
            GoogleDriveKeys::client_secret,
            "https://accounts.google.com/o/oauth2/v2/auth",
            "https://oauth2.googleapis.com/token",
            std::string(),
            {
                { "access_type", "offline"                               },
                { "scope",       "https://www.googleapis.com/auth/drive" }
            }
        };
    }

    else
    {
        throw ProgrammingErrorException();
    }
}
