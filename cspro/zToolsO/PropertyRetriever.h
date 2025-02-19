#pragma once


// the PropertyRetriever interface wraps objects that contain properties;
// implementations include:
//     - PropertyString::CreatePropertyRetriever
//     - JsonNode::CreatePropertyRetriever

class PropertyRetriever
{
public:
    virtual ~PropertyRetriever() { }

    // Gets a specific property (if defined).
    virtual std::optional<std::string> GetProperty(std::string_view attribute_sv) = 0;

    // Handles an invalid property value. The base behavior throws an exception.
    virtual void OnInvalidPropertyValue(std::string_view attribute_sv, const std::string& value)
    {
        throw CSProException("Unknown '%s' specifier: '%s'", std::string(attribute_sv).c_str(), value.c_str());
    }
};
