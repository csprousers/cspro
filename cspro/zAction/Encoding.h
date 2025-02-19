#pragma once

#include <zJson/JsonSerializer.h>

namespace ActionInvoker { class Runtime; }


enum class BinaryEncodingInput         { Autodetect = 0, Base64 = 1, Cache = 2, DataUrl = 3, Hex = 4,                   Text = 6 };
enum class BinaryEncodingResolvedInput {                 Base64 = 1, Cache = 2, DataUrl = 3, Hex = 4,                   Text = 6 };
enum class BinaryEncodingOutput        {                 Base64 = 1, Cache = 2, DataUrl = 3, Hex = 4, LocalhostUrl = 5           };

DECLARE_ENUM_JSON_SERIALIZER_CLASS(BinaryEncodingInput,)
DECLARE_ENUM_JSON_SERIALIZER_CLASS(BinaryEncodingOutput,)


class StringToBytesConverter
{
public:
    // looks at the specified input format (autodetect if not specified) and resolves it (BinaryEncodingInput -> BinaryEncodingResolvedInput)
    static BinaryEncodingResolvedInput ResolveBinaryEncodingInput(std::string_view bytes_sv, const JsonNode& json_node, std::string_view bytes_format_key_sv);

    // looks at the specified input format (autodetect if not specified) and returns a non-null pointer to the converted bytes,
    // potentially throwing an exception on a conversion error
    static std::shared_ptr<const std::vector<std::byte>> Convert(ActionInvoker::Runtime& runtime, std::string_view bytes_sv,
                                                                 const JsonNode& json_node, std::string_view bytes_format_key_sv,
                                                                 std::tuple<BinaryEncodingResolvedInput, std::string>* out_binary_encoding_resolved_input_and_data_url_mediatype = nullptr);

private:
    static std::shared_ptr<const std::vector<std::byte>> ConvertCache(ActionInvoker::Runtime& runtime, std::string_view bytes_sv);
};


class BytesToStringConverter
{
public:
    // looks at the specific output format (data URL if not specified) and creates an object to convert bytes to that format
    BytesToStringConverter(ActionInvoker::Runtime* runtime, std::optional<BinaryEncodingOutput> binary_encoding_format);
    BytesToStringConverter(ActionInvoker::Runtime* runtime, const JsonNode& json_node, std::string_view bytes_format_key_sv);

    BinaryEncodingOutput GetBinaryEncodingOutput() const { return m_binaryEncodingOutput; }

    // converts the bytes to a string in the format specified in the constructor
    template<typename BT, typename MT>
    std::string Convert(BT&& bytes, MT&& mime_type);

private:
    template<typename BT>
    static std::shared_ptr<const std::vector<std::byte>> GetSharedPointerFromBytes(BT&& bytes);

    std::string ConvertImmediately(const std::vector<std::byte>& bytes, const std::string& mime_type);
    std::string ConvertCache(std::shared_ptr<const std::vector<std::byte>> bytes);
    std::string ConvertLocalhost(std::shared_ptr<const std::vector<std::byte>> bytes, std::string mime_type);

private:
    ActionInvoker::Runtime* const m_runtime;
    const BinaryEncodingOutput m_binaryEncodingOutput;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename BT>
std::shared_ptr<const std::vector<std::byte>> BytesToStringConverter::GetSharedPointerFromBytes(BT&& bytes)
{
    if constexpr(IsPointer<BT>())
    {
        return std::forward<BT>(bytes);
    }

    else
    {
        return std::make_shared<const std::vector<std::byte>>(std::forward<BT>(bytes));
    }
}


template<typename BT, typename MT>
std::string BytesToStringConverter::Convert(BT&& bytes, MT&& mime_type)
{
    ASSERT(GetPointer(bytes) != nullptr);

    switch( m_binaryEncodingOutput )
    {
        case BinaryEncodingOutput::Cache:        return ConvertCache(GetSharedPointerFromBytes(std::forward<BT>(bytes)));
        case BinaryEncodingOutput::LocalhostUrl: return ConvertLocalhost(GetSharedPointerFromBytes(std::forward<BT>(bytes)), std::forward<MT>(mime_type));
        default:                                 return ConvertImmediately(*GetPointer(bytes), std::forward<MT>(mime_type));
    }
}
