#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ObexConstants.h>
#include <zSyncO/ObexHeader.h>

/**
* Data packet as defined by the OBEX protocol.
* All OBEX communication is via request and response packets. A standard packet
* has a one byte opcode that determines the packet type, 2 bytes that gives the total packet
* length, and and optional list of headers. Certain packet types such as connect and
* setPath are special and contain additional fields.
*/
class ObexPacket {
public:

    ObexPacket();

    // Regular packet
    ObexPacket(unsigned char code);
    ObexPacket(unsigned char code, const ObexHeaderList& headers);

    // Connect packet
    ObexPacket(unsigned char code, unsigned char obexVersion, unsigned char flags, size_t maxPacketSize, const ObexHeaderList& headers);

    // Packet code - for a request packet this will be an ObexOpCode and
    // for a response packet this will be an ObexResponseCode.
    unsigned char getCode() const;
    void setCode(unsigned char code);

    const ObexHeaderList& getHeaders() const;
    void setHeaders(const ObexHeaderList& headers);
    void addHeader(const ObexHeader& header);

    // Parameters only found in connect packet
    unsigned char getObexVersion() const;
    void setObexVersion(unsigned char version);
    unsigned char getFlags() const;
    void setFlags(unsigned char flags);
    size_t getMaxPacketSize() const;
    void setMaxPacketSize(size_t size);

private:

    unsigned char m_code;
    unsigned char m_version;
    unsigned char m_flags;
    size_t m_maxPacketSize;
    ObexHeaderList m_headers;
};
