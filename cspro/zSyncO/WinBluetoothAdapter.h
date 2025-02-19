#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/IBluetoothAdapter.h>

struct IObexTransport;
class SyncListener;
class WinBluetoothFunctions;
class WinBluetoothScanner;


// Bluetooth adapter for Windows that uses Win32 Winsock Bluetooth APIs

class SYNC_API WinBluetoothAdapter : public IBluetoothAdapter
{
private:
    WinBluetoothAdapter(std::shared_ptr<WinBluetoothFunctions> pBtFuncs);

public:
    ~WinBluetoothAdapter();

    static std::unique_ptr<WinBluetoothAdapter> Create();

    WinBluetoothAdapter(const WinBluetoothAdapter &) = delete;
    WinBluetoothAdapter(WinBluetoothAdapter&&) = delete;
    WinBluetoothAdapter& operator=(const WinBluetoothAdapter&) = delete;
    WinBluetoothAdapter& operator=(WinBluetoothAdapter &&)= delete;

    WinBluetoothScanner* GetScanner() { return m_pScanner; }

    // IBluetoothAdapter overrides
    std::unique_ptr<IObexTransport> ConnectToRemoteDevice(const std::string& remoteDeviceName, const std::string& remoteDeviceAddress,
                                                          GUID serviceUuid, SyncListener* sync_listener= nullptr) override;
    std::unique_ptr<IObexTransport> AcceptConnection(GUID serviceUuid, SyncListener* sync_listener = nullptr) override;
    void Enable() override;
    void Disable() override;
    bool IsEnabled() const override;
    std::string GetName() const override;
    void SetName(const std::string& bluetooth_name) override;

private:
    WinBluetoothScanner* m_pScanner;
    std::shared_ptr<WinBluetoothFunctions> m_pBtFuncs;
};
