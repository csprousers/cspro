#pragma once

#include <zSyncO/zSyncO.h>
struct IObexTransport;
class SyncObexHandler;
class ObexPacket;
class ObexPacketSerializer;
struct ISyncListener;
struct IObexResource;

/**
* Server side implementation for exchanging files and data via OBEX protocol.
* OBEX is a standard protocol for file exchange via Bluetooth and IR.
**/
class SYNC_API ObexServer {
public:

    // Run server.
    // Pass transport to already connected client.
    ObexServer(SyncObexHandler* pHandler);

    void run(IObexTransport* pTransport);

    void setListener(ISyncListener* pListener);

private:
    void handleConnect(ObexPacketSerializer& serializer, const ObexPacket& connectPacket);
    void handleDisconnect(ObexPacketSerializer& serializer, const ObexPacket& connectPacket);
    void handleGet(ObexPacketSerializer& serializer, const ObexPacket& getPacket);
    void handlePut(ObexPacketSerializer& serializer, const ObexPacket& putPacket);
    void destroyResource();

    SyncObexHandler* m_pHandler;
    size_t m_maxPacketSize;
    ISyncListener* m_pListener;
    std::unique_ptr<IObexResource> m_resource;
};