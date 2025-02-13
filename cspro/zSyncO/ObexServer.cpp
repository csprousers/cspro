#include "stdafx.h"
#include <cassert>
#include "ObexConstants.h"
#include "ObexServer.h"
#include "ObexPacketSerializer.h"
#include "SyncException.h"
#include "SyncObexHandler.h"
#include "ISyncListener.h"
#include <sstream>
#include <stdint.h>
#include <easyloggingwrapper.h>
#include <zZipo/ZipUtility.h>
#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

namespace {

    std::vector<char> getBody(const ObexPacket& packet)
    {
        std::vector<char> body;
        const ObexHeader* pBodyHeader = packet.getHeaders().find(OBEX_HEADER_BODY);
        if (!pBodyHeader) {
            // No body, check for end of body - should have one or the other
            pBodyHeader = packet.getHeaders().find(OBEX_HEADER_END_OF_BODY);
        }
        if (pBodyHeader && pBodyHeader->getByteSequenceDataSize() > 0) {
            body.resize(pBodyHeader->getByteSequenceDataSize());
            memcpy(&body[0], pBodyHeader->getByteSequenceData(), pBodyHeader->getByteSequenceDataSize());
        }
        return body;
    }

    ObexHeader createLengthHeader(int64_t totalSizeBytes)
    {
        if (totalSizeBytes <= UINT32_MAX) {
            // For small length use obex header that accepts uint32
            return ObexHeader(OBEX_HEADER_LENGTH, (uint32_t)totalSizeBytes);
        }
        else {
            // For bigger files per obex standard use http content-length header
            std::ostringstream ostr;
            ostr << "Content-Length: " << totalSizeBytes;
            std::string contentLengthHeader = ostr.str();
            return ObexHeader(OBEX_HEADER_HTTP, contentLengthHeader.c_str(), contentLengthHeader.size());
        }
    }
}

ObexServer::ObexServer(SyncObexHandler* pHandler)
    : m_pHandler(pHandler),
      m_maxPacketSize(OBEX_MIN_PACKET_SIZE),
      m_pListener(NULL)
{
}

void ObexServer::run(IObexTransport* pTransport)
{
    ObexPacketSerializer packetSerializer(pTransport);
    packetSerializer.setReceiveTimeout(15, 5);

    // Wait for the connect packet
    ObexPacket connectPacket = packetSerializer.receivePacket();
    handleConnect(packetSerializer, connectPacket);

    m_pListener->setProgressTotal(-1);
    m_pListener->onProgress(0);

    bool connected = true;
    while (connected) {
        try {
            CLOG(INFO, "sync") << "Waiting for request ";
            ObexPacket requestPacket = packetSerializer.receivePacket();

            switch (requestPacket.getCode()) {

            case OBEX_DISCONNECT:
                handleDisconnect(packetSerializer, requestPacket);
                connected = false;
                break;
            case OBEX_GET:
            case (OBEX_GET | OBEX_FINAL):
                handleGet(packetSerializer, requestPacket);
                break;
            case OBEX_PUT:
            case (OBEX_PUT | OBEX_FINAL):
                handlePut(packetSerializer, requestPacket);
                break;
            default:
                packetSerializer.sendPacket(ObexPacket(OBEX_NOT_IMPLEMENTED));
                break;
            }

            if (m_pListener && m_pListener->isCancelled()) {
                connected = false;
            }
        }
        catch (...) {
            destroyResource();
            throw;
        }
    }
}

