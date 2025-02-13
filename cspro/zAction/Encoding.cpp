#include "stdafx.h"
#include <zToolsO/Hash.h>
#include <zHtml/PortableLocalhost.h>
#include <zDataO/ConnectionStringProperties.h>


CREATE_JSON_VALUE(Base64)
CREATE_JSON_VALUE(hex)

DEFINE_ENUM_JSON_SERIALIZER_CLASS(TextEncoding,
    { TextEncoding::Ansi,    CSValue::ANSI },
    { TextEncoding::Utf8,    CSValue::UTF_8 },
    { TextEncoding::Utf8Bom, CSValue::UTF_8_BOM })

DEFINE_ENUM_JSON_SERIALIZER_CLASS(BinaryEncodingInput,
    { BinaryEncodingInput::Autodetect, _T("autodetect") },
    { BinaryEncodingInput::Base64,     JV::Base64 },
    { BinaryEncodingInput::DataUrl,    CSValue::dataUrl },
    { BinaryEncodingInput::Hex,        JV::hex },
    { BinaryEncodingInput::Text,       JK::text })

DEFINE_ENUM_JSON_SERIALIZER_CLASS(BinaryEncodingOutput,
    { BinaryEncodingOutput::Base64,       JV::Base64 },
    { BinaryEncodingOutput::DataUrl,      CSValue::dataUrl },
    { BinaryEncodingOutput::Hex,          JV::hex },
    { BinaryEncodingOutput::LocalhostUrl, _T("localhostUrl") })



// --------------------------------------------------------------------------
// StringToBytesConverter
// --------------------------------------------------------------------------

BinaryEncodingResolvedInput StringToBytesConverter::ResolveBinaryEncodingInput(const wstring_view bytes_sv, const JsonNode<wchar_t>& json_node, const wstring_view bytes_format_key_sv)
{
    BinaryEncodingInput binary_encoding_input = json_node.Contains(bytes_format_key_sv) ? json_node.Get<BinaryEncodingInput>(bytes_format_key_sv) :
                                                                                          BinaryEncodingInput::Autodetect;

    if( binary_encoding_input == BinaryEncodingInput::Autodetect )
    {
        binary_encoding_input = Encoders::IsDataUrl(bytes_sv) ? BinaryEncodingInput::DataUrl :
                                                                BinaryEncodingInput::Base64;
    }

    return static_cast<BinaryEncodingResolvedInput>(binary_encoding_input);
}


std::vector<std::byte> StringToBytesConverter::Convert(const wstring_view bytes_sv, const JsonNode<wchar_t>& json_node, const wstring_view bytes_format_key_sv)
{
    const BinaryEncodingResolvedInput binary_encoding_resolved_input = ResolveBinaryEncodingInput(bytes_sv, json_node, bytes_format_key_sv);

    // Base64
    if( binary_encoding_resolved_input == BinaryEncodingResolvedInput::Base64 )
    {
        return Base64::Decode<wstring_view, std::vector<std::byte>>(bytes_sv);
    }

    // data URL
    else if( binary_encoding_resolved_input == BinaryEncodingResolvedInput::DataUrl )
    {
        const auto [content, mediatype] = Encoders::FromDataUrl(bytes_sv);

        if( content == nullptr )
            throw CSProException("The data URL is not valid.");

        return *content;
    }

    // hex
    else if( binary_encoding_resolved_input == BinaryEncodingResolvedInput::Hex )
    {
        return Hash::HexStringToBytes(bytes_sv, true);
    }

    // text
    else
    {
        ASSERT(binary_encoding_resolved_input == BinaryEncodingResolvedInput::Text);

        return UTF8Convert::WideToUTF8Buffer(bytes_sv);
    }
}



// --------------------------------------------------------------------------
// BytesToStringConverter
// --------------------------------------------------------------------------

BytesToStringConverter::BytesToStringConverter(ActionInvoker::Runtime* runtime, const JsonNode<wchar_t>& json_node, const wstring_view bytes_format_key_sv)
    :   m_runtime(runtime),
        m_binaryEncodingOutput(json_node.Contains(bytes_format_key_sv) ? json_node.Get<BinaryEncodingOutput>(bytes_format_key_sv) :
                                                                         BinaryEncodingOutput::DataUrl)
{
    ASSERT(m_runtime != nullptr);
}


std::wstring BytesToStringConverter::ConvertImmediately(const std::vector<std::byte>& bytes, const wstring_view mime_type_sv)
{
    // Base64
    if( m_binaryEncodingOutput == BinaryEncodingOutput::Base64 )
    {
        return Base64::Encode<std::wstring>(bytes);
    }

    // data URL
    else if( m_binaryEncodingOutput == BinaryEncodingOutput::DataUrl )
    {
        return Encoders::ToDataUrl(bytes, mime_type_sv);
    }

    // hex
    else 
    {
        ASSERT(m_binaryEncodingOutput == BinaryEncodingOutput::Hex);

        return Hash::BytesToHexString(bytes.data(), bytes.size());
    }
}


std::wstring BytesToStringConverter::ConvertLocalhost(std::shared_ptr<const std::vector<std::byte>> bytes, wstring_view mime_type_sv)
{
    ASSERT(m_binaryEncodingOutput == BinaryEncodingOutput::LocalhostUrl);

    if( mime_type_sv == MimeType::Type::Text )
        mime_type_sv = MimeType::ServerType::TextUtf8;

    auto virtual_file_mapping_handler = std::make_unique<DataVirtualFileMappingHandler<std::shared_ptr<const std::vector<std::byte>>>>(std::move(bytes), mime_type_sv);

    PortableLocalhost::CreateVirtualFile(*virtual_file_mapping_handler);
    std::wstring url = virtual_file_mapping_handler->GetUrl();

    m_runtime->m_localHostVirtualFileMappingHandlers.emplace_back(std::move(virtual_file_mapping_handler));

    return std::move(url);
}
