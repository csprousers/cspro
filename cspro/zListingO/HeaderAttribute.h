#pragma once

class CDataDict;
namespace Listing { struct HeaderAttribute; }


struct Listing::HeaderAttribute
{
    HeaderAttribute(std::string description_, std::optional<std::string> secondary_description_,
                    std::variant<std::string, ConnectionString> value_, const CDataDict* dictionary_ = nullptr)
        :   description(std::move(description_)),
            secondary_description(std::move(secondary_description_)),
            value(std::move(value_)),
            dictionary(dictionary_)
    {
    }

    HeaderAttribute(std::string description_, std::variant<std::string, ConnectionString> value_)
        :   HeaderAttribute(std::move(description_), std::nullopt, std::move(value_))
    {
    }

    const std::string description;
    std::optional<std::string> secondary_description;
    std::variant<std::string, ConnectionString> value;
    const CDataDict* const dictionary;
};