void ObexServer::handleConnect(ObexPacketSerializer& packetSerializer, const ObexPacket& connectPacket)
{
    if (connectPacket.getCode() != OBEX_CONNECT) {
        packetSerializer.sendPacket(ObexPacket(OBEX_BAD_REQUEST));
        throw SyncError(100101, L"Invalid packet received from client - expecting connect");
    }

    // Valid connect packet

    const ObexHeader* pTargetHeader = connectPacket.getHeaders().find(OBEX_HEADER_TARGET);
    ObexResponseCode result;
    if (pTargetHeader) {
        result = m_pHandler->onConnect(pTargetHeader->getByteSequenceData(), (int) pTargetHeader->getByteSequenceDataSize());
    }
    else {
        result = m_pHandler->onConnect(NULL, 0);
    }

    // Receive client Bluetooth protocol version from client, so compatibility can be detected
    unsigned int clientBluetoothProtocolVersion = 0; // If version header is not found, default to 0
    const ObexHeader* pBluetoothProtocolVersionHeader = connectPacket.getHeaders().find(OBEX_HEADER_BLUETOOTH_PROTOCOL_VERSION);
    if (pBluetoothProtocolVersionHeader) {
        clientBluetoothProtocolVersion = pBluetoothProtocolVersionHeader->getIntData();
    }

    ObexHeaderList headers;
    // Pass server Bluetooth protocol version to client, so compatibility can be detected
    ObexHeader bluetoothProtocolVersionHeader(OBEX_HEADER_BLUETOOTH_PROTOCOL_VERSION, BLUETOOTH_PROTOCOL_VERSION);
    headers.add(bluetoothProtocolVersionHeader);

    m_maxPacketSize = std::min(connectPacket.getMaxPacketSize(), OBEX_MAX_PACKET_SIZE);
    const bool isConnect = true;
    packetSerializer.sendPacket(ObexPacket(result, OBEX_VERSION, 0, m_maxPacketSize, headers), isConnect);

    CLOG(INFO, "sync") << "Bluetooth protocol version: " << BLUETOOTH_PROTOCOL_VERSION;
    if (clientBluetoothProtocolVersion != BLUETOOTH_PROTOCOL_VERSION) {
        CLOG(ERROR, "sync") << "Bluetooth protocol version mismatch: server ("
            << BLUETOOTH_PROTOCOL_VERSION << ") != client (" << clientBluetoothProtocolVersion << ")";

        throw SyncError(100145);
    }
    else if (result != OBEX_OK) {
        throw SyncError(100101, ObexResponseCodeToString(result));
    }
}

void ObexServer::handleDisconnect(ObexPacketSerializer& serializer, const ObexPacket& )
{
    CLOG(INFO, "sync") << "Client disconnected";
    serializer.sendPacket(ObexPacket(OBEX_OK));
}

