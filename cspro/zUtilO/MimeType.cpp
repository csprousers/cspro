#include "StdAfx.h"
#include "MimeType.h"
#include <zToolsO/CaseInsensitiveComparer.h>


namespace MimeTypeMap
{
    const std::vector<std::tuple<const char*, const char*>>& GetMimeTypeAndDefaultExtensions();
    const std::map<std::string, unsigned, cs::case_insensitive_less>& GetExtensionMap();
}


namespace
{
    // splits the text into its type and subtype;
    // note that string_view objects are returned, so the function argument should not be a temporary string
    std::tuple<std::string_view, std::string_view> GetTypeSubtype(const std::string_view mime_type_text_sv)
    {
        const size_t slash_pos = mime_type_text_sv.find('/');

        return ( slash_pos == std::string_view::npos ) ? std::make_tuple(SO::Trim(mime_type_text_sv), std::string_view()) :
                                                         std::make_tuple(SO::Trim(mime_type_text_sv.substr(0, slash_pos)), SO::Trim(mime_type_text_sv.substr(slash_pos + 1)));
    }

#ifdef _DEBUG
    bool TypeSubtypeAreTrimmed(const std::string_view mime_type_text_sv)
    {
        const auto [type, subtype] = GetTypeSubtype(mime_type_text_sv);
        return ( mime_type_text_sv == SO::Concatenate(type, "/", subtype) );
    }
#endif


    using SupportedType = std::variant<std::monostate, AudioType, GeometryType, ImageType>;

    constexpr size_t SupportedIndexType(const ContentType content_type)
    {
        return ( content_type == ContentType::Audio )    ? SupportedType(AudioType()).index() :
               ( content_type == ContentType::Geometry ) ? SupportedType(GeometryType()).index() :
               ( content_type == ContentType::Image )    ? SupportedType(ImageType()).index() :
                                                           SupportedType({ }).index();
    }


    // the mapping of MIME types to extensions
    struct ExtensionMapping
    {
        const char* const type_and_subtype;
        std::string_view type_sv;
        // std::string_view subtype_sv; /* uncomment if the subtype is ever needed, along with the commented-out line below */
        const char* const extension;
        SupportedType supported_type;
    };

    const std::vector<ExtensionMapping>& GetExtensionMap()
    {
        static const std::vector<ExtensionMapping> extension_map =
        {
#define CREATE_MAPPING(type_and_subtype, extension, supported_type)  { type_and_subtype,                                   \
                                                                       std::get<0>(GetTypeSubtype(type_and_subtype)),      \
                                                                    /* std::get<1>(GetTypeSubtype(type_and_subtype)), \ */ \
                                                                       extension,                                          \
                                                                       supported_type }

            CREATE_MAPPING(MimeType::Type::GeoJson,     "geojson",  GeometryType::GeoJSON),

            CREATE_MAPPING(MimeType::Type::Text,        "txt",      { }),

            // audio
            CREATE_MAPPING("audio/mpeg",                "mp3",      { }),
            CREATE_MAPPING(MimeType::Type::AudioM4A,    "m4a",      AudioType::M4A),
            CREATE_MAPPING("audio/wav",                 "wav",      { }),

            // image
            CREATE_MAPPING(MimeType::Type::ImageBitmap, "bmp",      ImageType::Bitmap),
            CREATE_MAPPING("image/gif",                 "gif",      { }),
            CREATE_MAPPING(MimeType::Type::ImageJpeg,   "jpg",      ImageType::Jpeg),
            CREATE_MAPPING(MimeType::Type::ImageJpeg,   "jpeg",     ImageType::Jpeg),
            CREATE_MAPPING(MimeType::Type::ImagePng,    "png",      ImageType::Png),

#undef CREATE_MAPPING
        };

        return extension_map;
    };


    const ExtensionMapping* FindExtensionMappingFromMimeType(const std::string_view mime_type_text_sv)
    {
        ASSERT(TypeSubtypeAreTrimmed(mime_type_text_sv));

        for( const ExtensionMapping& extension_mapping : GetExtensionMap() )
        {
            if( SO::EqualsNoCase(mime_type_text_sv, extension_mapping.type_and_subtype) )
                return &extension_mapping;
        }

        return nullptr;
    }


    const ExtensionMapping* FindExtensionMappingFromFileExtension(const std::string_view extension_sv)
    {
        ASSERT(extension_sv.empty() || extension_sv.front() != '.');

        for( const ExtensionMapping& extension_mapping : GetExtensionMap() )
        {
            if( SO::EqualsNoCase(extension_sv, extension_mapping.extension) )
                return &extension_mapping;
        }

        return nullptr;
    }
}


