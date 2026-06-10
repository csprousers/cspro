#include "StdAfx.h"
#include "Hash.h"
#include "Encoders.h"
#include <istream>

extern "C"
{
#include "md5.h"
#include <external/scrypt/sha256.h>
}


// --------------------------------------------------------------------------
// Hash: PBKDF2_SHA256
// --------------------------------------------------------------------------

std::vector<std::byte> Hash::Create(const std::byte* const data, const size_t data_length,
                                    const std::byte* const salt, const size_t salt_length,
                                    const size_t hash_length, const size_t iterations/* = DefaultIterations*/)
{
    static_assert(sizeof(uint8_t) == sizeof(std::byte));
    std::vector<std::byte> hash(hash_length);

    PBKDF2_SHA256(reinterpret_cast<const uint8_t*>(data), data_length,
                  reinterpret_cast<const uint8_t*>(salt), salt_length,
                  iterations,
                  reinterpret_cast<uint8_t*>(hash.data()),
                  hash_length);

    return hash;
}


std::string Hash::Create(const std::byte* const data, const size_t data_length, const size_t hash_length, const std::string_view salt_sv)
{
    const std::vector<std::byte> hash = Create(data, data_length,
                                               reinterpret_cast<const std::byte*>(salt_sv.data()), salt_sv.length(),
                                               hash_length);

    return BytesToHexString(hash.data(), hash.size());
}


std::string Hash::Create(const std::byte* const data, const size_t data_length, const size_t hash_length/* = DefaultHashLength*/)
{
    const std::vector<std::byte> hash = Create(data, data_length,
                                               nullptr, 0,
                                               hash_length);

    return BytesToHexString(hash.data(), hash.size());
}


std::string Hash::Create(const std::string_view text_sv, const size_t hash_length, const std::string_view salt_sv)
{
    return Create(reinterpret_cast<const std::byte*>(text_sv.data()), text_sv.length(), hash_length, salt_sv);
}


std::string Hash::Create(const std::string_view text_sv, const size_t hash_length/* = DefaultHashLength*/)
{
    return Create(reinterpret_cast<const std::byte*>(text_sv.data()), text_sv.length(), hash_length);
}


std::string Hash::BytesToHexString(const void* const bytes, const size_t bytes_length)
{
    const unsigned char* bytes_itr = static_cast<const unsigned char*>(bytes);
    const unsigned char* const bytes_end = bytes_itr + bytes_length;

    std::string hex_string;
    hex_string.resize(bytes_length * 2);
    char* hex_string_itr = hex_string.data();

    while( bytes_itr != bytes_end )
    {
        const unsigned char this_byte = static_cast<unsigned char>(*(bytes_itr++));
        *(hex_string_itr++) = Encoders::HexChars[this_byte >> 4];
        *(hex_string_itr++) = Encoders::HexChars[this_byte & 0x0F];
    }

    return hex_string;
}


