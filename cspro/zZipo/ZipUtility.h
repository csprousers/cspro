#pragma once
#include <zZipo/zZipo.h>

#include <iostream>

namespace ZipUtility
{
    /// <summary>Compress a stream</summary>
    CLASS_DECL_ZZIPO int compression(std::istream &in, std::ostream &out);
    /// <summary>Decompress a stream</summary>
    CLASS_DECL_ZZIPO int decompression(std::istream &in, std::ostream &out);
    /// <summary>Convenience wrapper to compress a string in place</summary>
    CLASS_DECL_ZZIPO int compression(std::string &data);
    /// <summary>Convenience wrapper to decompress a string in place</summary>
    CLASS_DECL_ZZIPO int decompression(std::string &data);
    /// <summary>Determine if a string is compressed using compression function</summary>
    CLASS_DECL_ZZIPO bool isCompressed(std::string_view data);
}
