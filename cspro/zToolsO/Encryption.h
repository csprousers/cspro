#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/span.h>


class CLASS_DECL_ZTOOLSO Encryptor
{
public:
    class Impl;
    enum class Type { RijndaelHex = 1, RijndaelBase64 = 2 };

    Encryptor(Type type, wstring_view key_sv = wstring_view());
    ~Encryptor();

    // encrypts or decrypts a string, returning it in hex or Base64;
    // a null terminator is added to the encryption buffer, indicating the length of the string
    std::wstring Encrypt(wstring_view text_sv);
    std::wstring Decrypt(wstring_view encrypted_text_sv);

    // encrypts or decrypts a buffer;
    // the length of the source buffer is added at the end encryption buffer
    std::vector<std::byte> Encrypt(cs::span<const std::byte> buffer);
    std::vector<std::byte> Decrypt(cs::span<const std::byte> encrypted_buffer);

private:
    std::unique_ptr<Impl> m_encryptor;
};
