#pragma once

#include <zToolsO/Serializer.h>
#include <zToolsO/bzlib.h>
#include <zToolsO/PortableFunctions.h>


class PenSerializer : public SerializerImpl
{
public:
    PenSerializer();
    ~PenSerializer();

    void Open(const std::wstring& filename) override;
    void Create(const std::wstring& filename) override;
    void Close() override;
    void Read(void* buffer, int length) override;
    void Write(const void* buffer, int length) override;

private:
    FILE* m_file;
    BZFILE* m_bzFile;
    int m_bzError;
    bool m_saving;
};


PenSerializer::PenSerializer()
    :   m_file(nullptr),
        m_bzFile(nullptr),
        m_bzError(BZ_OK),
        m_saving(false)
{
}


PenSerializer::~PenSerializer()
{
    try
    {
        Close();
    }

    catch( const SerializationException& ) { }
}


void PenSerializer::Open(const std::wstring& filename)
{
    Close();

    m_saving = false;

    m_file = PortableFunctions::FileOpen(filename, _T("rb"));

    if( m_file == nullptr )
        throw SerializationException(_T("Could not open the serialization archive: %s"), filename.c_str());

    m_bzFile = BZ2_bzReadOpen(&m_bzError, m_file, 0, 0, nullptr, 0);

    if( m_bzError != BZ_OK )
        Close();
}


void PenSerializer::Create(const std::wstring& filename)
{
    Close();

    m_saving = true;

    m_file = PortableFunctions::FileOpen(filename, _T("wb"));

    if( m_file == nullptr )
        throw SerializationException(_T("Could not create the serialization archive: %s"), filename.c_str());

    // 9 = the block size (900,000 bytes), 30 = the work factor (the recommended value)
    m_bzFile = BZ2_bzWriteOpen(&m_bzError, m_file, 9, 0, 30);

    if( m_bzError != BZ_OK )
        Close();
}


void PenSerializer::Close()
{
    if( m_file == nullptr )
        return;

    if( m_saving )
    {
        BZ2_bzWriteClose(&m_bzError, m_bzFile, 0, nullptr, nullptr);
    }

    else
    {
        BZ2_bzReadClose(&m_bzError, m_bzFile);
    }

    fclose(m_file);
    m_file = nullptr;

    if( m_bzError != BZ_OK )
        throw SerializationException("Error closing the serialization archive.");
}


void PenSerializer::Read(void* buffer, int length)
{
    ASSERT(!m_saving);

    if( length <= 0 )
        return;

    BZ2_bzRead(&m_bzError, m_bzFile, buffer, length);

    if( m_bzError != BZ_OK && m_bzError != BZ_STREAM_END )
        throw SerializationException("Error reading from the serialization archive.");
}


void PenSerializer::Write(const void* buffer, int length)
{
    ASSERT(m_saving);

    if( length <= 0 )
        return;

    BZ2_bzWrite(&m_bzError, m_bzFile, (void*)buffer, length);

    if( m_bzError != BZ_OK )
        throw SerializationException("Error writing to the serialization archive.");
}
