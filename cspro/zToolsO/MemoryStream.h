#pragma once

#include <streambuf>


// based on https://stackoverflow.com/questions/13059091/creating-an-input-stream-from-constant-memory

struct MemoryStreamBuffer : public std::streambuf
{
    MemoryStreamBuffer(const std::byte* data, size_t size)
    {
        char* data_as_char = const_cast<char*>(reinterpret_cast<const char*>(data));
        setg(data_as_char, data_as_char, data_as_char + size);
    }
};


struct MemoryStream : virtual public MemoryStreamBuffer, public std::istream
{
    MemoryStream(const std::byte* data, size_t size)
        :   MemoryStreamBuffer(data, size),
            std::istream(static_cast<std::streambuf*>(this))
    {
    }
};
