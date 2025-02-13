#include "stdafx.h"
#include "ObexPacket.h"

ObexPacket::ObexPacket()
{
}

ObexPacket::ObexPacket(unsigned char code)
    : m_code(code)
{
}

ObexPacket::ObexPacket(unsigned char code, const ObexHeaderList& headers)
    : m_code(code),
      m_headers(headers)
{
}

// Connect packet
ObexPacket::ObexPacket(unsigned char code, unsigned char obexVersion, unsigned char flags, size_t maxPacketSize, const ObexHeaderList& headers)
    : m_code(code),
      m_version(obexVersion),
      m_flags(flags),
      m_maxPacketSize(maxPacketSize),
      m_headers(headers)
{
}

unsigned char ObexPacket::getCode() const
{
    return m_code;
}

void ObexPacket::setCode(unsigned char code)
{
    m_code = code;
}

const ObexHeaderList& ObexPacket::getHeaders() const
{
    return m_headers;
}

void ObexPacket::setHeaders(const ObexHeaderList& headers)
{
    m_headers = headers;
}

void ObexPacket::addHeader(const ObexHeader& header)
{
    m_headers.add(header);
}

unsigned char ObexPacket::getObexVersion() const
{
    return m_version;
}

void ObexPacket::setObexVersion(unsigned char version)
{
    m_version = version;
}

unsigned char ObexPacket::getFlags() const
{
    return m_flags;
}

void ObexPacket::setFlags(unsigned char flags)
{
    m_flags = flags;
}

size_t ObexPacket::getMaxPacketSize() const
{
    return m_maxPacketSize;
}

void ObexPacket::setMaxPacketSize(size_t size)
{
    m_maxPacketSize = size;
}
