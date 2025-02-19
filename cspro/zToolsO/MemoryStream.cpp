#include "StdAfx.h"
#include "MemoryStream.h"


// based on https://stackoverflow.com/questions/13059091/creating-an-input-stream-from-constant-memory

class MemoryStream::Buffer : public std::streambuf
{
public:
    Buffer(const std::byte* const data, const size_t size)
        :   m_dataStart(const_cast<char*>(reinterpret_cast<const char*>(data))),
            m_dataEnd(m_dataStart + size)
    {
        setg(m_dataStart, m_dataStart, m_dataEnd);
    }

protected:
    pos_type seekoff(const off_type off, const ios_base::seekdir way, const ios_base::openmode which = ios_base::in | ios_base::out) override
    {
        char* const from_pos = ( way == std::ios::beg ) ? m_dataStart :
                               ( way == std::ios::cur ) ? gptr() :
                               ( way == std::ios::end ) ? m_dataEnd :
                                                          ReturnProgrammingError(m_dataStart);

        return SeekWorker(from_pos + std::streamoff(off), which);
    }

    pos_type seekpos(const pos_type sp, const std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
    {
        return SeekWorker(m_dataStart + std::streamoff(sp), which);
    }

private:
    pos_type SeekWorker(char* const next_pos, const std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        ASSERT(( which & std::ios_base::in ) != 0);

        if( next_pos < m_dataStart || next_pos > m_dataEnd )
            return -1;

        setg(m_dataStart, next_pos, m_dataEnd);

        return next_pos - m_dataStart;
    }

private:
    char* m_dataStart;
    char* m_dataEnd;
};



MemoryStream::MemoryStream(std::unique_ptr<Buffer> stream_buffer)
    :   std::istream(stream_buffer.get()),
        m_streamBuffer(std::move(stream_buffer))
{
}


MemoryStream::MemoryStream(const std::byte* const data, const size_t size)
        :   MemoryStream(std::make_unique<Buffer>(data, size))
{
}


MemoryStream::~MemoryStream()
{
}
