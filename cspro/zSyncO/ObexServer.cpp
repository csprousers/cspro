#include "stdafx.h"
#include "ObexServer.h"
#include "ObexConstants.h"
#include "ObexHeader.h"
#include "ObexPacket.h"
#include "ObexPacketSerializer.h"
#include "SyncObexHandler.h"


namespace
{
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
        if (totalSizeBytes <= std::numeric_limits<uint32_t>::max()) {
            // For small length use obex header that accepts uint32
            return ObexHeader(OBEX_HEADER_LENGTH, static_cast<uint32_t>(totalSizeBytes));
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
    :   m_pHandler(pHandler),
        m_maxPacketSize(OBEX_MIN_PACKET_SIZE)
{
}


void ObexServer::run(IObexTransport* pTransport)
{
    ObexPacketSerializer packetSerializer(pTransport);
    packetSerializer.setReceiveTimeout(15, 5);

    // Wait for the connect packet
    ObexPacket connectPacket = packetSerializer.receivePacket();
    handleConnect(packetSerializer, connectPacket);

    if (m_syncListener != nullptr) {
        m_syncListener->SetProgressTotal(-1);
        m_syncListener->Progress(0);
    }

    bool connected = true;
    while (connected) {
        try {
            SYNCLOG_INFO << "Waiting for request ";
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

            if (m_syncListener != nullptr && m_syncListener->IsCanceled()) {
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
        throw SyncConnectionError("Invalid packet received from client - expecting connect");
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

    SYNCLOG_INFO << "Bluetooth protocol version: " << BLUETOOTH_PROTOCOL_VERSION;
    if (clientBluetoothProtocolVersion != BLUETOOTH_PROTOCOL_VERSION) {
        SYNCLOG_ERROR << "Bluetooth protocol version mismatch: server ("
            << BLUETOOTH_PROTOCOL_VERSION << ") != client (" << clientBluetoothProtocolVersion << ")";

        throw SyncError(100145);
    }
    else if (result != OBEX_OK) {
        throw SyncConnectionError(ObexResponseCodeToString(result));
    }
}


void ObexServer::handleDisconnect(ObexPacketSerializer& serializer, const ObexPacket& )
{
    SYNCLOG_INFO << "Client disconnected";
    serializer.sendPacket(ObexPacket(OBEX_OK));
}


void ObexServer::handleGet(ObexPacketSerializer& packetSerializer, const ObexPacket& firstRequestPacket)
{
    CString name;
    std::string type;
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
                type = std::string(ih->getByteSequenceData(), ih->getByteSequenceDataSize());
                break;
            case OBEX_HEADER_HTTP:
                requestHeaders.Add(std::string(ih->getByteSequenceData(), ih->getByteSequenceDataSize()));
                break;
            }
        }

        if (m_syncListener != nullptr) {
            m_syncListener->Progress(0, 100107, UTF8_TODO::GetUtf8(name).c_str());
            if (m_syncListener->IsCanceled()) {
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
            throw SyncConnectionError("Unexpected packet received from client during get");
        }
    }

    ObexResponseCode result = m_pHandler->onGet(UTF8_TODO::GetCString(type), name, requestHeaders, m_resource);
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
        throw SyncConnectionError("Error getting " + UTF8_TODO::GetUtf8(name) + ". " + ObexResponseCodeToString(result));
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

            for( const std::string& header : m_resource->getHeaders().GetHeaders() )
            {
                ObexHeader obex_header(OBEX_HEADER_HTTP, header.c_str(), header.size());
                maxBodySize -= obex_header.getTotalSizeBytes();
                responsePacket.addHeader(std::move(obex_header));
            }
        }

        std::vector<char> bodyData(maxBodySize);
        m_resource->getIStream()->read(&bodyData[0], maxBodySize);
        if (m_resource->getIStream()->bad()) {
            packetSerializer.sendPacket(ObexPacket(OBEX_INTERNAL_SERVER_ERROR));
            throw SyncConnectionError("Error reading file " + UTF8_TODO::GetUtf8(name));
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
                throw SyncConnectionError("Unexpected packet received from client during get");
            }
        }
        if (m_syncListener != nullptr) {
            m_syncListener->Progress(bytesSent);
            if (m_syncListener->IsCanceled()) {
                packetSerializer.sendPacket(OBEX_CANCELED_BY_USER);
                destroyResource();
                return;
            }
        }
    }

