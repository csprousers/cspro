#include "stdafx.h"
#include "OauthTokenResponse.h"

OauthTokenResponse::OauthTokenResponse(CString accessToken,
    int expiresIn,
    CString tokenType,
    CString scope,
    CString refreshToken)
    : m_accessToken(accessToken),
    m_expiresIn(expiresIn),
    m_tokenType(tokenType),
    m_scope(scope),
    m_refreshToken(refreshToken)
{
}
