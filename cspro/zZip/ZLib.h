#pragma once

#include <zZip/zZip.h>
#include <iosfwd>


class CLASS_DECL_ZZIP ZLib
{
public:
    // Compress a stream.
    static bool Deflate(std::istream& in, std::ostream& out);

    // Decompress a stream.
    static bool Inflate(std::istream& in, std::ostream& out);

    // Convenience wrapper to compress a string in place.
    static bool Deflate(std::string& data);

    // Convenience wrapper to decompress a string in place.
    static bool Inflate(std::string& data);

    // Determine if a string is compressed using the deflate routine.
    static bool IsDeflated(cs::string_sz data);

    // Compress bytes, returning the compressed bytes as a string.
    static std::string Deflate(const BinaryBlock& bytes);

private:
    template<bool IsDeflate>
    static bool DeflateInflate(std::istream& in, std::ostream& out);

    template<bool IsDeflate>
    static bool DeflateInflate(std::string& data);
};
