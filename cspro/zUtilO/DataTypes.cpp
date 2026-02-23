#include "StdAfx.h"
#include "DataTypes.h"


// --------------------------------------------------------------------------
// DataType
// --------------------------------------------------------------------------

namespace
{
    constexpr std::tuple<const char*, const char*> DataTypeStrings[] =
    {
        // ToString      // JSON
        { "Numeric",    "numeric" },
        { "String",     "string"  },
        { "Binary",     "binary"  },
    };
}


const char* ToString(const DataType data_type)
{
    const size_t index = static_cast<size_t>(data_type);
    ASSERT(index < _countof(DataTypeStrings));
    return std::get<0>(DataTypeStrings[index]);
}


DataType JsonSerializer<DataType>::CreateFromJson(const JsonNode& json_node)
{
    const std::string_view text_sv = json_node.Get<std::string_view>();

    for( size_t i = 0; i < _countof(DataTypeStrings); ++i )
    {
        if( text_sv == std::get<1>(DataTypeStrings[i]) )
            return static_cast<DataType>(i);
    }

    throw JsonParseException("'%s' is not a valid data type", std::string(text_sv).c_str());
}


void JsonSerializer<DataType>::WriteJson(JsonWriter& json_writer, const DataType value)
{
    const size_t index = static_cast<size_t>(value);
    ASSERT(index < _countof(DataTypeStrings));
    json_writer.Write(std::get<1>(DataTypeStrings[index]));
}



// --------------------------------------------------------------------------
// ContentType
// --------------------------------------------------------------------------

namespace
{
    constexpr const char* ContentTypeStrings[] =
    {
        "Numeric",
        "Alpha",
        "Document",
        "Audio",
        "Image",
        "Geometry",
        "Video",
    };
}


const std::vector<ContentType>& GetContentTypesSupportedByDictionary()
{
    static const std::vector<ContentType> ContentTypes
    {
        ContentType::Numeric,
        ContentType::Alpha,
        ContentType::Audio,
        ContentType::Document,
        ContentType::Geometry,
        ContentType::Image,
        // VIDEO_TODO_RESTORE_FOR_CSPRO8X ContentType::Video,
    };

    return ContentTypes;
}


const char* ToString(const ContentType content_type)
{
    const size_t index = static_cast<size_t>(content_type);
    ASSERT(index < _countof(ContentTypeStrings));
    return ContentTypeStrings[index];
}


std::string ToString(const ContentType content_type, const bool json_format)
{
    if( json_format && ( content_type == ContentType::Numeric || content_type == ContentType::Alpha ) )
        return SO::TitleToCamelCase(ToString(content_type));

    return ToString(content_type);
}


template<> std::optional<ContentType> FromString<ContentType>(const std::string_view text_sv)
{
    for( size_t i = 0; i < _countof(ContentTypeStrings); ++i )
    {
        if( SO::EqualsNoCase(text_sv, ContentTypeStrings[i]) )
            return static_cast<ContentType>(i);
    }

    return std::nullopt;
}


ContentType JsonSerializer<ContentType>::CreateFromJson(const JsonNode& json_node)
{
    const std::string_view text_sv = json_node.Get<std::string_view>();
    const std::optional<ContentType> content_type = FromString<ContentType>(text_sv);

    if( content_type.has_value() )
        return *content_type;

    throw JsonParseException("'%s' is not a valid content type", std::string(text_sv).c_str());
}


void JsonSerializer<ContentType>::WriteJson(JsonWriter& json_writer, const ContentType value)
{
    json_writer.Write(ToString(value, true));
}



void CONTENT_TYPE_REFACTOR::LOOK_AT(const char* /*message = nullptr*/)
{
    // CONTENT_TYPE_TODO eventually delete this function, but for now you can
    // enable the asserts if you want to see what needs refactoring
    // ASSERT(false);
}
