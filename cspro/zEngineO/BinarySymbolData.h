#pragma once

#include <zToolsO/PointerClasses.h>
#include <zUtilO/BinaryDataAccessor.h>

class BinarySymbol;
class BinarySymbolDataContentValidator;


// --------------------------------------------------------------------------
// BinarySymbolData
// --------------------------------------------------------------------------

class BinarySymbolData
{
public:
    BinarySymbolData();
    BinarySymbolData(cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor);
    BinarySymbolData(const BinarySymbolData& binary_symbol_data) = delete;
    BinarySymbolData(BinarySymbolData&& binary_symbol_data) = default;

    BinarySymbolData& operator=(const BinarySymbolData& binary_symbol_data);

    const BinaryDataAccessor& GetBinaryDataAccessor() const { return *m_binaryDataAccessor; }
    BinaryDataAccessor& GetBinaryDataAccessor()             { return *m_binaryDataAccessor; }

    bool IsDefined() const { return m_binaryDataAccessor->IsDefined(); }

    void Reset();

    const std::vector<std::byte>& GetContent() const                       { ASSERT(IsDefined()); return m_binaryDataAccessor->GetBinaryData().GetContent(); }
    std::shared_ptr<const std::vector<std::byte>> GetSharedContent() const { ASSERT(IsDefined()); return m_binaryDataAccessor->GetBinaryData().GetSharedContent(); }

    const BinaryDataMetadata& GetMetadata() const    { ASSERT(IsDefined()); return m_binaryDataAccessor->GetBinaryDataMetadata(); }
    BinaryDataMetadata& GetMetadataForModification() { ASSERT(IsDefined()); return m_binaryDataAccessor->GetBinaryDataMetadataForModification(); }

    template<typename T> void SetBinaryData(T&& content_or_callback, BinaryDataMetadata binary_data_metadata);
    template<typename T> void SetBinaryData(T&& content_or_callback, std::wstring path_or_filename);
    template<typename T> void SetBinaryData(T&& content_or_callback, std::wstring path_or_filename, std::wstring mime_type);

    // when setting the content without specifying any metadata, the current metadata (which must exist) is maintained
    template<typename T> void SetBinaryData(T&& content_or_callback);

    // the path is non-blank only if the binary data was loaded from / saved to the disk in the
    // current application's session, meaning that a persistent symbol's path will be initially blank
    const std::wstring& GetPath() const { return m_path; }
    void ClearPath()                    { m_path.clear(); }
    void SetPath(std::wstring path);

    // returns the filename (without directory information) from the binary data metadata; it can be blank
    std::wstring GetFilenameOnly() const;

    // returns the filename, or if it is blank, creates a fake filename with an extension based on the MIME type (if applicable);
    // if a symbol is passed, the symbol's name will be used as the base filename
    std::wstring CreateFilenameBasedOnMimeType() const                     { return CreateFilenameBasedOnMimeType(nullptr); }
    std::wstring CreateFilenameBasedOnMimeType(const Symbol& symbol) const { return CreateFilenameBasedOnMimeType(&symbol); }

    // writes the content and metadata to JSON format
    void WriteSymbolValueToJson(const BinarySymbol& binary_symbol, JsonWriter& json_writer,
                                const std::function<void()>* content_writer_override = nullptr) const;

    // updates the content and metadata from JSON format
    void UpdateSymbolValueFromJson(BinarySymbol& binary_symbol, const JsonNode<wchar_t>& json_node, BinarySymbolDataContentValidator* content_validator = nullptr,
                                   const std::function<BinaryData::ContentCallbackType(const JsonNode<wchar_t>&)>* non_url_content_reader = nullptr);

    // updates the content and metadata from a data URL, using binary_data_metadata as the base metadata
    void UpdateSymbolValueFromDataUrl(BinarySymbol& binary_symbol, wstring_view data_url_sv, BinaryDataMetadata binary_data_metadata = BinaryDataMetadata(),
                                      BinarySymbolDataContentValidator* content_validator = nullptr);

private:
    std::wstring CreateFilenameBasedOnMimeType(const Symbol* symbol) const;

private:
    cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> m_binaryDataAccessor;
    std::wstring m_path;
};



// --------------------------------------------------------------------------
// BinarySymbolDataContentValidator
// --------------------------------------------------------------------------

class BinarySymbolDataContentValidator
{
public:
    virtual ~BinarySymbolDataContentValidator() { }

    // return false, or throw an exception, if the non-null content is invalid
    virtual bool ValidateContent(std::shared_ptr<const std::vector<std::byte>> content) = 0;
};



// --------------------------------------------------------------------------
// BinarySymbolData inline implementations
// --------------------------------------------------------------------------

inline BinarySymbolData::BinarySymbolData()
    :   m_binaryDataAccessor(std::make_shared<BinaryDataAccessor>())
{
}


inline BinarySymbolData::BinarySymbolData(cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor)
    :   m_binaryDataAccessor(std::move(binary_data_accessor))
{
}


inline void BinarySymbolData::Reset()
{
    m_binaryDataAccessor->Reset();
    m_path.clear();
}


template<typename T>
void BinarySymbolData::SetBinaryData(T&& content_or_callback, BinaryDataMetadata binary_data_metadata)
{
    m_binaryDataAccessor->SetBinaryData(std::forward<T>(content_or_callback), std::move(binary_data_metadata));
}


template<typename T>
void BinarySymbolData::SetBinaryData(T&& content_or_callback)
{
    SetBinaryData(std::forward<T>(content_or_callback), IsDefined() ? GetMetadata() :
                                                                      ReturnProgrammingError(BinaryDataMetadata()));
}
