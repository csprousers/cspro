#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/PropertyString.h>


class CLASS_DECL_ZUTILO BinaryDataMetadata : public PropertyString
{
public:
    BinaryDataMetadata() { }

    // gets the filename (with any directory information removed)
    std::optional<std::string> GetFilename() const;

    // sets the filename (removing any directory information);
    // if the MIME type for the filename (based on the extension) is known, it will be set;
    // if the MIME type is not known, it will be cleared
    void SetFilename(std::string_view path_or_filename_sv);

    std::optional<std::string> GetMimeType() const;
    void SetMimeType(std::string mime_type);

    // gets the extension from either the filename or the MIME type
    std::optional<std::string> GetEvaluatedExtension() const;

    // gets the MIME type from either the filename or the MIME type
    std::optional<std::string> GetEvaluatedMimeType() const;
    std::string GetEvaluatedMimeType(const char* default_mime_type) const;

    // gets the evaluated label, which will be the first defined value of:
    // - a property with the attribute "label"
    // - the filename
    // - a blank string
    std::string GetEvaluatedLabel() const;

    // serialization
    static BinaryDataMetadata CreateFromJson(const JsonNode& json_node);
    // WriteJson is inherited from PropertyString
};
