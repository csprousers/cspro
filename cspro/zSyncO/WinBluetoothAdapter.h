#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/IBluetoothAdapter.h>

struct IObexTransport;
struct ISyncListener;
class WinBluetoothFunctions;
class WinBluetoothScanner;


// Bluetooth adapter for Windows that uses Win32 Winsock Bluetooth APIs
class SYNC_API WinBluetoothAdapter : public IBluetoothAdapter
{
private:
    WinBluetoothAdapter(std::shared_ptr<WinBluetoothFunctions> pBtFuncs);

public:
    static WinBluetoothAdapter* create();

    ~WinBluetoothAdapter();

    WinBluetoothAdapter(WinBluetoothAdapter const&) = delete;
    WinBluetoothAdapter(WinBluetoothAdapter&&) = delete;
    WinBluetoothAdapter& operator=(WinBluetoothAdapter const&) = delete;
    WinBluetoothAdapter& operator=(WinBluetoothAdapter &&)= delete;

    IObexTransport* connectToRemoteDevice(CString remoteDeviceName,
        CString remoteDeviceAddress,
        GUID service, ISyncListener* pListener = NULL) override;

    IObexTransport* acceptConnection(
        GUID service,
        ISyncListener* pListener = NULL) override;

    void enable() override;

    void disable() override;

    bool isEnabled() const override;

    WinBluetoothScanner* scanner();

    std::wstring getName() const override;

    void setName(const CString& bluetooth_name) override;

private:
    WinBluetoothScanner* m_pScanner;
    std::shared_ptr<WinBluetoothFunctions> m_pBtFuncs;
};
