#pragma once

#include <zToolsO/zToolsO.h>


// --------------------------------------------------------------------------
// WinMemoryStream
//
// A class that wraps a block of memory in an IStream* object.
// The destructor releases the stream.
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO WinMemoryStream
{
private:
    WinMemoryStream(IStream* stream) noexcept;

public:
    ~WinMemoryStream() noexcept;

    IStream* GetStream() noexcept { return m_stream; }

    // Instantiates an object, copying the block of memory.
    // On error (e.g., no memory available), null is returned.
    static std::unique_ptr<WinMemoryStream> Create(const void* buffer, size_t size) noexcept;

private:
    IStream* m_stream;
};
