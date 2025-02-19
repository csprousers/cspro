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
                     JsonNode,
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
        CreateConstructor(JsonNode)
        CreateConstructor(std::function<void(JsonWriter&)>)
#undef CreateConstructor

        JsonObjectCreatorWrapper(const char* value)           : JsonObjectCreatorWrapper(std::string_view(value)) { }
        JsonObjectCreatorWrapper(const unsigned char* value)  : JsonObjectCreatorWrapper(std::string_view(reinterpret_cast<const char*>(value))) { }
        JsonObjectCreatorWrapper(const std::string& value)    : JsonObjectCreatorWrapper(std::string_view(value.data(), value.length())) { }
        JsonObjectCreatorWrapper(const SharableString& value) : JsonObjectCreatorWrapper(std::string_view(value->data(), value->length())) { }
    };
}



// --------------------------------------------------------------------------
// JsonObjectCreator
// --------------------------------------------------------------------------

class ZJSON_API JsonObjectCreator
{
    using BasicJson = jsoncons::basic_json<char, jsoncons::order_preserving_policy, std::allocator<char>>;

public:
    JsonObjectCreator();

    JsonNode GetJsonNode() const { return JsonNode(m_json); }

    JsonObjectCreator& Set(std::string_view key_sv, Json::JsonObjectCreatorWrapper value);

private:
    std::shared_ptr<BasicJson> m_json;
};



// --------------------------------------------------------------------------
// JsonNodeCreator
// --------------------------------------------------------------------------

class ZJSON_API JsonNodeCreator
{
    using BasicJson = jsoncons::basic_json<char, jsoncons::order_preserving_policy, std::allocator<char>>;

public:
    static JsonNode Null();

    static JsonNode Value(Json::JsonObjectCreatorWrapper value);

    template<typename T>
    static JsonNode FromWriteJson(const T& value)
    {
        return Value(std::function<void(JsonWriter&)>([&](JsonWriter& json_writer) { json_writer.Write(value); }));
    }
};
