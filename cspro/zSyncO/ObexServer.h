#pragma once

struct IObexResource;
struct IObexTransport;
class ObexPacket;
class ObexPacketSerializer;
class SyncListener;
class SyncObexHandler;


//  Server side implementation for exchanging files and data via OBEX protocol.
//  OBEX is a standard protocol for file exchange via Bluetooth and IR.

class ObexServer
{
public:
    // Run server.
    // Pass transport to already connected client.
    ObexServer(SyncObexHandler* pHandler);

    void run(IObexTransport* pTransport);

    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) { m_syncListener = std::move(sync_listener); }

private:
    void handleConnect(ObexPacketSerializer& serializer, const ObexPacket& connectPacket);
    void handleDisconnect(ObexPacketSerializer& serializer, const ObexPacket& connectPacket);
    void handleGet(ObexPacketSerializer& serializer, const ObexPacket& getPacket);
    void handlePut(ObexPacketSerializer& serializer, const ObexPacket& putPacket);
    void destroyResource();

private:
    SyncObexHandler* m_pHandler;
    size_t m_maxPacketSize;
    std::shared_ptr<SyncListener> m_syncListener;
    std::unique_ptr<IObexResource> m_resource;
};
