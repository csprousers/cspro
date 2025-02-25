#pragma once

#include <zUtilO/zUtilO.h>
#include <zJson/Json.h>


class CLASS_DECL_ZUTILO CredentialStore
{
public:
    virtual ~CredentialStore() { }

    // string-based functions (that can be overridden)
    virtual void Store(const std::string& attribute, const std::string& secret_value);

    virtual std::string Retrieve(const std::string& attribute);

    // JSON-based functions (that throw JSON serialization exceptions)
    template<typename T>
    void StoreAsJson(const std::string& attribute, T& secret_value);

    template<typename T>
    T RetrieveFromJson(const std::string& attribute);

    template<typename T>
    std::optional<T> RetrieveOptionalFromJson(const std::string& attribute) noexcept;

protected:
    virtual std::string PrefixAttribute(const std::string& attribute) = 0;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
void CredentialStore::StoreAsJson(const std::string& attribute, T& secret_value)
{
    Store(attribute, Json::ToJson(secret_value, JsonFormattingOptions::Compact));
}


template<typename T>
T CredentialStore::RetrieveFromJson(const std::string& attribute)
{
    return Json::FromJson<T>(Retrieve(attribute));
}


template<typename T>
std::optional<T> CredentialStore::RetrieveOptionalFromJson(const std::string& attribute) noexcept
{
    try
    {
        const std::string json_text = Retrieve(attribute);

        if( !json_text.empty() )
            return Json::FromJson<T>(json_text);
    }
    catch(...) { } // ignore errors

    return std::nullopt;
}
