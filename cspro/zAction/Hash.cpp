#include "stdafx.h"
#include <zDataO/EncryptedSQLiteRepository.h>


CREATE_JSON_KEY(iterations)
CREATE_JSON_KEY(salt)
CREATE_JSON_KEY(saltFormat)


ActionInvoker::Result ActionInvoker::Runtime::Hash_createHash(const JsonNode& json_node, Caller& caller)
{
    // default to PBKDF2_SHA256
    const size_t hash_type = json_node.Contains(JK::type) ?
        json_node.GetFromStringOptions(JK::type, { "MD5", "EncryptedCSProDB", "PBKDF2_SHA256" }) :
        2;

    // Hash.createHash can be used to create a MD5
    if( hash_type == 0 )
        return Hash_createMd5(json_node, caller);

    const char* const input_type = GetUniqueKeyFromChoices(json_node, JK::path, JK::text, JK::bytes);
    std::shared_ptr<const std::vector<std::byte>> content;

    // path
    if( input_type == JK::path )
    {
        const std::string path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::path));

        content = FileIO::Read(path);
    }

    // text
    else if( input_type == JK::text )
    {
        content = std::make_unique<std::vector<std::byte>>(SO::CreateByteVector(json_node.Get<std::string_view>(JK::text)));
    }

    // bytes
    else
    {
        ASSERT(input_type == JK::bytes);

        const std::string_view bytes_sv = json_node.Get<std::string_view>(JK::bytes);
        content = StringToBytesConverter::Convert(*this, bytes_sv, json_node, JK::bytesFormat);
    }

    ASSERT(content != nullptr);

    int length;
    int iterations;
    std::shared_ptr<const std::vector<std::byte>> salt;

    // Hash.createHash can create the hash necessary to open a .csdbe file
    if( hash_type == 1 )
    {
        length = EncryptedSQLiteRepository::PasswordHashSize;
        iterations = EncryptedSQLiteRepository::PasswordHashIterations;

        static_assert(sizeof(std::byte) == sizeof(EncryptedSQLiteRepository::FixedSalt[0]));
        const std::byte* const csdbe_salt = reinterpret_cast<const std::byte*>(EncryptedSQLiteRepository::FixedSalt);
        salt = std::make_unique<std::vector<std::byte>>(csdbe_salt, csdbe_salt + _countof(EncryptedSQLiteRepository::FixedSalt));
    }

    // otherwise we will use PBKDF2_SHA256
    else
    {
        ASSERT(hash_type == 2);

        length = Hash::DefaultHashLength;
        iterations = Hash::DefaultIterations;

        if( json_node.Contains(JK::length) )
        {
            length = json_node.Get<int>(JK::length);

            if( length < 1 || length > Hash::MaxHashLength )
                throw CSProException("The hash length must be between 1-%d.", Hash::MaxHashLength);
        }

        if( json_node.Contains(JK::iterations) )
        {
            iterations = json_node.Get<int>(JK::iterations);

            if( iterations < 1 )
                throw CSProException("The number of hash iterations must a positive integer.");
        }

        if( json_node.Contains(JK::salt) )
        {
            const std::string_view salt_sv = json_node.Get<std::string_view>(JK::salt);
            salt = StringToBytesConverter::Convert(*this, salt_sv, json_node, JK::saltFormat);
        }
    }

    const std::vector<std::byte> hash = Hash::Create(
        content->data(),
        content->size(),
        ( salt != nullptr ) ? salt->data() : nullptr,
        ( salt != nullptr ) ? salt->size() : 0,
        length,
        iterations
    );

    return Result::String(Hash::BytesToHexString(hash.data(), hash.size()));
}


ActionInvoker::Result ActionInvoker::Runtime::Hash_createMd5(const JsonNode& json_node, Caller& caller)
{
    const char* const input_type = GetUniqueKeyFromChoices(json_node, JK::path, JK::text, JK::bytes);
    std::string md5;

    // path
    if( input_type == JK::path )
    {
        const std::string path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::path));

        md5 = Hash::Md5::CreateFromFile(path, true);
    }

    // text
    else if( input_type == JK::text )
    {
        md5 = Hash::Md5::Create(json_node.Get<std::string_view>(JK::text));
    }

    // bytes
    else
    {
        ASSERT(input_type == JK::bytes);

        const std::string_view bytes_sv = json_node.Get<std::string_view>(JK::bytes);
        const std::shared_ptr<const std::vector<std::byte>> bytes = StringToBytesConverter::Convert(*this, bytes_sv, json_node, JK::bytesFormat);

        md5 = Hash::Md5::Create(*bytes);
    }

    ASSERT(SO::IsLower(md5) && md5.length() == 32);

    return Result::String(std::move(md5));
}
