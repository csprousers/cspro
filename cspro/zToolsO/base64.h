#pragma once

#include <zToolsO/zToolsO.h>


class Base64
{
public:
    static std::string Encode(const void* bytes, size_t length)       { return base64_encode(static_cast<const unsigned char*>(bytes), length); }
    static std::string Encode(std::string_view text_sv)               { return base64_encode(reinterpret_cast<const unsigned char*>(text_sv.data()), uint32_cast(text_sv.length())); }
    static std::string Encode(const std::vector<std::byte>& contents) { return base64_encode(reinterpret_cast<const unsigned char*>(contents.data()), uint32_cast(contents.size())); }

    static std::string DecodeToString(std::string_view encoded_string_sv)            { return base64_decode<std::string>(encoded_string_sv); }
    static std::vector<std::byte> DecodeToBuffer(std::string_view encoded_string_sv) { return base64_decode<std::vector<std::byte>>(encoded_string_sv); }

private:
    CLASS_DECL_ZTOOLSO static std::string base64_encode(const unsigned char* bytes_to_encode, size_t in_len);

    template<typename ReturnType>
    static ReturnType base64_decode(std::string_view encoded_string_sv);
};
