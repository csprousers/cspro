#pragma once

struct IObexTransport;
struct ISyncListener;

///<summary>Interface to Bluetooth adapter on the local device</summary>
struct IBluetoothAdapter
{
public:
    virtual ~IBluetoothAdapter() {}

    /// <summary>Connect to a remote device as a client.</summary>
    /// <param name="remoteDeviceName">Search for device with this name</param>
    /// <param name="remoteDeviceAddress">Search for device with this address - if not empty name is ignored</param>
    /// <param name="service">ID of service to connect to - will only connect if this service is available</param>
    /// <param name="pListener">Optional listener for progress/cancel</param>
    /// <returns>Transport for reading/writing to/from remote device</returns>
    virtual IObexTransport* connectToRemoteDevice(
        CString remoteDeviceName,
        CString remoteDeviceAddress,
        GUID service,
        ISyncListener* pListener = NULL) = 0;

    /// <summary>Act as server allowing connections from other devices.</summary>
    /// <param name="service">ID of service to publish</param>
    /// <param name="pListener">Optional listener for progress/cancel</param>
    /// <returns>Transport for reading/writing to/from first device that connects or null if cancelled</returns>
    virtual IObexTransport* acceptConnection(
        GUID service,
        ISyncListener* pListener = NULL) = 0;

    virtual void enable() = 0;

    virtual void disable() = 0;

    virtual bool isEnabled() const = 0;

    virtual std::wstring getName() const = 0;

    // throws a CSProException on failure
    virtual void setName(const CString& bluetooth_name) = 0;
};
