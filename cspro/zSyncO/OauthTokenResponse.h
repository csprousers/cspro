#pragma once

#include <zSyncO/zSyncO.h>

class SYNC_API OauthTokenResponse {
public:

    OauthTokenResponse(CString accessToken,
        int expiresIn,
        CString tokenType,
        CString scope,
        CString refreshToken);

    CString getAccessToken() const
    {
        return m_accessToken;
    }

    int getExpiresIn() const
    {
        return m_expiresIn;
    }

    CString getTokenType() const
    {
        return m_tokenType;
    }

    CString getScope() const
    {
        return m_scope;
    }

    CString getRefreshToken() const
    {
        return m_refreshToken;
    }

private:
    CString m_accessToken;
    int m_expiresIn;
    CString m_tokenType;
    CString m_scope;
    CString m_refreshToken;
};
