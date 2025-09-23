#include "StdAfx.h"
#include "WinMemoryStream.h"


WinMemoryStream::WinMemoryStream(IStream* const stream) noexcept
    :   m_stream(stream)
{
    ASSERT(m_stream != nullptr);
}


WinMemoryStream::~WinMemoryStream() noexcept
{
    m_stream->Release();
}


std::unique_ptr<WinMemoryStream> WinMemoryStream::Create(const void* const buffer, const size_t size) noexcept
{
    // allocate the memory
    HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, size);

    if( hMem == nullptr )
        return nullptr;

    // lock and copy the buffer
    void* const destination_buffer = ::GlobalLock(hMem);

    if( destination_buffer == nullptr )
    {
        ::GlobalFree(hMem);
        return nullptr;
    }

    memcpy(destination_buffer, buffer, size);

    ::GlobalUnlock(hMem);

    // wrap in an IStream, with the memory deleted on release
    IStream* stream;

    if( FAILED(::CreateStreamOnHGlobal(hMem, TRUE, &stream)) )
    {
        ::GlobalFree(hMem);
        return nullptr;
    }

    return std::unique_ptr<WinMemoryStream>(new WinMemoryStream(stream));
}
