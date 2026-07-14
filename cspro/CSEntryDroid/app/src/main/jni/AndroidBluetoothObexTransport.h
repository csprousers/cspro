#pragma once
#include <zSyncO/IObexTransport.h>

/**
* Bluetooth transport implementation for Android
* that calls through to Java BluetoothSocket class via JNI.
*/
class AndroidBluetoothObexTransport : public IObexTransport {

public:

    AndroidBluetoothObexTransport(JNIEnv* pEnv, jobject socket);

    ~AndroidBluetoothObexTransport();

    // Write pData to remote device and return bytes written.
    int write(const char* pData, int dataSizeBytes);

    // Read up to bufferSizeBytes from remote device into pBuffer and
    // return number of bytes read.
    int read(char* pBuffer, int bufferSizeBytes, int timeoutMs);

    void close();

private:
    JNIEnv* m_pEnv;
    jobject m_javaImpl;
};
