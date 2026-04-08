#include "stdafx.h"
#include <zToolsO/Hash.h>
#include <zUtilO/CustomUri.h>
#include <zHtml/PortableLocalhost.h>
#include <zDataO/ConnectionStringProperties.h>


CREATE_JSON_VALUE(Base64)
CREATE_JSON_VALUE(cache)
CREATE_JSON_VALUE(hex)

DEFINE_ENUM_JSON_SERIALIZER_CLASS(BinaryEncodingInput,
    { BinaryEncodingInput::Autodetect, "autodetect" },
    { BinaryEncodingInput::Base64,     JV::Base64 },
    { BinaryEncodingInput::Cache,      JV::cache },
    { BinaryEncodingInput::DataUrl,    CSValue::dataUrl },
    { BinaryEncodingInput::Hex,        JV::hex },
    { BinaryEncodingInput::Text,       JK::text })

DEFINE_ENUM_JSON_SERIALIZER_CLASS(BinaryEncodingOutput,
    { BinaryEncodingOutput::Base64,       JV::Base64 },
    { BinaryEncodingOutput::Cache,        JV::cache },
    { BinaryEncodingOutput::DataUrl,      CSValue::dataUrl },
    { BinaryEncodingOutput::Hex,          JV::hex },
    { BinaryEncodingOutput::LocalhostUrl, "localhostUrl" })



// --------------------------------------------------------------------------
// StringToBytesConverter
// --------------------------------------------------------------------------

BinaryEncodingResolvedInput StringToBytesConverter::ResolveBinaryEncodingInput(const std::string_view bytes_sv, const JsonNode& json_node, const std::string_view bytes_format_key_sv)
{
    BinaryEncodingInput binary_encoding_input = json_node.Contains(bytes_format_key_sv) ? json_node.Get<BinaryEncodingInput>(bytes_format_key_sv) :
                                                                                          BinaryEncodingInput::Autodetect;

    if( binary_encoding_input == BinaryEncodingInput::Autodetect )
    {
        binary_encoding_input =
            Encoders::IsDataUrl(bytes_sv)                                   ? BinaryEncodingInput::DataUrl :
            CustomUri::UsesCSProScheme(bytes_sv, CustomUri::UriType::Cache) ? BinaryEncodingInput::Cache :
                                                                              BinaryEncodingInput::Base64;
    }

    return static_cast<BinaryEncodingResolvedInput>(binary_encoding_input);
}


std::shared_ptr<const std::vector<std::byte>> StringToBytesConverter::Convert(ActionInvoker::Runtime& runtime, const std::string_view bytes_sv,
                                                                              const JsonNode& json_node, const std::string_view bytes_format_key_sv,
                                                                              std::tuple<BinaryEncodingResolvedInput, std::string>* const out_binary_encoding_resolved_input_and_data_url_mediatype/* = nullptr*/)
{
    const BinaryEncodingResolvedInput binary_encoding_resolved_input = ResolveBinaryEncodingInput(bytes_sv, json_node, bytes_format_key_sv);

    if( out_binary_encoding_resolved_input_and_data_url_mediatype != nullptr )
        std::get<BinaryEncodingResolvedInput>(*out_binary_encoding_resolved_input_and_data_url_mediatype) = binary_encoding_resolved_input;

    // Base64
    if( binary_encoding_resolved_input == BinaryEncodingResolvedInput::Base64 )
    {
        return std::make_unique<std::vector<std::byte>>(Base64::DecodeToBuffer(bytes_sv));
    }

    // cache
    else if( binary_encoding_resolved_input == BinaryEncodingResolvedInput::Cache )
    {
        return ConvertCache(runtime, bytes_sv);
    }

    // data URL
    else if( binary_encoding_resolved_input == BinaryEncodingResolvedInput::DataUrl )
    {
        auto [content, mediatype] = Encoders::FromDataUrl(bytes_sv);

        if( content == nullptr )
            throw CSProException("The data URL is not valid.");

        if( out_binary_encoding_resolved_input_and_data_url_mediatype != nullptr )
            std::get<std::string>(*out_binary_encoding_resolved_input_and_data_url_mediatype) = std::move(mediatype);

        return std::move(content);
    }

    // hex
    else if( binary_encoding_resolved_input == BinaryEncodingResolvedInput::Hex )
    {
        return std::make_unique<std::vector<std::byte>>(Hash::HexStringToBytes(bytes_sv, true));
    }

    // text
    else
    {
        ASSERT(binary_encoding_resolved_input == BinaryEncodingResolvedInput::Text);

        return std::make_unique<std::vector<std::byte>>(SO::CreateByteVector(bytes_sv));
    }
}


