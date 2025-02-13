#pragma once

#include <zSyncO/zSyncO.h>
#include <zAppO/SyncTypes.h>


// Response from call to server connect remote call.
class SYNC_API ConnectResponse {
public:

    ConnectResponse(const DeviceId& serverId, const CString& serverName, const CString& userName = CString(), float apiVersion = 0.0f);
    ConnectResponse(const DeviceId& serverId, float apiVersion = 0.0f);

    const DeviceId& getServerDeviceId() const;
    const CString& getServerName() const;
    const CString& getUserName() const;
    const float getApiVersion() const;

    void setServerName(CString serverName);
    void setUserName(CString userName);
private:

    const DeviceId m_serverDeviceId;
    CString m_serverName;
    CString m_userName;
    const float m_apiVersion;
};
