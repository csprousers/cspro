#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/DataTypes.h>
#include <zToolsO/StringNoCase.h>


class CLASS_DECL_ZUTILO MimeType
{
public:
    // returns the file extension for known MIME types (or null if none); the returned extension does not include the dot
    static const TCHAR* GetFileExtensionFromType(wstring_view mime_type_text_sv);

    // returns the file extension(s) for known MIME types; the returned extensions do not include the dot;
    // if subtype is *, all known extensions for the type are returned
    static std::vector<const TCHAR*> GetFileExtensionsFromTypeWithWildcardSupport(wstring_view mime_type_text_sv);

    // returns the typical MIME type for a given extension; the extension should not include the dot
    static std::optional<std::wstring> GetTypeFromFileExtension(const StringNoCase& extension);
    static std::optional<std::wstring> GetServerTypeFromFileExtension(const StringNoCase& extension);

    // returns the file extensions (without leading dots) that CSPro supports for the given type
    static std::vector<const TCHAR*> GetExtensionsForSupportedContentType(ContentType content_type);

    // returns true if the text matches the appropriate type in our list of MIME types;
    // the type may not be fully supported by CSPro (e.g., a .gif file)
    static bool IsAudioType(wstring_view mime_type_text_sv);
    static bool IsImageType(wstring_view mime_type_text_sv);

    // returns std::nullopt if the text does not match a type fully supported by CSPro
    static std::optional<AudioType> GetSupportedAudioType(wstring_view mime_type_text_sv);
    static std::optional<ImageType> GetSupportedImageType(wstring_view mime_type_text_sv);

    // returns std::nullopt if the file extension does not match a type fully supported by CSPro
    static std::optional<AudioType> GetSupportedAudioTypeFromFileExtension(wstring_view extension_sv);
    static std::optional<ImageType> GetSupportedImageTypeFromFileExtension(wstring_view extension_sv);

    // returns the MIME type for the supported CSPro type
    static const TCHAR* GetType(AudioType audio_type);
    static const TCHAR* GetType(ImageType image_type);


    // some common types
    struct Type
    {
        static constexpr const char* Unknown      = "application/octet-stream";

        static constexpr const TCHAR* Text        = _T("text/plain");

        static constexpr const TCHAR* Html        = _T("text/html");
        static constexpr const TCHAR* JavaScript  = _T("application/javascript");
        static constexpr const TCHAR* Json        = _T("application/json");

        static constexpr const TCHAR* GeoJson     = _T("application/geo+json");

        static constexpr const TCHAR* AudioM4A    = _T("audio/mp4");

        static constexpr const TCHAR* ImageBitmap = _T("image/bmp");
        static constexpr const TCHAR* ImageJpeg   = _T("image/jpeg");
        static constexpr const TCHAR* ImagePng    = _T("image/png");
    };

    struct ServerType
    {
        static constexpr const TCHAR* TextUtf8    = _T("text/plain;charset=UTF-8");
    };
};
