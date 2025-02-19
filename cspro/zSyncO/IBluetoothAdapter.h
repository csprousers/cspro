#pragma once

struct IObexTransport;
class SyncListener;


// Interface to Bluetooth adapter on the local device

class IBluetoothAdapter
{
public:
    virtual ~IBluetoothAdapter() { }

    /// <summary>Connect to a remote device as a client.</summary>
    /// <param name="remoteDeviceName">Search for device with this name</param>
    /// <param name="remoteDeviceAddress">Search for device with this address - if not empty name is ignored</param>
    /// <param name="service">ID of service to connect to - will only connect if this service is available</param>
    /// <param name="pListener">Optional listener for progress/cancel</param>
    /// <returns>Transport for reading/writing to/from remote device</returns>
    virtual std::unique_ptr<IObexTransport> ConnectToRemoteDevice(const std::string& remoteDeviceName, const std::string& remoteDeviceAddress,
                                                                  GUID serviceUuid, SyncListener* sync_listener = nullptr) = 0;

    /// <summary>Act as server allowing connections from other devices.</summary>
    /// <param name="service">ID of service to publish</param>
    /// <param name="pListener">Optional listener for progress/cancel</param>
    /// <returns>Transport for reading/writing to/from first device that connects or null if cancelled</returns>
    virtual std::unique_ptr<IObexTransport> AcceptConnection(GUID serviceUuid, SyncListener* sync_listener = nullptr) = 0;

    virtual void Enable() = 0;

    virtual void Disable() = 0;

    virtual bool IsEnabled() const = 0;

    virtual std::string GetName() const = 0;

    // throws a CSProException on failure
    virtual void SetName(const std::string& bluetooth_name) = 0;
};
