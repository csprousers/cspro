#include "stdafx.h"
#include "OauthTokenRequest.h"

OauthTokenRequest::OauthTokenRequest(
    CString clientId,
    CString clientSecret,
    GrantType grantType,
    CString username,
    CString password,
    CString refreshToken)
    : m_clientId(clientId),
      m_clientSecret(clientSecret),
      m_grantType(grantType),
      m_username(username),
      m_password(password),
      m_refreshToken(refreshToken)
{
}

OauthTokenRequest OauthTokenRequest::CreatePasswordRequest(CString clientId, CString clientSecret, CString username, CString password)
{
    return OauthTokenRequest(clientId, clientSecret, GrantType::Password, username, password, CString());
}

OauthTokenRequest OauthTokenRequest::CreateRefreshRequest(CString clientId, CString clientSecret, CString refreshToken)
{
    return OauthTokenRequest(clientId, clientSecret, GrantType::Refresh, CString(), CString(), refreshToken);
}
