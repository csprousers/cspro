#include "stdafx.h"
#include <zToolsO/Hash.h>
#include <zDataO/EncryptedSQLiteRepository.h>


CREATE_JSON_KEY(iterations)
CREATE_JSON_KEY(salt)
CREATE_JSON_KEY(saltFormat)


ActionInvoker::Result ActionInvoker::Runtime::Hash_createHash(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    // default to PBKDF2_SHA256
    const size_t hash_type = json_node.Contains(JK::type) ?
        json_node.GetFromStringOptions(JK::type, std::initializer_list<const TCHAR*>({ _T("MD5"), _T("EncryptedCSProDB"), _T("PBKDF2_SHA256") })) :
        2;

    // Hash.createHash can be used to create a MD5
    if( hash_type == 0 )
        return Hash_createMd5(json_node, caller);

    const TCHAR* const input_type = GetUniqueKeyFromChoices(json_node, JK::path, JK::text, JK::bytes);
    std::vector<std::byte> content;

    // path
    if( input_type == JK::path )
    {
        const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));

        content = std::move(*FileIO::Read(path));
    }

    // text
    else if( input_type == JK::text )
    {
        content = UTF8Convert::WideToUTF8Buffer(json_node.Get<wstring_view>(JK::text));
    }

    // bytes
    else
    {
        ASSERT(input_type == JK::bytes);

        const wstring_view bytes_sv = json_node.Get<wstring_view>(JK::bytes);
        content = StringToBytesConverter::Convert(bytes_sv, json_node, JK::bytesFormat);
    }

    int length;
    int iterations;
    std::vector<std::byte> salt;

    // Hash.createHash can create the hash necessary to open a .csdbe file
    if( hash_type == 1 )
    {
        length = EncryptedSQLiteRepository::PasswordHashSize;
        iterations = EncryptedSQLiteRepository::PasswordHashIterations;

        static_assert(sizeof(std::byte) == sizeof(EncryptedSQLiteRepository::FixedSalt[0]));
        const std::byte* const csdbe_salt = reinterpret_cast<const std::byte*>(EncryptedSQLiteRepository::FixedSalt);
        salt.assign(csdbe_salt, csdbe_salt + _countof(EncryptedSQLiteRepository::FixedSalt));
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
                throw CSProException(_T("The hash length must be between 1-%d."), Hash::MaxHashLength);
        }

        if( json_node.Contains(JK::iterations) )
        {
            iterations = json_node.Get<int>(JK::iterations);

            if( iterations < 1 )
                throw CSProException("The number of hash iterations must a positive integer.");
        }

        if( json_node.Contains(JK::salt) )
        {
            const wstring_view salt_sv = json_node.Get<wstring_view>(JK::salt);
            salt = StringToBytesConverter::Convert(salt_sv, json_node, JK::saltFormat);
        }
    }

    const std::vector<std::byte> hash = Hash::Hash(content.data(), content.size(), salt.data(), salt.size(), length, iterations);

    return Result::String(Hash::BytesToHexString(hash.data(), hash.size()));
}


ActionInvoker::Result ActionInvoker::Runtime::Hash_createMd5(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const TCHAR* const input_type = GetUniqueKeyFromChoices(json_node, JK::path, JK::text, JK::bytes);
    std::wstring md5;

    // path
    if( input_type == JK::path )
    {
        const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));

        md5 = PortableFunctions::FileMd5(path);

        // PortableFunctions::FileMd5 returns a blank string if the file cannot be opened
        if( md5.empty() )
            throw FileIO::Exception::FileNotFound(path);
    }

    // text
    else if( input_type == JK::text )
    {
        md5 = PortableFunctions::StringMd5(UTF8Convert::WideToUTF8(json_node.Get<wstring_view>(JK::text)));
    }

    // bytes
    else
    {
        ASSERT(input_type == JK::bytes);

        const wstring_view bytes_sv = json_node.Get<wstring_view>(JK::bytes);
        const std::vector<std::byte> bytes = StringToBytesConverter::Convert(bytes_sv, json_node, JK::bytesFormat);

        md5 = PortableFunctions::BinaryMd5(bytes);
    }

    ASSERT(SO::IsLower(md5) && md5.length() == 32);

    return Result::String(std::move(md5));
}
