#pragma once


// to add JSON serialization to an object, add these methods to a class:
//
// - static <ValueType> CreateFromJson(const JsonNode& json_node);
// - void WriteJson(JsonWriter& json_writer) const;
//
// or add the methods to a JsonSerializer object:

template<typename ValueType>
struct JsonSerializer
{
    // static ValueType CreateFromJson(const JsonNode& json_node);
    // static void WriteJson(JsonWriter& json_writer, const ValueType& value);
};


template<typename ValueType>
class JsonSerializerTester
{
private:
    // from https://stackoverflow.com/a/63823318
    typedef char one;
    struct two { char x[2]; };

    template<typename C> static one CreateFromJsonTest(decltype(&C::CreateFromJson));
    template<typename C> static two CreateFromJsonTest(...);

    template<typename C> static one WriteJsonTest(decltype(&C::WriteJson));
    template<typename C> static two WriteJsonTest(...);

public:
    static constexpr bool HasCreateFromJson() { return ( sizeof(CreateFromJsonTest<ValueType>(0)) == sizeof(char) ); }
    static constexpr bool HasWriteJson()      { return ( sizeof(WriteJsonTest<ValueType>(0)) == sizeof(char) ); }
};


#define DECLARE_ENUM_JSON_SERIALIZER_CLASS(EnumType, API)                       \
template<>                                                                      \
class API JsonSerializer<EnumType>                                              \
{                                                                               \
    static_assert(std::is_enum<EnumType>::value, #EnumType " must be an enum"); \
                                                                                \
private:                                                                        \
    static const std::vector<std::tuple<EnumType, std::string>>& GetMapping();  \
                                                                                \
public:                                                                         \
    static EnumType CreateFromJson(const JsonNode& json_node);                  \
    static void WriteJson(JsonWriter& json_writer, EnumType value);             \
};


#define DEFINE_ENUM_JSON_SERIALIZER_CLASS(EnumType, ...)                                             \
const std::vector<std::tuple<EnumType, std::string>>& JsonSerializer<EnumType>::GetMapping()         \
{                                                                                                    \
    static const std::vector<std::tuple<EnumType, std::string>> mapping =                            \
    {                                                                                                \
        __VA_ARGS__                                                                                  \
    };                                                                                               \
                                                                                                     \
    return mapping;                                                                                  \
}                                                                                                    \
                                                                                                     \
EnumType JsonSerializer<EnumType>::CreateFromJson(const JsonNode& json_node)                         \
{                                                                                                    \
    const std::string_view text_sv = json_node.Get<std::string_view>();                              \
                                                                                                     \
    const auto& mapping = GetMapping();                                                              \
    const auto& lookup = std::find_if(mapping.cbegin(), mapping.cend(),                              \
                                      [&](const auto& m) { return ( text_sv == std::get<1>(m) ); }); \
                                                                                                     \
    if( lookup != mapping.cend() )                                                                   \
        return std::get<0>(*lookup);                                                                 \
                                                                                                     \
    throw JsonParseException("'%s' is not a valid " #EnumType, std::string(text_sv).c_str());        \
}                                                                                                    \
                                                                                                     \
void JsonSerializer<EnumType>::WriteJson(JsonWriter& json_writer, EnumType value)                    \
{                                                                                                    \
    const auto& mapping = GetMapping();                                                              \
    const auto& lookup = std::find_if(mapping.cbegin(), mapping.cend(),                              \
                                      [&](const auto& m) { return ( value == std::get<0>(m) ); });   \
                                                                                                     \
    if( lookup != mapping.cend() )                                                                   \
    {                                                                                                \
        json_writer.Write(std::get<1>(*lookup));                                                     \
    }                                                                                                \
                                                                                                     \
    else                                                                                             \
    {                                                                                                \
        ASSERT(false);                                                                               \
        json_writer.WriteNull();                                                                     \
    }                                                                                                \
}


#define CREATE_ENUM_JSON_SERIALIZER(EnumType, ...)           \
    DECLARE_ENUM_JSON_SERIALIZER_CLASS(EnumType,)            \
    DEFINE_ENUM_JSON_SERIALIZER_CLASS(EnumType, __VA_ARGS__)
