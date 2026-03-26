#pragma once

#include <zUtilO/zUtilO.h>
#include <zJson/Json.h>


// --------------------------------------------------------------------------
// CredentialStore
//
// The base implementation does not add a prefix to the attributes.
//
// The subclass DefinedPrefixCredentialStore can be used when there is a
// fixed prefix.
//
// Alternatively, a subclass can override PrefixAttribute.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO CredentialStore
{
public:
    virtual ~CredentialStore() { }

    // string-based functions (that can be overridden)
    virtual void Store(std::string_view attribute_sv, const std::string& secret_value);

    virtual std::string Retrieve(std::string_view attribute_sv);

    // JSON-based functions (that throw JSON serialization exceptions)
    template<typename T>
    void StoreAsJson(std::string_view attribute_sv, T& secret_value);

    template<typename T>
    T RetrieveFromJson(std::string_view attribute_sv);

    template<typename T>
    std::optional<T> RetrieveOptionalFromJson(std::string_view attribute_sv) noexcept;

protected:
    virtual std::string PrefixAttribute(std::string_view attribute_sv);
};



// --------------------------------------------------------------------------
// DefinedPrefixCredentialStore
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO DefinedPrefixCredentialStore : public CredentialStore
{
public:
    DefinedPrefixCredentialStore(std::string prefix);

protected:
    std::string PrefixAttribute(std::string_view attribute_sv) override;

private:
    std::string m_prefix;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
void CredentialStore::StoreAsJson(const std::string_view attribute_sv, T& secret_value)
{
    Store(attribute_sv, Json::ToJson(secret_value, JsonFormattingOptions::Compact));
}


template<typename T>
T CredentialStore::RetrieveFromJson(const std::string_view attribute_sv)
{
    return Json::FromJson<T>(Retrieve(attribute_sv));
}


template<typename T>
std::optional<T> CredentialStore::RetrieveOptionalFromJson(const std::string_view attribute_sv) noexcept
{
    try
    {
        const std::string json_text = Retrieve(attribute_sv);

        if( !json_text.empty() )
            return Json::FromJson<T>(json_text);
    }
    catch(...) { } // ignore errors

    return std::nullopt;
}
