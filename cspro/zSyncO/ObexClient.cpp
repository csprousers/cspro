#include "stdafx.h"
#include <sstream>
#include <assert.h>
#include <ctime>
#include "ObexClient.h"
#include "IObexTransport.h"
#include "ObexPacket.h"
#include "SyncException.h"
#include "ISyncListener.h"
#include <easyloggingwrapper.h>
#include "IDataChunk.h"

namespace {
    const int receivePacketTimeoutSecs = 10;
}

ObexClient::ObexClient(IObexTransport* pTransport)
    : m_pTransport(pTransport),
    m_packetSerializer(pTransport),
    m_maxPacketSize(OBEX_MIN_PACKET_SIZE),
    m_pListener(NULL)
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
        throw SyncError(100101, L"Invalid packet size");

    // Receive server Bluetooth protocol version from server, so compatibility can be detected
    unsigned int serverBluetoothProtocolVersion = 0; // If version header is not found, default to 0
    const ObexHeader* pBluetoothProtocolVersionHeader = connectResponse.getHeaders().find(OBEX_HEADER_BLUETOOTH_PROTOCOL_VERSION);
    if (pBluetoothProtocolVersionHeader) {
        serverBluetoothProtocolVersion = pBluetoothProtocolVersionHeader->getIntData();
    }

    m_maxPacketSize = connectResponse.getMaxPacketSize();

    CLOG(INFO, "sync") << "Bluetooth protocol version: " << BLUETOOTH_PROTOCOL_VERSION;
    if (serverBluetoothProtocolVersion != BLUETOOTH_PROTOCOL_VERSION) {
        CLOG(ERROR, "sync") << "Bluetooth protocol version mismatch: client ("
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

ObexResponseCode ObexClient::get(const ObexHeaderList& headers,
    std::ostream& content, ObexHeaderList& responseHeaders)
{
    ObexPacket getPacket(OBEX_GET | OBEX_FINAL, headers);
    ObexPacket responsePacket;
    bool haveBodyLengthFromHeader = false;
    uint64_t responseBodyLengthFromHeader = 0;
    uint64_t responseBodyBytesReceived = 0;
    bool firstPacket = true;

    if (m_pListener) {
        CString name;
        const ObexHeader* nameHeader = headers.find(OBEX_HEADER_NAME);
        if (nameHeader) {
            name = nameHeader->getStringData();
        }
        m_pListener->onProgress(0, 100107, (LPCTSTR) name);

        if (m_pListener->isCancelled())
            throw SyncCancelException();
    }

    while (true) {

        m_packetSerializer.sendPacket(getPacket);

        responsePacket = m_packetSerializer.receivePacket();

        if (IsObexError((ObexResponseCode) responsePacket.getCode())) {
            return (ObexResponseCode) responsePacket.getCode();
        }

        if (responsePacket.getHeaders().getBodyLength(responseBodyLengthFromHeader)) {
            haveBodyLengthFromHeader = true;
            if (m_pListener)
                m_pListener->setProgressTotal(responseBodyLengthFromHeader);

            if (firstPacket) {
                m_dataChunk.optimize(responseBodyLengthFromHeader, m_maxPacketSize);
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

        if (m_pListener) {
            if (haveBodyLengthFromHeader) {
                m_pListener->onProgress(responseBodyBytesReceived);
            }
            if (m_pListener->isCancelled()) {
                m_packetSerializer.sendPacket(OBEX_ABORT);
                m_packetSerializer.receivePacket();
                throw SyncCancelException();
            }
        }
    }

    if (haveBodyLengthFromHeader && responseBodyLengthFromHeader != responseBodyBytesReceived) {
        // Bytes received did not match number specified in length header
        throw SyncError(100101, L"Missing data in get");
    }

    return (ObexResponseCode) responsePacket.getCode();
}

ObexResponseCode ObexClient::put(const ObexHeaderList& headers,
    std::istream& content, std::ostream& response, ObexHeaderList& responseHeaders)
{
    size_t headerLength = headers.getTotalSizeBytes();
    bool firstPacket = true;
    uint64_t bodyLength = 0;
    bool haveBodyLength = headers.getBodyLength(bodyLength);
    uint64_t bodyBytesReceivedSoFar = 0;
    if (m_pListener) {
        CString name;
        const ObexHeader* nameHeader = headers.find(OBEX_HEADER_NAME);
        if (nameHeader) {
            name = nameHeader->getStringData();
        }
        if (m_pListener->getProgressTotal() <= 0)
            m_pListener->setProgressTotal(bodyLength);
        m_pListener->onProgress(0, 100108, (LPCTSTR) name);
        if (m_pListener->isCancelled())
            throw SyncCancelException();
    }

    ObexPacket responsePacket;

    // Send the request
    while (true) {
        size_t maxBodySize = m_maxPacketSize - OBEX_BASE_PACKET_SIZE - ObexHeader::getBaseSize(OBEX_HEADER_BODY);
        if (firstPacket) {
            maxBodySize -= headerLength;
            if (haveBodyLength) {
                m_dataChunk.optimize(bodyLength, m_maxPacketSize);
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

        if (m_pListener) {
            m_pListener->onProgress(bodyBytesReceivedSoFar);
            if (m_pListener->isCancelled()) {
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

        if (m_pListener) {

            if (m_pListener->isCancelled()) {
                m_packetSerializer.sendPacket(OBEX_ABORT);
                throw SyncCancelException();
            }
        }

        m_packetSerializer.sendPacket(requestPacket);
    }

    if (haveBodyLengthFromHeader && responseBodyLengthFromHeader != responseBodyBytesReceived) {
        // Bytes received did not match number specified in length header
        throw SyncError(100101, L"Missing data in put response");
    }

    return OBEX_OK;
}

IDataChunk& ObexClient::getChunk()
{
    return m_dataChunk;
}

void ObexClient::setListener(ISyncListener* pListener)
{
    m_pListener = pListener;
}

