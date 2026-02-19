#pragma once

#include <zToolsO/zToolsO.h>
#include <istream>


class CLASS_DECL_ZTOOLSO MemoryStream : public std::istream
{
private:
    class Buffer;
    MemoryStream(std::unique_ptr<Buffer> stream_buffer);

public:
    template<typename T>
    MemoryStream(const T* data, size_t size);

    template<typename T>
    MemoryStream(const T& data);

    ~MemoryStream();

    [[nodiscard]] size_t size() const;

private:
    // parameters flipped from the public constructor's to make it unique
    MemoryStream(size_t size, const std::byte* data);

private:
    std::unique_ptr<Buffer> m_streamBuffer;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
MemoryStream::MemoryStream(const T* const data, const size_t size)
    :   MemoryStream(size, reinterpret_cast<const std::byte*>(data))
{
    static_assert(sizeof(T) == sizeof(std::byte));
}


template<typename T>
MemoryStream::MemoryStream(const T& data)
    :   MemoryStream(data.data(), data.size())
{
}
