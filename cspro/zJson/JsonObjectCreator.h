#pragma once

#include <zJson/zJson.h>
#include <zJson/JsonNode.h>
#include <zJson/JsonWriter.h>


namespace Json
{
    struct JsonObjectCreatorWrapper
    {
        std::variant<bool,
                     int,
                     unsigned int,
                     int64_t,
                     uint64_t,
                     double,
                     std::string_view,
                     wstring_view,
                     JsonNode<wchar_t>,
                     std::function<void(JsonWriter&)>> data;

        template<typename ValueType>
        JsonObjectCreatorWrapper(const ValueType& value)
            :   data([&value](JsonWriter& json_writer)
                {
                    json_writer.Write(value);
                })
        {
        }

#define CreateConstructor(ValueType) JsonObjectCreatorWrapper(ValueType value) : data(std::move(value)) { }
        CreateConstructor(bool)
        CreateConstructor(int)
        CreateConstructor(unsigned int)
        CreateConstructor(int64_t)
        CreateConstructor(uint64_t)
        CreateConstructor(double)
        CreateConstructor(std::string_view)
        CreateConstructor(wstring_view)
        CreateConstructor(JsonNode<wchar_t>)
        CreateConstructor(std::function<void(JsonWriter&)>)
#undef CreateConstructor

        JsonObjectCreatorWrapper(const char* value)          : JsonObjectCreatorWrapper(std::string_view(value)) { }
        JsonObjectCreatorWrapper(const unsigned char* value) : JsonObjectCreatorWrapper(std::string_view(reinterpret_cast<const char*>(value))) { }
        JsonObjectCreatorWrapper(const std::string& value)   : JsonObjectCreatorWrapper(std::string_view(value.data(), value.length())) { }
        JsonObjectCreatorWrapper(const wchar_t* value)       : JsonObjectCreatorWrapper(wstring_view(value)) { }
        JsonObjectCreatorWrapper(const std::wstring& value)  : JsonObjectCreatorWrapper(wstring_view(value.data(), value.length())) { }
        JsonObjectCreatorWrapper(const CString& value)       : JsonObjectCreatorWrapper(wstring_view(value.GetString(), value.GetLength())) { }
    };
}



// --------------------------------------------------
// JsonObjectCreator
// --------------------------------------------------

template<typename CharType>
class ZJSON_API JsonObjectCreator
{
    using BasicJson = jsoncons::basic_json<CharType, jsoncons::order_preserving_policy, std::allocator<char>>;
    using StringView = typename std::conditional_t<std::is_same_v<CharType, char>, std::string_view, wstring_view>;

public:
    JsonObjectCreator();

    JsonNode<CharType> GetJsonNode() const { return JsonNode<CharType>(m_json); }

    JsonObjectCreator<CharType>& Set(StringView key, Json::JsonObjectCreatorWrapper value);

private:
    std::shared_ptr<BasicJson> m_json;
};



// --------------------------------------------------
// JsonNodeCreator
// --------------------------------------------------

template<typename CharType>
class ZJSON_API JsonNodeCreator
{
    using BasicJson = jsoncons::basic_json<CharType, jsoncons::order_preserving_policy, std::allocator<char>>;

public:
    static JsonNode<CharType> Null();

    static JsonNode<CharType> Value(Json::JsonObjectCreatorWrapper value);

    template<typename T>
    static JsonNode<CharType> FromWriteJson(const T& value)
    {
        return Value(std::function<void(JsonWriter&)>([&](JsonWriter& json_writer) { json_writer.Write(value); }));
    }
};
