#pragma once

#include <zNetwork/SyncListener.h>


namespace StreamCopier
{
    void Copy(SyncListener* sync_listener, std::istream& input_stream, std::ostream& output_stream, const int64_t size_bytes);
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void StreamCopier::Copy(SyncListener* const sync_listener, std::istream& input_stream, std::ostream& output_stream, const int64_t size_bytes)
{
    constexpr size_t MaxBufferSize = 32768;
    const size_t buffer_size = std::min(MaxBufferSize, static_cast<size_t>(size_bytes));
    auto buffer = std::make_unique_for_overwrite<char[]>(buffer_size);

    if( sync_listener != nullptr )
    {
        sync_listener->SetProgressTotal(size_bytes);
        sync_listener->Progress(0);
    }

    int64_t bytes_copied = 0;

    do
    {
        input_stream.read(buffer.get(), buffer_size);
        output_stream.write(buffer.get(), input_stream.gcount());

        if( sync_listener != nullptr )
        {
            if( sync_listener->IsCanceled() )
                throw SyncCancelException();

            bytes_copied += input_stream.gcount();
            sync_listener->Progress(bytes_copied);
        }

    } while( input_stream.gcount() > 0 );

    ASSERT(bytes_copied == size_bytes || sync_listener == nullptr);
}
