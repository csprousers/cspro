#pragma once
#include <zSyncO/ObexPacket.h>
struct IObexTransport;

class ObexPacketSerializer {

public:

    ObexPacketSerializer(IObexTransport* pTransport);

    void sendPacket(const ObexPacket& packet, bool isConnectResponse = false);

    ObexPacket receivePacket(bool isConnectResponse = false);

    void setReceiveTimeout(
        int packetStartTimeoutSecs, // secs to wait for start (first 3 bytes) of packet (-1 to wait indefinitely)
        int packetContinuationTimeoutSecs); // secs to wait for addl data after receiving start

private:

    void waitForData(char* data, int sizeBytes, int timeoutSecs);
    IObexTransport* m_pTransport;
    int m_packetStartTimeoutSecs;
    int m_packetContinuationTimeoutSecs;
};