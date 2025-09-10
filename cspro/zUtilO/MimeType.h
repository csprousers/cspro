#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/DataTypes.h>


class CLASS_DECL_ZUTILO MimeType
{
public:
    // Returns the file extension for known MIME types (or null if none).
    // The returned extension does not include the dot.
    static const char* GetFileExtensionFromType(std::string_view mime_type_text_sv);

    // Returns the file extension(s) for known MIME types
    // The returned extensions do not include the dot.
    // If subtype is *, all known extensions for the type are returned.
    static std::vector<const char*> GetFileExtensionsFromTypeWithWildcardSupport(std::string_view mime_type_text_sv);

    // Returns the typical MIME type for a given extension.
    // The extension should not include the dot.
    static std::optional<std::string> GetTypeFromFileExtension(std::string_view extension_sv);
    static std::optional<std::string> GetServerTypeFromFileExtension(std::string_view extension_sv);

    // Returns whether the MIME type is "text", or is a known text type such as "application/json".
    static bool IsTextTypeOrTextBased(std::string_view mime_type_text_sv);

    // Returns the file extensions (without leading dots) that CSPro supports for the given type.
    static std::vector<const char*> GetExtensionsForSupportedContentType(ContentType content_type);

    // Returns true if the text matches the appropriate type in our list of MIME types.
    // The type may not be fully supported by CSPro (e.g., a .gif file).
    static bool IsAudioType(std::string_view mime_type_text_sv);
    static bool IsImageType(std::string_view mime_type_text_sv);
    static bool IsVideoType(std::string_view mime_type_text_sv);

    // Returns std::nullopt if the text does not match a type fully supported by CSPro.
    static std::optional<AudioType> GetSupportedAudioType(std::string_view mime_type_text_sv);
    static std::optional<ImageType> GetSupportedImageType(std::string_view mime_type_text_sv);
    static std::optional<VideoType> GetSupportedVideoType(std::string_view mime_type_text_sv);

    // Returns std::nullopt if the file extension does not match a type fully supported by CSPro.
    static std::optional<AudioType> GetSupportedAudioTypeFromFileExtension(std::string_view extension_sv);
    static std::optional<ImageType> GetSupportedImageTypeFromFileExtension(std::string_view extension_sv);
    static std::optional<VideoType> GetSupportedVideoTypeFromFileExtension(std::string_view extension_sv);

    // Returns the MIME type for the supported CSPro type.
    static const char* GetType(AudioType audio_type);
    static const char* GetType(ImageType image_type);
    static const char* GetType(VideoType video_type);


    // Some common types:
    struct Type
    {
        static constexpr const char* Unknown     = "application/octet-stream";

        static constexpr const char* Text        = "text/plain";

        static constexpr const char* Html        = "text/html";
        static constexpr const char* Markdown    = "text/markdown";

        static constexpr const char* JavaScript  = "application/javascript";
        static constexpr const char* Json        = "application/json";

        static constexpr const char* GeoJson     = "application/geo+json";

        static constexpr const char* AudioM4A    = "audio/mp4";

        static constexpr const char* ImageBitmap = "image/bmp";
        static constexpr const char* ImageJpeg   = "image/jpeg";
        static constexpr const char* ImagePng    = "image/png";
        static constexpr const char* ImageWebP   = "image/webp";

        static constexpr const char* VideoWebM   = "video/webm";
    };

    struct ServerType
    {
        static constexpr const char* TextUtf8    = "text/plain;charset=UTF-8";
    };
};