const char* MimeType::GetFileExtensionFromType(const std::string_view mime_type_text_sv)
{
    ASSERT(TypeSubtypeAreTrimmed(mime_type_text_sv));

#ifdef _DEBUG
    auto get_extension_from_our_map = [&]() -> std::optional<std::string>
    {
        for( const ExtensionMapping& extension_mapping : GetExtensionMap() )
        {
            if( SO::EqualsNoCase(mime_type_text_sv, extension_mapping.type_and_subtype) )
                return extension_mapping.extension;
        }

        return nullptr;
    };
#endif

    const std::vector<std::tuple<const char*, const char*>>& mime_type_and_default_extensions = MimeTypeMap::GetMimeTypeAndDefaultExtensions();

    const auto& lookup = std::find_if(mime_type_and_default_extensions.cbegin(), mime_type_and_default_extensions.cend(),
        [&](const std::tuple<const char*, const char*>& mime_type_and_default_extension)
        {
            return SO::EqualsNoCase(mime_type_text_sv, std::get<0>(mime_type_and_default_extension));
        });

    if( lookup != mime_type_and_default_extensions.cend() )
    {
        ASSERT(std::get<1>(*lookup) == get_extension_from_our_map() || !get_extension_from_our_map().has_value());
        return std::get<1>(*lookup);
    }

    return nullptr;
}


std::vector<const char*> MimeType::GetFileExtensionsFromTypeWithWildcardSupport(const std::string_view mime_type_text_sv)
{
    std::vector<const char*> extensions;

    auto add_extension = [&](const char* const default_extension)
    {
        extensions.emplace_back(default_extension);

        // multiple extensions may be mapped to the MIME type/subtype
        const std::map<std::string, unsigned, cs::case_insensitive_less>& extension_map = MimeTypeMap::GetExtensionMap();

        const auto& lookup = extension_map.find(default_extension);

        if( lookup == extension_map.cend() )
        {
            ASSERT(false);
            return;
        }

        for( const auto& [extension, extension_index] : extension_map )
        {
            if( extension_index == lookup->second && extension != default_extension )
                extensions.emplace_back(extension.c_str());
        }
    };

    // if no wildcards are used, use GetFileExtensionFromType
    if( mime_type_text_sv.find('*') == std::string_view::npos )
    {
        const char* const default_extension = GetFileExtensionFromType(mime_type_text_sv);

        if( default_extension != nullptr )
            add_extension(default_extension);
    }

    else
    {
        const auto [type, subtype] = GetTypeSubtype(mime_type_text_sv);

        // only add extensions when using a wildcard for the entire subtype (and not for the type)
        if( type != "*" && subtype == "*" )
        {
            // add all extensions that match the type
            for( const auto& [mime_type, default_extension] : MimeTypeMap::GetMimeTypeAndDefaultExtensions() )
            {
                if( SO::StartsWithNoCase(mime_type, type) && mime_type[type.length()] == '/' )
                    add_extension(default_extension);
            }
        }
    }

    return extensions;
}


std::optional<std::string> MimeType::GetTypeFromFileExtension(const std::string_view extension_sv)
{
#ifdef _DEBUG
    auto get_type_from_our_map = [&]() -> std::optional<std::string>
    {
        const ExtensionMapping* const extension_mapping = FindExtensionMappingFromFileExtension(extension_sv);

        if( extension_mapping != nullptr )
            return extension_mapping->type_and_subtype;

        return std::nullopt;
    };
#endif

    const std::map<std::string, unsigned, cs::case_insensitive_less>& extension_map = MimeTypeMap::GetExtensionMap();
    const auto& lookup = extension_map.find(extension_sv);

    if( lookup != extension_map.cend() )
    {
        const std::vector<std::tuple<const char*, const char*>>& mime_type_and_default_extensions = MimeTypeMap::GetMimeTypeAndDefaultExtensions();
        ASSERT(std::get<0>(mime_type_and_default_extensions[lookup->second]) == get_type_from_our_map() || !get_type_from_our_map().has_value());
        return std::get<0>(mime_type_and_default_extensions[lookup->second]);
    }

    return std::nullopt;
}


std::optional<std::string> MimeType::GetServerTypeFromFileExtension(const std::string_view extension_sv)
{
    std::optional<std::string> mime_type = GetTypeFromFileExtension(extension_sv);

    if( mime_type == Type::Text )
        return ServerType::TextUtf8;

    return mime_type;
}


