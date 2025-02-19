#include "StdAfx.h"
#include "BinaryDataMetadata.h"
#include "MimeType.h"


namespace MetadataKey
{
    constexpr std::string_view Filename_sv = "filename";
    constexpr std::string_view Label_sv    = "label";
    constexpr std::string_view MimeType_sv = "mime";
}


std::optional<std::string> BinaryDataMetadata::GetFilename() const
{
    const std::string* const filename = GetProperty(MetadataKey::Filename_sv);

    if( filename != nullptr )
        return PortableFunctions::PathGetFilename(*filename);

    return std::nullopt;
}


void BinaryDataMetadata::SetFilename(const std::string_view path_or_filename_sv)
{
    SetOrClearProperty(MetadataKey::Filename_sv, PortableFunctions::PathGetFilename(path_or_filename_sv));

    const std::string extension = PortableFunctions::PathGetFileExtension(path_or_filename_sv);
    std::optional<std::string> mime_type = MimeType::GetTypeFromFileExtension(extension);

    if( mime_type.has_value() )
    {
        SetMimeType(std::move(*mime_type));
    }

    else
    {
        ClearProperty(MetadataKey::MimeType_sv);
    }
}


std::optional<std::string> BinaryDataMetadata::GetMimeType() const
{
    const std::string* const mime_type = GetProperty(MetadataKey::MimeType_sv);

    return ( mime_type != nullptr ) ? std::make_optional(*mime_type) :
                                      std::nullopt;
}


void BinaryDataMetadata::SetMimeType(std::string mime_type)
{
    SetOrClearProperty(MetadataKey::MimeType_sv, std::move(mime_type));
}


std::optional<std::string> BinaryDataMetadata::GetEvaluatedExtension() const
{
    // try to get the extension from the filename
    const std::optional<std::string> filename = GetFilename();

    if( filename.has_value() )
        return PortableFunctions::PathGetFileExtension(*filename);

    // if not available, try to get the extension from the MIME type
    const std::optional<std::string> mime_type = GetMimeType();

    if( mime_type.has_value() )
    {
        const char* const extension = MimeType::GetFileExtensionFromType(*mime_type);

        if( extension != nullptr )
            return extension;
    }

    return std::nullopt;
}


std::optional<std::string> BinaryDataMetadata::GetEvaluatedMimeType() const
{
    std::optional<std::string> mime_type = GetMimeType();

    if( mime_type.has_value() )
        return mime_type;

    // if the MIME type is not explicity set, try to get it from the filename
    const std::optional<std::string> filename = GetFilename();

    return filename.has_value() ? MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(*filename)) :
                                  std::nullopt;
}


std::string BinaryDataMetadata::GetEvaluatedMimeType(const char* const default_mime_type) const
{
    ASSERT(default_mime_type != nullptr);

    std::optional<std::string> mime_type = GetEvaluatedMimeType();

    return mime_type.has_value() ? std::move(*mime_type) :
                                   std::string(default_mime_type);
}


std::string BinaryDataMetadata::GetEvaluatedLabel() const
{
    const std::string* const label = GetProperty(MetadataKey::Label_sv);

    if( label != nullptr )
        return *label;

    std::optional<std::string> filename = GetFilename();

    if( filename.has_value() )
        return *filename;

    return std::string();
}


BinaryDataMetadata BinaryDataMetadata::CreateFromJson(const JsonNode& json_node)
{
    BinaryDataMetadata binary_data_metadata;

    json_node.ForeachNode(
        [&](const std::string_view key_sv, const JsonNode& attribute_value_node)
        {
            // only add metadata that can be represented as a string
            std::optional<std::string> attribute = attribute_value_node.GetOptional<std::string>();

            if( attribute.has_value() )
                binary_data_metadata.SetProperty(key_sv, std::move(*attribute));
        });

    return binary_data_metadata;
}
