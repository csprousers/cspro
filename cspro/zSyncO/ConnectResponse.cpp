#include "stdafx.h"
#include "ConnectResponse.h"


ConnectResponse::ConnectResponse(const DeviceId& serverDeviceId, const CString& serverName, const CString& userName, float apiVersion)
    : m_serverDeviceId(serverDeviceId),
      m_serverName(serverName),
      m_userName(userName),
      m_apiVersion(apiVersion)
{
}

ConnectResponse::ConnectResponse(const DeviceId& serverDeviceId, float apiVersion)
    : m_serverDeviceId(serverDeviceId),
      m_apiVersion(apiVersion) {
}

const DeviceId& ConnectResponse::getServerDeviceId() const
{
    return m_serverDeviceId;
}

const CString& ConnectResponse::getServerName() const
{
    return m_serverName;
}

const CString& ConnectResponse::getUserName() const
{
    return m_userName;
}

const float ConnectResponse::getApiVersion() const
{
    return m_apiVersion;
}

void ConnectResponse::setServerName(CString serverName)
{
    m_serverName = serverName;
}

void ConnectResponse::setUserName(CString userName)
{
    m_userName = userName;
}