void Hash::HexStringToBytesBuffer(const std::string_view hex_string_sv, std::byte* bytes, const bool throw_exceptions)
{
    constexpr const char* ExceptionMessage = "The hex string is not valid";

    if( throw_exceptions && hex_string_sv.size() % 2 != 0 )
        throw CSProException(ExceptionMessage);

    for( size_t i = 1; i < hex_string_sv.size(); i += 2 )
    {
        const char* first_hex_char = strchr(Encoders::HexChars, std::tolower(hex_string_sv[i - 1]));
        const char* second_hex_char = strchr(Encoders::HexChars, std::tolower(hex_string_sv[i]));

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


std::vector<std::byte> Hash::HexStringToBytes(const std::string_view hex_string_sv, const bool throw_exceptions)
{
    std::vector<std::byte> bytes(hex_string_sv.length() / 2);
    HexStringToBytesBuffer(hex_string_sv, bytes.data(), throw_exceptions);
    return bytes;
}



// --------------------------------------------------------------------------
// Hash::CreatorHelper
// --------------------------------------------------------------------------

class Hash::CreatorHelper
{
public:
    template<size_t digest_size,
             typename ContextT,
             auto InitFunc,
             auto UpdateFunc, typename UpdateFuncBufferT,
             auto FinalFunc>
    static std::string CreateFromFile(const char* digest_type, const InterfaceString& file_path, bool throw_exception_on_read_error);
};


template<size_t digest_size,
         typename ContextT,
         auto InitFunc,
         auto UpdateFunc, typename UpdateFuncBufferT,
         auto FinalFunc>
std::string Hash::CreatorHelper::CreateFromFile(const char* const digest_type,
                                                const InterfaceString& file_path,
                                                const bool throw_exception_on_read_error)
{
    auto return_error = [&]()
    {
        if( throw_exception_on_read_error )
            throw CSProException("A %s could not be created for: %s", digest_type, file_path.c_str_utf8());

        return std::string();
    };

    FILE* const file = !file_path.empty() ? PortableFunctions::FileOpen(file_path, "rb") :
                                            nullptr;

    if( file == nullptr )
        return return_error();

    ContextT ctx;
    InitFunc(&ctx);

    constexpr size_t BufferSize = 64 * 1024;
    static_assert(BufferSize <= std::numeric_limits<UpdateFuncBufferT>::max());

    auto buffer = std::make_unique_for_overwrite<char[]>(BufferSize);

    class DigestError { };
    bool success = true;

    try
    {
        do
        {
            const size_t bytes_read = fread(buffer.get(), 1, BufferSize, file);

            if( ferror(file) )
                throw DigestError();

            UpdateFunc(&ctx, buffer.get(), static_cast<UpdateFuncBufferT>(bytes_read));

        } while( !feof(file) );
    }

    catch( const DigestError& )
    {
        success = false;
    }

    fclose(file);

    uint8_t digest[digest_size];
    FinalFunc(digest, &ctx);

    if( !success )
        return return_error();

    return BytesToHexString(digest, digest_size);
}



// --------------------------------------------------------------------------
// Hash::Md5
// --------------------------------------------------------------------------

std::string Hash::Md5::CreateFromFile(const InterfaceString& file_path, const bool throw_exception_on_read_error/* = false*/)
{
    return CreatorHelper::CreateFromFile<
        16,
        MD5_CTX,
        MD5_Init,
        MD5_Update, unsigned long,
        MD5_Final>(
            "MD5", file_path, throw_exception_on_read_error
        );
}


template<typename CF>
std::string Hash::Md5::GenerateMd5(const CF& md5_update_callback)
{
    MD5_CTX ctx;
    MD5_Init(&ctx);

    do { } while( md5_update_callback(ctx) );

    constexpr size_t HexSequences = 16;

    unsigned char result[HexSequences];
    MD5_Final(result, &ctx);

    return BytesToHexString(result, HexSequences);
}


std::string Hash::Md5::CreateFromStream(std::istream& input_stream)
{
    if( input_stream )
    {
        class Md5Error { };

        try
        {
            constexpr size_t BufferSize = 64 * 1024;
            auto buffer = std::make_unique_for_overwrite<char[]>(BufferSize);

            return GenerateMd5(
                [&](MD5_CTX& ctx) -> bool
                {
                    input_stream.read(buffer.get(), BufferSize);

                    const std::streamsize bytes_read = input_stream.gcount();

                    if( bytes_read > 0 )
                    {
                        MD5_Update(&ctx, buffer.get(), static_cast<unsigned long>(bytes_read));
                        return true;
                    }

                    else if( input_stream.eof() )
                    {
                        return false;
                    }

                    else
                    {
                        throw Md5Error();
                    }

                });
        }

        catch( const Md5Error& ) { }
    }

    throw CSProException("A MD5 could not be created for the input stream.");
}


std::string Hash::Md5::Create(const std::byte* const contents, const size_t size)
{
    return GenerateMd5([&](MD5_CTX& ctx) -> bool
    {
        MD5_Update(&ctx, contents, uint32_cast(size));
        return false;
    });
}



// --------------------------------------------------------------------------
// Hash::Sha256
// --------------------------------------------------------------------------

std::string Hash::Sha256::CreateFromFile(const InterfaceString& file_path, const bool throw_exception_on_read_error/* = false*/)
{
    return CreatorHelper::CreateFromFile<
        32,
        SHA256_CTX,
        SHA256_Init,
        SHA256_Update, size_t,
        SHA256_Final>(
            "SHA-256", file_path, throw_exception_on_read_error
        );
}


std::string Hash::Sha256::Create(const std::byte* const contents, const size_t size)
{
    uint8_t sha256[32];
    SHA256_Buf(contents, size, sha256);
    return BytesToHexString(sha256, sizeof(sha256));
}
