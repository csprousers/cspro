#pragma once

#include <zToolsO/zToolsO.h>


namespace Hash
{
    constexpr size_t DefaultHashLength = 32;
    constexpr size_t MaxHashLength     = 500;

    constexpr size_t DefaultIterations = 1024;

    // Hashes the data (with an optional salt).
    CLASS_DECL_ZTOOLSO std::vector<std::byte> Hash(const std::byte* data, size_t data_length,
                                                   const std::byte* salt, size_t salt_length,
                                                   size_t hash_length, size_t iterations = DefaultIterations);

    // Returns a hex string (of length 2 * length) of the data with a UTF-8 representation of the salt.
    CLASS_DECL_ZTOOLSO std::string Hash(const std::byte* data, size_t data_length, size_t hash_length, std::string_view salt_sv);
    CLASS_DECL_ZTOOLSO std::string Hash(const std::byte* data, size_t data_length, size_t hash_length = DefaultHashLength);

    // Returns a hex string (of length 2 * length) of the UTF-8 representation of the text and salt.
    CLASS_DECL_ZTOOLSO std::string Hash(std::string_view text_sv, size_t hash_length, std::string_view salt_sv);
    CLASS_DECL_ZTOOLSO std::string Hash(std::string_view text_sv, size_t hash_length = DefaultHashLength);

    // Returns a string with the hex representation of the bytes.
    CLASS_DECL_ZTOOLSO std::string BytesToHexString(const std::byte* bytes, size_t bytes_length);

    // Converts a hex string to bytes. The bytes buffer must be allocated to be at least half of hex_string's length.
    CLASS_DECL_ZTOOLSO void HexStringToBytesBuffer(std::string_view hex_string_sv, std::byte* bytes, bool throw_exceptions);

    // Converts a hex string to bytes.
    CLASS_DECL_ZTOOLSO std::vector<std::byte> HexStringToBytes(std::string_view hex_string_sv, bool throw_exceptions);

    template<class ST, class HT>
    void Combine(ST& seed, HT&& v)
    {
        // from boost: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
        std::hash<std::remove_cvref_t<HT>> hasher;
        seed ^= static_cast<ST>(hasher(std::forward<HT>(v))) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    }
}
