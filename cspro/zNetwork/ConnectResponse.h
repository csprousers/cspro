#pragma once

#include <zNetwork/zNetwork.h>
#include <zAppO/SyncTypes.h>


// Response from call to server connect remote call.

class ConnectResponse
{
public:
    ConnectResponse(DeviceId server_device_id, std::string server_name, std::string username = std::string(), double api_version = 0);
    ConnectResponse(DeviceId server_device_id, double api_version = 0);

    const DeviceId& GetServerDeviceId() const { return m_serverDeviceId; }

    const std::string& GetServerName() const    { return m_serverName; }
    void SetServerName(std::string server_name) { m_serverName = std::move(server_name); }

    const std::string& GetUsername() const { return m_username; }
    void SetUsername(std::string username) { m_username = std::move(username); }

    double GetApiVersion() const { return m_apiVersion; }

    // throws an exception if not valid
    ZNETWORK_API static ConnectResponse CreateFromJson(const JsonNode& json_node);
    ZNETWORK_API void WriteJson(JsonWriter& json_writer) const;

private:
    DeviceId m_serverDeviceId;
    std::string m_serverName;
    std::string m_username;
    double m_apiVersion;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline ConnectResponse::ConnectResponse(DeviceId server_device_id, std::string server_name, std::string username/* = std::string()*/, const double api_version/* = 0*/)
    :   m_serverDeviceId(std::move(server_device_id)),
        m_serverName(std::move(server_name)),
        m_username(std::move(username)),
        m_apiVersion(api_version)
{
}


inline ConnectResponse::ConnectResponse(DeviceId server_device_id, const double api_version/* = 0*/)
    :   m_serverDeviceId(std::move(server_device_id)),
        m_apiVersion(api_version)
{
}
