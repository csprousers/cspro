#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/PropertyString.h>

template<typename CharType> class JsonNode;


class CLASS_DECL_ZUTILO BinaryDataMetadata : public PropertyString
{
public:
    BinaryDataMetadata() { }
    BinaryDataMetadata(wstring_view binary_data_metadata_text_sv);

    const std::wstring& GetBinaryDataKey() const        { return m_binaryDataKey; }
    void SetBinaryDataKey(std::wstring binary_data_key) { m_binaryDataKey = std::move(binary_data_key); }

    // gets the filename (with any directory information removed)
    std::optional<std::wstring> GetFilename() const;

    // sets the filename (removing any directory information);
    // if the MIME type for the filename (based on the extension) is known, it will be set;
    // if the MIME type is not known, it will be cleared
    void SetFilename(NullTerminatedString filename);

    std::optional<std::wstring> GetMimeType() const;
    void SetMimeType(std::wstring mime_type);

    // gets the extension from either the filename or the MIME type
    std::optional<std::wstring> GetEvaluatedExtension() const;

    // gets the MIME type from either the filename or the MIME type
    std::optional<std::wstring> GetEvaluatedMimeType() const;

    // gets the evaluated label, which will be the first defined value of:
    // - a property with the attribute "label"
    // - the filename
    // - a blank string
    std::wstring GetEvaluatedLabel() const;

    // serialization
    std::wstring ToString() const;

    static BinaryDataMetadata CreateFromJson(const JsonNode<wchar_t>& json_node);
    // WriteJson is inherited from PropertyString

protected:
    void SetMainValue(std::wstring main_value) override { m_binaryDataKey = std::move(main_value); }

private:
    std::wstring m_binaryDataKey;
};
