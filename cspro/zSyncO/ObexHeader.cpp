#include "stdafx.h"
#include "ObexHeader.h"
#include <assert.h>
#include <iterator>
#include <sstream>

namespace {

    enum class HeaderFormat {
        LENGTH_PREFIXED_NULL_TERMINATED_UNICODE = 0x0,
        LENGTH_PREFIXED_BYTE_SEQUENCE = 0x40,
        SINGLE_BYTE = 0x80,
        FOUR_BYTE = 0xC0
    };

    const int HEADER_CODE_SIZE = 1;
    const int HEADER_LENGTH_SIZE = 2;

    HeaderFormat getHeaderFormat(ObexHeaderCode code)
    {
        HeaderFormat format;
        if (code == OBEX_HEADER_BLUETOOTH_PROTOCOL_VERSION
            || code == OBEX_HEADER_IS_LAST_FILE_CHUNK) {
            // Handle user defined header
            format = HeaderFormat::FOUR_BYTE;
        }
        else {
            // Header format is 2 high order bits of header code
            format = (HeaderFormat)((unsigned char) (code & 0xC0));
        }

        return format;
    }
};

ObexHeader::ObexHeader()
{
}

ObexHeader::ObexHeader(ObexHeaderCode code, CString data)
    : m_code(code),
      m_stringData(data)
{
}

ObexHeader::ObexHeader(ObexHeaderCode code, uint32_t data)
    : m_code(code),
      m_intData(data)
{
}

ObexHeader::ObexHeader(ObexHeaderCode code, const char* data, size_t dataSizeBytes)
    : m_code(code)
{
    if (dataSizeBytes > 0) {
        m_byteSequenceData.resize(dataSizeBytes);
        memcpy(&m_byteSequenceData[0], data, dataSizeBytes);
    }
    else {
        m_byteSequenceData.clear();
    }
}

size_t ObexHeader::getTotalSizeBytes() const
{
    size_t baseSize = getBaseSize(m_code);
    switch (getHeaderFormat(m_code)) {
    case HeaderFormat::LENGTH_PREFIXED_NULL_TERMINATED_UNICODE:
        return m_stringData.GetLength() * 2 + baseSize;
    case HeaderFormat::LENGTH_PREFIXED_BYTE_SEQUENCE:
        return m_byteSequenceData.size() + baseSize;
    case HeaderFormat::SINGLE_BYTE:
        return baseSize + 1;
    case HeaderFormat::FOUR_BYTE:
        return baseSize + 4;
    }
    assert(false);
    return 0;
}

size_t ObexHeader::getBaseSize(ObexHeaderCode code)
{
    switch (getHeaderFormat(code)) {
    case HeaderFormat::LENGTH_PREFIXED_NULL_TERMINATED_UNICODE:
        return HEADER_CODE_SIZE + HEADER_LENGTH_SIZE + 4; // include header, length and 4 byte null terminator
    case HeaderFormat::LENGTH_PREFIXED_BYTE_SEQUENCE:
        return HEADER_CODE_SIZE + HEADER_LENGTH_SIZE; // include header and length
    case HeaderFormat::SINGLE_BYTE:
    case HeaderFormat::FOUR_BYTE:
        return HEADER_CODE_SIZE;
    }
    assert(false);
    return 0;
}

SYNC_API std::ostream& operator<<(std::ostream& os, const ObexHeader& h)
{
    os << (unsigned char)h.getCode();

    switch (getHeaderFormat(h.getCode())) {
    case HeaderFormat::LENGTH_PREFIXED_NULL_TERMINATED_UNICODE:
    {
        const CString& stringData = h.getStringData();

        size_t length = h.getTotalSizeBytes();
        os << (unsigned char)(length >> 8);
        os << (unsigned char)(length & 0xFF);

        for (int i = 0; i < stringData.GetLength(); i++)
        {
            wchar_t c = stringData[i];
            os << (unsigned char)(c >> 8);
            os << (unsigned char)(c & 0xFF);
        }

        // Add null terminators
        os << (unsigned char)0;
        os << (unsigned char)0;
        os << (unsigned char)0;
        os << (unsigned char)0;
        break;
    }
    case HeaderFormat::LENGTH_PREFIXED_BYTE_SEQUENCE:
    {
        size_t length = h.getTotalSizeBytes();
        os << (unsigned char)(length >> 8);
        os << (unsigned char)(length & 0xFF);
        if (h.getByteSequenceDataSize() > 0)
            os.write((const char*) h.getByteSequenceData(), h.getByteSequenceDataSize());
        break;
    }
    case HeaderFormat::SINGLE_BYTE:
        os << h.getByteData();
        break;
    case HeaderFormat::FOUR_BYTE:
    {
        int data = h.getIntData();
        const unsigned char* dataBytes = (const unsigned char*)&data;
        os << dataBytes[3];
        os << dataBytes[2];
        os << dataBytes[1];
        os << dataBytes[0];
        break;
    }
    }

    return os;
}

