#include "stdafx.h"
#include <sstream>
#include <assert.h>
#include "ObexPacketSerializer.h"
#include "IObexTransport.h"
#include "ObexPacket.h"
#include "SyncException.h"

ObexPacketSerializer::ObexPacketSerializer(IObexTransport * pTransport)
    : m_pTransport(pTransport),
    m_packetStartTimeoutSecs(15),
    m_packetContinuationTimeoutSecs(5)
{
}

void ObexPacketSerializer::sendPacket(const ObexPacket & packet, bool isConnect /*= false*/)
{
    std::ostringstream buffer;
    buffer << (unsigned char)packet.getCode();
    int totalSize = (int) (packet.getHeaders().getTotalSizeBytes() + OBEX_BASE_PACKET_SIZE);
    if (isConnect) {
        totalSize += 4;
    }
    buffer << (unsigned char)(totalSize >> 8);
    buffer << (unsigned char)(totalSize & 0xFF);

    if (isConnect) {
        buffer << packet.getObexVersion();
        buffer << packet.getFlags();
        buffer << (unsigned char)(packet.getMaxPacketSize() >> 8);
        buffer << (unsigned char)(packet.getMaxPacketSize() & 0xFF);
    }
    buffer << packet.getHeaders();

    std::string packetData = buffer.str();
    assert(packetData.size() == (size_t) totalSize);
    int bytesWritten = m_pTransport->write(&packetData[0], totalSize);
    if (bytesWritten != totalSize)
        throw SyncError(100101, L"Error sending data to server");
}

ObexPacket ObexPacketSerializer::receivePacket(bool isConnectResponse /* = false */)
{
    ObexPacket packet;

    // Read the header and packet size
    char basePacket[OBEX_BASE_PACKET_SIZE];
    waitForData(basePacket, OBEX_BASE_PACKET_SIZE, m_packetStartTimeoutSecs);

    packet.setCode(basePacket[0]);
    int receivedPacketSize = (((unsigned char)basePacket[1]) << 8) | ((unsigned char)basePacket[2]);
    if (receivedPacketSize < OBEX_BASE_PACKET_SIZE)
        throw SyncError(100101, L"Invalid packet size");
    int remainingPacketSize = receivedPacketSize - OBEX_BASE_PACKET_SIZE;
    if (remainingPacketSize == 0)
        return packet;

    // Read the rest of the packet
    std::string receivedPacket;
    receivedPacket.resize(remainingPacketSize);
    waitForData(&receivedPacket[0], remainingPacketSize, m_packetContinuationTimeoutSecs);
    std::istringstream receivedPacketStream(receivedPacket);

    if (((unsigned char) basePacket[0]) == OBEX_CONNECT || isConnectResponse) {
        if (receivedPacketSize < OBEX_BASE_PACKET_SIZE + 4)
            throw SyncError(100101, L"Invalid packet size");
        packet.setObexVersion((unsigned char)receivedPacketStream.get());
        packet.setFlags((unsigned char)receivedPacketStream.get());
        int packetSize = ((unsigned char)receivedPacketStream.get()) << 8 | ((unsigned char)receivedPacketStream.get());
        packet.setMaxPacketSize(packetSize);
    }

    // parse the headers
    while (receivedPacketStream) {
        ObexHeader h;
        receivedPacketStream >> h;
        packet.addHeader(h);
    }

    if (!receivedPacketStream.eof())
        throw SyncError(100101, L"OBEX packet is not expected size");

    return packet;
}

void ObexPacketSerializer::setReceiveTimeout(int packetStartTimeoutSecs, int packetContinuationTimeoutSecs)
{
    m_packetStartTimeoutSecs = packetStartTimeoutSecs;
    m_packetContinuationTimeoutSecs = packetContinuationTimeoutSecs;
}

void ObexPacketSerializer::waitForData(char * data, int sizeBytes, int timeoutSecs)
{
    time_t startTime;
    time(&startTime);

    const int timeoutMs = timeoutSecs < 0 ? -1 : timeoutSecs * 1000;
    int sizeRemaining = sizeBytes;
    char* buffPtr = data;
    while (sizeRemaining > 0) {
        int numRead = m_pTransport->read(buffPtr, sizeRemaining, timeoutMs);
        if (numRead < 0)
            throw SyncError(100159);

        sizeRemaining -= numRead;
        buffPtr += numRead;

        if (numRead == 0) {
            // No data - check to see if we went past timeout
            time_t currentTime;
            time(&currentTime);
            if (timeoutSecs != -1 && difftime(currentTime, startTime) > timeoutSecs)
                throw SyncError(100101, L"Timeout waiting for response from server");
        }
        else {
            // Got some data - restart the timeout
            time(&startTime);
        }
    }
}
