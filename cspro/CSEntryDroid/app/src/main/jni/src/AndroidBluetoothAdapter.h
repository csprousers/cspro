#pragma once

#include <zPlatformO/PortableMFC.h>
#include <zSyncO/IBluetoothAdapter.h>
#include <jni.h>

/**
* Bluetooth adapter implementation for Android
* that calls through to Java BluetoothAdapter class via JNI.
*/
class AndroidBluetoothAdapter : public IBluetoothAdapter
{
private:
    AndroidBluetoothAdapter(jobject impl);

public:
    static AndroidBluetoothAdapter* create();

    ~AndroidBluetoothAdapter();

    IObexTransport* connectToRemoteDevice(
            CString remoteDeviceName,
            CString remoteDeviceAddress,
            GUID service,
            ISyncListener* pListener = NULL) override;

    IObexTransport* acceptConnection(
        GUID service,
        ISyncListener* pListener = NULL) override;

    void enable() override;

    void disable() override;

    bool isEnabled() const override;

    std::wstring getName() const override;

    void setName(const CString& bluetooth_name) override;

private:
    jobject m_javaImpl;
};