void ObexServer::handleGet(ObexPacketSerializer& packetSerializer, const ObexPacket& firstRequestPacket)
{
    CString name;
    CString type;
    HeaderList requestHeaders;
    ObexPacket currentRequestPacket = firstRequestPacket;
    while (true) {

        for (std::vector<ObexHeader>::const_iterator ih = currentRequestPacket.getHeaders().getHeaders().begin();
        ih != currentRequestPacket.getHeaders().getHeaders().end();
            ++ih)
        {
            switch (ih->getCode()) {
            case OBEX_HEADER_NAME:
                name = ih->getStringData();
                break;
            case OBEX_HEADER_TYPE:
                // For some strange reason OBEX wants the type to be ASCII text
                // in binary format instead of in unicode string format.
                type = UTF8Convert::UTF8ToWide<CString>(ih->getByteSequenceData(), (int) ih->getByteSequenceDataSize());
                break;
            case OBEX_HEADER_HTTP:
                requestHeaders.push_back(UTF8Convert::UTF8ToWide<CString>(ih->getByteSequenceData(), (int) ih->getByteSequenceDataSize()));
                break;
            }
        }

        if (m_pListener) {
            m_pListener->onProgress(0, 100107, (LPCTSTR) name);
            if (m_pListener->isCancelled()) {
                packetSerializer.sendPacket(OBEX_CANCELED_BY_USER);
                destroyResource();
                return;
            }
        }

        if (currentRequestPacket.getCode() & OBEX_FINAL) {
            break;
        }

        packetSerializer.sendPacket(ObexPacket(OBEX_CONTINUE));
        currentRequestPacket = packetSerializer.receivePacket();
    }

    if (currentRequestPacket.getCode() != (OBEX_GET | OBEX_FINAL)) {
        if (currentRequestPacket.getCode() == OBEX_ABORT) {
            // Cancel from client
            packetSerializer.sendPacket(OBEX_OK);
            throw SyncError(100122);
        }
        else {
            throw SyncError(100101, L"Unexpected packet received from client during get");
        }
    }

    ObexResponseCode result = m_pHandler->onGet(type, name, requestHeaders, m_resource);
    if (result == OBEX_NOT_MODIFIED) {
        packetSerializer.sendPacket(ObexPacket(result));
        destroyResource();
        return;
    }

    if (result != OBEX_OK) {
        packetSerializer.sendPacket(ObexPacket(result));
        destroyResource();
        if (result == OBEX_NOT_IMPLEMENTED) // let the client handle not implemented as an error if it wants to
            return;
        throw SyncError(100101, L"Error getting " + name + L". " + ObexResponseCodeToString(result));
    }

    result = m_resource->openForReading();

    if (IsObexError(result)) {
        packetSerializer.sendPacket(ObexPacket(result));
        destroyResource();
        return;
    }

    const int64_t totalSizeBytes = (int64_t)m_resource->getTotalSize();
    int64_t bytesSent = 0;

    bool firstPacket = true;
    while (!m_resource->getIStream()->eof()) {

        ObexPacket responsePacket(OBEX_CONTINUE);

        size_t maxBodySize = m_maxPacketSize - OBEX_BASE_PACKET_SIZE - ObexHeader::getBaseSize(OBEX_HEADER_BODY);

        if (firstPacket) {
            firstPacket = false;
            if (totalSizeBytes > 0) {
                ObexHeader lengthHeader = createLengthHeader(totalSizeBytes);
                maxBodySize -= lengthHeader.getTotalSizeBytes();
                responsePacket.addHeader(lengthHeader);
            }

            HeaderList headers = m_resource->getHeaders();
            for (HeaderList::const_iterator ih = headers.begin(); ih != headers.end(); ++ih) {
                std::string hutf8 = UTF8Convert::WideToUTF8(*ih);
                ObexHeader header(OBEX_HEADER_HTTP, hutf8.c_str(), hutf8.size());
                maxBodySize -= header.getTotalSizeBytes();
                responsePacket.addHeader(header);
            }
        }

        std::vector<char> bodyData(maxBodySize);
        m_resource->getIStream()->read(&bodyData[0], maxBodySize);
        if (m_resource->getIStream()->bad()) {
            packetSerializer.sendPacket(ObexPacket(OBEX_INTERNAL_SERVER_ERROR));
            throw SyncError(100101, L"Error reading file " + name);
        }
        int bytesRead = (int)m_resource->getIStream()->gcount();

        ObexHeader bodyHeader(OBEX_HEADER_BODY, &bodyData[0], bytesRead);
        responsePacket.addHeader(bodyHeader);

        packetSerializer.sendPacket(responsePacket);

        bytesSent += bytesRead;

        ObexPacket continuationPacket = packetSerializer.receivePacket();
        if (continuationPacket.getCode() != (OBEX_GET | OBEX_FINAL)) {
            if (continuationPacket.getCode() == OBEX_ABORT) {
                // Cancel from client
                packetSerializer.sendPacket(ObexPacket(OBEX_OK));
                throw SyncError(100122);
            }
            else {
                throw SyncError(100101, L"Unexpected packet received from client during get");
            }
        }
        if (m_pListener) {
            m_pListener->onProgress(bytesSent);
            if (m_pListener->isCancelled()) {
                packetSerializer.sendPacket(OBEX_CANCELED_BY_USER);
                destroyResource();
                return;
            }
        }
    }

    const bool isFile = ( type == OBEX_BINARY_FILE_MEDIA_TYPE || type == OBEX_SYNC_PARADATA_TYPE );
    const bool isLastFileChunk = ( result == OBEX_IS_LAST_FILE_CHUNK );
    if (!isFile || (isFile && isLastFileChunk)) {
        destroyResource();
    }

    // Send the final packet - empty end of body header
    ObexHeader bodyHeader(OBEX_HEADER_END_OF_BODY, NULL, 0);
    ObexPacket finalPacket(result);
    finalPacket.addHeader(bodyHeader);
    packetSerializer.sendPacket(finalPacket);
}

