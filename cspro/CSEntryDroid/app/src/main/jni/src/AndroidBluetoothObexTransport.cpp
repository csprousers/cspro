#include <jni.h>
#include <engine/StandardSystemIncludes.h>
#include "JNIHelpers.h"
#include "AndroidBluetoothObexTransport.h"


AndroidBluetoothObexTransport::AndroidBluetoothObexTransport(JNIEnv* pEnv, jobject socket)
    : m_pEnv(pEnv)
{
    jobject impl = pEnv->NewObject(JNIReferences::classBluetoothObexTransport,
            JNIReferences::methodBluetoothObexTransportConstructor, socket);
    m_javaImpl = pEnv->NewGlobalRef(impl);

}

AndroidBluetoothObexTransport::~AndroidBluetoothObexTransport()
{
    close();
}

// Write pData to remote device and return bytes written.
int AndroidBluetoothObexTransport::write(const char* pData, int dataSizeBytes)
{
    jbyteArray jdata = m_pEnv->NewByteArray(dataSizeBytes);
    m_pEnv->SetByteArrayRegion(jdata, 0, dataSizeBytes, (jbyte*) pData);
    m_pEnv->CallVoidMethod(m_javaImpl,
            JNIReferences::methodBluetoothObexTransportWrite, jdata);

    // Write could throw an IOException
    if (m_pEnv->ExceptionCheck()) {
        m_pEnv->ExceptionClear();
        m_pEnv->DeleteLocalRef(jdata);
        return -1;
    }

    m_pEnv->DeleteLocalRef(jdata);
    return dataSizeBytes;
}

// Read up to bufferSizeBytes from remote device into pBuffer and return bytes read.
int AndroidBluetoothObexTransport::read(char* pBuffer, int bufferSizeBytes, int timeoutMs)
{
    jbyteArray jdata = m_pEnv->NewByteArray(bufferSizeBytes);
    jint bytesRead = m_pEnv->CallIntMethod(m_javaImpl,
            JNIReferences::methodBluetoothObexTransportRead,
            jdata, bufferSizeBytes, timeoutMs);

    // Read could throw an IOException
    if (m_pEnv->ExceptionCheck()) {
        m_pEnv->ExceptionClear();
        m_pEnv->DeleteLocalRef(jdata);
        return -1;
    }

    if (bytesRead > 0) {
        m_pEnv->GetByteArrayRegion(jdata, 0, bytesRead, (jbyte*) pBuffer);
    }
    m_pEnv->DeleteLocalRef(jdata);
    return bytesRead;
}

void AndroidBluetoothObexTransport::close()
{
    if (m_javaImpl != NULL) {
        m_pEnv->CallVoidMethod(m_javaImpl,
                JNIReferences::methodBluetoothObexTransportClose);
        if (m_pEnv->ExceptionCheck()) {
            m_pEnv->ExceptionClear();
        }
        m_pEnv->DeleteGlobalRef(m_javaImpl);
        m_javaImpl = NULL;
    }
}
