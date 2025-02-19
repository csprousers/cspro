#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/BarcodeCredentials.h>

class Encryptor;


class SYNC_API BarcodeCredentials
{
public:
    static std::string Encode(std::string application_name, const std::string& username, const std::string& password);

    static std::optional<std::tuple<std::string, std::string>> Decode(std::string application_name, std::string_view encrypted_credential_string_sv);

private:
    static Encryptor CreateEncryptor(std::string application_name);

    static std::optional<std::tuple<std::string, std::string>> Decode(Encryptor& encryptor, std::string_view encrypted_credential_string_sv);
    static std::optional<std::tuple<std::string, std::string>> DecodeV1(const std::wstring& credential_string);
    static std::optional<std::tuple<std::string, std::string>> DecodeV2(const std::string& credential_string);
};
