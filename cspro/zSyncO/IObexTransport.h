#pragma once

#include <zSyncO/zSyncO.h>

/**
* Interface for underlying communications protocol used by OBEX client and server.
*
**/
struct IObexTransport {

    // Write pData to remote device and return bytes written.
    virtual int write(const char* pData, int dataSizeBytes) = 0;

    // Read up to bufferSizeBytes from remote device into pBuffer and return bytes read.
    virtual int read(char* pBuffer, int bufferSizeBytes, int timeoutMs) = 0;

    virtual void close() = 0;

    virtual ~IObexTransport() {};
};