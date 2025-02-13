#pragma once

class CDataDict;


namespace Listing
{   
    struct HeaderAttribute
    {
        HeaderAttribute(std::wstring description_, std::optional<std::wstring> secondary_description_,
                        std::variant<std::wstring, ConnectionString> value_, const CDataDict* dictionary_ = nullptr)
            :   description(std::move(description_)),
                secondary_description(std::move(secondary_description_)),
                value(std::move(value_)),
                dictionary(dictionary_)
        {
        }

        HeaderAttribute(std::wstring description_, std::variant<std::wstring, ConnectionString> value_)
            :   HeaderAttribute(std::move(description_), std::nullopt, std::move(value_))
        {
        }

        std::wstring description;
        std::optional<std::wstring> secondary_description;
        std::variant<std::wstring, ConnectionString> value;
        const CDataDict* dictionary;
    };
}
