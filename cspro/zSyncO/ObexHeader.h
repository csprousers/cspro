#pragma once
#include <zSyncO/zSyncO.h>
#include <zSyncO/ObexConstants.h>
#include <ostream>
#include <istream>

/**
* Headers sent with Obex requests.
**/
class SYNC_API ObexHeader {
public:

    ObexHeader();

    ObexHeader(ObexHeaderCode code, CString data);

    ObexHeader(ObexHeaderCode code, uint32_t data);

    ObexHeader(ObexHeaderCode code, const char* data, size_t dataSizeBytes);

    ObexHeaderCode getCode() const
    {
        return m_code;
    }

    void setCode(ObexHeaderCode code)
    {
        m_code = code;
    }

    // Get size in bytes of entire header including both fixed (code, length...)
    // and variable portion (data itself)
    size_t getTotalSizeBytes() const;

    // Get size of just fixed portion for this type of header
    static size_t getBaseSize(ObexHeaderCode code);

    const CString& getStringData() const
    {
        return m_stringData;
    }

    void setStringData(CString s)
    {
        m_stringData = s;
    }

    uint32_t getIntData() const
    {
        return m_intData;
    }

    void setIntData(uint32_t i)
    {
        m_intData = i;
    }

    unsigned char getByteData() const
    {
        return m_byteData;
    }

    void setByteData(unsigned char b)
    {
        m_byteData = b;
    }

    const char* getByteSequenceData() const
    {
        return &m_byteSequenceData[0];
    }

    size_t getByteSequenceDataSize() const
    {
        return m_byteSequenceData.size();
    }

    void setByteSequenceData(const char* data, size_t size)
    {
        m_byteSequenceData.resize(size);
        memcpy(&m_byteSequenceData[0], data, size);
    }

private:
    ObexHeaderCode m_code;
    CString m_stringData;
    uint32_t m_intData;
    unsigned char m_byteData;
    std::string m_byteSequenceData;
};

class ObexHeaderList {

public:

    void add(const ObexHeader& h);
    void add(const ObexHeaderList& h);
    const std::vector<ObexHeader>& getHeaders() const;
    const ObexHeader* find(ObexHeaderCode code) const;
    size_t getTotalSizeBytes() const;
    bool getBodyLength(uint64_t &length) const;
    std::string findHttpHeader(const char* headerName) const;

private:
    std::vector<ObexHeader> m_headers;
};

SYNC_API std::ostream& operator<<(std::ostream& os, const ObexHeader& h);
SYNC_API std::ostream& operator<<(std::ostream& os, const ObexHeaderList& hs);
SYNC_API std::istream& operator>>(std::istream& is, ObexHeader& h);
