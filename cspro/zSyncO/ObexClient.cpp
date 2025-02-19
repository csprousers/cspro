#include "stdafx.h"
#include "ObexClient.h"
#include "IObexTransport.h"
#include "ObexPacket.h"


ObexClient::ObexClient(IObexTransport* pTransport)
    :   m_pTransport(pTransport),
        m_packetSerializer(pTransport),
        m_maxPacketSize(OBEX_MIN_PACKET_SIZE)
{
}


ObexResponseCode ObexClient::connect(const ObexHeaderList& headers)
{
    ObexPacket connectRequest(OBEX_CONNECT, OBEX_VERSION, 0, OBEX_MAX_PACKET_SIZE, headers);

    const bool isConnect = true;
    m_packetSerializer.sendPacket(connectRequest, isConnect);

    ObexPacket connectResponse = m_packetSerializer.receivePacket(isConnect);
    if (connectResponse.getMaxPacketSize() > OBEX_MAX_PACKET_SIZE ||
        connectResponse.getMaxPacketSize() < OBEX_MIN_PACKET_SIZE)
        throw SyncConnectionError("Invalid packet size");

    // Receive server Bluetooth protocol version from server, so compatibility can be detected
    unsigned int serverBluetoothProtocolVersion = 0; // If version header is not found, default to 0
    const ObexHeader* pBluetoothProtocolVersionHeader = connectResponse.getHeaders().find(OBEX_HEADER_BLUETOOTH_PROTOCOL_VERSION);
    if (pBluetoothProtocolVersionHeader) {
        serverBluetoothProtocolVersion = pBluetoothProtocolVersionHeader->getIntData();
    }

    m_maxPacketSize = connectResponse.getMaxPacketSize();

    SYNCLOG_INFO << "Bluetooth protocol version: " << BLUETOOTH_PROTOCOL_VERSION;
    if (serverBluetoothProtocolVersion != BLUETOOTH_PROTOCOL_VERSION) {
        SYNCLOG_ERROR << "Bluetooth protocol version mismatch: client ("
            << BLUETOOTH_PROTOCOL_VERSION << ") != server (" << serverBluetoothProtocolVersion << ")";

        throw SyncError(100145);
    }

    return OBEX_OK;
}


ObexResponseCode ObexClient::disconnect(const ObexHeaderList& headers)
{
    ObexPacket request(OBEX_DISCONNECT, headers);
    m_packetSerializer.sendPacket(request);

    ObexPacket response = m_packetSerializer.receivePacket();
    return (ObexResponseCode) response.getCode();
}


ObexResponseCode ObexClient::get(const ObexHeaderList& headers, std::ostream& content, ObexHeaderList& responseHeaders)
{
    ObexPacket getPacket(OBEX_GET | OBEX_FINAL, headers);
    ObexPacket responsePacket;
    bool haveBodyLengthFromHeader = false;
    uint64_t responseBodyLengthFromHeader = 0;
    uint64_t responseBodyBytesReceived = 0;
    bool firstPacket = true;

    if (m_syncListener != nullptr) {
        CString name;
        const ObexHeader* nameHeader = headers.find(OBEX_HEADER_NAME);
        if (nameHeader) {
            name = nameHeader->getStringData();
        }
        m_syncListener->Progress(0, 100107, UTF8_TODO::GetUtf8(name).c_str());

        if (m_syncListener->IsCanceled()) {
            throw SyncCancelException();
        }
    }

    while (true) {

        m_packetSerializer.sendPacket(getPacket);

        responsePacket = m_packetSerializer.receivePacket();

        if (IsObexError((ObexResponseCode) responsePacket.getCode())) {
            return (ObexResponseCode) responsePacket.getCode();
        }

        if (responsePacket.getHeaders().getBodyLength(responseBodyLengthFromHeader)) {
            haveBodyLengthFromHeader = true;
            if (m_syncListener != nullptr) {
                m_syncListener->SetProgressTotal(responseBodyLengthFromHeader);
            }

            if (firstPacket) {
                m_dataChunk.Optimize(responseBodyLengthFromHeader, m_maxPacketSize);
                firstPacket = false;
            }
        }

        for (std::vector<ObexHeader>::const_iterator ih = responsePacket.getHeaders().getHeaders().begin();
            ih != responsePacket.getHeaders().getHeaders().end();
            ++ih)
        {
            switch (ih->getCode()) {
            case OBEX_HEADER_BODY:
            case OBEX_HEADER_END_OF_BODY:
                content.write(ih->getByteSequenceData(), ih->getByteSequenceDataSize());
                responseBodyBytesReceived += ih->getByteSequenceDataSize();
                break;
            default:
                responseHeaders.add(*ih);
            }
        }

        if (responsePacket.getCode() != OBEX_CONTINUE) {
            break;
        }

        if (m_syncListener != nullptr) {
            if (haveBodyLengthFromHeader) {
                m_syncListener->Progress(responseBodyBytesReceived);
            }
            if (m_syncListener->IsCanceled()) {
                m_packetSerializer.sendPacket(OBEX_ABORT);
                m_packetSerializer.receivePacket();
                throw SyncCancelException();
            }
        }
    }

    if (haveBodyLengthFromHeader && responseBodyLengthFromHeader != responseBodyBytesReceived) {
        // Bytes received did not match number specified in length header
        throw SyncConnectionError("Missing data in get");
    }

    return (ObexResponseCode) responsePacket.getCode();
}


