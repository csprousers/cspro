#include "stdafx.h"
#include "ZipUtility.h"
#include <limits.h>
#include <sstream>

#define MINIZ_HEADER_FILE_ONLY
#include <external/miniz/miniz.c>


namespace ZipUtility
{

using uint8 = unsigned char;
using uint = unsigned int;

const uint bufSize = 256000;
const int level = Z_BEST_COMPRESSION;

int compression(std::istream &in, std::ostream &out)
{
    uint8 inbuf[bufSize] = {0};
    uint8 outbuf[bufSize] = {0};

    // Determine input's size.
    in.seekg(0, std::ios::end);
    const std::streampos inputSize = in.tellg();
    in.seekg(0, std::ios::beg);

    if ((inputSize < 0) || (inputSize > std::numeric_limits<int>::max()))
    {
        // Maximum input size is 2 GB. Remove limitation with a chunking implementation.
        return EXIT_FAILURE;
    }

    if (inputSize == std::streampos(0)) {
        // Empty stream, so do nothing
        return EXIT_SUCCESS;
    }

    // Init the z_stream
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = inbuf;
    stream.avail_in = 0;
    stream.next_out = outbuf;
    stream.avail_out = bufSize;

    // Compression.
    uint inputRemaining = static_cast<uint>(inputSize);

    if (deflateInit(&stream, level) != Z_OK)
    {
        return EXIT_FAILURE;
    }

    for (;;)
    {
        int status;
        if (!stream.avail_in)
        {
            // Input buffer is empty, so read more bytes from input file.
            uint n = std::min(bufSize, inputRemaining);

            if (!in.read(reinterpret_cast<char*>(inbuf), n))
            {
                return EXIT_FAILURE;
            }

            stream.next_in = inbuf;
            stream.avail_in = n;

            inputRemaining -= n;
        }

        status = deflate(&stream, inputRemaining ? Z_NO_FLUSH : Z_FINISH);

        if ((status == Z_STREAM_END) || (!stream.avail_out))
        {
            // Output buffer is full, or compression is done, so write buffer to output file.
            uint n = bufSize - stream.avail_out;

            if (!out.write(reinterpret_cast<char*>(outbuf), n))
            {
                return EXIT_FAILURE;
            }

            stream.next_out = outbuf;
            stream.avail_out = bufSize;
        }

        if (status == Z_STREAM_END)
        {
            break;
        }
        else if (status != Z_OK)
        {
            return EXIT_FAILURE;
        }
    }

    if (deflateEnd(&stream) != Z_OK)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int decompression(std::istream &in, std::ostream &out)
{
    uint8 inbuf[bufSize] = {0};
    uint8 outbuf[bufSize] = {0};

    in.seekg(0, std::ios::end);
    const std::streampos inputSize = in.tellg();
    in.seekg(0, std::ios::beg);

    if ((inputSize < 0) || (inputSize > std::numeric_limits<int>::max()))
    {
        // Maximum input size is 2 GB. Remove limitation with a chunking implementation.
        return EXIT_FAILURE;
    }

    if (inputSize == std::streampos(0)) {
        // Empty stream, so do nothing
        return EXIT_SUCCESS;
    }

    // Init the z_stream
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = inbuf;
    stream.avail_in = 0;
    stream.next_out = outbuf;
    stream.avail_out = bufSize;

    // Decompression.
    uint inputRemaining = static_cast<uint>(inputSize);

    if (inflateInit(&stream))
    {
        return EXIT_FAILURE;
    }

    for (;;)
    {
        int status;
        if (!stream.avail_in)
        {
            // Input buffer is empty, so read more bytes from input file.
            uint n = std::min(bufSize, inputRemaining);

            if (!in.read(reinterpret_cast<char*>(inbuf), n))
            {
                return EXIT_FAILURE;
            }

            stream.next_in = inbuf;
            stream.avail_in = n;

            inputRemaining -= n;
        }

        status = inflate(&stream, Z_SYNC_FLUSH);

        if ((status == Z_STREAM_END) || (!stream.avail_out))
        {
            // Output buffer is full, or decompression is done, so write buffer to output file.
            uint n = bufSize - stream.avail_out;

            if (!out.write(reinterpret_cast<char*>(outbuf), n))
            {
                return EXIT_FAILURE;
            }
            stream.next_out = outbuf;
            stream.avail_out = bufSize;
        }

        if (status == Z_STREAM_END)
        {
            break;
        }
        else if (status != Z_OK)
        {
            return EXIT_FAILURE;
        }
    }

    if (inflateEnd(&stream) != Z_OK)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int compression(std::string &data)
{
    std::istringstream in(data);
    std::ostringstream out;

    const int result = compression(in, out);
    if (result == EXIT_SUCCESS)
    {
        data = out.str();
    }

    return result;
}

int decompression(std::string &data)
{
    std::istringstream in(data);
    std::ostringstream out;

    const int result = decompression(in, out);
    if (result == EXIT_SUCCESS)
    {
        data = out.str();
    }

    return result;
}

bool isCompressed(std::string_view data)
{
    // First byte is CMF, second byte is FLG
    // CMF will be 78 and FLG will depend on compression level
    // but bits 0-4 of FLG are check bits such that CMF and FLG
    // as MSB 16 bit integer is divisible by 31
    // https://tools.ietf.org/html/rfc1950
    return data[0] == 0x78 && ((data[0] << 8) | data[1]) % 31 == 0;
}

} // namespace ZipUtility
