#pragma once

#include <zSyncO/zSyncO.h>

class SYNC_API OauthTokenRequest
{

public:

    enum class GrantType
    {
        Password,
        Refresh
    };

    static OauthTokenRequest CreatePasswordRequest(
        CString clientId,
        CString clientSecret,
        CString username,
        CString password);

    static OauthTokenRequest CreateRefreshRequest(
        CString clientId,
        CString clientSecret,
        CString refreshToken);

    CString getClientId() const
    {
        return m_clientId;
    }

    CString getClientSecret() const
    {
        return m_clientSecret;
    }

    GrantType getGrantType() const
    {
        return m_grantType;
    }

    CString getUsername() const
    {
        return m_username;
    }

    CString getPassword() const
    {
        return m_password;
    }

    CString getRefreshToken() const
    {
        return m_refreshToken;
    }

protected:

    OauthTokenRequest(CString clientId,
        CString clientSecret,
        GrantType grantType,
        CString username,
        CString password,
        CString refreshToken);

private:
    CString m_clientId;
    CString m_clientSecret;
    GrantType m_grantType;
    CString m_username;
    CString m_password;
    CString m_refreshToken;
};
