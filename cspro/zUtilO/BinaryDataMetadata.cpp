#include "StdAfx.h"
#include "BinaryDataMetadata.h"
#include "MimeType.h"


namespace MetadataKey
{
    constexpr wstring_view Filename_sv = _T("filename");
    constexpr wstring_view Label_sv    = _T("label");
    constexpr wstring_view MimeType_sv = _T("mime");
}


BinaryDataMetadata::BinaryDataMetadata(wstring_view binary_data_metadata_text_sv)
{
    InitializeFromString(binary_data_metadata_text_sv);
}


std::optional<std::wstring> BinaryDataMetadata::GetFilename() const
{
    const std::wstring* filename = GetProperty(MetadataKey::Filename_sv);

    if( filename != nullptr )
        return PortableFunctions::PathGetFilename(*filename);

    return std::nullopt;
}


void BinaryDataMetadata::SetFilename(NullTerminatedString filename)
{
    SetOrClearProperty(MetadataKey::Filename_sv, PortableFunctions::PathGetFilename(filename));

    std::wstring extension = PortableFunctions::PathGetFileExtension(filename);
    std::optional<std::wstring> mime_type = MimeType::GetTypeFromFileExtension(extension);

    if( mime_type.has_value() )
    {
        SetMimeType(std::move(*mime_type));
    }

    else
    {
        ClearProperty(MetadataKey::MimeType_sv);
    }
}


std::optional<std::wstring> BinaryDataMetadata::GetMimeType() const
{
    const std::wstring* mime_type = GetProperty(MetadataKey::MimeType_sv);

    return ( mime_type != nullptr ) ? std::make_optional(*mime_type) :
                                      std::nullopt;
}


void BinaryDataMetadata::SetMimeType(std::wstring mime_type)
{
    SetOrClearProperty(MetadataKey::MimeType_sv, std::move(mime_type));
}


std::optional<std::wstring> BinaryDataMetadata::GetEvaluatedExtension() const
{
    // try to get the extension from the filename
    std::optional<std::wstring> filename = GetFilename();

    if( filename.has_value() )
        return PortableFunctions::PathGetFileExtension(*filename);

    // if not available, try to get the extension from the MIME type
    std::optional<std::wstring> mime_type = GetMimeType();

    if( mime_type.has_value() )
    {
        const TCHAR* const extension = MimeType::GetFileExtensionFromType(*mime_type);

        if( extension != nullptr )
            return extension;
    }

    return std::nullopt;
}


std::optional<std::wstring> BinaryDataMetadata::GetEvaluatedMimeType() const
{
    std::optional<std::wstring> mime_type = GetMimeType();

    if( mime_type.has_value() )
        return mime_type;

    // if the MIME type is not explicity set, try to get it from the filename
    std::optional<std::wstring> filename = GetFilename();

    return filename.has_value() ? MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(*filename)) :
                                  std::nullopt;
}


std::wstring BinaryDataMetadata::GetEvaluatedLabel() const
{
    const std::wstring* label = GetProperty(MetadataKey::Label_sv);

    if( label != nullptr )
        return *label;

    std::optional<std::wstring> filename = GetFilename();

    if( filename.has_value() )
        return *filename;

    return std::wstring();
}


std::wstring BinaryDataMetadata::ToString() const
{
    return PropertyString::ToString(m_binaryDataKey, m_properties);
}


BinaryDataMetadata BinaryDataMetadata::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    BinaryDataMetadata binary_data_metadata;

    json_node.ForeachNode(
        [&](std::wstring_view key, const JsonNode<wchar_t>& attribute_value_node)
        {
            // only add metadata that can be represented as a string
            std::optional<std::wstring> attribute = attribute_value_node.GetOptional<std::wstring>();

            if( attribute.has_value() )
                binary_data_metadata.SetProperty(key, *attribute);
        });

    return binary_data_metadata;
}
