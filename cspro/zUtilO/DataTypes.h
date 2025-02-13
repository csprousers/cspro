#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/FromString.h>
#include <zJson/JsonSerializer.h>


// --------------------------------------------------------------------------
// DataType
// --------------------------------------------------------------------------

enum class DataType : int
{
    Numeric = 0,
    String  = 1,
    Binary  = 2,
};

constexpr bool IsNumeric(DataType data_type) { return ( data_type == DataType::Numeric ); }
constexpr bool IsString(DataType data_type)  { return ( data_type == DataType::String  ); }
constexpr bool IsBinary(DataType data_type)  { return ( data_type == DataType::Binary  ); }

template<typename... Arguments>
bool DataTypeIsOneOf(DataType data_type, DataType first_data_type_to_match, Arguments... more_data_types_to_match)
{
    for( DataType data_type_to_match : { first_data_type_to_match, more_data_types_to_match... } )
    {
        if( data_type == data_type_to_match )
            return true;
    }

    return false;
}

CLASS_DECL_ZUTILO const TCHAR* ToString(DataType data_type);

template<> struct CLASS_DECL_ZUTILO JsonSerializer<DataType>
{
    static DataType CreateFromJson(const JsonNode<wchar_t>& json_node);
    static void WriteJson(JsonWriter& json_writer, DataType value);
};



// --------------------------------------------------------------------------
// ContentType
// --------------------------------------------------------------------------

enum class ContentType : int
{
    Numeric  = 0,
    Alpha    = 1,
    Document = 2,
    Audio    = 3,
    Image    = 4,
    Geometry = 5,
};


CLASS_DECL_ZUTILO const std::vector<ContentType>& GetContentTypesSupportedByDictionary();

constexpr bool IsNumeric(ContentType content_type) { return ( content_type == ContentType::Numeric ); }

constexpr bool IsString(ContentType content_type)  { return ( content_type == ContentType::Alpha ); }

constexpr bool IsBinary(ContentType content_type)  { return ( content_type == ContentType::Document ||
                                                              content_type == ContentType::Audio ||
                                                              content_type == ContentType::Image ||
                                                              content_type == ContentType::Geometry ); }

CLASS_DECL_ZUTILO const TCHAR* ToString(ContentType content_type);

template<> CLASS_DECL_ZUTILO std::optional<ContentType> FromString<ContentType>(wstring_view text_sv);

template<> struct CLASS_DECL_ZUTILO JsonSerializer<ContentType>
{
    static ContentType CreateFromJson(const JsonNode<wchar_t>& json_node);
    static void WriteJson(JsonWriter& json_writer, ContentType value);
};




namespace CONTENT_TYPE_REFACTOR
{
    CLASS_DECL_ZUTILO void LOOK_AT(const char* message = nullptr);

    [[noreturn]] inline void LOOK_AT_AND_THROW(const char* message = nullptr)
    {
        LOOK_AT(message);
        // CONTENT_TYPE_TODO zLogicCLR and zEngineCLR have references to zToolsO that can be
        //removed once this function is removed
        throw ProgrammingErrorException();
    }
}



// --------------------------------------------------------------------------
// AudioType 
// --------------------------------------------------------------------------

enum class AudioType : int
{
    M4A = 0
};



// --------------------------------------------------------------------------
// ImageType 
// --------------------------------------------------------------------------

enum class ImageType : int
{
    Jpeg   = 0,
    Png    = 1,
    Bitmap = 2
};



// --------------------------------------------------------------------------
// GeometryType
// --------------------------------------------------------------------------

enum class GeometryType : int
{
    GeoJSON = 0
};
