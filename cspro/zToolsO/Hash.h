#pragma once

#include <zToolsO/zToolsO.h>

// --------------------------------------------------------------------------
// Hash
//
// The Hash class:
//     - Hashes data using PBKDF2_SHA256.
//     - Provides bytes <-> hex string conversions.
//
// The Hash::Md5 class creates Message-Digest (RFC 1321) hashes.
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO Hash
{
public:
    class Md5;

    constexpr static size_t DefaultHashLength = 32;
    constexpr static size_t MaxHashLength     = 500;

    constexpr static size_t DefaultIterations = 1024;

    // Hashes the data (with an optional salt).
    static std::vector<std::byte> Create(const std::byte* data, size_t data_length,
                                         const std::byte* salt, size_t salt_length,
                                         size_t hash_length, size_t iterations = DefaultIterations);

    // Returns a hex string (of length 2 * length) of the data with a UTF-8 representation of the salt.
    static std::string Create(const std::byte* data, size_t data_length, size_t hash_length, std::string_view salt_sv);
    static std::string Create(const std::byte* data, size_t data_length, size_t hash_length = DefaultHashLength);

    // Returns a hex string (of length 2 * length) of the UTF-8 representation of the text and salt.
    static std::string Create(std::string_view text_sv, size_t hash_length, std::string_view salt_sv);
    static std::string Create(std::string_view text_sv, size_t hash_length = DefaultHashLength);

    // Returns a string with the hex representation of the bytes.
    static std::string BytesToHexString(const void* bytes, size_t bytes_length);

    // Converts a hex string to bytes. The bytes buffer must be allocated to be at least half of hex_string's length.
    static void HexStringToBytesBuffer(std::string_view hex_string_sv, std::byte* bytes, bool throw_exceptions);

    // Converts a hex string to bytes.
    static std::vector<std::byte> HexStringToBytes(std::string_view hex_string_sv, bool throw_exceptions);

    template<class ST, class HT>
    static void Combine(ST& seed, HT&& v)
    {
        // from boost: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
        std::hash<std::remove_cvref_t<HT>> hasher;
        seed ^= static_cast<ST>(hasher(std::forward<HT>(v))) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    }
};



// --------------------------------------------------------------------------
// Hash::Md5
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO Hash::Md5
{
public:
    // Returns the MD5 of a file.
    // If throw_exception_on_read_error is false, an empty string is returned on error.
    static std::string CreateFromFile(const InterfaceString& file_path, bool throw_exception_on_read_error = false);

    // Returns the MD5 of a stream, throwing an exception on error.
    // The stream position is not reset after the calculation.
    static std::string CreateFromStream(std::istream& input_stream);

    // Returns the MD5 of a block of memory.
    static std::string Create(const std::byte* contents, size_t size);

    // Returns the MD5 of a block of memory of an object that has data and size members.
    template<typename T>
    static std::string Create(const T& contents)
    {
        static_assert(sizeof(std::remove_pointer_t<decltype(contents.data())>) == sizeof(std::byte));
        return Create(reinterpret_cast<const std::byte*>(contents.data()), contents.size());
    }

private:
    template<typename CF>
    static std::string GenerateMd5(const CF& md5_update_callback);
};