void ObexHeaderList::add(const ObexHeader& h)
{
    m_headers.push_back(h);
}

void ObexHeaderList::add(const ObexHeaderList& h)
{
    std::copy(h.m_headers.begin(), h.m_headers.end(), std::back_inserter(m_headers));
}


const std::vector<ObexHeader>& ObexHeaderList::getHeaders() const
{
    return m_headers;
}

const ObexHeader* ObexHeaderList::find(ObexHeaderCode code) const
{
    for (std::vector<ObexHeader>::const_iterator i = m_headers.begin(); i != m_headers.end(); ++i) {
        if (i->getCode() == code) {
            return &(*i);
        }
    }
    return NULL;
}

size_t ObexHeaderList::getTotalSizeBytes() const
{
    size_t total = 0;
    for (std::vector<ObexHeader>::const_iterator i = m_headers.begin(); i != m_headers.end(); ++i)
        total += i->getTotalSizeBytes();

    return total;
}

bool ObexHeaderList::getBodyLength(uint64_t & length) const
{
    const ObexHeader* lengthHeader = find(OBEX_HEADER_LENGTH);
    if (lengthHeader) {
        length = lengthHeader->getIntData();
        return true;
    }

    // No length header - check for an HTTP header, for lengths greater than
    // 4 gigabytes - 1 the HTTP header is used since the Obex length header
    // is only 4 bytes.
    std::string httpContentLength = findHttpHeader("Content-Length");
    if (!httpContentLength.empty()) {
        length = std::strtoul(httpContentLength.c_str(), NULL, 0); // TODO - strtoull for uint64_t
        return true;
    }

    return false;
}

std::string ObexHeaderList::findHttpHeader(const char* headerName) const
{
    for (std::vector<ObexHeader>::const_iterator ih = m_headers.begin(); ih != m_headers.end(); ++ih) {
        if (ih->getCode() == OBEX_HEADER_HTTP) {
            std::string header(ih->getByteSequenceData(), ih->getByteSequenceDataSize());
            size_t colonPos = header.find(':');
            if (colonPos != std::string::npos && colonPos < header.length() && header.compare(0, colonPos, headerName) == 0) {
                size_t valStart = header.find_first_not_of(' ', colonPos + 1);
                if (valStart != std::string::npos) {
                    return std::string(header, valStart);
                }
            }
        }
    }

    return std::string();
}

SYNC_API std::ostream& operator<<(std::ostream& os, const ObexHeaderList& hs)
{
    for (std::vector<ObexHeader>::const_iterator i = hs.getHeaders().begin(); i != hs.getHeaders().end(); ++i)
        os << *i;
    return os;
}

SYNC_API std::istream& operator>>(std::istream& is, ObexHeader& h)
{
    unsigned char code = (unsigned char) is.get();
    h.setCode((ObexHeaderCode) code);

    switch (getHeaderFormat(h.getCode())) {
    case HeaderFormat::LENGTH_PREFIXED_NULL_TERMINATED_UNICODE:
    {
        unsigned char lengthByte1 = (unsigned char)is.get();
        unsigned char lengthByte2 = (unsigned char)is.get();
        int length = (lengthByte1 << 8) | lengthByte2;
        int dataLength = length - HEADER_CODE_SIZE - HEADER_LENGTH_SIZE;
        CString s;
        for (int i = 0; i < dataLength/2 - 2; i++) // loop to dataLength/2 - 2 to ignore null chars
        {
            unsigned char charByte1 = (unsigned char)is.get();
            unsigned char charByte2 = (unsigned char)is.get();
            wchar_t c = (charByte1 << 8) | charByte2;
            s += c;
        }

        // Skip over the 4 null terminator bytes
        for (int i = 0; i < 4; ++i) {
            is.get();
        }
        h.setStringData(s);

        break;
    }
    case HeaderFormat::LENGTH_PREFIXED_BYTE_SEQUENCE:
    {
        unsigned char lengthByte1 = (unsigned char)is.get();
        unsigned char lengthByte2 = (unsigned char)is.get();
        int length = (lengthByte1 << 8) | lengthByte2;
        int dataLength = length - HEADER_CODE_SIZE - HEADER_LENGTH_SIZE;
        if (dataLength > 0) {
            std::string buffer;
            buffer.resize(dataLength);
            is.read(&buffer[0], dataLength);
            h.setByteSequenceData(&buffer[0], dataLength);
        }
        else {
            h.setByteSequenceData(NULL, 0);
        }
        break;
    }
    case HeaderFormat::SINGLE_BYTE:
        h.setByteData((unsigned char) is.get());
        break;
    case HeaderFormat::FOUR_BYTE:
    {
        unsigned char bytes[4];
        is.read(reinterpret_cast<char*>(bytes), 4);
        int data = bytes[3] | ((int)bytes[2]) << 8 | ((int)bytes[1]) << 16 | ((int)bytes[0]) << 24;
        h.setIntData(data);
        break;
    }
    }

    return is;
}
