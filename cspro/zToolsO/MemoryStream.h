#pragma once

#include <zToolsO/zToolsO.h>
#include <istream>


class CLASS_DECL_ZTOOLSO MemoryStream : public std::istream
{
private:
    class Buffer;
    MemoryStream(std::unique_ptr<Buffer> stream_buffer);

public:
    MemoryStream(const std::byte* data, size_t size);

    MemoryStream(const char* data, size_t size) : MemoryStream(reinterpret_cast<const std::byte*>(data), size) { }

    template<typename T>
    MemoryStream(const T& data) : MemoryStream(reinterpret_cast<const std::byte*>(data.data()), data.size()) { }

    ~MemoryStream();

private:
    std::unique_ptr<Buffer> m_streamBuffer;
};
