#include "stdafx.h"
#include "ZLib.h"
#include <zToolsO/MemoryStream.h>
#include <sstream>


namespace
{
    constexpr size_t MaxBufferSize = 256000;
    constexpr int CompressionLevel = Z_BEST_COMPRESSION;
}


template<bool IsDeflate>
bool ZLib::DeflateInflate(std::istream& in, std::ostream& out)
{
    // Determine input's size.
    in.seekg(0, std::ios::end);
    const std::streampos inputSize = in.tellg();
    in.seekg(0, std::ios::beg);

    // Maximum input size is 2 GB. Remove limitation with a chunking implementation.
    if( inputSize < 0 || inputSize > std::numeric_limits<int>::max() )
        return false;

    // Empty stream, so do nothing
    if( inputSize == std::streampos(0) ) 
        return true;

    size_t input_remaining = static_cast<size_t>(inputSize);
    const size_t buffer_size = std::min(MaxBufferSize, input_remaining);
    auto buffer_in = std::make_unique_for_overwrite<char[]>(buffer_size);
    auto buffer_out = std::make_unique_for_overwrite<char[]>(buffer_size);

    // Init the z_stream
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = reinterpret_cast<const unsigned char*>(buffer_in.get());
    stream.avail_in = 0;
    stream.next_out = reinterpret_cast<unsigned char*>(buffer_out.get());
    stream.avail_out = buffer_size;

    // initialize compression or decompression
    if constexpr(IsDeflate)
    {
        if( deflateInit(&stream, CompressionLevel) != Z_OK )
            return false;
    }

    else
    {
        if( inflateInit(&stream) != Z_OK )
            return false;
    }

    while( true )
    {
        // Input buffer is empty, so read more bytes from input stream.
        if( stream.avail_in == 0 )
        {
            const size_t read_size = std::min(buffer_size, input_remaining);

            if( !in.read(buffer_in.get(), read_size) )
                return false;

            stream.next_in = reinterpret_cast<const unsigned char*>(buffer_in.get());
            stream.avail_in = read_size;

            input_remaining -= read_size;
        }

        int status;

        if constexpr(IsDeflate)
        {
            status = deflate(&stream, ( input_remaining != 0 ) ? Z_NO_FLUSH : Z_FINISH);
        }

        else
        {
            status = inflate(&stream, Z_SYNC_FLUSH);
        }

        // Output buffer is full, or compression/decompression is done, so write buffer to output stream.
        if( status == Z_STREAM_END || stream.avail_out == 0 )
        {
            if( !out.write(buffer_out.get(), buffer_size - stream.avail_out) )
                return false;

            stream.next_out = reinterpret_cast<unsigned char*>(buffer_out.get());
            stream.avail_out = buffer_size;
        }

        if( status == Z_STREAM_END )
            break;

        if( status != Z_OK )
            return false;
    }

    if constexpr(IsDeflate)
    {
        return ( deflateEnd(&stream) == Z_OK );
    }

    else
    {
        return ( inflateEnd(&stream) == Z_OK );
    }
}


bool ZLib::Deflate(std::istream& in, std::ostream& out)
{
    return DeflateInflate<true>(in, out);
}


bool ZLib::Inflate(std::istream& in, std::ostream& out)
{
    return DeflateInflate<false>(in, out);
}


template<bool IsDeflate>
bool ZLib::DeflateInflate(std::string& data)
{
    std::istringstream in(data);
    std::ostringstream out;

    if( DeflateInflate<IsDeflate>(in, out) )
    {
        data = out.str();
        return true;
    }

    return false;
}


bool ZLib::Deflate(std::string& data)
{
    return DeflateInflate<true>(data);
}


bool ZLib::Inflate(std::string& data)
{
    return DeflateInflate<false>(data);
}


bool ZLib::IsDeflated(const cs::string_sz data)
{
    // First byte is CMF, second byte is FLG
    // CMF will be 0x78 and FLG will depend on compression level
    // but bits 0-4 of FLG are check bits such that CMF and FLG
    // as MSB 16 bit integer is divisible by 31
    // https://tools.ietf.org/html/rfc1950

    const unsigned int check = data.front();

    return ( ( check == 0x78 ) &&
             ( ( ( ( check << 8 ) | static_cast<unsigned char>(data[1]) ) % 31 ) == 0 ) );
}


std::string ZLib::Deflate(const BinaryBlock& bytes)
{
    MemoryStream in(bytes);
    std::ostringstream out;

    if( DeflateInflate<true>(in, out) )
        return out.str();

    return ReturnProgrammingError(std::string());
}