void ObexServer::handlePut(ObexPacketSerializer& packetSerializer, const ObexPacket& firstRequestPacket)
{
    CString name;
    CString type;
    HeaderList requestHeaders;
    int lastFileChunk = 0;
    uint64_t bodyLength = 0;
    uint64_t totalBodyBytesRead = 0;
    bool haveBodyLength = false;

    ObexPacket currentRequestPacket = firstRequestPacket;

    // Read packets until we get a body packet
    while (true) {

        if (!haveBodyLength) {
            haveBodyLength = currentRequestPacket.getHeaders().getBodyLength(bodyLength);
            if (haveBodyLength && m_pListener)
                m_pListener->setProgressTotal(bodyLength);
        }

        for (std::vector<ObexHeader>::const_iterator ih = currentRequestPacket.getHeaders().getHeaders().begin();
        ih != currentRequestPacket.getHeaders().getHeaders().end();
            ++ih)
        {
            switch (ih->getCode()) {
            case OBEX_HEADER_NAME:
                name = ih->getStringData();
                break;
            case OBEX_HEADER_TYPE:
                // For some strange reason OBEX wants the type to be ASCII text
                // in binary format instead of in unicode string format.
                type = UTF8Convert::UTF8ToWide<CString>(ih->getByteSequenceData(), (int) ih->getByteSequenceDataSize());
                break;
            case OBEX_HEADER_HTTP:
                requestHeaders.push_back(UTF8Convert::UTF8ToWide<CString>(ih->getByteSequenceData(), (int) ih->getByteSequenceDataSize()));
                break;
            case OBEX_HEADER_IS_LAST_FILE_CHUNK:
                lastFileChunk = ih->getIntData();
                break;
            }
        }

        if (m_pListener) {
            m_pListener->onProgress(0, 100108, (LPCTSTR)name);

            if (m_pListener->isCancelled()) {
                packetSerializer.sendPacket(OBEX_CANCELED_BY_USER);
                destroyResource();
                return;
            }
        }

        if (currentRequestPacket.getHeaders().find(OBEX_HEADER_BODY) ||
            currentRequestPacket.getHeaders().find(OBEX_HEADER_END_OF_BODY) ||
            currentRequestPacket.getCode() & OBEX_FINAL) {
            break;
        }

        packetSerializer.sendPacket(ObexPacket(OBEX_CONTINUE));
        currentRequestPacket = packetSerializer.receivePacket();
    }

    if (currentRequestPacket.getCode() == OBEX_ABORT) {
        // Cancel from client
        packetSerializer.sendPacket(OBEX_OK);
        throw SyncError(100122);
    }

    // Since we have a body packet, the name, type and length should be filled in
    // from the headers if they ever will be. We can now use name and type to get
    // the resource from the handler.
    ObexResponseCode result = m_pHandler->onPut(type, name, requestHeaders, m_resource);
    if (result != OBEX_OK) {
        packetSerializer.sendPacket(ObexPacket(result));
        destroyResource();
        return;
    }

    result = m_resource->openForWriting();
    if (result != OBEX_OK) {
        packetSerializer.sendPacket(ObexPacket(result));
        destroyResource();
        return;
    }

    std::string allPackets;
    // Read in the request body from the incoming packets. The packets are accumulated
    // in the case compressed data is being sent. Decompression can only be done on the
    // entirety of compressed data (not slices).
    while (true) {

        std::vector<char> packetBodyBytes = getBody(currentRequestPacket);
        if (!packetBodyBytes.empty()) {
            std::string packetBodyStr(packetBodyBytes.begin(), packetBodyBytes.end());
            allPackets.append(packetBodyStr);

            totalBodyBytesRead += packetBodyBytes.size();
        }

        if (m_pListener) {
            m_pListener->onProgress(totalBodyBytesRead);

            if (m_pListener->isCancelled()) {
                packetSerializer.sendPacket(OBEX_CANCELED_BY_USER);
                destroyResource();
                return;
            }
        }

        if (currentRequestPacket.getCode() & OBEX_FINAL) {
            break;
        }

        packetSerializer.sendPacket(ObexPacket(OBEX_CONTINUE));
        currentRequestPacket = packetSerializer.receivePacket();
    }

    if (currentRequestPacket.getCode() != (OBEX_PUT | OBEX_FINAL)) {
        if (currentRequestPacket.getCode() == OBEX_ABORT) {
            packetSerializer.sendPacket(OBEX_OK);
            throw SyncError(100122);
        }
        else {
            throw SyncError(100101, L"Unexpected packet received from client during put");
        }
    }

    std::istringstream inStrm(allPackets);
    if (ZipUtility::decompression(inStrm, *(m_resource->getOStream()))) {
        CLOG(ERROR, "sync") << "Decompression failed";
        throw SyncError(100139);
    }

    // Were all packets written to resource successfully?
    if (m_resource->getOStream()->fail()) {
        packetSerializer.sendPacket(ObexPacket(OBEX_INTERNAL_SERVER_ERROR));
        throw SyncError(100101, L"Error writing file " + name);
    }

    // Send the response - read from resource and write to packets
    result = m_resource->openForReading();
    if (result != OBEX_OK) {
        packetSerializer.sendPacket(ObexPacket(result));
        throw SyncError(100101, L"Error writing " + name + L". " + ObexResponseCodeToString(result));
    }

    const int64_t totalResponseSizeBytes = m_resource->getTotalSize();
    if (m_pListener)
        m_pListener->setProgressTotal(totalResponseSizeBytes);

    int64_t bytesSent = 0;
    bool firstPacket = true;
    while (firstPacket || (m_resource->getIStream() != nullptr && !m_resource->getIStream()->eof())) {

        ObexPacket responsePacket(OBEX_CONTINUE);

        size_t maxBodySize = m_maxPacketSize - OBEX_BASE_PACKET_SIZE - ObexHeader::getBaseSize(OBEX_HEADER_BODY);

        if (firstPacket) {
            firstPacket = false;
            if (totalResponseSizeBytes > 0) {
                ObexHeader lengthHeader = createLengthHeader(totalResponseSizeBytes);
                maxBodySize -= lengthHeader.getTotalSizeBytes();
                responsePacket.addHeader(lengthHeader);
            }

            for (HeaderList::const_iterator ih = m_resource->getHeaders().begin(); ih != m_resource->getHeaders().end(); ++ih) {
                std::string hutf8 = UTF8Convert::WideToUTF8(*ih);
                ObexHeader header(OBEX_HEADER_HTTP, hutf8.c_str(), hutf8.size());
                maxBodySize -= header.getTotalSizeBytes();
                responsePacket.addHeader(header);
            }
        }

        if (m_resource->getIStream() != nullptr && !m_resource->getIStream()->eof()) {
            std::vector<char> bodyData(maxBodySize);
            m_resource->getIStream()->read(&bodyData[0], maxBodySize);
            if (m_resource->getIStream()->bad()) {
                packetSerializer.sendPacket(ObexPacket(OBEX_INTERNAL_SERVER_ERROR));
                throw SyncError(100101, L"Error reading file " + name);
            }
            int bytesRead = (int)m_resource->getIStream()->gcount();
            bytesSent += bytesRead;

            ObexHeader bodyHeader(OBEX_HEADER_BODY, &bodyData[0], bytesRead);
            responsePacket.addHeader(bodyHeader);
        }

        packetSerializer.sendPacket(responsePacket);

        ObexPacket continuationPacket = packetSerializer.receivePacket();
        if (continuationPacket.getCode() != (OBEX_PUT | OBEX_FINAL)) {
            if (continuationPacket.getCode() == OBEX_ABORT) {
                packetSerializer.sendPacket(ObexPacket(OBEX_OK));
                throw SyncError(100122);
            }
            else {
                throw SyncError(100101, L"Unexpected packet received from client during get");
            }
        }

        if (m_pListener) {

            if (m_pListener->isCancelled()) {
                packetSerializer.sendPacket(OBEX_CANCELED_BY_USER);
                destroyResource();
                return;
            }
        }
    }

    const bool isFile = ( type == OBEX_BINARY_FILE_MEDIA_TYPE || type == OBEX_SYNC_PARADATA_TYPE );
    const bool isLastFileChunk = static_cast<bool>(lastFileChunk);
    if (!isFile || (isFile && isLastFileChunk)) {
        ObexResponseCode closeResult = m_resource->close();
        if (closeResult != OBEX_OK) {
            packetSerializer.sendPacket(ObexPacket(closeResult));
            throw SyncError(100101, L"Error writing " + name + L". " + ObexResponseCodeToString(closeResult));
        }

        destroyResource();
    }

    // Send the final packet - empty end of body header
    ObexHeader bodyHeader(OBEX_HEADER_END_OF_BODY, NULL, 0);
    ObexPacket finalPacket(OBEX_OK);
    finalPacket.addHeader(bodyHeader);
    packetSerializer.sendPacket(finalPacket);
}

void ObexServer::setListener(ISyncListener* pListener)
{
    m_pListener = pListener;
}

void ObexServer::destroyResource()
{
    // Manually manage lifetime of m_resource.
    // The reason this is necessary is that a file GET/PUT may be transmitted in multiple chunks. Previously,
    // files were transmitted in a single chunk. The change allows for compression and decompression of file
    // chunks, otherwise the entire file would have to be held in memory. However, it is now necessary to extend
    // the lifetime of m_resource, so multiple chunks can be written to a temporary file. Otherwise, the
    // temporary file will be deleted prematurely when m_resource goes out of scope.
    if (m_resource.get() != nullptr) {
        m_resource.reset();
    }
}
