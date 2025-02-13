#pragma once

#include <zJson/JsonSerializer.h>

namespace ActionInvoker { class Runtime; }


enum class TextEncoding { Ansi, Utf8, Utf8Bom };

enum class BinaryEncodingInput         { Autodetect = 0, Base64 = 1, DataUrl = 2, Hex = 3, Text = 4                   };
enum class BinaryEncodingResolvedInput {                 Base64 = 1, DataUrl = 2, Hex = 3, Text = 4                   };
enum class BinaryEncodingOutput        {                 Base64 = 1, DataUrl = 2, Hex = 3,           LocalhostUrl = 5 };

DECLARE_ENUM_JSON_SERIALIZER_CLASS(TextEncoding,)
DECLARE_ENUM_JSON_SERIALIZER_CLASS(BinaryEncodingInput,)
DECLARE_ENUM_JSON_SERIALIZER_CLASS(BinaryEncodingOutput,)


class StringToBytesConverter
{
public:
    // looks at the specified input format (autodetect if not specified) and resolves it (BinaryEncodingInput -> BinaryEncodingResolvedInput)
    static BinaryEncodingResolvedInput ResolveBinaryEncodingInput(wstring_view bytes_sv, const JsonNode<wchar_t>& json_node, wstring_view bytes_format_key_sv);

    // looks at the specified input format (autodetect if not specified) and returns the converted bytes, potentially throwing an exception on a conversion error
    static std::vector<std::byte> Convert(wstring_view bytes_sv, const JsonNode<wchar_t>& json_node, wstring_view bytes_format_key_sv);
};


class BytesToStringConverter
{
public:
    // looks at the specific output format (data URL if not specified) and creates an object to convert bytes to that format
    BytesToStringConverter(ActionInvoker::Runtime* runtime, const JsonNode<wchar_t>& json_node, wstring_view bytes_format_key_sv);

    // converts the bytes to a string in the format specified in the constructor
    template<typename T>
    std::wstring Convert(T&& bytes, wstring_view mime_type_sv);

    template<typename T>
    std::wstring Convert(T&& bytes, const std::optional<std::wstring>& mime_type)
    {
        return Convert<T>(std::forward<T>(bytes), mime_type.has_value() ? wstring_view(*mime_type) : wstring_view());
    }

private:
    std::wstring ConvertImmediately(const std::vector<std::byte>& bytes, wstring_view mime_type_sv);
    std::wstring ConvertLocalhost(std::shared_ptr<const std::vector<std::byte>> bytes, wstring_view mime_type_sv);

private:
    ActionInvoker::Runtime* const m_runtime;
    const BinaryEncodingOutput m_binaryEncodingOutput;
};


template<typename T>
std::wstring BytesToStringConverter::Convert(T&& bytes, const wstring_view mime_type_sv)
{
    ASSERT(GetPointer(bytes) != nullptr);

    if( m_binaryEncodingOutput == BinaryEncodingOutput::LocalhostUrl )
    {
        if constexpr(IsPointer<T>())
        {
            return ConvertLocalhost(std::move(bytes), mime_type_sv);
        }

        else
        {
            return ConvertLocalhost(std::make_shared<const std::vector<std::byte>>(std::forward<T>(bytes)), mime_type_sv);
        }        
    }

    else
    {
        return ConvertImmediately(*GetPointer(bytes), mime_type_sv);
    }            
}
