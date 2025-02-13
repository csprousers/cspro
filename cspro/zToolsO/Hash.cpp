#include "StdAfx.h"
#include "Hash.h"
#include "Encoders.h"

extern "C"
{
#include "scrypt/sha256.h"
}


std::vector<std::byte> Hash::Hash(const std::byte* data, size_t data_length,
                                  const std::byte* salt, size_t salt_length,
                                  const size_t hash_length, const size_t iterations/* = DefaultIterations*/)
{
    ASSERT(sizeof(uint8_t) == sizeof(std::byte));
    std::vector<std::byte> hash(hash_length);

    PBKDF2_SHA256(reinterpret_cast<const uint8_t*>(data), data_length,
                  reinterpret_cast<const uint8_t*>(salt), salt_length,
                  iterations,
                  reinterpret_cast<uint8_t*>(hash.data()),
                  hash_length);

    return hash;
}


std::wstring Hash::Hash(const std::byte* const data, const size_t data_length, const size_t hash_length, const wstring_view salt_sv)
{
    const std::string utf8_salt = UTF8Convert::WideToUTF8(salt_sv);

    std::vector<std::byte> hash = Hash(data, data_length,
                                       reinterpret_cast<const std::byte*>(utf8_salt.c_str()), utf8_salt.length(),
                                       hash_length);

    return BytesToHexString(hash.data(), hash.size());
}


std::wstring Hash::Hash(const std::byte* const data, const size_t data_length, const size_t hash_length/* = DefaultHashLength*/)
{
    std::vector<std::byte> hash = Hash(data, data_length,
                                       nullptr, 0,
                                       hash_length);

    return BytesToHexString(hash.data(), hash.size());
}


std::wstring Hash::Hash(const wstring_view text_sv, const size_t hash_length, const wstring_view salt_sv)
{
    const std::string utf8_text = UTF8Convert::WideToUTF8(text_sv);

    return Hash(reinterpret_cast<const std::byte*>(utf8_text.c_str()), utf8_text.length(), hash_length, salt_sv);
}

std::wstring Hash::Hash(const wstring_view text_sv, const size_t hash_length/* = DefaultHashLength*/)
{
    const std::string utf8_text = UTF8Convert::WideToUTF8(text_sv);

    return Hash(reinterpret_cast<const std::byte*>(utf8_text.c_str()), utf8_text.length(), hash_length);
}


std::wstring Hash::BytesToHexString(const std::byte* bytes, const size_t bytes_length)
{
    std::wstring hex_string;
    hex_string.resize(bytes_length * 2);

    TCHAR* ch = hex_string.data();

    for( const std::byte* bytes_end = bytes + bytes_length; bytes != bytes_end; ++bytes )
    {
        const unsigned char this_byte = static_cast<unsigned char>(*bytes);
        *(ch++) = Encoders::HexChars[this_byte >> 4];
        *(ch++) = Encoders::HexChars[this_byte & 0x0F];
    }

    return hex_string;
}


void Hash::HexStringToBytesBuffer(const wstring_view hex_string_sv, std::byte* bytes, const bool throw_exceptions)
{
    constexpr const char* ExceptionMessage = "The hex string is not valid";

    if( throw_exceptions && hex_string_sv.size() % 2 != 0 )
        throw CSProException(ExceptionMessage);

    for( size_t i = 1; i < hex_string_sv.size(); i += 2 )
    {
        const TCHAR* first_hex_char = _tcschr(Encoders::HexChars, std::towlower(hex_string_sv[i - 1]));
        const TCHAR* second_hex_char = _tcschr(Encoders::HexChars, std::towlower(hex_string_sv[i]));

        if( first_hex_char != nullptr && second_hex_char != nullptr )
        {
            *(bytes++) = static_cast<std::byte>( ( ( first_hex_char - Encoders::HexChars ) << 4 ) | ( second_hex_char - Encoders::HexChars ) );
        }

        else if( throw_exceptions )
        {
            throw CSProException(ExceptionMessage);
        }
    }
}


std::vector<std::byte> Hash::HexStringToBytes(const wstring_view hex_string_sv, const bool throw_exceptions)
{
    std::vector<std::byte> bytes(hex_string_sv.length() / 2);
    HexStringToBytesBuffer(hex_string_sv, bytes.data(), throw_exceptions);
    return bytes;
}
