#pragma once

#include <zUtilO/zUtilO.h>

class JsonWriter;


// A property string contains some text, and then optionally a | separator along with some number of
// attribute/value pairs with the values encoded using percent-encoding.
// The most common subclass of this is the ConnectionString.

class CLASS_DECL_ZUTILO PropertyString
{
protected:
    PropertyString() { }

public:
    virtual ~PropertyString() { }

    // Gets all properties.
    const std::vector<std::tuple<std::wstring, std::wstring>>& GetProperties() const { return m_properties; }

    // Gets a specific property (null if not defined).
    const std::wstring* GetProperty(wstring_view attribute_sv) const;

    // Returns whether or not a property is set, with or without a value.
    bool HasProperty(wstring_view attribute_sv) const;

    // Returns whether or not a property is set with the specified value.
    template<typename T>
    bool HasProperty(wstring_view attribute_sv, T&& value) const;

    // Returns whether or not a property is set with the specified value.
    // If the property is set but without a value, default_value_if_only_property_is_set is returned.
    template<typename T>
    bool HasProperty(wstring_view attribute_sv, T&& value, bool default_value_if_only_property_is_set) const;

    // Returns whether or not a property is set with the specified value.
    // If the property is set but without a value, or not set at all, default_value is returned.
    template<typename T>
    bool HasPropertyOrDefault(wstring_view attribute_sv, T&& value, bool default_value) const;

    // Sets a property.
    void SetProperty(wstring_view attribute_sv, std::wstring value);
    void SetProperty(wstring_view attribute_sv, double value);

    // Sets a property, or clears it if the value is blank.
    void SetOrClearProperty(wstring_view attribute_sv, std::wstring value);

    // Clears a property.
    void ClearProperty(wstring_view attribute_sv);

    // Writes the property string's properties.
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;

protected:
    // Parses a property string.
    void InitializeFromString(wstring_view property_string_text_sv);

    // Converts the property string to a string representation.
    std::wstring ToString(std::wstring main_value, const std::vector<std::tuple<std::wstring, std::wstring>>& properties) const;

    // A subclass can override this method to do something special with the main value.
    virtual void SetMainValue(std::wstring /*main_value*/) { }

    // A subclass can override this method to do something special with a property.
    // The function returns true if the property was handled.
    virtual bool PreprocessProperty(wstring_view /*attribute_sv*/, wstring_view /*value_sv*/) { return false; }

protected:
    std::vector<std::tuple<std::wstring, std::wstring>> m_properties;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline bool PropertyString::HasProperty(const wstring_view attribute_sv) const
{
    const std::wstring* property = GetProperty(attribute_sv);

    return ( property != nullptr );
}


template<typename T>
bool PropertyString::HasProperty(const wstring_view attribute_sv, T&& value) const
{
    const std::wstring* property = GetProperty(attribute_sv);

    return ( property != nullptr &&
             SO::EqualsNoCase(*property, std::forward<T>(value)) );
}


template<typename T>
bool PropertyString::HasProperty(const wstring_view attribute_sv, T&& value, const bool default_value_if_only_property_is_set) const
{
    const std::wstring* property = GetProperty(attribute_sv);

    return ( property == nullptr ) ? false :
           ( property->empty() )   ? default_value_if_only_property_is_set :
                                     SO::EqualsNoCase(*property, std::forward<T>(value));
}


template<typename T>
bool PropertyString::HasPropertyOrDefault(const wstring_view attribute_sv, T&& value, const bool default_value) const
{
    const std::wstring* property = GetProperty(attribute_sv);

    return ( property == nullptr || property->empty() ) ? default_value :
                                                          SO::EqualsNoCase(*property, std::forward<T>(value));
}
