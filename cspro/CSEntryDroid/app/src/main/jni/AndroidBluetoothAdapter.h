#pragma once

#include <zPlatformO/PortableMFC.h>
#include <zSyncO/IBluetoothAdapter.h>
#include <jni.h>

//  Bluetooth adapter implementation for Android that calls through to Java BluetoothAdapter class via JNI.

class AndroidBluetoothAdapter : public IBluetoothAdapter
{
private:
    AndroidBluetoothAdapter(jobject impl);

public:
    static std::unique_ptr<AndroidBluetoothAdapter> Create();

    ~AndroidBluetoothAdapter();

    // IBluetoothAdapter overrides
    std::unique_ptr<IObexTransport> ConnectToRemoteDevice(const std::string& remoteDeviceName, const std::string& remoteDeviceAddress,
                                                          GUID serviceUuid, SyncListener* sync_listener = nullptr) override;
    std::unique_ptr<IObexTransport> AcceptConnection(GUID serviceUuid, SyncListener* sync_listener = nullptr) override;
    void Enable() override;
    void Disable() override;
    bool IsEnabled() const override;
    std::string GetName() const override;
    void SetName(const std::string& bluetooth_name) override;

private:
    jobject m_javaImpl;
};
