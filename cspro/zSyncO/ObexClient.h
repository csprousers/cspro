#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ObexHeader.h>
#include <zSyncO/ObexConstants.h>
#include "ObexPacketSerializer.h"
#include "BluetoothChunk.h"
struct IObexTransport;
class ObexPacket;
struct ISyncListener;
struct IDataChunk;

/**
* Client side implementation for exchanging files and data via OBEX protocol.
* OBEX is a standard protocol for file exchange via Bluetooth and IR.
**/
class SYNC_API ObexClient {
public:

    // Create client.
    // Pass transport to already connected server.
    ObexClient(IObexTransport* pTransport);

    // Send the connect packet to the remote server.
    // This must be sent first before any other operations.
    ObexResponseCode connect(const ObexHeaderList& headers);

    // Send disconnect packet to remote server.
    ObexResponseCode disconnect(const ObexHeaderList& headers);

    // Download a resource from remote server.
    ObexResponseCode get(const ObexHeaderList& headers, std::ostream& response,
        ObexHeaderList& responseHeaders);

    // Upload a resource to remote server.
    ObexResponseCode put(const ObexHeaderList& headers, std::istream& content,
        std::ostream& response, ObexHeaderList& responseHeaders);

    // Get mutable chunk object.
    IDataChunk& getChunk();

    void setListener(ISyncListener* pListener);

private:

    IObexTransport* m_pTransport;
    ObexPacketSerializer m_packetSerializer;
    size_t m_maxPacketSize;
    ISyncListener* m_pListener;
    BluetoothDataChunk m_dataChunk;
};