#include "stdafx.h"
#include <Winsock2.h>
#include "WinObexBluetoothTransport.h"

// Socket should already be connected.
WinObexBluetoothTransport::WinObexBluetoothTransport(SOCKET socket)
    : m_socket(socket)
{
}

WinObexBluetoothTransport::~WinObexBluetoothTransport()
{
    close();
}

int WinObexBluetoothTransport::write(const char* pData, int dataSizeBytes)
{
    return send(m_socket, pData, dataSizeBytes, 0);
}

int WinObexBluetoothTransport::read(char* pBuffer, int bufferSizeBytes, int timeoutMs)
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_socket, &readSet);
    timeval timeout;
    timeval* pTimeout;
    if (timeoutMs > 0) {
        timeout.tv_sec = timeoutMs/1000;
        timeout.tv_usec = timeoutMs % 1000 * 1000;
        pTimeout = &timeout;
    }
    else {
        pTimeout = NULL;
    }
    int selectResult = select(m_socket, &readSet, NULL, NULL, pTimeout);
    if (selectResult == 0)
    {
        // timed out
        return 0;
    }

    return recv(m_socket, pBuffer, bufferSizeBytes, 0);
}

void WinObexBluetoothTransport::close()
{
    if (m_socket != INVALID_SOCKET)
        closesocket(m_socket);
    m_socket = INVALID_SOCKET;
}