std::shared_ptr<const std::vector<std::byte>> StringToBytesConverter::ConvertCache(ActionInvoker::Runtime& runtime, const std::string_view bytes_sv)
{
    if( !CustomUri::UsesCSProScheme(bytes_sv, CustomUri::UriType::Cache) )
        throw CSProException("The cache key is not specified correctly.");

    auto [actual_cache_key_sv, query_string_sv] = SO::GetTextOnEitherSideOfCharacter(bytes_sv, '?');
    const bool keep_data_in_cache = SO::Equals(query_string_sv, "clear=false");

    const std::string actual_cache_key(actual_cache_key_sv);
    auto lookup = runtime.m_cachedBinaryContent.find(actual_cache_key);

    if( lookup == runtime.m_cachedBinaryContent.cend() )
        throw CSProException("No cached binary data is associated with the key '%s'.", actual_cache_key.c_str());

    std::shared_ptr<const std::vector<std::byte>> content = lookup->second;

    if( !keep_data_in_cache )
        runtime.m_cachedBinaryContent.erase(lookup);

    return content;
}



// --------------------------------------------------------------------------
// BytesToStringConverter
// --------------------------------------------------------------------------

BytesToStringConverter::BytesToStringConverter(ActionInvoker::Runtime* const runtime, const std::optional<BinaryEncodingOutput> binary_encoding_format)
    :   m_runtime(runtime),
        m_binaryEncodingOutput(binary_encoding_format.value_or(BinaryEncodingOutput::DataUrl))
{
    ASSERT(m_runtime != nullptr);
}


BytesToStringConverter::BytesToStringConverter(ActionInvoker::Runtime* const runtime, const JsonNode& json_node, const std::string_view bytes_format_key_sv)
    :   BytesToStringConverter(runtime, json_node.GetOptional<BinaryEncodingOutput>(bytes_format_key_sv))
{
}


std::string BytesToStringConverter::ConvertImmediately(const std::vector<std::byte>& bytes, const std::string& mime_type)
{
    // Base64
    if( m_binaryEncodingOutput == BinaryEncodingOutput::Base64 )
    {
        return Base64::Encode(bytes);
    }

    // data URL
    else if( m_binaryEncodingOutput == BinaryEncodingOutput::DataUrl )
    {
        return Encoders::ToDataUrl(bytes, mime_type);
    }

    // hex
    else
    {
        ASSERT(m_binaryEncodingOutput == BinaryEncodingOutput::Hex);

        return Hash::BytesToHexString(bytes.data(), bytes.size());
    }
}


std::string BytesToStringConverter::ConvertCache(std::shared_ptr<const std::vector<std::byte>> bytes)
{
    ASSERT(m_binaryEncodingOutput == BinaryEncodingOutput::Cache && bytes != nullptr);

    std::string cache_key = CustomUri::CreateCacheUri();

    m_runtime->m_cachedBinaryContent.try_emplace(cache_key, std::move(bytes));

    return cache_key;
}


std::string BytesToStringConverter::ConvertLocalhost(std::shared_ptr<const std::vector<std::byte>> bytes, std::string mime_type)
{
    ASSERT(m_binaryEncodingOutput == BinaryEncodingOutput::LocalhostUrl && bytes != nullptr);

    if( mime_type == MimeType::Type::Text )
        mime_type = MimeType::ServerType::TextUtf8;

    auto virtual_file_mapping_handler = std::make_unique<DataVirtualFileMappingHandler<std::shared_ptr<const std::vector<std::byte>>>>(std::move(bytes), std::move(mime_type));

    PortableLocalhost::CreateVirtualFile(*virtual_file_mapping_handler);
    std::string url = virtual_file_mapping_handler->GetUrl();

    m_runtime->m_localHostVirtualFileMappingHandlers.emplace_back(std::move(virtual_file_mapping_handler));

    return std::move(url);
}
