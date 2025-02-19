#pragma once

#include <zSyncO/IObexTransport.h>


// Obex transport using winsock bluetooth APIs
class WinObexBluetoothTransport : public IObexTransport
{
public:
    /// <summary>Create transport from winsock socket</summary>
    /// Socket should already be connected.
    WinObexBluetoothTransport(SOCKET socket);

    ~WinObexBluetoothTransport();

    int write(const char* pData, int dataSizeBytes);

    int read(char* pBuffer, int bufferSizeBytes, int timeoutMs);

    void close();

private:
    SOCKET m_socket;
};