bool MimeType::IsTextTypeOrTextBased(const std::string_view mime_type_text_sv)
{
    if( SO::StartsWith(mime_type_text_sv, "text/") )
        return true;

    const auto [type, subtype] = GetTypeSubtype(mime_type_text_sv);

    if( type == "application" )
    {
        return ( subtype == "javascript" ||
                 subtype == "json" ||
                 subtype == "geo+json" );
    }

    else if( type == "text" )
    {
        return ReturnProgrammingError(true);
    }

    return false;
}


std::vector<const char*> MimeType::GetExtensionsForSupportedContentType(const ContentType content_type)
{
    const size_t supported_type_index = SupportedIndexType(content_type);
    std::vector<const char*> extensions;

    if( supported_type_index == SupportedType({ }).index() )
    {
        ASSERT(false);
        return extensions;
    }

    for( const ExtensionMapping& extension_mapping : GetExtensionMap() )
    {
        if( extension_mapping.supported_type.index() == supported_type_index )
            extensions.emplace_back(extension_mapping.extension);
    }

    ASSERT(!extensions.empty());
    return extensions;
}


namespace
{
    bool IsTypeWorker(const std::string_view mime_type_text_sv, const char* const type_text)
    {
        ASSERT(TypeSubtypeAreTrimmed(mime_type_text_sv));

        const ExtensionMapping* const extension_mapping = FindExtensionMappingFromMimeType(mime_type_text_sv);

        return ( extension_mapping != nullptr &&
                 extension_mapping->type_sv == type_text );
    }
}

bool MimeType::IsAudioType(const std::string_view mime_type_text_sv) { return IsTypeWorker(mime_type_text_sv, "audio"); }
bool MimeType::IsImageType(const std::string_view mime_type_text_sv) { return IsTypeWorker(mime_type_text_sv, "image"); }


namespace
{
    template<typename T>
    std::optional<T> GetSupportedTypeWorker(const std::string_view mime_type_text_sv)
    {
        ASSERT(TypeSubtypeAreTrimmed(mime_type_text_sv));

        const ExtensionMapping* const extension_mapping = FindExtensionMappingFromMimeType(mime_type_text_sv);
        constexpr size_t supported_type_index = SupportedType(T()).index();

        if( extension_mapping != nullptr && extension_mapping->supported_type.index() == supported_type_index )
            return std::get<T>(extension_mapping->supported_type);

        return std::nullopt;
    }


    template<typename T>
    std::optional<T> GetSupportedTypeFromFileExtensionWorker(const std::string_view extension_sv)
    {
        const ExtensionMapping* const extension_mapping = FindExtensionMappingFromFileExtension(extension_sv);
        constexpr size_t supported_type_index = SupportedType(T()).index();

        if( extension_mapping != nullptr && extension_mapping->supported_type.index() == supported_type_index )
            return std::get<T>(extension_mapping->supported_type);

        return std::nullopt;
    }
}

std::optional<AudioType> MimeType::GetSupportedAudioType(const std::string_view mime_type_text_sv) { return GetSupportedTypeWorker<AudioType>(mime_type_text_sv); }
std::optional<ImageType> MimeType::GetSupportedImageType(const std::string_view mime_type_text_sv) { return GetSupportedTypeWorker<ImageType>(mime_type_text_sv); }

std::optional<AudioType> MimeType::GetSupportedAudioTypeFromFileExtension(const std::string_view extension_sv) { return GetSupportedTypeFromFileExtensionWorker<AudioType>(extension_sv); }
std::optional<ImageType> MimeType::GetSupportedImageTypeFromFileExtension(const std::string_view extension_sv) { return GetSupportedTypeFromFileExtensionWorker<ImageType>(extension_sv); }


namespace
{
    template<typename T>
    const char* GetTypeWorker(const T type)
    {
        for( const ExtensionMapping& extension_mapping : GetExtensionMap() )
        {
            if( std::holds_alternative<T>(extension_mapping.supported_type) && std::get<T>(extension_mapping.supported_type) == type )
                return extension_mapping.type_and_subtype;
        }

        return ReturnProgrammingError(nullptr);
    }
}

const char* MimeType::GetType(const AudioType audio_type) { return GetTypeWorker(audio_type); }
const char* MimeType::GetType(const ImageType image_type) { return GetTypeWorker(image_type); }
