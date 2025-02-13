#include "AndroidBluetoothAdapter.h"
#include "AndroidBluetoothObexTransport.h"
#include "JNIHelpers.h"
#include <zSyncO/ISyncListener.h>
#include <zSyncO/SyncException.h>


#define JNI_VERSION JNI_VERSION_1_6

namespace {

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
 : m_javaImpl(impl)
{
}

AndroidBluetoothAdapter* AndroidBluetoothAdapter::create()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jobject impl = pEnv->CallStaticObjectMethod(JNIReferences::classAndroidBluetoothAdapter, JNIReferences::methodAndroidBluetoothAdapterCreate);
    if (impl)
        return new AndroidBluetoothAdapter(pEnv->NewGlobalRef(impl));
    else
        return nullptr; // Bluetooth not supported
}

AndroidBluetoothAdapter::~AndroidBluetoothAdapter()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    // Release ref to java implementation
    pEnv->DeleteGlobalRef(m_javaImpl);
}

IObexTransport* AndroidBluetoothAdapter::connectToRemoteDevice(
        CString remoteDeviceName,
        CString remoteDeviceAddress,
        GUID serviceUuid, ISyncListener* pListener)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    // Convert args to java
    jstring jRemoteDeviceName = WideToJava(pEnv, remoteDeviceName);
    jstring jRemoteDeviceAddress = WideToJava(pEnv, remoteDeviceAddress);
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
    if (socket == NULL && pListener) {
        pListener->onProgress(); // to make sure isCancelled is up to date
        if (pListener->isCancelled())
            throw SyncCancelException();
    }

    AndroidBluetoothObexTransport* transport = (socket == NULL) ?
            NULL :
            new AndroidBluetoothObexTransport(pEnv, socket);
    return transport;
}

IObexTransport* AndroidBluetoothAdapter::acceptConnection(
    GUID serviceUuid,
    ISyncListener* pListener /*= NULL*/)
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
    if (socket == NULL && pListener) {
        pListener->onProgress(); // to make sure isCancelled is up to date
        if (pListener->isCancelled())
            throw SyncCancelException();
    }

    return socket == NULL ? NULL : new AndroidBluetoothObexTransport(pEnv, pEnv->NewGlobalRef(socket));
}

void AndroidBluetoothAdapter::enable()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    pEnv->CallVoidMethod(m_javaImpl,
            JNIReferences::methodAndroidBluetoothAdapterEnable);
}

void AndroidBluetoothAdapter::disable()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    pEnv->CallVoidMethod(m_javaImpl,
            JNIReferences::methodAndroidBluetoothAdapterDisable);
}

bool AndroidBluetoothAdapter::isEnabled() const
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    return pEnv->CallBooleanMethod(m_javaImpl,
            JNIReferences::methodAndroidBluetoothAdapterIsEnabled);

}

std::wstring AndroidBluetoothAdapter::getName() const
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jstring jName = (jstring) pEnv->CallObjectMethod(m_javaImpl,
                JNIReferences::methodAndroidBluetoothAdapterGetName);
    std::wstring name = JavaToWSZ(pEnv, jName);
    pEnv->DeleteLocalRef(jName);
    return name;
}

void AndroidBluetoothAdapter::setName(const CString& bluetooth_name)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jBluetoothName(pEnv, WideToJava(pEnv, bluetooth_name));

    jstring jExceptionMessage = (jstring)pEnv->CallObjectMethod(m_javaImpl,
        JNIReferences::methodAndroidBluetoothAdapterSetName, jBluetoothName.get());

    if( jExceptionMessage != nullptr )
        throw CSProException(JavaToWSZ(pEnv, jExceptionMessage));
}
