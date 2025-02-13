#include "stdafx.h"
#include "JsonObjectCreator.h"
#include "Json.h"
#include <zToolsO/VariantVisitOverload.h>


// --------------------------------------------------------------------------
// CreateObject + CreateObjectString
// --------------------------------------------------------------------------

namespace
{
    template<typename CharType, typename JsonType, typename StringView>
    void SetObjectValue(JsonType& json, StringView key, const Json::JsonObjectCreatorWrapper& value)
    {
        std::visit(
            overload
            {
                [&](const std::function<void(JsonWriter&)>& write_function)
                {
                    auto jsw = Json::CreateStringWriter();

                    write_function(*jsw);

                    json.try_emplace(key, JsonNode<CharType>(jsw->GetString()).GetBasicJson());
                },

                [&](std::string_view value)
                {
                    json.try_emplace(key, UTF8Convert::UTF8ToWide(value).c_str());
                },

                [&](const JsonNode<wchar_t>& value)
                {
                    json.try_emplace(key, value.GetBasicJson());
                },

                [&](auto value)
                {
                    json.try_emplace(key, value);
                }

            }, value.data);
    }
}


JsonNode<wchar_t> Json::CreateObject(std::initializer_list<std::tuple<wstring_view, JsonObjectCreatorWrapper>> keys_and_values)
{
    auto json = std::make_unique<jsoncons::basic_json<wchar_t, jsoncons::order_preserving_policy, std::allocator<char>>>();

    for( const auto& [key, value] : keys_and_values )
        SetObjectValue<wchar_t>(*json, key, value);

    return JsonNode<wchar_t>(std::move(json));
}


std::wstring Json::CreateObjectString(std::initializer_list<std::tuple<wstring_view, JsonObjectCreatorWrapper>> keys_and_values)
{
    auto jsw = CreateStringWriter();

    jsw->BeginObject();

    for( const auto& [key, value] : keys_and_values )
    {
        jsw->Key(key);

        std::visit(
            overload
            {
                [&](const std::function<void(JsonWriter&)>& write_function)
                {
                    write_function(*jsw);
                },

                [&](const auto& value)
                {
                    jsw->Write(value);
                }

            }, value.data);
    }        

    jsw->EndObject();

    return jsw->GetString();
}



// --------------------------------------------------------------------------
// JsonObjectCreator
// --------------------------------------------------------------------------

template<typename CharType>
JsonObjectCreator<CharType>::JsonObjectCreator()
    :   m_json(std::make_shared<BasicJson>())
{
}


template<typename CharType>
JsonObjectCreator<CharType>& JsonObjectCreator<CharType>::Set(StringView key, Json::JsonObjectCreatorWrapper value)
{
    SetObjectValue<CharType>(*m_json, key, value);

    return *this;
}

template class JsonObjectCreator<wchar_t>;



// --------------------------------------------------------------------------
// JsonNodeCreator
// --------------------------------------------------------------------------

template<typename CharType>
JsonNode<CharType> JsonNodeCreator<CharType>::Null()
{
    return JsonNode<CharType>(std::make_shared<BasicJson>(BasicJson::null()));
}


template<typename CharType>
JsonNode<CharType> JsonNodeCreator<CharType>::Value(Json::JsonObjectCreatorWrapper value)
{
    std::variant<std::unique_ptr<BasicJson>, std::basic_string<CharType>> json_or_string;

    std::visit(
        overload
        {
            [&](const std::function<void(JsonWriter&)>& write_function)
            {
                auto jsw = Json::CreateStringWriter();

                write_function(*jsw);

                json_or_string = jsw->GetString();
            },

            [&](std::string_view value)
            {
                json_or_string = std::make_unique<BasicJson>(UTF8Convert::UTF8ToWide(value).c_str());
            },

            [&](const JsonNode<wchar_t>& value)
            {
                json_or_string = std::make_unique<BasicJson>(value.GetBasicJson());
            },

            [&](auto value)
            {
                json_or_string = std::make_unique<BasicJson>(value);
            }

        }, value.data);

    if( std::holds_alternative<std::unique_ptr<BasicJson>>(json_or_string) )
    {
        return JsonNode<CharType>(std::move(std::get<std::unique_ptr<BasicJson>>(json_or_string)));
    }

    else
    {
        return Json::Parse(std::get<std::basic_string<CharType>>(json_or_string));
    }
}

template class JsonNodeCreator<wchar_t>;