ObexResponseCode ObexClient::put(const ObexHeaderList& headers, std::istream& content, std::ostream& response, ObexHeaderList& responseHeaders)
{
    size_t headerLength = headers.getTotalSizeBytes();
    bool firstPacket = true;
    uint64_t bodyLength = 0;
    bool haveBodyLength = headers.getBodyLength(bodyLength);
    uint64_t bodyBytesReceivedSoFar = 0;
    if (m_syncListener != nullptr) {
        CString name;
        const ObexHeader* nameHeader = headers.find(OBEX_HEADER_NAME);
        if (nameHeader) {
            name = nameHeader->getStringData();
        }
        if (m_syncListener->GetProgressTotal() <= 0) {
            m_syncListener->SetProgressTotal(bodyLength);
        }
        m_syncListener->Progress(0, 100108, UTF8_TODO::GetUtf8(name).c_str());
        if (m_syncListener->IsCanceled())
            throw SyncCancelException();
    }

    ObexPacket responsePacket;

    // Send the request
    while (true) {
        size_t maxBodySize = m_maxPacketSize - OBEX_BASE_PACKET_SIZE - ObexHeader::getBaseSize(OBEX_HEADER_BODY);
        if (firstPacket) {
            maxBodySize -= headerLength;
            if (haveBodyLength) {
                m_dataChunk.Optimize(bodyLength, m_maxPacketSize);
            }
        }

        std::string bodyData;
        bodyData.resize(maxBodySize);
        content.read(&bodyData[0], maxBodySize);
        int actualBodySize = (int) content.gcount();

        ObexHeader bodyHeader(OBEX_HEADER_BODY, &bodyData[0], actualBodySize);

        ObexPacket requestPacket;
        requestPacket.setCode(OBEX_PUT);
        if (firstPacket) {
            requestPacket.setHeaders(headers);
        }
        requestPacket.addHeader(bodyHeader);

        m_packetSerializer.sendPacket(requestPacket);
        bodyBytesReceivedSoFar += actualBodySize;

        responsePacket = m_packetSerializer.receivePacket();
        if (IsObexError((ObexResponseCode) responsePacket.getCode())) {
            return (ObexResponseCode) responsePacket.getCode();
        }

        if (content.eof()) {
            break;
        }

        firstPacket = false;

        if (m_syncListener != nullptr) {
            m_syncListener->Progress(bodyBytesReceivedSoFar);
            if (m_syncListener->IsCanceled()) {
                m_packetSerializer.sendPacket(OBEX_ABORT);
                throw SyncCancelException();
            }
        }
    }

    // Send the final body packet with end of body header
    ObexHeader bodyHeader(OBEX_HEADER_END_OF_BODY, NULL, 0);
    ObexPacket endofBodyPacket;
    endofBodyPacket.setCode(OBEX_PUT | OBEX_FINAL);
    endofBodyPacket.addHeader(bodyHeader);
    m_packetSerializer.sendPacket(endofBodyPacket);

    // Get the response
    ObexPacket requestPacket;
    requestPacket.setCode(OBEX_PUT | OBEX_FINAL);
    bool haveBodyLengthFromHeader = false;
    uint64_t responseBodyLengthFromHeader;
    uint64_t responseBodyBytesReceived = 0;
    while (true) {

        responsePacket = m_packetSerializer.receivePacket();

        if (IsObexError((ObexResponseCode) responsePacket.getCode())) {
            return (ObexResponseCode) responsePacket.getCode();
        }

        if (responsePacket.getHeaders().getBodyLength(responseBodyLengthFromHeader)) {
            haveBodyLengthFromHeader = true;
        }

        for (std::vector<ObexHeader>::const_iterator ih = responsePacket.getHeaders().getHeaders().begin();
        ih != responsePacket.getHeaders().getHeaders().end();
            ++ih)
        {
            switch (ih->getCode()) {
            case OBEX_HEADER_BODY:
            case OBEX_HEADER_END_OF_BODY:
                response.write(ih->getByteSequenceData(), ih->getByteSequenceDataSize());
                responseBodyBytesReceived += ih->getByteSequenceDataSize();
                break;
            default:
                responseHeaders.add(*ih);
            }
        }

        if (responsePacket.getCode() != OBEX_CONTINUE) {
            break;
        }

        if (m_syncListener != nullptr) {
            if (m_syncListener->IsCanceled()) {
                m_packetSerializer.sendPacket(OBEX_ABORT);
                throw SyncCancelException();
            }
        }

        m_packetSerializer.sendPacket(requestPacket);
    }

    if (haveBodyLengthFromHeader && responseBodyLengthFromHeader != responseBodyBytesReceived) {
        // Bytes received did not match number specified in length header
        throw SyncConnectionError("Missing data in put response");
    }

    return OBEX_OK;
}
