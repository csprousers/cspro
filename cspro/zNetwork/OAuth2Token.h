#pragma once

#include <zNetwork/zNetwork.h>


// --------------------------------------------------------------------------
// OAuth2Token
// --------------------------------------------------------------------------

class OAuth2Token
{
public:
    OAuth2Token(std::string access_token, int expires_in, std::string token_type, std::string scope, std::string refresh_token);

    const std::string& GetAccessToken() const { return m_accessToken; }

    int GetExpiresIn() const { return m_expiresIn; }

    const std::string& GetTokenType() const { return m_tokenType; }

    const std::string& GetScope() const { return m_scope; }

    const std::string& GetRefreshToken() const { return m_refreshToken; }

    // throws an exception if not valid
    ZNETWORK_API static OAuth2Token CreateFromJson(const JsonNode& json_node);
    ZNETWORK_API void WriteJson(JsonWriter& json_writer) const;

private:
    std::string m_accessToken;
    int m_expiresIn;
    std::string m_tokenType;
    std::string m_scope;
    std::string m_refreshToken;
};



// --------------------------------------------------------------------------
// OAuth2TokenRequest
// --------------------------------------------------------------------------

class OAuth2TokenRequest
{
public:
    enum class GrantType { Password, Refresh };

protected:
    OAuth2TokenRequest(std::string client_id, std::string client_secret, GrantType grant_type, std::string username, std::string password, std::string refresh_token);

public:
    static OAuth2TokenRequest CreatePasswordRequest(std::string client_id, std::string client_secret, std::string username, std::string password);
    static OAuth2TokenRequest CreateRefreshRequest(std::string client_id, std::string client_secret, std::string refresh_token);

    const std::string& GetClientId() const { return m_clientId; }

    const std::string& GetClientSecret() const { return m_clientSecret; }

    GrantType GetGrantType() const { return m_grantType; }

    const std::string& GetUsername() const { return m_username; }

    const std::string& GetPassword() const { return m_password; }

    const std::string& GetRefreshToken() const { return m_refreshToken; }

    ZNETWORK_API void WriteJson(JsonWriter& json_writer) const;

private:
    std::string m_clientId;
    std::string m_clientSecret;
    GrantType m_grantType;
    std::string m_username;
    std::string m_password;
    std::string m_refreshToken;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline OAuth2Token::OAuth2Token(std::string access_token, int expires_in, std::string token_type, std::string scope, std::string refresh_token)
    :   m_accessToken(std::move(access_token)),
        m_expiresIn(expires_in),
        m_tokenType(std::move(token_type)),
        m_scope(std::move(scope)),
        m_refreshToken(std::move(refresh_token))
{
}


inline OAuth2TokenRequest::OAuth2TokenRequest(std::string client_id, std::string client_secret, GrantType grant_type, std::string username, std::string password, std::string refresh_token)
    :   m_clientId(std::move(client_id)),
        m_clientSecret(std::move(client_secret)),
        m_grantType(grant_type),
        m_username(std::move(username)),
        m_password(std::move(password)),
        m_refreshToken(std::move(refresh_token))
{
}


inline OAuth2TokenRequest OAuth2TokenRequest::CreatePasswordRequest(std::string client_id, std::string client_secret, std::string username, std::string password)
{
    return OAuth2TokenRequest(std::move(client_id), std::move(client_secret), GrantType::Password, std::move(username), std::move(password), std::string());
}


inline OAuth2TokenRequest OAuth2TokenRequest::CreateRefreshRequest(std::string client_id, std::string client_secret, std::string refresh_token)
{
    return OAuth2TokenRequest(std::move(client_id), std::move(client_secret), GrantType::Refresh, std::string(), std::string(), std::move(refresh_token));
}
