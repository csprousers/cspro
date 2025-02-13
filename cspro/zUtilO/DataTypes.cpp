#include "StdAfx.h"
#include "DataTypes.h"


// --------------------------------------------------------------------------
// DataType
// --------------------------------------------------------------------------

namespace
{
    std::tuple<const TCHAR*, const TCHAR*> DataTypeStrings[] =
    {
        // ToString      // JSON
        { _T("Numeric"), _T("numeric") },
        { _T("String"),  _T("string")  },
        { _T("Binary"),  _T("binary")  },
    };
}


const TCHAR* ToString(DataType data_type)
{
    const size_t index = static_cast<size_t>(data_type);
    ASSERT(index < _countof(DataTypeStrings));
    return std::get<0>(DataTypeStrings[index]);
}


DataType JsonSerializer<DataType>::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    wstring_view text_sv = json_node.Get<wstring_view>();

    for( size_t i = 0; i < _countof(DataTypeStrings); ++i )
    {
        if( text_sv == std::get<1>(DataTypeStrings[i]) )
            return static_cast<DataType>(i);
    }

    throw JsonParseException(_T("'%s' is not a valid data type"), std::wstring(text_sv).c_str());
}


void JsonSerializer<DataType>::WriteJson(JsonWriter& json_writer, DataType value)
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
    const TCHAR* ContentTypeStrings[] =
    {
        _T("Numeric"),
        _T("Alpha"),
        _T("Document"),
        _T("Audio"),
        _T("Image"),
        _T("Geometry"),
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
    };

    return ContentTypes;
}


const TCHAR* ToString(ContentType content_type)
{
    const size_t index = static_cast<size_t>(content_type);
    ASSERT(index < _countof(ContentTypeStrings));
    return ContentTypeStrings[index];
}


template<> std::optional<ContentType> FromString<ContentType>(wstring_view text_sv)
{
    for( size_t i = 0; i < _countof(ContentTypeStrings); ++i )
    {
        if( SO::EqualsNoCase(text_sv, ContentTypeStrings[i]) )
            return static_cast<ContentType>(i);
    }

    return std::nullopt;
}


ContentType JsonSerializer<ContentType>::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    const wstring_view text_sv = json_node.Get<wstring_view>();
    std::optional<ContentType> content_type = FromString<ContentType>(text_sv);

    if( content_type.has_value() )
        return *content_type;

    throw JsonParseException(_T("'%s' is not a valid content type"), std::wstring(text_sv).c_str());    
}


void JsonSerializer<ContentType>::WriteJson(JsonWriter& json_writer, ContentType value)
{
    if( value == ContentType::Numeric || value == ContentType::Alpha )
    {
        json_writer.Write(SO::TitleToCamelCase(ToString(value)));
    }

    else
    {
        json_writer.Write(ToString(value));
    }
}



void CONTENT_TYPE_REFACTOR::LOOK_AT(const char* /*message = nullptr*/)
{
    // CONTENT_TYPE_TODO eventually delete this function, but for now you can
    // enable the asserts if you want to see what needs refactoring
    // ASSERT(false);
}
