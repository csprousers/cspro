#include "stdafx.h"
#include "BarcodeCredentials.h"
#include <zToolsO/Encryption.h>


namespace
{
    static constexpr char FormatVersionV1      = '1';
    static constexpr char FormatVersionV2      = '2';
    static constexpr char FormatVersionCurrent = FormatVersionV2;
    static constexpr char DecryptionVerifier   = '@';
    static constexpr wchar_t LengthOffset      = 32;

    // V1: <verifier><wchar_t: wide length of username><wchar_t: wide length of password><username><password><verifier>
    // V2: <verifier><wchar_t as UTF-8: length of username><wchar_t as UTF-8: length of password><username><password><verifier>
}


Encryptor BarcodeCredentials::CreateEncryptor(std::string application_name)
{
    // they encryptor key will be the upper-case application name
    SO::MakeUpper(application_name);

    return Encryptor(Encryptor::Type::RijndaelBase64, application_name);
}


std::string BarcodeCredentials::Encode(std::string application_name, const std::string& username, const std::string& password)
{
    Encryptor encryptor = CreateEncryptor(std::move(application_name));

    const std::string credential_string = FormatText("%c%s%s%s%s%c",
                                                     DecryptionVerifier,
                                                     TC::GetUtf8ForWideChar(LengthOffset + static_cast<wchar_t>(username.length())).c_str(),
                                                     TC::GetUtf8ForWideChar(LengthOffset + static_cast<wchar_t>(password.length())).c_str(),
                                                     username.c_str(),
                                                     password.c_str(),
                                                     DecryptionVerifier);

    ASSERT(SO::WideLength(credential_string) == ( 4 + SO::WideLength(username) + SO::WideLength(password) ));

    // the credential version will be prepended to the Base64 representation of the credential string
    std::string encrypted_credential_string = FormatVersionCurrent + encryptor.Encrypt(credential_string);

    ASSERT(Decode(encryptor, encrypted_credential_string) == std::make_tuple(username, password));

    return encrypted_credential_string;
}


std::optional<std::tuple<std::string, std::string>> BarcodeCredentials::Decode(std::string application_name, const std::string_view encrypted_credential_string_sv)
{
    Encryptor encryptor = CreateEncryptor(std::move(application_name));
    return Decode(encryptor, encrypted_credential_string_sv);
}


std::optional<std::tuple<std::string, std::string>> BarcodeCredentials::Decode(Encryptor& encryptor, const std::string_view encrypted_credential_string_sv)
{
    // decode based on the credential version
    switch( !encrypted_credential_string_sv.empty() ? encrypted_credential_string_sv.front() : 0 )
    {
        case FormatVersionV1: return DecodeV1(TC::ToWide(encryptor.Decrypt(encrypted_credential_string_sv.substr(1))));
        case FormatVersionV2: return DecodeV2(encryptor.Decrypt(encrypted_credential_string_sv.substr(1)));
        default:              return std::nullopt;
    }
}


std::optional<std::tuple<std::string, std::string>> BarcodeCredentials::DecodeV1(const std::wstring& credential_string)
{
    // check the credential string
    if( credential_string.length() < 4 ||
        credential_string.front() != DecryptionVerifier ||
        credential_string.back() != DecryptionVerifier )
    {
        return std::nullopt;
    }

    const size_t username_length = static_cast<size_t>(credential_string[1] - LengthOffset);
    const size_t password_length = static_cast<size_t>(credential_string[2] - LengthOffset);

    if( credential_string.length() != ( 4 + username_length + password_length ) )
        return std::nullopt;
        
    return std::make_tuple(TC::ToUtf8(credential_string.substr(3, username_length)),
                           TC::ToUtf8(credential_string.substr(3 + username_length, password_length)));
}


std::optional<std::tuple<std::string, std::string>> BarcodeCredentials::DecodeV2(const std::string& credential_string)
{
    // check the credential string
    if( credential_string.length() < 4 ||
        credential_string.front() != DecryptionVerifier ||
        credential_string.back() != DecryptionVerifier )
    {
        return std::nullopt;
    }

    const char* credential_string_itr = credential_string.data() + 1;

    const size_t ch1_length = TC::Utf8BytesFromFirstByte(*credential_string_itr);
    const size_t username_length = static_cast<size_t>(TC::GetWideCharFromUtf8Sequence(credential_string_itr, ch1_length) - LengthOffset);
    credential_string_itr += ch1_length;

    const size_t ch2_length = TC::Utf8BytesFromFirstByte(*credential_string_itr);
    const size_t password_length = static_cast<size_t>(TC::GetWideCharFromUtf8Sequence(credential_string_itr, ch2_length) - LengthOffset);
    credential_string_itr += ch2_length;

    if( credential_string.length() != ( 2 + ch1_length + ch2_length + username_length + password_length ) )
        return std::nullopt;

    std::string username(credential_string_itr, username_length);
    credential_string_itr += username_length;
        
    return std::make_tuple(std::move(username),
                           std::string(credential_string_itr, password_length));
}
