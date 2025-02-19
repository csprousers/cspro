#include <engine/StandardSystemIncludes.h>
#include "AndroidBluetoothAdapter.h"
#include "AndroidBluetoothObexTransport.h"
#include <zNetwork/SyncException.h>
#include <zNetwork/SyncListener.h>
#include "JNIHelpers.h"


#define JNI_VERSION JNI_VERSION_1_6

namespace
{
    jobject guidToJava(JNIEnv* pEnv, const GUID& guid)
    {
        jlong mostSig = ((jlong) guid.Data1) << 32 | ((jlong) guid.Data2) << 16 | (jlong) guid.Data3;
        jlong leastSig = ((jlong) guid.Data4[0]) << 56 |
                         ((jlong) guid.Data4[1]) << 48 |
                         ((jlong) guid.Data4[2]) << 40 |
                         ((jlong) guid.Data4[3]) << 32 |
                         ((jlong) guid.Data4[4]) << 24 |
                         ((jlong) guid.Data4[5]) << 16 |
                         ((jlong) guid.Data4[6]) << 8 |
                         ((jlong) guid.Data4[7]);
        return pEnv->NewObject(JNIReferences::classUuid, JNIReferences::methodUuidConstructor, mostSig, leastSig);
    }
}


AndroidBluetoothAdapter::AndroidBluetoothAdapter(jobject impl)
    :   m_javaImpl(impl)
{
}


std::unique_ptr<AndroidBluetoothAdapter> AndroidBluetoothAdapter::Create()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jobject impl = pEnv->CallStaticObjectMethod(JNIReferences::classAndroidBluetoothAdapter, JNIReferences::methodAndroidBluetoothAdapterCreate);

    if (impl)
        return std::unique_ptr<AndroidBluetoothAdapter>(new AndroidBluetoothAdapter(pEnv->NewGlobalRef(impl)));

    return nullptr; // Bluetooth not supported
}


AndroidBluetoothAdapter::~AndroidBluetoothAdapter()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    // Release ref to java implementation
    pEnv->DeleteGlobalRef(m_javaImpl);
}


std::unique_ptr<IObexTransport> AndroidBluetoothAdapter::ConnectToRemoteDevice(const std::string& remoteDeviceName, const std::string& remoteDeviceAddress,
                                                                               GUID serviceUuid, SyncListener* const sync_listener/* = nullptr*/)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    // Convert args to java
    jstring jRemoteDeviceName = JavaString::ToJava(*pEnv, remoteDeviceName);
    jstring jRemoteDeviceAddress = JavaString::ToJava(*pEnv, remoteDeviceAddress);
    jobject jServiceUuid = guidToJava(pEnv, serviceUuid);

    jobject socket = (jobject) pEnv->CallObjectMethod(m_javaImpl,
            JNIReferences::methodAndroidBluetoothAdapterConnectToRemoteDevice,
            jRemoteDeviceName, jRemoteDeviceAddress, jServiceUuid);
    pEnv->DeleteLocalRef(jRemoteDeviceName);
    pEnv->DeleteLocalRef(jRemoteDeviceAddress);
    pEnv->DeleteLocalRef(jServiceUuid);

    // The java side just returns null to signal error (dealing with exceptions
    // across JNI is a pain) and it also returns null on cancel so look
    // at listener to distinguish and match behavior of Windows version.
    if (socket == nullptr && sync_listener != nullptr) {
        sync_listener->Progress(); // to make sure IsCanceled is up to date
        if (sync_listener->IsCanceled())
            throw SyncCancelException();
    }

    if( socket != nullptr )
        return std::make_unique<AndroidBluetoothObexTransport>(pEnv, socket);

    return nullptr;
}


std::unique_ptr<IObexTransport> AndroidBluetoothAdapter::AcceptConnection(GUID serviceUuid, SyncListener* const sync_listener/* = nullptr*/)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    // Convert args to java
    jobject jServiceUuid = guidToJava(pEnv, serviceUuid);

    jobject socket = (jobject) pEnv->CallObjectMethod(m_javaImpl,
            JNIReferences::methodAndroidBluetoothAdapterAcceptConnection,
            jServiceUuid);
    pEnv->DeleteLocalRef(jServiceUuid);

    // The java side just returns null to signal error (dealing with exceptions
    // across JNI is a pain) and it also returns null on cancel so look
    // at listener to distinguish and match behavior of Windows version.
    if (socket == nullptr && sync_listener != nullptr) {
        sync_listener->Progress(); // to make sure IsCanceled is up to date
        if (sync_listener->IsCanceled())
            throw SyncCancelException();
    }

    if( socket != nullptr )
        return std::make_unique<AndroidBluetoothObexTransport>(pEnv, pEnv->NewGlobalRef(socket));

    return nullptr;
}


void AndroidBluetoothAdapter::Enable()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    pEnv->CallVoidMethod(m_javaImpl, JNIReferences::methodAndroidBluetoothAdapterEnable);
}


void AndroidBluetoothAdapter::Disable()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    pEnv->CallVoidMethod(m_javaImpl, JNIReferences::methodAndroidBluetoothAdapterDisable);
}


bool AndroidBluetoothAdapter::IsEnabled() const
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    return pEnv->CallBooleanMethod(m_javaImpl, JNIReferences::methodAndroidBluetoothAdapterIsEnabled);
}


std::string AndroidBluetoothAdapter::GetName() const
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jstring jName = (jstring)pEnv->CallObjectMethod(m_javaImpl, JNIReferences::methodAndroidBluetoothAdapterGetName);
    std::string name = JavaString::ToUtf8(*pEnv, jName);
    pEnv->DeleteLocalRef(jName);
    return name;
}


void AndroidBluetoothAdapter::SetName(const std::string& bluetooth_name)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jBluetoothName(pEnv, JavaString::ToJava(*pEnv, bluetooth_name));

    jstring jExceptionMessage = (jstring)pEnv->CallObjectMethod(m_javaImpl,
        JNIReferences::methodAndroidBluetoothAdapterSetName, jBluetoothName.get());

    if( jExceptionMessage != nullptr )
        throw CSProException(JavaString::ToUtf8(*pEnv, jExceptionMessage));
}
