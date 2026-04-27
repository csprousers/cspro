#pragma once

#include <zUtilO/zUtilO.h>

class PropertyRetriever;


// --------------------------------------------------------------------------
// PropertyString
//
// A property string contains some text, and then optionally a | separator
// along with some number of attribute/value pairs with the values encoded
// using percent-encoding.
//
// The most common subclasses of this are ConnectionString and
// SyncConnectionString.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO PropertyString
{
public:
    static constexpr char PropertySeparatorInitial    = '|';
    static constexpr char PropertySeparatorAdditional = '&';

protected:
    PropertyString() { }

public:
    virtual ~PropertyString() { }

    PropertyString(const PropertyString&) = default;
    PropertyString(PropertyString&&) = default;

    PropertyString& operator=(const PropertyString&) = default;
    PropertyString& operator=(PropertyString&&) = default;

    // Gets all properties.
    const std::vector<std::tuple<std::string, std::string>>& GetProperties() const { return m_properties; }

    // Gets a specific property (null if not defined).
    const std::string* GetProperty(std::string_view attribute_sv) const;

    // Returns whether or not a property is set, with or without a value.
    bool HasProperty(std::string_view attribute_sv) const;

    // Returns whether or not a property is set with a value.
    bool HasPropertyWithValue(std::string_view attribute_sv) const;

    // Returns whether or not a property is set with the specified value.
    template<typename T>
    bool HasProperty(std::string_view attribute_sv, T&& value) const;

    // Returns whether or not a property is set with the specified value.
    // If the property is set but without a value, default_value_if_only_property_is_set is returned.
    template<typename T>
    bool HasProperty(std::string_view attribute_sv, T&& value, bool default_value_if_only_property_is_set) const;

    // Returns whether or not a property is set with the specified value.
    // If the property is set but without a value, or not set at all, default_value is returned.
    template<typename T>
    bool HasPropertyOrDefault(std::string_view attribute_sv, T&& value, bool default_value) const;

    // Sets a property.
    void SetProperty(std::string_view attribute_sv, std::string value);
    void SetProperty(std::string_view attribute_sv, double value);

    // Sets a property, or clears it if the value is blank.
    void SetOrClearProperty(std::string_view attribute_sv, std::string value);

    // Clears a property.
    void ClearProperty(std::string_view attribute_sv);

    // Writes the property string's properties.
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;

    // Returns a PropertyRetriever object that can retrieve properties from the property string.
    // The object throws a CSProException when processing a property with an invalid value.
    std::unique_ptr<PropertyRetriever> CreatePropertyRetriever() const;

protected:
    // Parses a property string.
    void InitializeFromString(std::string_view property_string_text_sv);

    // Converts the property string to a string representation.
    static std::string ToString(std::string main_value, const std::vector<std::tuple<std::string, std::string>>& properties);

    // A subclass can override this method to do something special with the main value.
    virtual void SetMainValue(std::string_view /*main_value_sv*/) { }

    // A subclass can override this method to do something special with a property.
    // The function returns true if the property was handled.
    virtual bool PreprocessProperty(std::string_view /*attribute_sv*/, std::string_view /*value_sv*/) { return false; }

protected:
    std::vector<std::tuple<std::string, std::string>> m_properties;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline bool PropertyString::HasProperty(const std::string_view attribute_sv) const
{
    const std::string* const property = GetProperty(attribute_sv);

    return ( property != nullptr );
}


template<typename T>
bool PropertyString::HasProperty(const std::string_view attribute_sv, T&& value) const
{
    const std::string* const property = GetProperty(attribute_sv);

    return ( property != nullptr &&
             SO::EqualsNoCase(*property, std::forward<T>(value)) );
}


template<typename T>
bool PropertyString::HasProperty(const std::string_view attribute_sv, T&& value, const bool default_value_if_only_property_is_set) const
{
    const std::string* const property = GetProperty(attribute_sv);

    return ( property == nullptr ) ? false :
           ( property->empty() )   ? default_value_if_only_property_is_set :
                                     SO::EqualsNoCase(*property, std::forward<T>(value));
}


template<typename T>
bool PropertyString::HasPropertyOrDefault(const std::string_view attribute_sv, T&& value, const bool default_value) const
{
    const std::string* const property = GetProperty(attribute_sv);

    return ( property == nullptr || property->empty() ) ? default_value :
                                                          SO::EqualsNoCase(*property, std::forward<T>(value));
}
