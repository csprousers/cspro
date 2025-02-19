#include "stdafx.h"
#include "JsonObjectCreator.h"
#include "Json.h"
#include <zToolsO/VariantVisitOverload.h>


// --------------------------------------------------------------------------
// CreateObject + CreateObjectString
// --------------------------------------------------------------------------

namespace
{
    template<typename JsonType>
    void SetObjectValue(JsonType& json, const std::string_view key_sv, const Json::JsonObjectCreatorWrapper& value)
    {
        std::visit(
            overload
            {
                [&](const std::function<void(JsonWriter&)>& write_function)
                {
                    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(JsonFormattingOptions::Compact);

                    write_function(*json_writer);

                    json.try_emplace(key_sv, JsonNode(json_writer->GetString()).GetBasicJson());
                },

                [&](const std::string_view value_sv)
                {
                    json.try_emplace(key_sv, value_sv);
                },

                [&](const JsonNode& value)
                {
                    json.try_emplace(key_sv, value.GetBasicJson());
                },

                [&](const auto& value)
                {
                    json.try_emplace(key_sv, value);
                }

            }, value.data);
    }
}


JsonNode Json::CreateObject(const std::initializer_list<std::tuple<std::string_view, JsonObjectCreatorWrapper>> keys_and_values)
{
    auto json_node = std::make_unique<jsoncons::basic_json<char, jsoncons::order_preserving_policy, std::allocator<char>>>();

    for( const auto& [key_sv, value] : keys_and_values )
        SetObjectValue(*json_node, key_sv, value);

    return JsonNode(std::move(json_node));
}


std::string Json::CreateObjectString(const std::initializer_list<std::tuple<std::string_view, JsonObjectCreatorWrapper>> keys_and_values)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(JsonFormattingOptions::Compact);

    json_writer->BeginObject();

    for( const auto& [key_sv, value] : keys_and_values )
    {
        json_writer->Key(key_sv);

        std::visit(
            overload
            {
                [&](const std::function<void(JsonWriter&)>& write_function)
                {
                    write_function(*json_writer);
                },

                [&](const auto& value)
                {
                    json_writer->Write(value);
                }

            }, value.data);
    }        

    json_writer->EndObject();

    return json_writer->ReleaseString();
}



// --------------------------------------------------------------------------
// JsonObjectCreator
// --------------------------------------------------------------------------

JsonObjectCreator::JsonObjectCreator()
    :   m_json(std::make_shared<BasicJson>())
{
}


JsonObjectCreator& JsonObjectCreator::Set(const std::string_view key_sv, const Json::JsonObjectCreatorWrapper value)
{
    SetObjectValue(*m_json, key_sv, value);

    return *this;
}



// --------------------------------------------------------------------------
// JsonNodeCreator
// --------------------------------------------------------------------------

JsonNode JsonNodeCreator::Null()
{
    return JsonNode(std::make_unique<BasicJson>(BasicJson::null()));
}


JsonNode JsonNodeCreator::Value(const Json::JsonObjectCreatorWrapper value)
{
    std::variant<std::monostate, std::unique_ptr<BasicJson>, std::string> json_or_string;

    std::visit(
        overload
        {
            [&](const std::function<void(JsonWriter&)>& write_function)
            {
                const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(json_or_string.emplace<std::string>(), JsonFormattingOptions::Compact);
                write_function(*json_writer);
            },

            [&](const std::string_view value_sv)
            {
                json_or_string = std::make_unique<BasicJson>(value_sv);
            },

            [&](const JsonNode& value)
            {
                json_or_string = std::make_unique<BasicJson>(value.GetBasicJson());
            },

            [&](const auto& value)
            {
                json_or_string = std::make_unique<BasicJson>(value);
            }

        }, value.data);

    if( std::holds_alternative<std::unique_ptr<BasicJson>>(json_or_string) )
    {
        return JsonNode(std::move(std::get<std::unique_ptr<BasicJson>>(json_or_string)));
    }

    else if( std::holds_alternative<std::string>(json_or_string) )
    {
        return Json::Parse(std::get<std::string>(json_or_string));
    }

    else
    {
        return ReturnProgrammingError(JsonNodeCreator::Null());
    }
}