    const bool isFile = ( type == UTF8_TODO::GetUtf8(OBEX_BINARY_FILE_MEDIA_TYPE) || type == UTF8_TODO::GetUtf8(OBEX_SYNC_PARADATA_TYPE) );
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
    std::string type;
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
            if (haveBodyLength && m_syncListener != nullptr) {
                m_syncListener->SetProgressTotal(bodyLength);
            }
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
                type = std::string(ih->getByteSequenceData(), ih->getByteSequenceDataSize());
                break;
            case OBEX_HEADER_HTTP:
                requestHeaders.Add(std::string(ih->getByteSequenceData(), ih->getByteSequenceDataSize()));
                break;
            case OBEX_HEADER_IS_LAST_FILE_CHUNK:
                lastFileChunk = ih->getIntData();
                break;
            }
        }

        if (m_syncListener != nullptr) {
            m_syncListener->Progress(0, 100108, UTF8_TODO::GetUtf8(name).c_str());

            if (m_syncListener->IsCanceled()) {
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
    ObexResponseCode result = m_pHandler->onPut(UTF8_TODO::GetCString(type), name, requestHeaders, m_resource);
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

        if (m_syncListener != nullptr) {
            m_syncListener->Progress(totalBodyBytesRead);

            if (m_syncListener->IsCanceled()) {
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
            throw SyncConnectionError("Unexpected packet received from client during put");
        }
    }

    std::istringstream inStrm(allPackets);
    if (!ZLib::Inflate(inStrm, *(m_resource->getOStream()))) {
        SYNCLOG_ERROR << "Decompression failed";
        throw SyncError(100139);
    }

    // Were all packets written to resource successfully?
    if (m_resource->getOStream()->fail()) {
        packetSerializer.sendPacket(ObexPacket(OBEX_INTERNAL_SERVER_ERROR));
        throw SyncConnectionError("Error writing file " + UTF8_TODO::GetUtf8(name));
    }

    // Send the response - read from resource and write to packets
    result = m_resource->openForReading();
    if (result != OBEX_OK) {
        packetSerializer.sendPacket(ObexPacket(result));
        throw SyncConnectionError("Error writing " + UTF8_TODO::GetUtf8(name) + ". " + ObexResponseCodeToString(result));
    }

    const int64_t totalResponseSizeBytes = m_resource->getTotalSize();
    if (m_syncListener != nullptr) {
        m_syncListener->SetProgressTotal(totalResponseSizeBytes);
    }

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

            for( const std::string& header : m_resource->getHeaders().GetHeaders() )
            {
                ObexHeader obex_header(OBEX_HEADER_HTTP, header.c_str(), header.size());
                maxBodySize -= obex_header.getTotalSizeBytes();
                responsePacket.addHeader(std::move(obex_header));
            }
        }

        if (m_resource->getIStream() != nullptr && !m_resource->getIStream()->eof()) {
            std::vector<char> bodyData(maxBodySize);
            m_resource->getIStream()->read(&bodyData[0], maxBodySize);
            if (m_resource->getIStream()->bad()) {
                packetSerializer.sendPacket(ObexPacket(OBEX_INTERNAL_SERVER_ERROR));
                throw SyncConnectionError("Error reading file " + UTF8_TODO::GetUtf8(name));
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
                throw SyncConnectionError("Unexpected packet received from client during get");
            }
        }

        if (m_syncListener != nullptr) {
            if (m_syncListener->IsCanceled()) {
                packetSerializer.sendPacket(OBEX_CANCELED_BY_USER);
                destroyResource();
                return;
            }
        }
    }

    const bool isFile = ( type == UTF8_TODO::GetUtf8(OBEX_BINARY_FILE_MEDIA_TYPE) || type == UTF8_TODO::GetUtf8(OBEX_SYNC_PARADATA_TYPE) );
    const bool isLastFileChunk = static_cast<bool>(lastFileChunk);
    if (!isFile || (isFile && isLastFileChunk)) {
        ObexResponseCode closeResult = m_resource->close();
        if (closeResult != OBEX_OK) {
            packetSerializer.sendPacket(ObexPacket(closeResult));
            throw SyncConnectionError("Error writing " + UTF8_TODO::GetUtf8(name) + ". " + ObexResponseCodeToString(closeResult));
        }

        destroyResource();
    }

    // Send the final packet - empty end of body header
    ObexHeader bodyHeader(OBEX_HEADER_END_OF_BODY, NULL, 0);
    ObexPacket finalPacket(OBEX_OK);
    finalPacket.addHeader(bodyHeader);
    packetSerializer.sendPacket(finalPacket);
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
